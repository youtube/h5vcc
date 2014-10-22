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

#ifndef SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_CONTEXT_IMPL_SHELL_H_
#define SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_CONTEXT_IMPL_SHELL_H_

#include "lb_shell/lb_gl_headers.h"

#include "base/compiler_specific.h"
#include "ui/gl/gl_context.h"

namespace gfx {

class GLSurface;

// Encapsulates a GLX OpenGL context.
class GL_EXPORT GLContextShell : public GLContext {
 public:
  explicit GLContextShell(GLShareGroup* share_group);

  // Implement GLContext.
  virtual bool Initialize(
      GLSurface* compatible_surface, GpuPreference gpu_preference) OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual bool MakeCurrent(GLSurface* surface) OVERRIDE;
  virtual void ReleaseCurrent(GLSurface* surface) OVERRIDE;
  virtual bool IsCurrent(GLSurface* surface) OVERRIDE;
  virtual void* GetHandle() OVERRIDE;
  virtual void SetSwapInterval(int interval) OVERRIDE;
  virtual std::string GetExtensions() OVERRIDE;
  virtual bool GetTotalGpuMemory(size_t* bytes) OVERRIDE;
  virtual bool WasAllocatedUsingRobustnessExtension() OVERRIDE;

 protected:
  virtual ~GLContextShell();

 private:
  GLXContext context_;
  Display* display_;

  DISALLOW_COPY_AND_ASSIGN(GLContextShell);
};

}  // namespace gfx

#endif  // SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_CONTEXT_IMPL_SHELL_H_
