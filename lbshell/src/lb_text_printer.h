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
  constructed, it offers most of its functionality through Print
  and Printf, which output the desired string to the specified location on
  screen.  It assumes a drawing area of bottom-left/(-1,-1) to
  top-right/(1,1).  It will always output text with z values of 0.

  The file path to the font to use is currently hard coded in the
  constructor of this object, as well as the text height.  The text
  color is also hard coded in the implementation.
*/

#ifndef SRC_LB_TEXT_PRINTER_H_
#define SRC_LB_TEXT_PRINTER_H_

#include <stdio.h>

#include <string>

#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/base/string_piece.h"

class LBWebGraphicsContext3D;
class LBGraphics;

namespace LB {

class TextPrinter {
 public:
  TextPrinter(const std::string& shaders_dir,
                  int screen_width, int screen_height,
                  const std::string& ttf_font_file,
                  LBWebGraphicsContext3D* context,
                  LBGraphics* graphics);
  ~TextPrinter();

  // Render the given string at the given location on screen using
  // OpenGL.  The top left corner of the first character will be placed at
  // the given (x,y) coordinate.
  void Print(float x, float y, const base::StringPiece& str) const;

  // Render the formatted string to the given location on screen.
  // This helper function essentially just does printf formatting and
  // passes the resulting string to Print.
  void Printf(float x, float y, const char* fmt, ...) const;

  // Returns the height of a line, in normalized device coordinates (i.e.
  // coordinates where the screen rows range from -1.0 to 1.0).
  float GetLineHeight() const;

  // Returns the width that a line will be advanced by were the given string
  // to be printed to the screen.  Output given in normalized device coordinates
  // (i.e. between -1.0 and 1.0).
  float GetStringWidth(const base::StringPiece& input) const;

  // Returns the number of characters that will render before the string will
  // need to wrap to the next line, given the width of the line in normalized
  // device coordinates
  int NumCharactersBeforeWrap(const base::StringPiece& input,
                              const float width) const;

 private:
  // In order to avoid having to include implementation specific headers
  // such as Freetype, we use a private implementation.
  class Pimpl;
  scoped_ptr<Pimpl> pimpl_;
};

}  // namespace LB

#endif  // SRC_LB_TEXT_PRINTER_H_
