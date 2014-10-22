/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lb_text_printer.h"

#if defined (__cplusplus_winrt)
// FreeType uses 'generic' as a variable name.
// Rename it since it's a reserved word in CX
#define generic generic_redefined
#endif

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>

#if defined (__cplusplus_winrt)
#undef generic
#endif

#include <vector>
#include <string>

#include "external/chromium/base/file_util.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/base/platform_file.h"
#include "external/chromium/base/stringprintf.h"

#include "lb_gl_image_utils.h"
#if defined(__LB_WIIU__)
#include "lb_shell/lb_graphics_wiiu.h"
#elif defined(__LB_PS3__)
#include "lb_shell/lb_graphics_ps3.h"
#elif defined(__LB_PS4__)
#include "lb_shell/lb_graphics_ps4.h"
#include "lb_shell/lb_shader_binary_manager.h"
#else
#include "lb_graphics.h"
#endif
#include "lb_web_graphics_context_3d.h"

namespace {
#if defined(__LB_ANDROID__) || defined(__LB_LINUX__) || defined(__LB_XB1__) \
    || defined(__LB_XB360__)
  const char* kVertexShaderFileName = "vertex_text.glsl";
  const char* kFragmentShaderFileName = "fragment_text.glsl";
#elif defined(__LB_WIIU__)
  const char* kVertexShaderFileName = "vertex_text.gsh";
  const char* kFragmentShaderFileName = "fragment_text.gsh";
#elif defined(__LB_PS3__)
  const char* kVertexShaderFileName = "vertex_text.vpo";
  const char* kFragmentShaderFileName = "fragment_text.fpo";
#elif defined(__LB_PS4__)
  const char* kVertexShaderFileName = "vertex_text.sb";
  const char* kFragmentShaderFileName = "pixel_text.sb";
#else
#error Platform not supported yet.
#endif
}  // namespace

// The text printer is implemented by creating a single texture
// containing every character in the font at a different region of
// the texture.  The texture is created using the FreeType library to
// render each character to a different location in the texture.
// The location of each character is recorded, so that
// when we wish to render a character we know where in the texture to
// find it.  Thus, for each character, we create a position/texture
// coordinate array buffer that encodes the character's position in the
// texture and the width of the character.
// To render a string, we use a special text vertex and fragment shader
// and then just go through character by character and render using
// the appropriate array buffer for each different character.
class LB::TextPrinter::Pimpl {
 public:
  Pimpl(const std::string& shaders_dir,
        int screen_width, int screen_height,
        const std::string& ttf_font_file,
        LBWebGraphicsContext3D* context,
        LBGraphics* graphics);
  ~Pimpl();

  void Print(float x, float y, const base::StringPiece& str) const;

  float GetLineHeight() const;

  float GetStringWidth(const base::StringPiece& input) const;

  int NumCharactersBeforeWrap(const base::StringPiece& input,
                              const float width) const;

 private:
  typedef std::vector<FT_BitmapGlyph> GlyphList;

  static const int kNumGlyphs = 128;

  struct FontMap {
    struct GlyphRenderInfo {
      // The x offset in the texture that this glyph starts at
      float tex_start;

      // The width/height of the glyph in units of max texture dimension
      float tex_width, tex_height;

      // The width/height of the glyph in pixels
      int pixel_width, pixel_height;

      // How much to translate this glyph by when placing it on the screen
      int pixel_trans_x, pixel_trans_y;

      // How much should we move forward after this character is output?
      int pixel_advance_x;
    };
    GlyphRenderInfo gri[kNumGlyphs];

    // The maximum height of all glyphs
    int max_height;

    int gl_texture;
  };

  // The structure for each vertex in the vertex buffer array
  struct GlyphVert {
    float vert_pos[2];
    float tex_coord[2];
  };

  // Since each glyph is rendered as a quad, there are 4 vertices.
  static const int kVertsPerGlyph = 4;
  struct GlyphQuad {
    GlyphVert verts[kVertsPerGlyph];
  };

  enum {
    kPositionAttrib,
    kTexcoordAttrib,
  };

  // Use FreeType to render a single character in to a bitmap glyph
  FT_BitmapGlyph MakeBitmapGlyphForChar(const FT_Face& face, char ch) const;

  FontMap RenderGlyphsToTexture(const std::vector<FT_BitmapGlyph>& glyphs,
                                const std::vector<float>& advances) const;

  // Attach the source code at the given location to the given shader
  // then compile it and attach the shader to the text program.
  void CompileAndAttachShader(unsigned int shader_id,
                              const std::string& shader_filename);

  void SetupShaders();

  // Setup the glyphs vertex buffer object that contains the texture
  // coordinates and vertex positions for each character in the font
  void SetupGlyphsVBO();

  std::string shaders_dir_;

  int screen_width_;
  int screen_height_;

  LBWebGraphicsContext3D* context_;
  LBGraphics* graphics_;

  // Information about the font texture and the glyphs in it
  FontMap font_map_;

  // The shader program used for rendering text
  int text_program_;
  // The vertex shader used for text rendering
  int vert_shader_id_;
  // The fragment shader used for text rendering
  int frag_shader_id_;

  // An array of glyph quads, one for each glyph
  GlyphQuad glyph_quads[kNumGlyphs];
};


LB::TextPrinter::Pimpl::Pimpl(const std::string& shaders_dir,
                              int screen_width, int screen_height,
                              const std::string& ttf_font_file,
                              LBWebGraphicsContext3D* context,
                              LBGraphics* graphics)
    : shaders_dir_(shaders_dir)
    , screen_width_(screen_width)
    , screen_height_(screen_height)
    , context_(context)
    , graphics_(graphics) {
  double pt_size = 1000.0;

  // Specify font resolution given a screen resolution of 1080p.  If the
  // screen resolution is not 1080p, the font resolution will be scaled
  // proportionally.
  int device_hdpi = 100 * (screen_width / 1920.0);
  int device_vdpi = 100 * (screen_height / 1080.0);

  // Setup FreeType
  FT_Error e;

  FT_Library ft_library;
  e = FT_Init_FreeType(&ft_library);
  DCHECK_EQ(e, 0);

  FT_Face ft_face;
  e = FT_New_Face(ft_library, ttf_font_file.c_str(), 0, &ft_face);
  DCHECK_EQ(e, 0);

  e = FT_Set_Char_Size(ft_face, 0, pt_size, device_hdpi, device_vdpi);
  DCHECK_EQ(e, 0);

  // Get a set of bitmap glyphs for each character.  Once these are
  // acquired, we will put them all in to one big texture and then
  // free them.
  GlyphList charBitmaps;
  std::vector<float> advances;  // Keep track of x pixel advances per char too
  for (int i = 0; i < kNumGlyphs; ++i) {
    charBitmaps.push_back(
      MakeBitmapGlyphForChar(ft_face, static_cast<char>(i)));

    // ft_face_->glyph is modified every time FT_Load_Glyph is called,
    // which is called in MakeBitmapGlyphForChar.
    advances.push_back(ft_face->glyph->advance.x / 64.0f);
  }

  font_map_ = RenderGlyphsToTexture(charBitmaps, advances);

  // We have extracted all the information we need from the
  // FreeType library, so close things down now.
  for (GlyphList::iterator iter = charBitmaps.begin();
      iter != charBitmaps.end(); ++iter) {
    FT_Done_Glyph(reinterpret_cast<FT_Glyph>(*iter));
  }
  FT_Done_Face(ft_face);
  FT_Done_FreeType(ft_library);

  SetupGlyphsVBO();
  SetupShaders();
}
LB::TextPrinter::Pimpl::~Pimpl() {
  context_->deleteProgram(text_program_);
  context_->deleteShader(frag_shader_id_);
  context_->deleteShader(vert_shader_id_);
  context_->deleteTexture(font_map_.gl_texture);
}

void LB::TextPrinter::Pimpl::Print(float x, float y,
                                   const base::StringPiece& str) const {
  if (!str.length())
    return;

  context_->useProgram(text_program_);
  context_->enable(GraphicsContext3D::BLEND);
  context_->blendFunc(GraphicsContext3D::SRC_ALPHA,
                      GraphicsContext3D::ONE_MINUS_SRC_ALPHA);

  context_->activeTexture(GraphicsContext3D::TEXTURE0);
  context_->bindTexture(GraphicsContext3D::TEXTURE_2D, font_map_.gl_texture);
  context_->texParameteri(GraphicsContext3D::TEXTURE_2D,
                          GraphicsContext3D::TEXTURE_MAG_FILTER,
                          GraphicsContext3D::LINEAR);
  context_->texParameteri(GraphicsContext3D::TEXTURE_2D,
                          GraphicsContext3D::TEXTURE_MIN_FILTER,
                          GraphicsContext3D::LINEAR);

  // Create a specialized vertex buffer for str
  int glyphs_vbo = context_->createBuffer();
  context_->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, glyphs_vbo);
  LB::ScopedVertexAttribBinding position_binding(
      context_,
      kPositionAttrib, 2, GraphicsContext3D::FLOAT, false, sizeof(GlyphVert),
      offsetof(GlyphVert, vert_pos));
  LB::ScopedVertexAttribBinding texcoord_binding(
      context_,
      kTexcoordAttrib, 2, GraphicsContext3D::FLOAT, false, sizeof(GlyphVert),
      offsetof(GlyphVert, tex_coord));

  // Create our index buffer that will be specific to str
  // We need 6 vertices per glyph because there are 2 triangles per glyph
  std::vector<GlyphVert> verts(str.size()*6);

  float inv_screen_width = 1.0f / screen_width_;

  int cur_vert_index = 0;
  float cur_x_pos = x;
  float cur_y_pos = y;
  for (base::StringPiece::const_iterator iter = str.begin();
      iter != str.end(); ++iter) {
    char cur_char = *iter;

    if (cur_char == '\n') {
      // If we encounter a newline character, drop down a line.
      cur_y_pos -= GetLineHeight();
      cur_x_pos = x;
      continue;
    }

    // We only support a subset of possible characters, so default
    // to a specific supported character if the actual character we want
    // is not supported.
    if (static_cast<unsigned char>(cur_char) >= kNumGlyphs) {
      cur_char = '?';
    }

    // Setup the two triangles for this glyph in our vertex buffer
    const GlyphQuad& glyph_quad = glyph_quads[cur_char];
    verts[cur_vert_index + 0] = glyph_quad.verts[0];
    verts[cur_vert_index + 1] = glyph_quad.verts[1];
    verts[cur_vert_index + 2] = glyph_quad.verts[2];
    verts[cur_vert_index + 3] = glyph_quad.verts[2];
    verts[cur_vert_index + 4] = glyph_quad.verts[1];
    verts[cur_vert_index + 5] = glyph_quad.verts[3];

    // Now adjust the vertex positions based on our position in the string
    for (int i = 0; i < 6; ++i) {
      verts[cur_vert_index + i].vert_pos[0] += cur_x_pos;
      verts[cur_vert_index + i].vert_pos[1] += cur_y_pos;
    }

    // Move index up to the next vertex's 2 triangles
    cur_vert_index += 6;

    // Advance our rendering x position
    float glyph_width =
      font_map_.gri[cur_char].pixel_width * inv_screen_width * 2;

    cur_x_pos += font_map_.gri[cur_char].pixel_advance_x *
                 inv_screen_width * 2;
  }

  context_->bufferData(GraphicsContext3D::ARRAY_BUFFER,
                       sizeof(verts[0]) * verts.size(), &verts[0],
                       GraphicsContext3D::STATIC_DRAW);
  // Setup the vertex buffer for the current character
  context_->drawArrays(GraphicsContext3D::TRIANGLES, 0, verts.size());

  context_->deleteBuffer(glyphs_vbo);
}

float LB::TextPrinter::Pimpl::GetLineHeight() const {
  return 2.0f * (static_cast<float>(font_map_.max_height) / screen_height_);
}

float LB::TextPrinter::Pimpl::GetStringWidth(
    const base::StringPiece& input) const {
  float ret_in_pixels = 0.0f;

  for (base::StringPiece::const_iterator iter = input.begin();
       iter != input.end(); ++iter) {
    ret_in_pixels += font_map_.gri[*iter].pixel_advance_x;
  }

  // Convert from pixels to normalized device coordinates before returning
  return ret_in_pixels * 2.0f / screen_width_;
}

int LB::TextPrinter::Pimpl::NumCharactersBeforeWrap(
    const base::StringPiece& input, const float width) const {
  int num_chars = 0;
  int current_pixel_width = 0;
  int width_in_pixels = screen_width_ * (width / 2.0f);

  for (base::StringPiece::const_iterator iter = input.begin();
      iter != input.end(); ++iter) {
    current_pixel_width += font_map_.gri[*iter].pixel_advance_x;
    if (current_pixel_width < width_in_pixels)
      num_chars++;
    else
      break;
  }
  return num_chars;
}

// Use FreeType to render a single character in to a bitmap glyph
FT_BitmapGlyph LB::TextPrinter::Pimpl::MakeBitmapGlyphForChar(
    const FT_Face& face, char ch) const {
  FT_Error e;

  e = FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT);
  DCHECK_EQ(e, 0);

  FT_Glyph glyph;
  e = FT_Get_Glyph(face->glyph, &glyph);
  DCHECK_EQ(e, 0);

  float foo = face->glyph->advance.x;

  e = FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
  DCHECK_EQ(e, 0);

  return reinterpret_cast<FT_BitmapGlyph>(glyph);
}

namespace {
int NextPowerOf2(int i) {
  int ret = 1;
  while (ret < i) ret *= 2;
  return ret;
}
}

LB::TextPrinter::Pimpl::FontMap LB::TextPrinter::Pimpl::RenderGlyphsToTexture(
    const std::vector<FT_BitmapGlyph>& glyphs,
    const std::vector<float>& advances) const {
  // This function will layout all character glyphs in a row, side-by-side,
  // in the same texture.

  // First iterate through the glyphs determining the maximum height and the
  // total width.
  int max_height = -1;
  int total_width = 0;

  for (GlyphList::const_iterator iter = glyphs.begin();
      iter != glyphs.end(); ++iter) {
    const FT_Bitmap& curBitmap = (*iter)->bitmap;

    max_height = (max_height > curBitmap.rows ? max_height : curBitmap.rows);
    total_width += curBitmap.width;
  }


  FontMap ret;
  ret.max_height = max_height;

  // Create a pixel buffer  with width and height equal to the next power
  // of 2 of the calculated total_width and max_height.
  int texture_width = NextPowerOf2(total_width);
  int texture_height = NextPowerOf2(max_height);
  std::vector<unsigned char> fontMapBuffer(texture_width*texture_height, 0);

  int curStart = 0;
  for (int i = 0; i < glyphs.size(); ++i) {
    const FT_Bitmap& curBitmap = glyphs[i]->bitmap;

    ret.gri[i].tex_start = static_cast<double>(curStart) / texture_width;

    ret.gri[i].pixel_width = curBitmap.width;
    ret.gri[i].pixel_height = curBitmap.rows;
    ret.gri[i].tex_width =
      static_cast<double>(ret.gri[i].pixel_width) / texture_width;
    ret.gri[i].tex_height =
      static_cast<double>(ret.gri[i].pixel_height) / texture_height;

    ret.gri[i].pixel_trans_x = glyphs[i]->left;
    ret.gri[i].pixel_trans_y = glyphs[i]->top - ret.gri[i].pixel_height;

    ret.gri[i].pixel_advance_x = advances[i];

    // Copy the bitmap in to the right place in the font map buffer
    for (int y = 0; y < curBitmap.rows; ++y) {
      for (int x = 0; x < curBitmap.width; ++x) {
        int dstX = x + curStart;
        int dstY = y;
        fontMapBuffer[dstX + dstY*texture_width] =
          curBitmap.buffer[x + y*curBitmap.width];
      }
    }

    curStart += curBitmap.width;
  }

  // Now create an OpenGL texture with the font map buffer as its data
  ret.gl_texture = context_->createTexture();
  context_->bindTexture(GraphicsContext3D::TEXTURE_2D, ret.gl_texture);
  context_->texImage2D(GraphicsContext3D::TEXTURE_2D,
                       0,
                       GraphicsContext3D::ALPHA,
                       texture_width, texture_height,
                       0,
                       GraphicsContext3D::ALPHA,
                       GraphicsContext3D::UNSIGNED_BYTE,
                       &fontMapBuffer[0]);

  return ret;
}

// Attach the source code at the given location to the given shader
// then compile it and attach the shader to the text program.
void LB::TextPrinter::Pimpl::CompileAndAttachShader(
    unsigned int shader_id, const std::string& shader_filename) {
#if defined(__LB_WIIU__)
  LBGraphicsWiiU::ShaderBinary binary =
      static_cast<LBGraphicsWiiU*>(graphics_)->GetShaderBinaryFromFilename(
          shader_filename);

  context_->shaderBinary(1, &shader_id, 0, binary.data, binary.length);
#elif defined(__LB_PS3__)
  LBGraphicsPS3::ShaderBinary binary =
      static_cast<LBGraphicsPS3*>(graphics_)->GetShaderBinaryFromFilename(
          shader_filename);

  context_->shaderBinary(1, &shader_id, 0, binary.data, binary.length);
#elif defined(__LB_PS4__)
  LB::ShaderBinaryManager::ShaderBinary binary =
      static_cast<LBGraphicsPS4*>(graphics_)->shader_manager()->
          GetShaderBinaryFromFilename(shader_filename.c_str());

  context_->shaderBinary(1, &shader_id, 0, binary.data, binary.size);
#else
  std::string shader_source;
  file_util::ReadFileToString(FilePath(shaders_dir_ + shader_filename),
                              &shader_source);
  context_->shaderSource(shader_id, &shader_source[0]);
#endif
  context_->compileShader(shader_id);

  context_->attachShader(text_program_, shader_id);
}

void LB::TextPrinter::Pimpl::SetupShaders() {
  text_program_ = context_->createProgram();

  // Compile the vertex shader
  vert_shader_id_ =
      context_->createShader(GraphicsContext3D::VERTEX_SHADER);
  CompileAndAttachShader(vert_shader_id_, kVertexShaderFileName);

  // Compile the fragment shader
  frag_shader_id_ =
      context_->createShader(GraphicsContext3D::FRAGMENT_SHADER);
  CompileAndAttachShader(frag_shader_id_, kFragmentShaderFileName);

  // Pull out the locations variables of interest from the shader
  context_->bindAttribLocation(text_program_, kPositionAttrib, "a_position");
  context_->bindAttribLocation(text_program_, kTexcoordAttrib, "a_texCoord");

  context_->linkProgram(text_program_);

  // Set the texture in the program to use texture 0
  int ref_tex_uniform = context_->getUniformLocation(text_program_,
                                                     "reftex");
  context_->useProgram(text_program_);
  context_->uniform1i(ref_tex_uniform, 0);
  context_->useProgram(0);
}


// Setup the glyphs vertex buffer object that contains the texture
// coordinates and vertex positions for each character in the font
void LB::TextPrinter::Pimpl::SetupGlyphsVBO() {
  float inv_screen_width = 1.0 / static_cast<double>(screen_width_);
  float inv_screen_height = 1.0 / static_cast<double>(screen_height_);

  float vert_order[8] = {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
  };

  // Loop through each glyph producing 4 vertices for it based entirely
  // from the already computed GlyphRenderInfo structs in font_map_.
  for (int i = 0; i < kNumGlyphs; ++i) {
    const FontMap::GlyphRenderInfo& cur_gri = font_map_.gri[i];

    for (int j = 0; j < kVertsPerGlyph; ++j) {
      GlyphVert& cur_vert = glyph_quads[i].verts[j];

      // *2 to the width/height because we go from -1 to 1, not 0 to 1
      cur_vert.vert_pos[0] = (cur_gri.pixel_trans_x
        + vert_order[j*2] * cur_gri.pixel_width) * inv_screen_width*2;
      cur_vert.vert_pos[1] = (cur_gri.pixel_trans_y
        + vert_order[j*2+1] * cur_gri.pixel_height) * inv_screen_height*2;

      cur_vert.tex_coord[0] =
        cur_gri.tex_start + vert_order[j*2] * cur_gri.tex_width;
      // Texture y coordinates are flipped from those of device coordinates
      cur_vert.tex_coord[1] = (vert_order[j*2+1] == 0 ? 1 : 0) *
                              cur_gri.tex_height;
    }
  }
}


namespace LB {

// These functions mostly just forward their calls to the PIMPL object
TextPrinter::TextPrinter(const std::string& shaders_dir,
                         int screen_width, int screen_height,
                         const std::string& ttf_font_file,
                         LBWebGraphicsContext3D* context,
                         LBGraphics* graphics)
    : pimpl_(new Pimpl(shaders_dir,
                       screen_width, screen_height,
                       ttf_font_file,
                       context,
                       graphics)) {
}

void TextPrinter::Print(
    float x, float y, const base::StringPiece& str) const {
  pimpl_->Print(x, y, str);
}

void TextPrinter::Printf(float x, float y, const char* fmt, ...) const {
  va_list ap;
  va_start(ap, fmt);

  std::string text_buffer = base::StringPrintV(fmt, ap);

  va_end(ap);

  Print(x, y, text_buffer);
}

float TextPrinter::GetLineHeight() const {
  return pimpl_->GetLineHeight();
}

float TextPrinter::GetStringWidth(const base::StringPiece& input) const {
  return pimpl_->GetStringWidth(input);
}

int TextPrinter::NumCharactersBeforeWrap(const base::StringPiece& input,
                                         const float overscan_ratio) const {
  return pimpl_->NumCharactersBeforeWrap(input, overscan_ratio);
}

TextPrinter::~TextPrinter() {
}

}  // namespace LB
