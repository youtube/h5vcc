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

#include "skia/ext/bitmap_platform_device.h"
#include "skia/ext/bitmap_platform_device_data.h"
#include "skia/ext/platform_canvas.h"

#include <stdio.h>

namespace skia {

// static
BitmapPlatformDevice* BitmapPlatformDevice::Create(int width, int height,
                                                   bool is_opaque,
                                                   uint8_t* data) {
  SkBitmap bitmap;
  bitmap.setConfig(SkBitmap::kARGB_8888_Config, width, height);
  if (bitmap.allocPixels() != true)
    return NULL;
  bitmap.setIsOpaque(is_opaque);
  bitmap.eraseARGB(0, 0, 0, 0);
  if (data) {
    bitmap.setPixels(data);
  }
  BitmapPlatformDevice* rv = new BitmapPlatformDevice(bitmap, NULL);
  return rv;
}

// static
BitmapPlatformDevice* BitmapPlatformDevice::Create(int width, int height,
                                                   bool is_opaque) {
  return Create(width, height, is_opaque, NULL /* data */);
}

bool BitmapPlatformDevice::IsVectorial() {
  return false;
}

PlatformSurface BitmapPlatformDevice::BeginPlatformPaint() {
  return 0;
}

void BitmapPlatformDevice::EndPlatformPaint() {
}

BitmapPlatformDevice::BitmapPlatformDevice(const SkBitmap& other,
    BitmapPlatformDeviceData* data)
    : SkDevice(other),
      data_(data) {
}

BitmapPlatformDevice::~BitmapPlatformDevice() {
}

// PlatformCanvas impl

SkCanvas* CreatePlatformCanvas(int width, int height, bool is_opaque,
                               uint8_t* data, OnFailureType failureType) {
  skia::RefPtr<SkDevice> dev = skia::AdoptRef(
      BitmapPlatformDevice::Create(width, height, is_opaque, data));
  return CreateCanvas(dev, failureType);
}

} // namespace skia
