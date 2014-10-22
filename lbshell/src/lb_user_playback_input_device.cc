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
#include "lb_user_playback_input_device.h"

#include <fcntl.h>

#include "lb_web_view_host.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

// Prefer a macro so that we can take advantage of compile-time
// format argument verification for sscanf and friends
#define FORMAT_STRING "%" PRIu64 " %u %u %hu %u\n"

LBInputRecorder::LBInputRecorder(const std::string& output_file_name) {
  recording_start_time_ = base::Time::Now();
  recording_file_ = fopen(output_file_name.c_str(), "w");
  DCHECK(recording_file_);
}

void LBInputRecorder::WriteEvent(int32_t type, int32_t key_code,
    int16_t char_code, int32_t modifiers) {
  if (recording_file_) {
    base::TimeDelta offset = base::Time::Now() - recording_start_time_;
    uint64_t offset_in_microseconds = offset.InMicroseconds();
    int bytes_written = fprintf(
        recording_file_,
        FORMAT_STRING,
        offset_in_microseconds,
        type,
        key_code,
        char_code,
        modifiers);
    if (bytes_written < 0) {
      CloseFile();
    }
  }
}

LBInputRecorder::~LBInputRecorder() {
  CloseFile();
}

void LBInputRecorder::CloseFile() {
  if (recording_file_ != NULL) {
    fclose(recording_file_);
  }
  recording_file_ = NULL;
}

LBPlaybackInputDevice::LBPlaybackInputDevice(LBWebViewHost *view,
                                             const std::string& input_file_name,
                                             bool repeat)
    : LBUserInputDevice(view)
    , repeat_(repeat) {
  playback_file_ = fopen(input_file_name.c_str(), "r");
  playback_start_time_ = base::Time::Now();
  ReadNextEvent();
}

void LBPlaybackInputDevice::ReadNextEvent() {
  if (playback_file_ != NULL) {
    bool start_of_file = (ftell(playback_file_) == 0);
    uint64_t offset_in_microseconds;

    char input_record[128];
    int items_read = 0;
    if (fgets(input_record, sizeof(input_record), playback_file_) != NULL) {
      items_read = sscanf(
          input_record,
          FORMAT_STRING,
          &offset_in_microseconds,
          &next_event_type_,
          &next_event_key_code_,
          &next_event_char_code_,
          &next_event_modifiers_);
    }

    if (items_read == 5) {
      // success
      next_event_time_ =
          base::TimeDelta::FromMicroseconds(offset_in_microseconds);
    } else if (feof(playback_file_) && !start_of_file && repeat_) {
      // Only loop upon reaching EOF if the repeat_ flag is set, and we have
      // read at least one valid event from the input stream.

      // back to top of file
      fseek(playback_file_, 0, SEEK_SET);
      // reset start time
      playback_start_time_ = base::Time::Now();
      // and read again
      ReadNextEvent();
    } else {
      // An error occurred, or we reached EOF and should not repeat.
      CloseFile();
    }
  }
}

void LBPlaybackInputDevice::CloseFile() {
  if (playback_file_ != NULL) {
    fclose(playback_file_);
  }
  playback_file_ = NULL;
}

LBPlaybackInputDevice::~LBPlaybackInputDevice() {
  CloseFile();
}

void LBPlaybackInputDevice::Poll() {
  if (playback_file_ != NULL) {
    base::TimeDelta diff = base::Time::Now() - playback_start_time_;
    // if we've passed the next event start time, send the event and read the
    // next one
    if (diff > next_event_time_) {
      view_->SendKeyEvent((WebKit::WebInputEvent::Type)next_event_type_,
          next_event_key_code_,
          next_event_char_code_,
          (WebKit::WebInputEvent::Modifiers)next_event_modifiers_,
          false);
      ReadNextEvent();
    }
  }
}
#endif
