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

#ifndef SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_SURFACE_IMPL_SHELL_H_
#define SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_SURFACE_IMPL_SHELL_H_

#include <string>

#include "lb_shell/lb_gl_headers.h"

#include "base/compiler_specific.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_surface.h"

namespace gfx {

// A surface used to render to an onscreen window
class GL_EXPORT ViewSurfaceShell : public GLSurface {
 public:
  explicit ViewSurfaceShell(Window window,
                            void* x_display,
                            void* config);

  // Implement GLSurfaceGLX.
  virtual bool Initialize() OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;
  virtual gfx::Size GetSize() OVERRIDE;
  virtual void* GetHandle() OVERRIDE;
  virtual void* GetConfig() OVERRIDE;
  virtual void* GetDisplay() OVERRIDE;

 protected:
  virtual ~ViewSurfaceShell();

 private:
  gfx::Size size_;
  Display* x_display_;
  Window window_;
  GLXFBConfig config_;

  DISALLOW_COPY_AND_ASSIGN(ViewSurfaceShell);
};

// A surface used to render to an offscreen pbuffer.
class GL_EXPORT OffscreenSurfaceShell : public GLSurface {
 public:
  explicit OffscreenSurfaceShell(const gfx::Size& size,
                                 void* x_display,
                                 void* config);

  // Implement GLSurfaceGLX.
  virtual bool Initialize() OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual bool IsOffscreen() OVERRIDE;
  virtual bool SwapBuffers() OVERRIDE;
  virtual gfx::Size GetSize() OVERRIDE;
  virtual void* GetHandle() OVERRIDE;
  virtual void* GetConfig() OVERRIDE;
  virtual void* GetDisplay() OVERRIDE;

 protected:
  virtual ~OffscreenSurfaceShell();

 private:
  gfx::Size size_;
  Display* x_display_;
  GLXFBConfig config_;
  XID pbuffer_;

  GLuint framebuffer_;
  GLuint texture_;

  DISALLOW_COPY_AND_ASSIGN(OffscreenSurfaceShell);
};

}  // namespace gfx

#endif  // SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_SURFACE_IMPL_SHELL_H_
