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

#ifndef SRC_LB_ON_SCREEN_DISPLAY_H_
#define SRC_LB_ON_SCREEN_DISPLAY_H_

#include <string>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"

#include "lb_heads_up_display.h"
#include "lb_on_screen_console.h"
#include "lb_text_printer.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

class LBTextPrinter;

namespace LB {

/* The OnScreenDisplay class is responsible for managing all Steel on screen
 * components, such as the heads up display and the on screen console.
 */
class OnScreenDisplay {
 public:
  static void Create(LBGraphics* graphics, LBWebGraphicsContext3D* context);
  static void Terminate();
  static OnScreenDisplay* GetPtr() { return instance_; }

  void Render();

  OnScreenConsole* GetConsole() const { return on_screen_console_.get(); }
  HeadsUpDisplay* GetHUD() const { return heads_up_display_.get(); }

  void SetGraphicsContext(LBWebGraphicsContext3D* context);

  void HideStats() { stats_visible_ = false; }
  void ShowStats() { stats_visible_ = true; }
  void ToggleStats() { stats_visible_ = !stats_visible_; }
  bool StatsVisible() const { return stats_visible_; }

  void HideConsole() { console_visible_ = false; }
  void ShowConsole() { console_visible_ = true; }
  void ToggleConsole() { console_visible_ = !console_visible_; }
  bool ConsoleVisible() const { return console_visible_; }

 private:
  OnScreenDisplay(LBGraphics* graphics, LBWebGraphicsContext3D* context);

  LBGraphics* graphics_;

  scoped_ptr<TextPrinter> text_printer_;
  scoped_ptr<OnScreenConsole> on_screen_console_;
  scoped_ptr<HeadsUpDisplay> heads_up_display_;

  bool stats_visible_;
  bool console_visible_;

  static OnScreenDisplay* instance_;
};
}  // namespace LB

#endif  // defined(__LB_SHELL__ENABLE_CONSOLE__)

#endif  // SRC_LB_ON_SCREEN_DISPLAY_H_
