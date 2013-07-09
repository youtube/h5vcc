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
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#ifndef GLX_GLXEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES 1
#endif

#include "lb_opengl_helpers.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/base/platform_file.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>


namespace GLHelpers
{
  const char* ErrorToString(GLenum err) {
    switch (err) {
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
      return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
      return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
    default:
      return "Unknown";
    }
  }


  // Check if the previous call to glCompileShader succeeded, and if
  // not, display a fatal error message (i.e. compiler error)
  void CheckShaderCompileStatus(GLuint shader, const std::string& shader_file)
  {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
      GLint log_length;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
      std::string shader_log(log_length + 1, '\0');
      glGetShaderInfoLog(shader, log_length, NULL, &shader_log[0]);
      shader_log[log_length] = '\0';
      DLOG(FATAL) << "Shader compile failed for " << shader_file <<
        ": " << shader_log;
    }
  }

  // Utility function to open a file and read its entire contents
  // in to a std::vector<char> and return it.  Useful for reading
  // shader source files.
  std::vector<char> ReadEntireFile(const std::string& path) {
    base::PlatformFileError e;
    base::PlatformFile plat_file = base::CreatePlatformFile(
      FilePath(path),
      base::PLATFORM_FILE_OPEN | base::PLATFORM_FILE_READ,
      NULL,
      &e);
    DCHECK_EQ(e, base::PLATFORM_FILE_OK);

    base::PlatformFileInfo file_info;
    bool result = base::GetPlatformFileInfo(plat_file, &file_info);
    DCHECK(result);

    std::vector<char> fileContents(file_info.size+1, '\0');
    int read_bytes = base::ReadPlatformFile(plat_file, 0, &fileContents[0],
                                            file_info.size);
    DCHECK_EQ(read_bytes, file_info.size);
    fileContents[file_info.size] = '\0';

    base::ClosePlatformFile(plat_file);

    return fileContents;
  }

}
