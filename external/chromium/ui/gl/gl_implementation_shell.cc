// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/logging.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_gl_api_proxy_implementation_shell.h"

namespace gfx {

void GetAllowedGLImplementations(std::vector<GLImplementation>* impls) {
  impls->push_back(kGLImplementationEGLGLES2);
  impls->push_back(kGLImplementationDesktopGL);
}

bool InitializeGLBindings(GLImplementation implementation) {
  DCHECK_EQ(GetGLImplementation(), kGLImplementationNone);

  // Setup the link to our OpenGL emulation layer
  g_current_gl_context = new GLApiProxyShell();

  SetGLImplementation(implementation);

  return true;
}

bool InitializeGLExtensionBindings(GLImplementation implementation,
    GLContext* context) {
  return true;
}

void InitializeDebugGLBindings() {
}

void ClearGLBindings() {
  delete g_current_gl_context;
  g_current_gl_context = NULL;

  SetGLImplementation(kGLImplementationNone);
}

}  // namespace gfx
