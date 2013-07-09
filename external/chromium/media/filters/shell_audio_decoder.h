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

#ifndef MEDIA_FILTERS_SHELL_AUDIO_DECODER_H_
#define MEDIA_FILTERS_SHELL_AUDIO_DECODER_H_

#include "media/base/audio_decoder.h"

namespace media {

class MEDIA_EXPORT ShellAudioDecoder : public AudioDecoder {
 public:
  // platform-specific static create and header size methods
  static ShellAudioDecoder* Create(
      const scoped_refptr<base::MessageLoopProxy>& message_loop);

  // AudioDecoder implementation.
  virtual void Initialize(const scoped_refptr<DemuxerStream>& stream,
                          const PipelineStatusCB& status_cb,
                          const StatisticsCB& statistics_cb) = 0;
  virtual void Read(const ReadCB& read_cb) = 0;
  virtual int bits_per_channel() = 0;
  virtual ChannelLayout channel_layout() = 0;
  virtual int samples_per_second() = 0;
  virtual void Reset(const base::Closure& closure) = 0;
};

}  // namespace media

#endif
