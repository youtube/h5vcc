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

#ifndef SRC_LB_ON_SCREEN_CONSOLE_H_
#define SRC_LB_ON_SCREEN_CONSOLE_H_

#include <string>

#include "base/synchronization/lock.h"

#include "lb_console_connection.h"
#include "lb_text_printer.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace LB {

/* The OnScreenConsole class is responsible for rendering console output
 * over multiple lines to the screen, and also rendering the input prompt.
 * It also handles logic for accepting input from the user for a new command.
 */
class OnScreenConsole : public LBConsoleConnection {
 public:
  // The rectangle coordinates define the region that the on screen console
  // will occupy on the screen, in normalized device coordinates (-1.0f to 1.0f)
  OnScreenConsole(TextPrinter* text_printer,
                  float left, float top, float right, float bottom);

  // To support switching graphics contexts
  void SetTextPrinter(TextPrinter* text_printer);

  // LBConsoleConnection:
  virtual void Output(const std::string& output) OVERRIDE {
    OutputPuts(output);
  }
  virtual void Close() OVERRIDE {
    // NO-OP
  }

  // Renders the on-screen console to the screen
  void Render();

  void InputClearAndPuts(const std::string &output);
  void OutputPuts(const std::string &output);
  void ClearInput();
  void ClearOutput();

 private:
  // Adds a string to the console log line ring buffer
  void AddStringToRingBuffer(const base::StringPiece& str);

  // Circular list for output console
  static const int kMaxConsoleLines = 100;
  std::string console_lines_[kMaxConsoleLines];
  int console_lines_end_index_;
  int num_console_lines_;

  TextPrinter* text_printer_;

  // Input console
  std::string input_buffer_;

  // The console may be accessed from multiple threads simultaneously all of
  // which update the console output.  We control for this with console_lock_.
  base::Lock console_lock_;

  // Rectangle attributes definining the region that we should print within
  float left_;
  float top_;
  float right_;
  float bottom_;
};

}  // namespace LB

#endif  // defined(__LB_SHELL__ENABLE_CONSOLE__)

#endif  // SRC_LB_ON_SCREEN_CONSOLE_H_
