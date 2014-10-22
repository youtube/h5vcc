// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_surface.h"

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "chromium/ui/gl/gl_surface_impl_shell.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"

namespace gfx {

bool GLSurface::InitializeOneOffInternal() {
  DCHECK_NE(GetGLImplementation(), kGLImplementationNone);

  return true;
}

namespace {
void* g_view_display = 0;
void* g_view_config = 0;
bool g_initialized_lb_params = false;
}

void GLSurface::InitializeDisplayAndConfig(void* display, void* config) {
  g_view_display = display;
  g_view_config = config;
  g_initialized_lb_params = true;
}

scoped_refptr<GLSurface> GLSurface::CreateViewGLSurface(
    bool software,
    gfx::AcceleratedWidget window) {
  TRACE_EVENT0("gpu", "GLSurface::CreateViewGLSurface");

  DCHECK(g_initialized_lb_params);
  scoped_refptr<GLSurface> surface(new ViewSurfaceShell(window,
                                                        g_view_display,
                                                        g_view_config));
  if (!surface->Initialize())
    return NULL;

  return surface;
}

scoped_refptr<GLSurface> GLSurface::CreateOffscreenGLSurface(
    bool software,
    const gfx::Size& size) {
  TRACE_EVENT0("gpu", "GLSurface::CreateOffscreenGLSurface");

  DCHECK(g_initialized_lb_params);
  scoped_refptr<GLSurface> surface(new OffscreenSurfaceShell(size,
                                                             g_view_display,
                                                             g_view_config));
  if (!surface->Initialize())
    return NULL;

  return surface;
}

}  // namespace gfx
