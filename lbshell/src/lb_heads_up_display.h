/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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

#ifndef SRC_LB_HEADS_UP_DISPLAY_H_
#define SRC_LB_HEADS_UP_DISPLAY_H_

#include <string>

#include "lb_memory_manager.h"
#include "lb_text_printer.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
namespace LB {

/* The HeadsUpDisplay class is responsible for displaying system values on
 * the screen.  Values may either be extracted from the system by the
 * HeadsUpDisplay object, or extracted from the console value system.
 */
class HeadsUpDisplay {
 public:
  // The rectangle coordinates define the region that the heads up display
  // will occupy on the screen, in normalized device coordinates (-1.0f to 1.0f)
  HeadsUpDisplay(TextPrinter* text_printer,
                 float left, float top, float right, float bottom);

  // To support switching graphics contexts
  void SetTextPrinter(TextPrinter* text_printer);

  void Render();

 private:
  void InsertLineBreaks(std::string* input);

  TextPrinter* text_printer_;

  // Increment by one every time Render() is called
  double last_mem_update_time_;

  // Rectangle attributes definining the region that we should print within
  float left_;
  float top_;
  float right_;
  float bottom_;
};

}  // namespace LB

#endif  // defined(__LB_SHELL__ENABLE_CONSOLE__)

#endif  // SRC_LB_HEADS_UP_DISPLAY_H_
