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
// Provides a abstract OpenGL interface to WebKit/Chromium.  In Chromium proper
// this object consumes commands from the layout engine for delegation to the
// renderer which is running in another process.  It is also assumed that one
// of these will be created for every page, tab, WebGL context, and accelerated
// canvas.  Since Leanback HTML5 doesn't use an accelerated canvas and this app
// is designed to render a single page in a single process this object
// delegates its commands directly to OpenGL and actually is configured to
// render directly into the device framebuffer, to eliminate expensive copy
// operations.  When prepareTexture() is called this object actually is telling
// the hardware to swap this texture from the back buffer to the front.

#ifndef _LB_WEB_GRAPHICS_CONTEXT_IMPL_H_
#define _LB_WEB_GRAPHICS_CONTEXT_IMPL_H_

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#ifndef GLX_GLXEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <semaphore.h>

#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebGraphicsContext3D.h"

#include "lb_web_graphics_context_3d.h"

// Avoid including glx.h or X11 headers.
typedef struct __GLXcontextRec *GLXContext;
typedef unsigned long Window;

class LBGraphics;
class LBGraphicsLinux;

class LBWebGraphicsContext3DLinux : public LBWebGraphicsContext3D {
 public:
  explicit LBWebGraphicsContext3DLinux(LBGraphicsLinux* graphics,
                                       int width, int height);
  virtual ~LBWebGraphicsContext3DLinux();

  // Returns true if the pixel buffer for the current frame is ready to
  // be rendered
  bool PixelBufferReady() const;
  // Waits for this pixel buffer to be finished with compositing and then
  // returns its OpenGL handle to the renderer for rendering
  GLuint TakeNextPixelBuffer();

  static const int kNumberOfWebKitPixelBuffers = 2;  // double-buffered

  GRAPHICS_CONTEXT_3D_METHOD_OVERRIDES

  virtual void shaderBinary(WGC3Dsizei n,
                            const WGC3Duint* shaders,
                            WGC3Denum binaryFormat,
                            const void* binary,
                            WGC3Dsizei length) OVERRIDE;

 protected:
  LBGraphicsLinux* graphics_;

  GLXContext gl_context_;
  int current_draw_buffer_;


  // The WebKit thread renders to this framebuffer with webkit_texture
  // attached to it.
  // In prepareTexture(), we copy that texture to a PBO,
  // then in a client can call PixelBufferReadh() to see that the PBO is
  // ready and TakeNextPixelBuffer() to grab it and advance the active
  // PBO
  GLuint framebuffer_;
  GLuint texture_;

  GLuint webkit_pbos_[kNumberOfWebKitPixelBuffers];
  int cur_pbo_;

  // prepareTexture()
  // Synchronization tokens.
  GLsync gl_sync_;

  // WebKit GL state
  WebKit::WebGraphicsContext3D::Attributes attributes_;
  bool has_synthetic_error_;
  unsigned int last_synthetic_error_;

  // Have we set this context to the current one yet?  This is useful
  // to know if we should do any extra setup on the first context
  // activation.
  bool have_set_context_;
};

#endif

