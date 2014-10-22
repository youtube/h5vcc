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
#include <malloc.h>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "media/audio/shell_audio_streamer.h"
#include "media/base/shell_media_platform.h"

namespace {

class ShellMediaPlatformLinux : public media::ShellMediaPlatform {
 public:
  ShellMediaPlatformLinux() {
    shell_buffer_space_ = static_cast<uint8*>(
        memalign(GetShellBufferSpaceAlignment(), GetShellBufferSpaceSize()));
    DCHECK(shell_buffer_space_);
  }

  ~ShellMediaPlatformLinux() {
    free(shell_buffer_space_);
  }

 private:
  virtual size_t GetShellBufferSpaceSize() const OVERRIDE {
    return 128 * 1024 * 1024;
  }
  virtual size_t GetShellBufferSpaceAlignment() const OVERRIDE {
    return 16;
  }
  virtual uint8* GetShellBufferSpace() const OVERRIDE {
    return shell_buffer_space_;
  }
  virtual size_t GetSourceBufferStreamAudioMemoryLimit() const OVERRIDE {
    return 3 * 1024 * 1024;
  }
  virtual size_t GetSourceBufferStreamVideoMemoryLimit() const OVERRIDE {
    return 30 * 1024 * 1024;
  }
  virtual void InternalInitialize() const OVERRIDE {
    // start the audio mixer
    media::ShellAudioStreamer::Initialize();
  }
  virtual void InternalTerminate() const OVERRIDE {
    // stop audio
    media::ShellAudioStreamer::Terminate();
  }

  uint8* shell_buffer_space_;

  DISALLOW_COPY_AND_ASSIGN(ShellMediaPlatformLinux);
};

}  // namespace

REGISTER_SHELL_MEDIA_PLATFORM(ShellMediaPlatformLinux)
