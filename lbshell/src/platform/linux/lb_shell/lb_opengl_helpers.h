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
#ifndef _LB_OPENGL_HELPERS_H_
#define _LB_OPENGL_HELPERS_H_

#include <GL/gl.h>

#include <string>
#include <vector>

namespace GLHelpers
{
  const char* ErrorToString(GLenum err);
  void CheckShaderCompileStatus(GLuint shader, const std::string& shader_file);
  std::vector<char> ReadEntireFile(const std::string& path);
}

#if defined(_DEBUG)
#define GLVERIFY() do {                                        \
    GLenum gl_error = glGetError();                            \
    while (gl_error != GL_NO_ERROR) {                          \
      DLOG(ERROR) << GLHelpers::ErrorToString(gl_error);       \
      DCHECK(0);                                               \
      gl_error = glGetError(); }                               \
  } while (false)
#else
#define GLVERIFY()
#endif


#endif

