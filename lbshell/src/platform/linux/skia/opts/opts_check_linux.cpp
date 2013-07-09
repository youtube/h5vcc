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
#include "third_party/skia/src/core/SkBitmapProcState.h"
#include "third_party/skia/src/core/SkBlitMask.h"
#include "SkBlitRow.h"
#include "SkUtils.h"

void SkBitmapProcState::platformProcs() {
}

SkBlitRow::Proc SkBlitRow::PlatformProcs4444(unsigned flags) {
  return NULL;
}

SkBlitRow::Proc SkBlitRow::PlatformProcs565(unsigned flags) {
  return NULL;
}

SkBlitRow::ColorRectProc PlatformColorRectProcFactory() {
  return NULL;
}

SkBlitRow::ColorProc SkBlitRow::PlatformColorProc() {
  return NULL;
}

SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
  return NULL;
}

SkBlitMask::ColorProc SkBlitMask::PlatformColorProcs(SkBitmap::Config dstConfig,
                                                     SkMask::Format maskFormat,
                                                     SkColor color) {
  return NULL;
}

SkBlitMask::RowProc SkBlitMask::PlatformRowProcs(SkBitmap::Config dstConfig,
                                                 SkMask::Format maskFormat,
                                                 RowFlags flags) {
  return NULL;
}

SkBlitMask::BlitLCD16RowProc SkBlitMask::PlatformBlitRowProcs16(bool isOpaque) {
    return NULL;
}

SkMemset16Proc SkMemset16GetPlatformProc() {
  return NULL;
}

SkMemset32Proc SkMemset32GetPlatformProc() {
  return NULL;
}
