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

#include "lb_on_screen_display.h"

#include "external/chromium/base/logging.h"

#include "lb_globals.h"
#include "lb_text_printer.h"
#include "lb_web_graphics_context_3d.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

static const float kOverscanRatio = 0.950f;

namespace LB {

// static
OnScreenDisplay* OnScreenDisplay::instance_ = NULL;

// static
void OnScreenDisplay::Create(LBGraphics* graphics,
                             LBWebGraphicsContext3D* context) {
  DCHECK(!instance_);
  instance_ = new OnScreenDisplay(graphics, context);
}

// static
void OnScreenDisplay::Terminate() {
  OnScreenDisplay* ptr = instance_;
  instance_ = NULL;
  delete ptr;
}

OnScreenDisplay::OnScreenDisplay(LBGraphics* graphics,
                                 LBWebGraphicsContext3D* context)
    : stats_visible_(true)
    , console_visible_(false) {
  graphics_ = graphics;

  SetGraphicsContext(context);

  on_screen_console_.reset(new OnScreenConsole(text_printer_.get(),
                                               -1.0f * kOverscanRatio,
                                               1.0f * kOverscanRatio,
                                               1.0f * kOverscanRatio,
                                               -0.84f));
  heads_up_display_.reset(new HeadsUpDisplay(text_printer_.get(),
                                             -1.0f * kOverscanRatio,
                                             -0.86f,
                                             1.0f * kOverscanRatio,
                                             -1.0f * kOverscanRatio));
}

void OnScreenDisplay::SetGraphicsContext(LBWebGraphicsContext3D* context) {
  if (context) {
    DCHECK(!text_printer_);
    const std::string game_content_path(GetGlobalsPtr()->game_content_path);
    text_printer_.reset(
        new TextPrinter(
            game_content_path + "/shaders/",
            context->width(), context->height(),
            game_content_path + "/fonts/DroidSans.ttf",
            context,
            graphics_));
  } else {
    text_printer_.reset(NULL);
  }

  if (on_screen_console_.get()) {
    on_screen_console_->SetTextPrinter(text_printer_.get());
  }
  if (heads_up_display_.get()) {
    heads_up_display_->SetTextPrinter(text_printer_.get());
  }
}

void OnScreenDisplay::Render() {
  if (!text_printer_)
    return;

  if (stats_visible_) {
    heads_up_display_->Render();
  }
  if (console_visible_) {
    on_screen_console_->Render();
  }
}

}  // namespace LB

#endif  // defined(__LB_SHELL__ENABLE_CONSOLE__)
