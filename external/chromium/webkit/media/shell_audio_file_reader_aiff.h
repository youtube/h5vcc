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

#ifndef WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_AIFF_H_
#define WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_AIFF_H_

#include "base/compiler_specific.h"
#include "webkit/media/shell_audio_file_reader.h"

namespace webkit_media {

class ShellAudioFileReaderAIFF : public ShellAudioFileReader {
 public:
  static ShellAudioFileReader* Create(const uint8* data, size_t data_size);

  // ShellAudioFileReader interface
  virtual bool parse_config() OVERRIDE;
  virtual size_t channels() OVERRIDE;
  virtual double sample_rate() OVERRIDE;
  virtual size_t number_of_frames() OVERRIDE;
  virtual size_t read(WebKit::WebAudioBus* bus) OVERRIDE;

 private:
  ShellAudioFileReaderAIFF(const uint8* form_data, uint32 form_chunk_size);
  virtual ~ShellAudioFileReaderAIFF();
  bool ParseAIFF_COMM(uint32 offset, uint32 size);
  bool ParseAIFF_FVER(uint32 offset, uint32 size);
  bool ParseAIFF_SSND(uint32 offset, uint32 size);
  // convert a big-endian IEEE 80-bit floating point number to a double.
  bool extendedToDouble(const uint8* extended, double& out_double);

  const uint8* form_data_;
  uint32 form_chunk_size_;
  bool config_parsed_;
  size_t channels_;
  double sample_rate_;
  size_t number_of_frames_;
  float* ssnd_samples_;
};

}  // namespace webkit_media

#endif  // WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_AIFF_H_
