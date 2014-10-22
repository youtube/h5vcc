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

#include "lb_on_screen_console.h"

#include <vector>

#include "base/logging.h"
#include "base/string_tokenizer.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace LB {

namespace {
void DCHECK_WITHIN_INCLUSIVE_RANGE(float x, float a, float b) {
  DCHECK_GE(x, a);
  DCHECK_LE(x, b);
}
}  // namespace

OnScreenConsole::OnScreenConsole(
    TextPrinter* text_printer,
    float left, float top, float right, float bottom) {
  text_printer_ = text_printer;

  DCHECK_WITHIN_INCLUSIVE_RANGE(left, -1.0f, 1.0f);
  DCHECK_WITHIN_INCLUSIVE_RANGE(top, -1.0f, 1.0f);
  DCHECK_WITHIN_INCLUSIVE_RANGE(right, -1.0f, 1.0f);
  DCHECK_WITHIN_INCLUSIVE_RANGE(bottom, -1.0f, 1.0f);
  DCHECK_LE(left, right);
  DCHECK_LE(bottom, top);

  left_ = left;
  top_ = top;
  right_ = right;
  bottom_ = bottom;

  // Set up circular list pointers
  ClearInput();
  ClearOutput();
}

// To support switching graphics contexts
void OnScreenConsole::SetTextPrinter(TextPrinter* text_printer) {
  text_printer_ = text_printer;
}

void OnScreenConsole::Render() {
  base::AutoLock lock(console_lock_);
  DCHECK(text_printer_);

  // Determine the number of console lines that can fit in the output rectangle
  // and render those lines.
  const float kOutputInputLineGap = 0.02f;  // Gap size between output and input
  float line_height = text_printer_->GetLineHeight();
  float output_bottom = bottom_ + kOutputInputLineGap + line_height;
  int lines_in_render_rectangle =
      static_cast<int>((top_ - output_bottom) / line_height);

  DCHECK_LE(num_console_lines_, kMaxConsoleLines);
  int lines_to_render = std::min(num_console_lines_, lines_in_render_rectangle);

  // Construct one big newline separated string that we will output as the
  // entire console output
  std::string output_string;
  int cur_line = console_lines_end_index_ - lines_to_render;
  if (cur_line < 0) cur_line += kMaxConsoleLines;
  do {
    output_string += console_lines_[cur_line];
    output_string += "\n";

    cur_line = (cur_line + 1) % kMaxConsoleLines;
  } while (cur_line != console_lines_end_index_);

  text_printer_->Print(left_, top_, output_string);

  // Output input prompt
  const float kInputIndent = 0.1f;
  text_printer_->Print(left_ + kInputIndent,
                       bottom_ + text_printer_->GetLineHeight(),
                       input_buffer_.c_str());
}

void OnScreenConsole::InputClearAndPuts(const std::string &input) {
  base::AutoLock lock(console_lock_);
  input_buffer_ = input;
}

void OnScreenConsole::AddStringToRingBuffer(const base::StringPiece& str) {
  console_lock_.AssertAcquired();

  console_lines_[console_lines_end_index_] = str.as_string();
  console_lines_end_index_ = (console_lines_end_index_ + 1) % kMaxConsoleLines;

  // If we're not full (and overwriting old entries), increment the number
  // of console lines marked as being used.
  if (num_console_lines_ < kMaxConsoleLines) {
    ++num_console_lines_;
  }
}

void OnScreenConsole::OutputPuts(const std::string &output) {
  base::AutoLock lock(console_lock_);

  // Break up output by explicit newlines and newlines induced by lines that
  // are too long for the screen width.
  StringTokenizer tokenizer(output, "\n");
  // The RETURN_DELIMS flag means that delimiters will be returned as explicit
  // tokens as well as the actual tokens.
  tokenizer.set_options(StringTokenizer::RETURN_DELIMS);
  while (tokenizer.GetNext()) {  // Iterate over each explicit line of text
    base::StringPiece cur_token = tokenizer.token_piece();
    if (tokenizer.token_is_delim()) {
      // Add blank newlines to the ring buffer
      AddStringToRingBuffer("");
      continue;
    }

    do {  // Search for lines that are too long for a single line
      int num_chars = text_printer_->NumCharactersBeforeWrap(cur_token,
                                                             right_ - left_);
      AddStringToRingBuffer(cur_token.substr(0, num_chars));

      cur_token.remove_prefix(num_chars);
    } while (cur_token.length());

    // Read past the next newline character that follows every non-newline
    // token.
    tokenizer.GetNext();
  }
}

void OnScreenConsole::ClearOutput() {
  base::AutoLock lock(console_lock_);
  num_console_lines_ = 0;
  console_lines_end_index_ = 0;
}

void OnScreenConsole::ClearInput() {
  base::AutoLock lock(console_lock_);
  input_buffer_ = ">";
}

}  // namespace LB

#endif  // defined(__LB_SHELL__ENABLE_CONSOLE__)
