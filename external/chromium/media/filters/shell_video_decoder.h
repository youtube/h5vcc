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

#ifndef MEDIA_FILTERS_SHELL_VIDEO_DECODER_H_
#define MEDIA_FILTERS_SHELL_VIDEO_DECODER_H_

#include "base/callback.h"
#include "media/base/decryptor.h"
#include "media/base/message_loop_factory.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"

namespace media {

class MEDIA_EXPORT ShellVideoDecoder : public VideoDecoder {
 public:
  // platform-specific factory method
  static ShellVideoDecoder* Create(
      const scoped_refptr<base::MessageLoopProxy>& message_loop);

  // VideoDecoder implementation.
  virtual void Initialize(const scoped_refptr<DemuxerStream>& stream,
                          const PipelineStatusCB& status_cb,
                          const StatisticsCB& statistics_cb) = 0;
  virtual void Read(const ReadCB& read_cb) = 0;
  virtual void Reset(const base::Closure& closure) = 0;
  virtual void Stop(const base::Closure& closure) = 0;
};

}  // namespace media

#endif
