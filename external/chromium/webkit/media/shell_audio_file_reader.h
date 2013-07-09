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

#ifndef WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_H_
#define WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_H_

#include "base/basictypes.h"

namespace WebKit {
  class WebAudioBus;
}

namespace webkit_media {

// Helper class, provides an abstract interface to different audio file
// reader classes used in DecodeAudioFileData().
class ShellAudioFileReader {
 public:
  virtual ~ShellAudioFileReader() {}
  // Returns a type-specific reader object that implements the following
  // abstract interface, or NULL if no such reader is available.
  // Caller is responsible for deleting this object.
  // |data| is a non-owning read-only pointer.
  static ShellAudioFileReader* Create(const uint8* data, size_t data_size);
  // The Create() method is designed to quickly check the provided data
  // and return a reader that supports the implied format. Call this method
  // next. If it returns true all other methods should return valid data.
  // If false this indicates a parsing error and the provided data are not
  // valid.
  virtual bool parse_config() = 0;
  // returns the number of channels in the data souce
  virtual size_t channels() = 0;
  // returns the sample rate of the provided data
  virtual double sample_rate() = 0;
  // returns the number of frames of audio samnples in the data
  virtual size_t number_of_frames() = 0;
  // read the data and populate the provided bus with the result.
  // Returns the number of frames that were actually read.
  virtual size_t read(WebKit::WebAudioBus* bus) = 0;
};

}  // namespace webkit_media

#endif  // WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_H_

