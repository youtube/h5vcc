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

#include "gl_surface_impl_shell.h"

#include <X11/xpm.h>

#include "base/basictypes.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop.h"
#include "base/process_util.h"
#include "base/synchronization/cancellation_flag.h"
#include "base/synchronization/lock.h"
#include "base/threading/non_thread_safe.h"
#include "base/threading/thread.h"
#include "base/time.h"

namespace gfx {

namespace {

// scoped_ptr functor for XFree(). Use as follows:
//   scoped_ptr_malloc<XVisualInfo, ScopedPtrXFree> foo(...);
// where "XVisualInfo" is any X type that is freed with XFree.
class ScopedPtrXFree {
 public:
  void operator()(void* x) const {
    ::XFree(x);
  }
};

}  // namespace

ViewSurfaceShell::ViewSurfaceShell(Window window,
                                   void* x_display,
                                   void* config) {
  window_ = window;
  x_display_ = static_cast<Display*>(x_display);
  config_ = static_cast<GLXFBConfig>(config);
}

bool ViewSurfaceShell::Initialize() {
  XWindowAttributes attributes;
  if (!XGetWindowAttributes(x_display_, window_, &attributes)) {
    LOG(ERROR) << "XGetWindowAttributes failed for window " << window_ << ".";
    return false;
  }
  size_ = gfx::Size(attributes.width, attributes.height);

  return true;
}

void ViewSurfaceShell::Destroy() {
}

bool ViewSurfaceShell::IsOffscreen() {
  return false;
}

bool ViewSurfaceShell::SwapBuffers() {
  glXSwapBuffers(x_display_, window_);
  return true;
}

gfx::Size ViewSurfaceShell::GetSize() {
  return size_;
}

void* ViewSurfaceShell::GetHandle() {
  return reinterpret_cast<void*>(window_);
}

void* ViewSurfaceShell::GetConfig() {
  return config_;
}

void* ViewSurfaceShell::GetDisplay() {
  return x_display_;
}

ViewSurfaceShell::~ViewSurfaceShell() {
  Destroy();
}



OffscreenSurfaceShell::OffscreenSurfaceShell(const gfx::Size& size,
                                             void* x_display,
                                             void* config) {
  size_ = size;
  x_display_ = static_cast<Display*>(x_display);
  pbuffer_ = 0;
}

bool OffscreenSurfaceShell::Initialize() {
  DCHECK(!pbuffer_);

  static const int config_attributes[] = {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
    GLX_DOUBLEBUFFER, False,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    0
  };
  int num_elements = 0;
  scoped_ptr_malloc<GLXFBConfig, ScopedPtrXFree> configs(
      glXChooseFBConfig(x_display_,
                        DefaultScreen(x_display_),
                        config_attributes,
                        &num_elements));
  if (!configs.get()) {
    LOG(ERROR) << "glXChooseFBConfig failed.";
    return false;
  }
  if (!num_elements) {
    LOG(ERROR) << "glXChooseFBConfig returned 0 elements.";
    return false;
  }

  config_ = configs.get()[0];

  // Very small pbuffer since we plan to actually be rendering to a
  // framebuffer object
  const int pbuffer_attributes[] = {
    GLX_PBUFFER_WIDTH, size_.width(),
    GLX_PBUFFER_HEIGHT, size_.height(),
    0
  };
  pbuffer_ = glXCreatePbuffer(x_display_,
                              config_,
                              pbuffer_attributes);
  if (!pbuffer_) {
    Destroy();
    LOG(ERROR) << "glXCreatePbuffer failed.";
    return false;
  }

  return true;
}

void OffscreenSurfaceShell::Destroy() {
  if (pbuffer_) {
    glXDestroyPbuffer(x_display_, pbuffer_);
    pbuffer_ = 0;
  }

  config_ = NULL;
}

bool OffscreenSurfaceShell::IsOffscreen() {
  return true;
}

bool OffscreenSurfaceShell::SwapBuffers() {
  NOTREACHED() << "Attempted to call SwapBuffers on a pbuffer.";
  return false;
}

gfx::Size OffscreenSurfaceShell::GetSize() {
  return size_;
}

void* OffscreenSurfaceShell::GetHandle() {
  return reinterpret_cast<void*>(pbuffer_);
}

void* OffscreenSurfaceShell::GetConfig() {
  return config_;
}

void* OffscreenSurfaceShell::GetDisplay() {
  return x_display_;
}

OffscreenSurfaceShell::~OffscreenSurfaceShell() {
  Destroy();
}

}  // namespace gfx
