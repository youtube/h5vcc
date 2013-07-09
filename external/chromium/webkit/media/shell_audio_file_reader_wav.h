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

#ifndef WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_WAV_H_
#define WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_WAV_H_

#include "base/compiler_specific.h"
#include "webkit/media/shell_audio_file_reader.h"

namespace webkit_media {

class ShellAudioFileReaderWAV : public ShellAudioFileReader {
 public:
  static ShellAudioFileReader* Create(const uint8* data, size_t data_size);

  // ShellAudioFileReader interface
  virtual bool parse_config() OVERRIDE;
  virtual size_t channels() OVERRIDE;
  virtual double sample_rate() OVERRIDE;
  virtual size_t number_of_frames() OVERRIDE;
  virtual size_t read(WebKit::WebAudioBus* bus) OVERRIDE;

 private:
  ShellAudioFileReaderWAV(const uint8* riff_data, uint32 riff_chunk_size);
  virtual ~ShellAudioFileReaderWAV();
  bool ParseWAV_fmt(uint32 offset, uint32 size);
  bool ParseWAV_data(uint32 offset, uint32 size);

  const uint8* riff_data_;
  uint32 riff_chunk_size_;
  bool config_parsed_;
  const uint8* data_samples_;
  size_t channels_;
  double sample_rate_;
  bool is_float_;
  size_t number_of_frames_;
};

}  // namespace webkit_media

#endif  // WEBKIT_MEDIA_SHELL_AUDIO_FILE_READER_WAV_H
