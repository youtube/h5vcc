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

#include "ui/gfx/native_theme_shell.h"

#include "base/basictypes.h"
#include "base/logging.h"

namespace {

const SkColor kInvalidColorIdColor = SkColorSetRGB(255, 0, 128);

// Theme colors returned by GetSystemColor().
const SkColor kBackgroundColor = SkColorSetRGB(0x0, 0x0, 0x0);
// FocusableBorder:
const SkColor kFocusedBorderColor = SkColorSetRGB(0x4D, 0x90, 0xFE);
const SkColor kUnfocusedBorderColor = SkColorSetRGB(0xD9, 0xD9, 0xD9);

// TextButton:
const SkColor kTextButtonBackgroundColor = SkColorSetRGB(0xde, 0xde, 0xde);
const SkColor kTextButtonEnabledColor = SkColorSetRGB(6, 45, 117);
const SkColor kTextButtonDisabledColor = SkColorSetRGB(161, 161, 146);
const SkColor kTextButtonHighlightColor = SkColorSetARGB(200, 255, 255, 255);
const SkColor kTextButtonHoverColor = kTextButtonEnabledColor;

}  // namespace

namespace gfx {

// static
const NativeTheme* NativeTheme::instance() {
  return NativeThemeShell::instance();
}

// static
const NativeThemeShell* NativeThemeShell::instance() {
  CR_DEFINE_STATIC_LOCAL(NativeThemeShell, s_native_theme, ());
  return &s_native_theme;
}

SkColor NativeThemeShell::GetSystemColor(ColorId color_id) const {
  switch (color_id) {
    case kColorId_DialogBackground:
      return kBackgroundColor;

    // FocusableBorder:
    case kColorId_FocusedBorderColor:
      return kFocusedBorderColor;
    case kColorId_UnfocusedBorderColor:
      return kUnfocusedBorderColor;

    // TextButton:
    case kColorId_TextButtonBackgroundColor:
      return kTextButtonBackgroundColor;
    case kColorId_TextButtonEnabledColor:
      return kTextButtonEnabledColor;
    case kColorId_TextButtonDisabledColor:
      return kTextButtonDisabledColor;
    case kColorId_TextButtonHighlightColor:
      return kTextButtonHighlightColor;
    case kColorId_TextButtonHoverColor:
      return kTextButtonHoverColor;

    default:
      NOTREACHED() << "Invalid color_id: " << color_id;
      break;
  }
  return kInvalidColorIdColor;
}

NativeThemeShell::NativeThemeShell() {
}

NativeThemeShell::~NativeThemeShell() {
}

}  // namespace gfx
