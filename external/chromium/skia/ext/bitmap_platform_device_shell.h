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

#ifndef SKIA_EXT_BITMAP_PLATFORM_DEVICE_SHELL_H_
#define SKIA_EXT_BITMAP_PLATFORM_DEVICE_SHELL_H_
#pragma once

#include "base/memory/ref_counted.h"
#include "skia/ext/platform_device.h"

namespace skia {

class BitmapPlatformDevice : public PlatformDevice, public SkDevice {
  // an empty class for the shell
  class BitmapPlatformDeviceData;

 public:
  // This object takes ownership of @data.
  BitmapPlatformDevice(const SkBitmap& other, BitmapPlatformDeviceData* data);

  // A stub copy constructor.  Needs to be properly implemented.
  BitmapPlatformDevice(const BitmapPlatformDevice& other);

  virtual ~BitmapPlatformDevice();

  BitmapPlatformDevice& operator=(const BitmapPlatformDevice& other);

  static BitmapPlatformDevice* Create(int width, int height, bool is_opaque);

  // This doesn't take ownership of |data|
  static BitmapPlatformDevice* Create(int width, int height,
                                      bool is_opaque, uint8_t* data);

  // Overridden from PlatformDevice:
  virtual bool IsVectorial();

  virtual PlatformSurface BeginPlatformPaint();
  virtual void EndPlatformPaint();

  // we are using the memory we render to currently as what gets
  // blitted to the screen.  No need to transfer to something else,
  // currently.  Current implementation therefore does nothing.
  virtual void DrawToNativeContext(PlatformSurface context,
    int x, int y, const PlatformRect * src_rect) { }

 private:
  BitmapPlatformDeviceData * data_;
};

}  // namespace skia

#endif  // SKIA_EXT_BITMAP_PLATFORM_DEVICE_SHELL_H_
