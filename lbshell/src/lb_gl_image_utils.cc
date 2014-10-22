/*
 * Copyright 2013 Google Inc. All Rights Reserved.
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

#include "lb_gl_image_utils.h"

#include <vector>

#include "external/chromium/base/file_util.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/third_party/libpng/png.h"

#if defined(__LB_WIIU__)
#include "lb_shell/lb_graphics_wiiu.h"
#elif defined(__LB_PS3__)
#include "lb_shell/lb_graphics_ps3.h"
#elif defined(__LB_PS4__)
#include "lb_shell/lb_graphics_ps4.h"
#else
#include "lb_graphics.h"
#endif

#include "lb_globals.h"
#include "lb_web_graphics_context_3d.h"

namespace LB {

// TODO(aabtop): This code should eventually be replaced to make use of some
// sort of Steel Shader Manager that provides a platform-independent interface.
// As it is, the #ifdefs are invasive and present in more than just this file,
// which is not ideal.
#if defined(__LB_ANDROID__) || defined(__LB_LINUX__) || defined(__LB_XB1__)
const char* kQuadVertexShaderFileName = "vertex_spinner.glsl";
const char* kQuadFragmentShaderFileName = "fragment_quad_drawer.glsl";
#elif defined(__LB_WIIU__)
const char* kQuadVertexShaderFileName = "vertex_spinner.gsh";
const char* kQuadFragmentShaderFileName = "fragment_quad_drawer.gsh";
#elif defined(__LB_PS3__)
const char* kQuadVertexShaderFileName = "vertex_spinner.vpo";
const char* kQuadFragmentShaderFileName = "fragment_quad_drawer.fpo";
#elif defined(__LB_PS4__)
const char* kQuadVertexShaderFileName = "vertex_quad_drawer.sb";
const char* kQuadFragmentShaderFileName = "pixel_quad_drawer.sb";
#elif defined(__LB_XB360__)
// TODO(iffy): Precompile these.
const char* kQuadVertexShaderFileName = "vertex_spinner.glsl";
const char* kQuadFragmentShaderFileName = "fragment_quad_drawer.glsl";
#else
#error Platform not supported yet.
#endif

enum {
  kPositionAttrib = 0,
  kTexcoordAttrib = 1,
};

// Setup a vertex buffer for rendering quads on the screen.  We reuse
// the same vertex array for all quads.
//
//    [-1,1]           [1,1]
//        1 +-----------+ 3
//          |           |
//          |           |
//          |     +     |
//          |           |
//          |           |
//        0 +-----------+ 2
//     [-1,-1]           [1,-1]
//
const Coord kQuadVertsPos[4] = {
  { -1.0f, -1.0f },
  { -1.0f, 1.0f },
  { 1.0f, -1.0f },
  { 1.0f, 1.0f },
};

// Texture coordinates for render, coordinate
// setup is as follows.
//
//           [0,1]            [1,1]
//              0 +-----------+ 2
//                |           |
//                |           |
//                |           |
//                |           |
//                |           |
//              1 +-----------+ 3
//             [0,0]         [1,0]
//
const Coord kQuadVertsTexcoordsDefault[4] = {
  { 0.0f, 1.0f },
  { 0.0f, 0.0f },
  { 1.0f, 1.0f },
  { 1.0f, 0.0f },
};

// An invalid quad, used to represent a flag to use the default quad.
const int kInvalidQuadVbo = -1;

Image2D::Image2D(const std::string& png_filename) {
  FILE* fp = fopen(png_filename.c_str(), "rb");
  DCHECK(fp) << "Unable to open: " << png_filename;
  uint8_t header[8];
  fread(header, 1, 8, fp);
  // make sure we have a valid png header
  DCHECK(!png_sig_cmp(header, 0, 8));
  // setup structures for read
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);
  DCHECK(png);
  png_infop info = png_create_info_struct(png);
  DCHECK(info);
  png_infop end = png_create_info_struct(png);
  // setup error callback to this if block
  if (setjmp(png->jmpbuf)) {
    NOTREACHED() << "libpng returned error reading " << png_filename;
    abort();
  }
  // set up for file i/o
  png_init_io(png, fp);
  // tell png we already read 8 bytes
  png_set_sig_bytes(png, 8);
  // read the image info
  png_read_info(png, info);

  // Transform PNGs into a canonical RGBA form.
  {
    if (png_get_bit_depth(png, info) == 16) {
      png_set_strip_16(png);
    }

    png_byte color = png_get_color_type(png, info);

    if (color == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png, info) < 8) {
      png_set_expand_gray_1_2_4_to_8(png);
    }

    // Convert from grayscale or palette to color.
    if (!(color & PNG_COLOR_MASK_COLOR)) {
      png_set_gray_to_rgb(png);
    } else if (color == PNG_COLOR_TYPE_PALETTE) {
      png_set_palette_to_rgb(png);
    }

    // Add an alpha channel if missing.
    if (!(color & PNG_COLOR_MASK_ALPHA)) {
      png_set_add_alpha(png, 0xff /* opaque */, PNG_FILLER_AFTER);
    }
  }

  // End transformations. Get the updated info, and then verify.
  png_read_update_info(png, info);
  DCHECK_EQ(PNG_COLOR_TYPE_RGBA, png_get_color_type(png, info));
  DCHECK_EQ(8, png_get_bit_depth(png, info));

  width_ = png_get_image_width(png, info);
  height_ = png_get_image_height(png, info);

  // allocate texture memory and set up row pointers for read
  int pitch = width_ * 4;  // Since everything is RGBA at this point.

  data_.resize(pitch * height_);
  std::vector<png_bytep> rows(height_);
  uint8_t* row = &data_[0];
  for (int i = 0; i < height_; ++i) {
    rows[i] = row;
    row += pitch;
  }
  // read png image data
  png_read_image(png, &rows[0]);
  // clean up
  png_read_end(png, end);
  png_destroy_read_struct(&png, &info, &end);
  fclose(fp);
}

int Image2D::ToGLTexture(LBWebGraphicsContext3D* context) {
  // Load the image data in to a texture
  int tex_handle = context->createTexture();
  context->bindTexture(GraphicsContext3D::TEXTURE_2D, tex_handle);
  context->texImage2D(GraphicsContext3D::TEXTURE_2D,
                      0,
                      GraphicsContext3D::RGBA,
                      width_,
                      height_,
                      0,
                      GraphicsContext3D::RGBA,
                      GraphicsContext3D::UNSIGNED_BYTE,
                      &data_[0]);
  context->bindTexture(GraphicsContext3D::TEXTURE_2D, 0);

  return tex_handle;
}

ScopedVertexAttribBinding::ScopedVertexAttribBinding(
    LBWebGraphicsContext3D* context,
    unsigned int attrib_index,
    int size,
    int type,
    bool normalized,
    size_t stride,
    int offset) {
  context_ = context;
  DCHECK(context_);
  attrib_index_ = attrib_index;
  context->vertexAttribPointer(attrib_index,
                               size,
                               type,
                               normalized,
                               stride,
                               offset);

  context->enableVertexAttribArray(attrib_index);
}

ScopedVertexAttribBinding::~ScopedVertexAttribBinding() {
  context_->disableVertexAttribArray(attrib_index_);
}

int CreateQuadVertexBuffer(LBWebGraphicsContext3D* context) {
  return CreateQuadVertexBufferTex(context, kQuadVertsTexcoordsDefault);
}

int CreateQuadVertexBufferTex(LBWebGraphicsContext3D* context,
                              const Coord texture_crop_coords[4]) {
  QuadVertex verts[4];
  for (int i = 0; i < 4; ++i) {
    verts[i].pos = kQuadVertsPos[i];
    verts[i].texCoord = texture_crop_coords[i];
  }

  int quad_vbo = context->createBuffer();
  context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, quad_vbo);
  context->bufferData(GraphicsContext3D::ARRAY_BUFFER, sizeof(verts), verts,
                      GraphicsContext3D::STATIC_DRAW);

  return quad_vbo;
}

QuadDrawer::QuadDrawer(LBGraphics* graphics, LBWebGraphicsContext3D* context) {
  graphics_ = graphics;
  context_ = context;
  initialized_ = false;
  offset_uniform_ = -1;
  scale_uniform_ = -1;
  rotation_uniform_ = -1;
  color_mult_uniform_ = -1;
  quad_program_ = -1;
  quad_vbo_ = -1;
}

QuadDrawer::~QuadDrawer() {
  if (initialized_) {
    context_->deleteBuffer(quad_vbo_);
  }
}

void QuadDrawer::DrawQuad(int texture_handle,
                          float offset_x, float offset_y,
                          float scale_x, float scale_y,
                          float rotation_ccw) {
  DrawQuadInternal(texture_handle, offset_x, offset_y, scale_x, scale_y,
                   rotation_ccw,
                   kInvalidQuadVbo,
                   1.0f, 1.0f, 1.0f, 1.0f);
}

void QuadDrawer::DrawQuadTex(int texture_handle,
                             float offset_x, float offset_y,
                             float scale_x, float scale_y,
                             float rotation_ccw,
                             const Coord texture_crop_coords[4]) {
  DCHECK_NE(reinterpret_cast<const void*>(&texture_crop_coords),
            reinterpret_cast<const void*>(&kQuadVertsTexcoordsDefault))
      << "Just call DrawQuad instead.";

  int override_quad_vbo = CreateQuadVertexBufferTex(context_,
                                                    texture_crop_coords);
  DrawQuadInternal(texture_handle, offset_x, offset_y, scale_x, scale_y,
                   rotation_ccw,
                   override_quad_vbo,
                   1.0f, 1.0f, 1.0f, 1.0f);
  context_->deleteBuffer(override_quad_vbo);
}

void QuadDrawer::DrawQuadColorMult(int texture_handle,
                                   float offset_x, float offset_y,
                                   float scale_x, float scale_y,
                                   float rotation_ccw,
                                   float color_mult_r, float color_mult_g,
                                   float color_mult_b, float color_mult_a) {
  DrawQuadInternal(texture_handle, offset_x, offset_y, scale_x, scale_y,
                   rotation_ccw,
                   kInvalidQuadVbo,
                   color_mult_r, color_mult_g, color_mult_b, color_mult_a);
}

void QuadDrawer::DrawQuadInternal(int texture_handle,
                                  float offset_x, float offset_y,
                                  float scale_x, float scale_y,
                                  float rotation_ccw,
                                  int override_quad_vbo,
                                  float color_mult_r, float color_mult_g,
                                  float color_mult_b, float color_mult_a) {
  if (!initialized_) {
    initialized_ = true;
    quad_vbo_ = CreateQuadVertexBuffer(context_);
    InitializeShaders();
  }

  context_->useProgram(quad_program_);

  int quad_vbo = kInvalidQuadVbo == override_quad_vbo ? quad_vbo_
                                                      : override_quad_vbo;
  context_->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, quad_vbo);
  ScopedVertexAttribBinding position_binding(
      context_,
      kPositionAttrib, 2, GraphicsContext3D::FLOAT, false, sizeof(QuadVertex),
      offsetof(QuadVertex, pos));
  ScopedVertexAttribBinding texcoord_binding(
      context_,
      kTexcoordAttrib, 2, GraphicsContext3D::FLOAT, false, sizeof(QuadVertex),
      offsetof(QuadVertex, texCoord));

  float offset_array[2];
  offset_array[0] = offset_x;
  offset_array[1] = offset_y;
  float scale_array[2];
  scale_array[0] = scale_x;
  scale_array[1] = scale_y;

  context_->uniform2fv(offset_uniform_, 1, offset_array);
  context_->uniform2fv(scale_uniform_, 1, scale_array);
  context_->uniform1f(rotation_uniform_, rotation_ccw);
  context_->uniform4f(color_mult_uniform_,
                      color_mult_r, color_mult_g,
                      color_mult_b, color_mult_a);

  context_->activeTexture(GraphicsContext3D::TEXTURE0);
  context_->bindTexture(GraphicsContext3D::TEXTURE_2D, texture_handle);

  context_->texParameteri(GraphicsContext3D::TEXTURE_2D,
                          GraphicsContext3D::TEXTURE_MIN_FILTER,
                          GraphicsContext3D::LINEAR);
  context_->texParameteri(GraphicsContext3D::TEXTURE_2D,
                          GraphicsContext3D::TEXTURE_MAG_FILTER,
                          GraphicsContext3D::LINEAR);
  context_->texParameteri(GraphicsContext3D::TEXTURE_2D,
                          GraphicsContext3D::TEXTURE_WRAP_S,
                          GraphicsContext3D::CLAMP_TO_EDGE);
  context_->texParameteri(GraphicsContext3D::TEXTURE_2D,
                          GraphicsContext3D::TEXTURE_WRAP_T,
                          GraphicsContext3D::CLAMP_TO_EDGE);

  context_->drawArrays(GraphicsContext3D::TRIANGLE_STRIP, 0, 4);
}

void QuadDrawer::InitializeShaders() {
  quad_program_ = context_->createProgram();

  unsigned int vs = context_->createShader(GraphicsContext3D::VERTEX_SHADER);
#if defined(__LB_WIIU__)
  LBGraphicsWiiU::ShaderBinary vs_binary =
      static_cast<LBGraphicsWiiU*>(graphics_)->GetShaderBinaryFromFilename(
          kQuadVertexShaderFileName);
  context_->shaderBinary(1, &vs, 0, vs_binary.data, vs_binary.length);
#elif defined(__LB_PS3__)
  LBGraphicsPS3::ShaderBinary vs_binary =
      static_cast<LBGraphicsPS3*>(graphics_)->GetShaderBinaryFromFilename(
          kQuadVertexShaderFileName);
  context_->shaderBinary(1, &vs, 0, vs_binary.data, vs_binary.length);
#elif defined(__LB_PS4__)
  LB::ShaderBinaryManager::ShaderBinary vs_binary =
      static_cast<LBGraphicsPS4*>(graphics_)->shader_manager()->
          GetShaderBinaryFromFilename(kQuadVertexShaderFileName);
  context_->shaderBinary(1, &vs, 0, vs_binary.data, vs_binary.size);
#else
  {
    std::string shader_source;
    const std::string game_content_path(GetGlobalsPtr()->game_content_path);
    file_util::ReadFileToString(
        FilePath(game_content_path + "/shaders/" + kQuadVertexShaderFileName),
        &shader_source);
    context_->shaderSource(vs, &shader_source[0]);
  }
#endif
  context_->compileShader(vs);

  unsigned int fs = context_->createShader(GraphicsContext3D::FRAGMENT_SHADER);
#if defined(__LB_WIIU__)
  LBGraphicsWiiU::ShaderBinary fs_binary =
      static_cast<LBGraphicsWiiU*>(graphics_)->GetShaderBinaryFromFilename(
          kQuadFragmentShaderFileName);
  context_->shaderBinary(1, &fs, 0, fs_binary.data, fs_binary.length);
#elif defined(__LB_PS3__)
  LBGraphicsPS3::ShaderBinary fs_binary =
      static_cast<LBGraphicsPS3*>(graphics_)->GetShaderBinaryFromFilename(
          kQuadFragmentShaderFileName);
  context_->shaderBinary(1, &fs, 0, fs_binary.data, fs_binary.length);
#elif defined(__LB_PS4__)
  LB::ShaderBinaryManager::ShaderBinary fs_binary =
      static_cast<LBGraphicsPS4*>(graphics_)->shader_manager()->
          GetShaderBinaryFromFilename(kQuadFragmentShaderFileName);
  context_->shaderBinary(1, &fs, 0, fs_binary.data, fs_binary.size);
#else
  {
    std::string shader_source;
    const std::string game_content_path(GetGlobalsPtr()->game_content_path);
    file_util::ReadFileToString(
        FilePath(game_content_path + "/shaders/" + kQuadFragmentShaderFileName),
        &shader_source);
    context_->shaderSource(fs, &shader_source[0]);
  }
#endif
  context_->compileShader(fs);

  context_->attachShader(quad_program_, vs);
  context_->attachShader(quad_program_, fs);

  context_->bindAttribLocation(quad_program_,
                               kPositionAttrib,
                               "a_position");
  context_->bindAttribLocation(quad_program_,
                               kTexcoordAttrib,
                               "a_texCoord");

  context_->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, quad_vbo_);
  ScopedVertexAttribBinding position_binding(
    context_,
    kPositionAttrib, 2, GraphicsContext3D::FLOAT, false, sizeof(QuadVertex),
    offsetof(QuadVertex, pos));
  ScopedVertexAttribBinding texcoord_binding(
    context_,
    kTexcoordAttrib, 2, GraphicsContext3D::FLOAT, false, sizeof(QuadVertex),
    offsetof(QuadVertex, texCoord));

  context_->linkProgram(quad_program_);

  offset_uniform_ = context_->getUniformLocation(quad_program_, "offset");
  scale_uniform_ = context_->getUniformLocation(quad_program_, "scale");
  rotation_uniform_ = context_->getUniformLocation(quad_program_, "rotation");
  color_mult_uniform_ =
      context_->getUniformLocation(quad_program_, "color_mult");
  int ref_tex_location = context_->getUniformLocation(quad_program_, "reftex");

  context_->useProgram(quad_program_);
  context_->uniform1i(ref_tex_location, 0);
  context_->useProgram(0);
}

}  // namespace LB
