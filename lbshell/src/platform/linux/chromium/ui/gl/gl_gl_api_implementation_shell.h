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

#ifndef SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_GL_API_IMPLEMENTATION_SHELL_H_
#define SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_GL_API_IMPLEMENTATION_SHELL_H_

#include "lb_shell/lb_gl_headers.h"

// We avoid override here because we want to re-use the interface
// defined in "gl_bindings_api_autogen_gl.h" but since we are not
// deriving any class, the OVERRIDE keywords cause errors.
// See "gl_gl_api_proxy_implementation_shell.h" for an explanation
// of why we're not a derived class.
#if defined(OVERRIDE)
#define OLD_OVERRIDE OVERRIDE
#endif
#undef OVERRIDE
#define OVERRIDE

namespace gfx {

class GLApiShell {
 public:
  GLApiShell();
  virtual ~GLApiShell();

  #include "gl_bindings_api_autogen_gl.h"
};

#undef OVERRIDE
#if defined(OLD_OVERRIDE)
#define OVERRIDE OLD_OVERRIDE
#endif

}  // namespace gfx

#endif  // SRC_PLATFORM_LINUX_CHROMIUM_UI_GL_GL_GL_API_IMPLEMENTATION_SHELL_H_
