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
#ifndef SRC_LB_WEB_AUDIO_DEVICE_H_
#define SRC_LB_WEB_AUDIO_DEVICE_H_

#include "third_party/WebKit/Source/Platform/chromium/public/WebAudioDevice.h"

class LBWebAudioDevice : public WebKit::WebAudioDevice {
 public:
  // LBWebAudioDeviceImpl will be used in Android, Linux, PS4 and WiiU.
  // XB1, XB360 and PS3 have platform-specific implementations.
  static WebKit::WebAudioDevice* Create(
      size_t buffer,
      unsigned numberOfChannels,
      double sampleRate,
      WebKit::WebAudioDevice::RenderCallback* callback);
  static double GetAudioHardwareSampleRate();
  static size_t GetAudioHardwareBufferSize();
};

#endif  // SRC_LB_WEB_AUDIO_DEVICE_H_
