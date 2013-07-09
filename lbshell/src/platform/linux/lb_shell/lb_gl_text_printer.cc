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

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#ifndef GLX_GLXEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES 1
#endif

#include "lb_gl_text_printer.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

#include <string>
#include <vector>

#include "external/chromium/base/logging.h"
#include "external/chromium/base/platform_file.h"
#include "lb_opengl_helpers.h"

// The text printer is implemented by creating a single OpenGL texture
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
class LBGLTextPrinter::Pimpl {
  public:
  Pimpl(const std::string& shaders_dir,
        int screen_width, int screen_height,
        const std::string& ttf_font_file)
    : shaders_dir_(shaders_dir)
    , screen_width_(screen_width)
    , screen_height_(screen_height) {
    double pt_size = 1000.0;
    int device_hdpi = 100;
    int device_vdpi = 100;

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
    for (int i = 0; i < NumGlyphs; ++i) {
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

    SetupShaders();
    SetupGlyphsVBO();
  }
  ~Pimpl() {
    glDeleteBuffers(NumGlyphs, glyphs_vbo_);
    glDeleteProgram(text_program_);
    glDeleteShader(frag_shader_id_);
    glDeleteShader(vert_shader_id_);
    glDeleteTextures(1, &font_map_.glTexture);
  }

  void RenderString(float x, float y, const std::string& str) {
    glUseProgram(text_program_);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_map_.glTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    float inv_screen_width = 1.0 / static_cast<double>(screen_width_);

    float cur_x_pos = x;
    for (std::string::const_iterator iter = str.begin();
        iter != str.end(); ++iter) {
      char cur_char = *iter;

      // We only support a subset of possible characters, so default
      // to a specific supported character if the actual character we want
      // is not supported.
      if (static_cast<unsigned char>(cur_char) >= NumGlyphs) {
        cur_char = '?';
      }

      // Setup the vertex buffer for the current character
      glBindBuffer(GL_ARRAY_BUFFER, glyphs_vbo_[cur_char]);
      glVertexAttribPointer(kPositionAttrib, 2, GL_FLOAT, GL_FALSE,
            sizeof(GlyphVert), 0);
      glVertexAttribPointer(kTexcoordAttrib, 2, GL_FLOAT, GL_FALSE,
            sizeof(GlyphVert), static_cast<const char*>(0) + sizeof(float)*2);
      glEnableVertexAttribArray(kPositionAttrib);
      glEnableVertexAttribArray(kTexcoordAttrib);

      glUniform2f(glyph_pos_uniform_, cur_x_pos, y);

      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      // Advance our rendering x position
      float glyph_width =
        font_map_.gri[cur_char].pixel_width * inv_screen_width * 2;

      cur_x_pos += font_map_.gri[cur_char].pixel_advance_x * inv_screen_width*2;
    }
  }

  private:
  std::string shaders_dir_;

  int screen_width_;
  int screen_height_;

  typedef std::vector<FT_BitmapGlyph> GlyphList;

  static const int NumGlyphs = 128;

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
    GlyphRenderInfo gri[NumGlyphs];
    int maxHeight;  // The maximum height of all glyphs

    GLuint glTexture;
  };
  FontMap font_map_;  // Information about the font texture and the glyphs in it

  GLuint text_program_;  // The shader program used for rendering text
  GLuint vert_shader_id_;  // The vertex shader used for text rendering
  GLuint frag_shader_id_;  // The fragment shader used for text rendering

  GLuint glyph_pos_uniform_;  // The on-screen glyph position uniform location

  GLuint glyphs_vbo_[NumGlyphs];  // The vertex buffer objects for each glyph

  enum {
    kPositionAttrib,
    kTexcoordAttrib,
  };

  // The structure for each vertex in the vertex buffer array
  struct GlyphVert {
    float vert_pos[2];
    float tex_coord[2];
  };

  // Use FreeType to render a single character in to a bitmap glyph
  FT_BitmapGlyph MakeBitmapGlyphForChar(const FT_Face& face, char ch) const {
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

  FontMap RenderGlyphsToTexture(const std::vector<FT_BitmapGlyph>& glyphs,
                                const std::vector<float>& advances) const {
    // This function will layout all character glyphs in a row, side-by-side,
    // in the same texture.

    // First iterate through the glyphs determining the maximum height and the
    // total width.
    int maxHeight = -1;
    int totalWidth = 0;

    for (GlyphList::const_iterator iter = glyphs.begin();
        iter != glyphs.end(); ++iter) {
      const FT_Bitmap& curBitmap = (*iter)->bitmap;

      maxHeight = (maxHeight > curBitmap.rows ? maxHeight : curBitmap.rows);
      totalWidth += curBitmap.width;
    }


    FontMap ret;
    ret.maxHeight = maxHeight;

    // Create a pixel buffer  with width and height equal to the next power
    // of 2 of the calculated totalWidth and maxHeight.
    int textureWidth = NextPowerOf2(totalWidth);
    int textureHeight = NextPowerOf2(maxHeight);
    std::vector<GLubyte> fontMapBuffer(textureWidth*textureHeight, 0);

    int curStart = 0;
    for (int i = 0; i < glyphs.size(); ++i) {
      const FT_Bitmap& curBitmap = glyphs[i]->bitmap;

      ret.gri[i].tex_start = static_cast<double>(curStart) / textureWidth;

      ret.gri[i].pixel_width = curBitmap.width;
      ret.gri[i].pixel_height = curBitmap.rows;
      ret.gri[i].tex_width =
        static_cast<double>(ret.gri[i].pixel_width)/textureWidth;
      ret.gri[i].tex_height =
        static_cast<double>(ret.gri[i].pixel_height)/textureHeight;

      ret.gri[i].pixel_trans_x = glyphs[i]->left;
      ret.gri[i].pixel_trans_y = glyphs[i]->top - ret.gri[i].pixel_height;

      ret.gri[i].pixel_advance_x = advances[i];

      // Copy the bitmap in to the right place in the font map buffer
      for (int y = 0; y < curBitmap.rows; ++y) {
        for (int x = 0; x < curBitmap.width; ++x) {
          int dstX = x + curStart;
          int dstY = y;
          fontMapBuffer[dstX + dstY*textureWidth] =
            curBitmap.buffer[x + y*curBitmap.width];
        }
      }

      curStart += curBitmap.width;
    }

    // Now create an OpenGL texture with the font map buffer as its data
    glGenTextures(1, &ret.glTexture);
    glBindTexture(GL_TEXTURE_2D, ret.glTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textureWidth, textureHeight,
                 0, GL_RED, GL_UNSIGNED_BYTE, &fontMapBuffer[0]);

    return ret;
  }

  int NextPowerOf2(int i) const {
    int ret = 1;
    while (ret < i) ret *= 2;
    return ret;
  }



  // Attach the source code at the given location to the given shader
  // then compile it and attach the shader to the text program.
  void CompileAndAttachShader(GLuint shader_id,
                              const std::string& shader_filename) {
    std::vector<char> shader_source =
      GLHelpers::ReadEntireFile(shaders_dir_ + shader_filename);

    GLchar* source_glchar = &shader_source[0];
    glShaderSource(shader_id, 1,
                    const_cast<const GLchar**>(&source_glchar), NULL);
    glCompileShader(shader_id);
    GLVERIFY();

    GLHelpers::CheckShaderCompileStatus(shader_id,
                                        shaders_dir_ + shader_filename);

    glAttachShader(text_program_, shader_id);
    GLVERIFY();
  }

  void SetupShaders() {
    text_program_ = glCreateProgram();

    std::string vertex_shader_filename = "vertex_text.glsl";
    std::string fragment_shader_filename = "fragment_text.glsl";

    // Compile the vertex shader
    vert_shader_id_ = glCreateShader(GL_VERTEX_SHADER);
    GLVERIFY();
    CompileAndAttachShader(vert_shader_id_, vertex_shader_filename);

    // Compile the fragment shader
    frag_shader_id_ = glCreateShader(GL_FRAGMENT_SHADER);
    GLVERIFY();
    CompileAndAttachShader(frag_shader_id_, fragment_shader_filename);

    // Pull out the locations variables of interest from the shader
    glBindAttribLocation(text_program_, kPositionAttrib, "a_position");
    GLVERIFY();
    glBindAttribLocation(text_program_, kTexcoordAttrib, "a_texCoord");
    GLVERIFY();

    glLinkProgram(text_program_);
    GLVERIFY();

    glyph_pos_uniform_ = glGetUniformLocation(text_program_, "glyph_pos");
    GLVERIFY();

    // Set the texture in the program to use texture 0
    GLuint ref_tex_uniform = glGetUniformLocation(text_program_, "reftex");
    GLVERIFY();
    glUseProgram(text_program_);
    glUniform1i(ref_tex_uniform, 0);
    glUseProgram(0);
    GLVERIFY();
  }


  // Setup the glyphs vertex buffer object that contains the texture
  // coordinates and vertex positions for each character in the font
  void SetupGlyphsVBO() {
    glGenBuffers(NumGlyphs, glyphs_vbo_);
    GLVERIFY();

    float inv_screen_width = 1.0/static_cast<double>(screen_width_);
    float inv_screen_height = 1.0/static_cast<double>(screen_height_);

    float vert_order[8] = {
      0.0f, 1.0f,
      0.0f, 0.0f,
      1.0f, 1.0f,
      1.0f, 0.0f,
    };

    // Loop through each glyph producing 4 vertices for it based entirely
    // from the already computed GlyphRenderInfo structs in font_map_.
    for (int i = 0; i < NumGlyphs; ++i) {
      const FontMap::GlyphRenderInfo& cur_gri = font_map_.gri[i];
      GlyphVert glyph_verts[4];

      for (int j = 0; j < 4; ++j) {
        GlyphVert& cur_vert = glyph_verts[j];

        // *2 to the width/height because we go from -1 to 1, not 0 to 1
        cur_vert.vert_pos[0] = (cur_gri.pixel_trans_x
          + vert_order[j*2] * cur_gri.pixel_width) * inv_screen_width*2;
        cur_vert.vert_pos[1] = (cur_gri.pixel_trans_y
          + vert_order[j*2+1] * cur_gri.pixel_height) * inv_screen_height*2;

        cur_vert.tex_coord[0] =
          cur_gri.tex_start + vert_order[j*2] * cur_gri.tex_width;
        // Texture y coordinates are flipped from those of device coordinates
        cur_vert.tex_coord[1] = (vert_order[j*2+1] == 0?1:0) *
                                cur_gri.tex_height;
      }

      // Store the computed vertex information for this glyph in
      // an array buffer
      glBindBuffer(GL_ARRAY_BUFFER, glyphs_vbo_[i]);
      GLVERIFY();
      glBufferData(GL_ARRAY_BUFFER,
        sizeof(glyph_verts), glyph_verts, GL_STATIC_DRAW);
      GLVERIFY();
    }
  }
};



// These functions mostly just forward their calls to the PIMPL object
LBGLTextPrinter::LBGLTextPrinter(const std::string& shaders_dir,
                                 int screen_width, int screen_height,
                                 const std::string& ttf_font_file)
    : pimpl_(new Pimpl(shaders_dir,
                       screen_width, screen_height,
                       ttf_font_file)) {
}

void LBGLTextPrinter::RenderString(float x, float y, const std::string& str) {
  pimpl_->RenderString(x, y, str);
}

void LBGLTextPrinter::Printf(float x, float y, const char* fmt, ...) {
  const int kBufferSize = 512;
  char textBuffer[kBufferSize];

  va_list ap;
  va_start(ap, fmt);
    vsprintf(textBuffer, fmt, ap);
  va_end(ap);

  RenderString(x, y, textBuffer);
}

LBGLTextPrinter::~LBGLTextPrinter() {
}

