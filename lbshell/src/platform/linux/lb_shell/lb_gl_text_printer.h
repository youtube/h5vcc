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

/*
  The class here will load up a font file from disk and setup all shaders
  and textures needed to render text to the display using OpenGL.  Once
  constructed, it offers most of its functionality through RenderString
  and Printf, which output the desired string to the specified location on
  screen.  It assumes a drawing area of bottom-left/(-1,-1) to
  top-right/(1,1).  It will always output text with z values of 0.

  The file path to the font to use is currently hard coded in the
  constructor of this object, as well as the text height.  The text
  color is also hard coded in the implementation.
*/

#ifndef _LB_GL_TEXT_PRINTER_H_
#define _LB_GL_TEXT_PRINTER_H_

#include <stdio.h>

#include <string>

#include "external/chromium/base/memory/scoped_ptr.h"

class LBGLTextPrinter {
 public:
  LBGLTextPrinter(const std::string& shaders_dir,
                  int screen_width, int screen_height,
                  const std::string& ttf_font_file);
  ~LBGLTextPrinter();

  // Render the given string at the given location on screen using
  // OpenGL.
  void RenderString(float x, float y, const std::string& str);

  // Render the formatted string to the given location on screen.
  // This helper function essentially just does printf formatting and
  // passes the resulting string to RenderString.
  void Printf(float x, float y, const char* fmt, ...);

 private:
  // In order to avoid having to include implementation specific headers
  // such as Freetype, we use a private implementation.
  class Pimpl;
  scoped_ptr<Pimpl> pimpl_;
};

#endif  // _LB_GL_TEXT_PRINTER_H_

