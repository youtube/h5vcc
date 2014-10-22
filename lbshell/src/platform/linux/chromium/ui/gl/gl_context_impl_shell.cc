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

#include "gl_context_impl_shell.h"

extern "C" {
#include <X11/Xlib.h>
}

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "ui/gl/gl_surface.h"

namespace gfx {

namespace {

GLXDrawable SurfaceToDrawable(GLSurface* surface) {
  return reinterpret_cast<GLXDrawable>(surface->GetHandle());
}

}  // namespace

GLContextShell::GLContextShell(GLShareGroup* share_group)
  : GLContext(share_group),
    context_(NULL),
    display_(NULL) {
}

typedef GLXContext (*glXCreateContextAttribsARBProc)(
    Display*, GLXFBConfig, GLXContext, Bool, const int*);

bool GLContextShell::Initialize(
    GLSurface* compatible_surface, GpuPreference gpu_preference) {
  display_ = static_cast<Display*>(compatible_surface->GetDisplay());

  GLXContext share_handle = static_cast<GLXContext>(
      share_group() ? share_group()->GetHandle() : NULL);

  context_ = glXCreateNewContext(
      display_,
      static_cast<GLXFBConfig>(compatible_surface->GetConfig()),
      GLX_RGBA_TYPE,
      share_handle,
      True);
  DCHECK(context_);

  return true;
}

void GLContextShell::Destroy() {
  if (context_) {
    glXDestroyContext(display_, context_);
    context_ = NULL;
  }
}

bool GLContextShell::MakeCurrent(GLSurface* surface) {
  DCHECK(context_);
  if (IsCurrent(surface))
    return true;

  TRACE_EVENT0("gpu", "GLContextShell::MakeCurrent");
  if (!glXMakeContextCurrent(
      display_,
      SurfaceToDrawable(surface),
      SurfaceToDrawable(surface),
      context_)) {
    LOG(ERROR) << "Couldn't make context current with X drawable.";
    Destroy();
    return false;
  }

  SetCurrent(this, surface);
  if (!InitializeExtensionBindings()) {
    ReleaseCurrent(surface);
    Destroy();
    return false;
  }

  if (!surface->OnMakeCurrent(this)) {
    LOG(ERROR) << "Could not make current.";
    ReleaseCurrent(surface);
    Destroy();
    return false;
  }

  return true;
}

void GLContextShell::ReleaseCurrent(GLSurface* surface) {
  if (!IsCurrent(surface))
    return;

  SetCurrent(NULL, NULL);
  if (!glXMakeContextCurrent(display_, 0, 0, 0))
    LOG(ERROR) << "glXMakeCurrent failed in ReleaseCurrent";
}

bool GLContextShell::IsCurrent(GLSurface* surface) {
  bool native_context_is_current =
      glXGetCurrentContext() == context_;

  // If our context is current then our notion of which GLContext is
  // current must be correct. On the other hand, third-party code
  // using OpenGL might change the current context.
  DCHECK(!native_context_is_current || (GetCurrent() == this));

  if (!native_context_is_current)
    return false;

  if (surface) {
    if (glXGetCurrentDrawable() != SurfaceToDrawable(surface)) {
      return false;
    }
  }

  return true;
}

void* GLContextShell::GetHandle() {
  return context_;
}

void GLContextShell::SetSwapInterval(int interval) {
  DCHECK(IsCurrent(NULL));

  NOTIMPLEMENTED();
}

std::string GLContextShell::GetExtensions() {
  DCHECK(IsCurrent(NULL));

  return GLContext::GetExtensions();
}

bool GLContextShell::GetTotalGpuMemory(size_t* bytes) {
  DCHECK(bytes);
  *bytes = 0;
  if (HasExtension("GL_NVX_gpu_memory_info")) {
    GLint kbytes = 0;
    glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &kbytes);
    *bytes = 1024*kbytes;
    return true;
  }
  return false;
}

bool GLContextShell::WasAllocatedUsingRobustnessExtension() {
  return false;
}

GLContextShell::~GLContextShell() {
  Destroy();
}

}  // namespace gfx
