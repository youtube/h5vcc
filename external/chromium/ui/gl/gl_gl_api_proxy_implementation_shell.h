// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_API_IMPLEMENTATION_PROXY_SHELL_H_
#define UI_GL_GL_API_IMPLEMENTATION_PROXY_SHELL_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_export.h"

namespace gfx {

class GLApiShell;

// This proxy class exists because the LBShell class may want to
// cal gl* functions directly.  This is not possible here because
// "gl_bindings.h" includes "gl_bindings_autogen_gl.h" which both
// declares GLApi, which we need, and also overrides all gl* functions
// to instead call the current context GLApi version of those functions.
// Thus, forwarding gl calls to the system OpenGL cannot be done if
// "gl_bindings.h" is included.
class GL_EXPORT GLApiProxyShell : public GLApi {
 public:
  GLApiProxyShell();
  virtual ~GLApiProxyShell();

  #include "gl_bindings_api_autogen_gl.h"

private:
  scoped_ptr<GLApiShell> gl_api_shell_;
};

}

#endif  // UI_GL_GL_API_IMPLEMENTATION_PROXY_SHELL_H_
