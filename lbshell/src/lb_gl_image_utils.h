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

#ifndef SRC_LB_GL_IMAGE_UTILS_H_
#define SRC_LB_GL_IMAGE_UTILS_H_

#include <string>
#include <vector>

class LBGraphics;
class LBWebGraphicsContext3D;

namespace LB {

class Image2D {
 public:
  // Given the filename of a PNG file, the PNG will be loaded from disk
  explicit Image2D(const std::string& png_filename);

  // Returns a handle to a newly created OpenGL texture object containing
  // the contents of this 2D image pixel data.
  int ToGLTexture(LBWebGraphicsContext3D* context);

  int width() const { return width_; }
  int height() const { return height_; }

 private:
  std::vector<unsigned char> data_;
  int width_;
  int height_;
};

int PNGFile2GLTexture(const std::string& png_filename,
                      LBWebGraphicsContext3D* context);

struct Coord {
  float x;
  float y;
};

struct QuadVertex {
  Coord pos;
  Coord texCoord;
};

int CreateQuadVertexBuffer(LBWebGraphicsContext3D* context);

int CreateQuadVertexBufferTex(LBWebGraphicsContext3D* context,
                              const Coord texture_crop_coords[4]);

// Creates and enables an OpenGL vertex attribute binding that is
// disabled when this object goes out of scope.
class ScopedVertexAttribBinding {
 public:
  ScopedVertexAttribBinding(LBWebGraphicsContext3D* context,
                            unsigned int attrib_index,
                            int size,
                            int type,
                            bool normalized,
                            size_t stride,
                            int offset);
  ~ScopedVertexAttribBinding();
 private:
  LBWebGraphicsContext3D* context_;
  unsigned int attrib_index_;
};

// Once constructed, this object can be used to draw quads to the screen
// using the given context.
class QuadDrawer {
 public:
  QuadDrawer(LBGraphics* graphics, LBWebGraphicsContext3D* context);
  ~QuadDrawer();

  // Draws a textured quad centered at the specified offset.  The vertices used
  // are the signed unit square (-1 to 1 along each dimension).  The size of the
  // quad can be adjusted by setting the scale.  Rotation around the center
  // of the quad can also be specified.
  void DrawQuad(int texture_handle,
                float offset_x, float offset_y,
                float scale_x, float scale_y,
                float rotation_ccw);

  // Draws a quad with the given cropped texture.  The texture cropping
  // is specified by texture_crop_coords.
  void DrawQuadTex(int texture_handle,
                   float offset_x, float offset_y,
                   float scale_x, float scale_y,
                   float rotation_ccw,
                   const Coord texture_crop_coords[4]);

  // This method draws a textured quad but point-wise multiplies the color of
  // each fragment by the passed in constant color value.  This can be used
  // to fade alpha or fade to black.
  void DrawQuadColorMult(int texture_handle,
                         float offset_x, float offset_y,
                         float scale_x, float scale_y,
                         float rotation_ccw,
                         float color_mult_r, float color_mult_g,
                         float color_mult_b, float color_mult_a);

 private:
  void InitializeShaders();

  void DrawQuadInternal(int texture_handle,
                        float offset_x, float offset_y,
                        float scale_x, float scale_y,
                        float rotation_ccw,
                        int override_quad_vbo,
                        float color_mult_r, float color_mult_g,
                        float color_mult_b, float color_mult_a);

  LBGraphics* graphics_;
  LBWebGraphicsContext3D* context_;

  bool initialized_;

  int offset_uniform_;
  int scale_uniform_;
  int rotation_uniform_;
  int color_mult_uniform_;
  int quad_program_;
  int quad_vbo_;
};
int CreatePositionTextureVertexBuffer(LBWebGraphicsContext3D* context);
void BindPositionTextureVertexBuffer(int buffer_handle,
                                     LBWebGraphicsContext3D* context);
}  // namespace LB

#endif  // SRC_LB_GL_IMAGE_UTILS_H_
