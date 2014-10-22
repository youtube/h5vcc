// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_context.h"

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "chromium/ui/gl/gl_context_impl_shell.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"

namespace gfx {

class GLShareGroup;

scoped_refptr<GLContext> GLContext::CreateGLContext(
    GLShareGroup* share_group,
    GLSurface* compatible_surface,
    GpuPreference gpu_preference) {
  TRACE_EVENT0("gpu", "GLContext::CreateGLContext");

  scoped_refptr<GLContext> context(new GLContextShell(share_group));
  if (!context->Initialize(compatible_surface, gpu_preference))
    return NULL;

  return context;
}

}  // namespace gfx
