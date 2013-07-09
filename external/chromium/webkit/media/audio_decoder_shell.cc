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

#include "webkit/media/audio_decoder.h"

#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebAudioBus.h"
#include "webkit/media/shell_audio_file_reader.h"

namespace webkit_media {

bool DecodeAudioFileData(
    WebKit::WebAudioBus* destination_bus,
    const char* data, size_t data_size, double sample_rate) {

  scoped_ptr<ShellAudioFileReader> reader(
      ShellAudioFileReader::Create((const uint8*)data, data_size));

  if (!reader) {
    DLOG(ERROR) << "reader creation failed.";
    return false;
  }
  if (!reader->parse_config()) {
    DLOG(ERROR) << "reader error parsing config data.";
    return false;
  }

  size_t number_of_channels = reader->channels();
  double file_sample_rate = reader->sample_rate();
  size_t number_of_frames = reader->number_of_frames();

  destination_bus->initialize(number_of_channels,
                              number_of_frames,
                              file_sample_rate);

  // read the data directly into the destination bus
  size_t actual_frames = reader->read(destination_bus);

  if (actual_frames != number_of_frames) {
    DCHECK_LE(actual_frames, number_of_frames);
    destination_bus->resizeSmaller(actual_frames);
  }

  return actual_frames > 0;
}

}  // namespace webkit_media
