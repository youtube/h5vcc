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

#include "webkit/media/filter_helpers.h"

#include "base/bind.h"
#include "media/base/filter_collection.h"
#include "media/filters/chunk_demuxer.h"
#include "media/filters/dummy_demuxer.h"
#include "media/filters/shell_audio_decoder.h"
#include "media/filters/shell_demuxer.h"
#include "media/filters/shell_video_decoder.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebURL.h"
#include "webkit/media/media_stream_client.h"

namespace webkit_media {

// Constructs and adds the default audio/video decoders to |filter_collection|.
// Note that decoders in the |filter_collection| are ordered. The first
// audio/video decoder in the |filter_collection| that supports the input
// audio/video stream will be selected as the audio/video decoder in the media
// pipeline. This is done by trying to initialize the decoder with the input
// stream. Some decoder may only accept certain types of streams.
static void AddDefaultDecodersToCollection(
    const scoped_refptr<base::MessageLoopProxy>& message_loop,
    media::FilterCollection* filter_collection) {
  filter_collection->GetAudioDecoders()->push_back(
      media::ShellAudioDecoder::Create(message_loop));
  filter_collection->GetVideoDecoders()->push_back(
      media::ShellVideoDecoder::Create(message_loop));
}

bool BuildMediaStreamCollection(
    const WebKit::WebURL& url,
    MediaStreamClient* client,
    const scoped_refptr<base::MessageLoopProxy>& message_loop,
    media::FilterCollection* filter_collection) {
  if (!client)
    return false;

  scoped_refptr<media::VideoDecoder> video_decoder = client->GetVideoDecoder(
      url, message_loop);
  if (!video_decoder)
    return false;

  // Remove all other decoders and just use the MediaStream one.
  // NOTE: http://crbug.com/110800 is about replacing this ad-hockery with
  // something more designed.
  filter_collection->GetVideoDecoders()->clear();
  filter_collection->GetVideoDecoders()->push_back(video_decoder);

  filter_collection->SetDemuxer(new media::DummyDemuxer(true, false));

  return true;
}

void BuildMediaSourceCollection(
    const scoped_refptr<media::ChunkDemuxer>& demuxer,
    const scoped_refptr<base::MessageLoopProxy>& message_loop,
    media::FilterCollection* filter_collection) {
  DCHECK(demuxer);
  filter_collection->SetDemuxer(demuxer);

  // Remove GPUVideoDecoder until it supports codec config changes.
  // TODO(acolwell): Remove this once http://crbug.com/151045 is fixed.
  DCHECK_LE(filter_collection->GetVideoDecoders()->size(), 1u);
  filter_collection->GetVideoDecoders()->clear();

  AddDefaultDecodersToCollection(message_loop, filter_collection);
}

void BuildDefaultCollection(
    const scoped_refptr<media::DataSource>& data_source,
    const scoped_refptr<base::MessageLoopProxy>& message_loop,
    media::FilterCollection* filter_collection) {
  filter_collection->SetDemuxer(new media::ShellDemuxer(
      message_loop, data_source));

  AddDefaultDecodersToCollection(message_loop, filter_collection);
}

}  // webkit_media
