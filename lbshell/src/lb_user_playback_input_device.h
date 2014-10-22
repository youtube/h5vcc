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
#ifndef SRC_LB_USER_PLAYBACK_INPUT_DEVICE_H_
#define SRC_LB_USER_PLAYBACK_INPUT_DEVICE_H_

#include "lb_user_input_device.h"

#include <stdio.h>

#include <string>

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
class LBInputRecorder {
 public:
  explicit LBInputRecorder(const std::string& output_file_name);
  void WriteEvent(int32_t type, int32_t key_code, int16_t char_code,
                  int32_t modifiers);
  ~LBInputRecorder();

 private:
  void CloseFile();
  base::Time recording_start_time_;
  FILE* recording_file_;
};

class LBPlaybackInputDevice : public LBUserInputDevice {
 public:
  LBPlaybackInputDevice(LBWebViewHost* view,
                        const std::string& input_file_name,
                        bool repeat);
  virtual ~LBPlaybackInputDevice();
  virtual void Poll() OVERRIDE;
  bool PlaybackComplete() { return playback_file_ == NULL; }

 protected:
  void ReadNextEvent();
  void CloseFile();
  bool repeat_;
  FILE* playback_file_;
  base::Time playback_start_time_;
  base::TimeDelta next_event_time_;
  int32_t next_event_type_;
  int32_t next_event_key_code_;
  int16_t next_event_char_code_;
  int32_t next_event_modifiers_;
};
#endif  // defined(__LB_SHELL__ENABLE_CONSOLE__)

#endif  // SRC_LB_USER_PLAYBACK_INPUT_DEVICE_H_
