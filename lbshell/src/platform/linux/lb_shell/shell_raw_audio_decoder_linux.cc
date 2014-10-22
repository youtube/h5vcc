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

#include <memory.h>

#include "lb_ffmpeg.h"
#include "base/bind.h"
#include "base/logging.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/audio_timestamp_helper.h"
#include "media/base/shell_buffer_factory.h"
#include "media/filters/shell_audio_decoder_impl.h"

using media::AudioTimestampHelper;
using media::ShellBuffer;

namespace {

struct QueuedAudioBuffer {
//  AudioDecoder::Status status;  // status is used to represent decode errors
  scoped_refptr<media::ShellBuffer> buffer;
};

void AllocateCallback(scoped_refptr<media::ShellBuffer>* target,
                      scoped_refptr<media::ShellBuffer> allocated) {
  *target = allocated;
}

bool IsEndOfStream(int result, int decoded_size,
                   scoped_refptr<media::ShellBuffer> buffer) {
  return result == 0 && decoded_size == 0 && buffer->IsEndOfStream();
}

class ShellRawAudioDecoderLinux : public media::ShellRawAudioDecoder {
 public:
  ShellRawAudioDecoderLinux();
  ~ShellRawAudioDecoderLinux();

  // When the input buffer is not NULL, it can be a normal buffer or an EOS
  // buffer. In this case the function will return the decoded buffer if there
  // is any.
  // The input buffer can be NULL, in this case the function will return a
  // queued buffer if there is any or return NULL if there is no queued buffer.
  virtual scoped_refptr<media::ShellBuffer> Decode(
      const scoped_refptr<media::ShellBuffer>& buffer) OVERRIDE;
  virtual bool Flush() OVERRIDE;
  virtual bool UpdateConfig(const AudioDecoderConfig& config) OVERRIDE;

 private:
  void ReleaseResource();
  void ResetTimestampState();
  void RunDecodeLoop(const scoped_refptr<ShellBuffer>& input,
                     bool skip_eos_append);

  AVCodecContext* codec_context_;
  AVFrame* av_frame_;

  // Decoded audio format.
  int bits_per_channel_;
  media::ChannelLayout channel_layout_;
  int samples_per_second_;

  // Used for computing output timestamps.
  scoped_ptr<media::AudioTimestampHelper> output_timestamp_helper_;
  int bytes_per_frame_;
  base::TimeDelta last_input_timestamp_;

  // Since multiple frames may be decoded from the same packet we need to
  // queue them up and hand them out as we receive Read() calls.
  std::list<QueuedAudioBuffer> queued_audio_;

  DISALLOW_COPY_AND_ASSIGN(ShellRawAudioDecoderLinux);
};

ShellRawAudioDecoderLinux::ShellRawAudioDecoderLinux()
    : codec_context_(NULL),
      av_frame_(NULL),
      bits_per_channel_(0),
      channel_layout_(media::CHANNEL_LAYOUT_NONE),
      samples_per_second_(0),
      bytes_per_frame_(0),
      last_input_timestamp_(media::kNoTimestamp()) {
  LB::EnsureFfmpegInitialized();
}

ShellRawAudioDecoderLinux::~ShellRawAudioDecoderLinux() {
  ReleaseResource();
}

scoped_refptr<media::ShellBuffer> ShellRawAudioDecoderLinux::Decode(
    const scoped_refptr<media::ShellBuffer>& buffer) {
  if (buffer && !buffer->IsEndOfStream()) {
    if (last_input_timestamp_ == media::kNoTimestamp()) {
      last_input_timestamp_ = buffer->GetTimestamp();
    } else if (buffer->GetTimestamp() != media::kNoTimestamp()) {
      DCHECK_GE(buffer->GetTimestamp().ToInternalValue(),
               last_input_timestamp_.ToInternalValue());
      last_input_timestamp_ = buffer->GetTimestamp();
    }
  }

  if (buffer && queued_audio_.empty())
    RunDecodeLoop(buffer, true);

  if (queued_audio_.empty())
    return NULL;
  scoped_refptr<media::ShellBuffer> result = queued_audio_.front().buffer;
  queued_audio_.pop_front();
  return result;
}

bool ShellRawAudioDecoderLinux::Flush() {
  avcodec_flush_buffers(codec_context_);
  ResetTimestampState();
  queued_audio_.clear();

  return true;
}

bool ShellRawAudioDecoderLinux::UpdateConfig(const AudioDecoderConfig& config) {
  if (!config.IsValidConfig()) {
    DLOG(ERROR) << "Invalid audio stream -"
                << " codec: " << config.codec()
                << " channel layout: " << config.channel_layout()
                << " bits per channel: " << config.bits_per_channel()
                << " samples per second: " << config.samples_per_second();
    return false;
  }

  if (codec_context_ &&
      (bits_per_channel_ != config.bits_per_channel() ||
       channel_layout_ != config.channel_layout() ||
       samples_per_second_ != config.samples_per_second())) {
    DVLOG(1) << "Unsupported config change :";
    DVLOG(1) << "\tbits_per_channel : " << bits_per_channel_
             << " -> " << config.bits_per_channel();
    DVLOG(1) << "\tchannel_layout : " << channel_layout_
             << " -> " << config.channel_layout();
    DVLOG(1) << "\tsample_rate : " << samples_per_second_
             << " -> " << config.samples_per_second();
    return false;
  }

  ReleaseResource();

  codec_context_ = avcodec_alloc_context3(NULL);
  DCHECK(codec_context_);
  codec_context_->codec_type = AVMEDIA_TYPE_AUDIO;
  codec_context_->codec_id = CODEC_ID_AAC;
  // request_sample_fmt is set by us, but sample_fmt is set by the decoder.
  codec_context_->request_sample_fmt = AV_SAMPLE_FMT_FLTP;

  codec_context_->channels =
      ChannelLayoutToChannelCount(config.channel_layout());
  codec_context_->sample_rate = config.samples_per_second();

  if (config.extra_data()) {
    codec_context_->extradata_size = config.extra_data_size();
    codec_context_->extradata = reinterpret_cast<uint8_t*>(
        av_malloc(config.extra_data_size() + FF_INPUT_BUFFER_PADDING_SIZE));
    memcpy(codec_context_->extradata, config.extra_data(),
           config.extra_data_size());
    memset(codec_context_->extradata + config.extra_data_size(), '\0',
           FF_INPUT_BUFFER_PADDING_SIZE);
  } else {
    codec_context_->extradata = NULL;
    codec_context_->extradata_size = 0;
  }

  AVCodec* codec = avcodec_find_decoder(codec_context_->codec_id);
  DCHECK(codec);

  int rv = avcodec_open2(codec_context_, codec, NULL);
  DCHECK_GE(rv, 0);
  if (rv < 0) {
    DLOG(ERROR) << "Unable to open codec, result = " << rv;
    return false;
  }

  // Ensure avcodec_open2() respected our format request.
  if (codec_context_->sample_fmt != codec_context_->request_sample_fmt) {
    DLOG(ERROR) << "Unable to configure a supported sample format,"
                << " sample_fmt = " << codec_context_->sample_fmt
                << " instead of " << codec_context_->request_sample_fmt;
    DLOG(ERROR) << "Supported formats:";
    const AVSampleFormat* fmt;
    for (fmt = codec_context_->codec->sample_fmts; *fmt != -1; ++fmt) {
      DLOG(ERROR) << "  " << *fmt
                  << " (" << av_get_sample_fmt_name(*fmt) << ")";
    }
    return false;
  }

  av_frame_ = avcodec_alloc_frame();
  DCHECK(av_frame_);

  bits_per_channel_ = config.bits_per_channel();
  channel_layout_ = config.channel_layout();
  samples_per_second_ = config.samples_per_second();
  output_timestamp_helper_.reset(new media::AudioTimestampHelper(
      config.bytes_per_frame(), config.samples_per_second()));

  ResetTimestampState();

  return true;
}

void ShellRawAudioDecoderLinux::ReleaseResource() {
  if (codec_context_) {
    av_free(codec_context_->extradata);
    avcodec_close(codec_context_);
    av_free(codec_context_);
    codec_context_ = NULL;
  }
  if (av_frame_) {
    av_free(av_frame_);
    av_frame_ = NULL;
  }
}

void ShellRawAudioDecoderLinux::ResetTimestampState() {
  output_timestamp_helper_->SetBaseTimestamp(media::kNoTimestamp());
  last_input_timestamp_ = media::kNoTimestamp();
}

void ShellRawAudioDecoderLinux::RunDecodeLoop(
    const scoped_refptr<ShellBuffer>& input, bool skip_eos_append) {
  AVPacket packet;
  av_init_packet(&packet);
  packet.data = input->GetWritableData();
  packet.size = input->GetDataSize();

  do {
    avcodec_get_frame_defaults(av_frame_);
    int frame_decoded = 0;
    int result = avcodec_decode_audio4(
        codec_context_, av_frame_, &frame_decoded, &packet);
    DCHECK_GE(result, 0);

    // Update packet size and data pointer in case we need to call the decoder
    // with the remaining bytes from this packet.
    packet.size -= result;
    packet.data += result;

    if (output_timestamp_helper_->base_timestamp() == media::kNoTimestamp() &&
        !input->IsEndOfStream()) {
      DCHECK_NE(input->GetTimestamp().ToInternalValue(),
                media::kNoTimestamp().ToInternalValue());
      output_timestamp_helper_->SetBaseTimestamp(input->GetTimestamp());
    }

    const uint8* decoded_audio_data = NULL;
    int decoded_audio_size = 0;
    if (frame_decoded) {
      decoded_audio_data = av_frame_->data[0];
      decoded_audio_size = av_samples_get_buffer_size(
          NULL, codec_context_->channels, av_frame_->nb_samples,
          codec_context_->sample_fmt, 1);
    }

    scoped_refptr<media::ShellBuffer> output;

    if (decoded_audio_size > 0) {
      // Copy the audio samples into an output buffer.
      media::ShellBufferFactory::Instance()->AllocateBuffer(
          decoded_audio_size, base::Bind(AllocateCallback, &output));
      DCHECK(output);
      // Interleave the planar samples to conform to the general decoder
      // requirement. This should eventually be lifted as WiiU is also planar.
      int total_frame = decoded_audio_size / sizeof(float) /
          codec_context_->channels;
      float* target = reinterpret_cast<float*>(output->GetWritableData());
      const float* source_l =
          reinterpret_cast<const float*>(decoded_audio_data);
      // source_r should be "source_l + total_frame". However there is a bug in
      // the channelsplit that mutes the second channel. So we duplicate the
      // first channel into the second.
      const float* source_r = source_l;
      for (int i = 0; i < total_frame; ++i) {
        *target = *source_l;
        ++target;
        ++source_l;
        *target = *source_r;
        ++target;
        ++source_r;
      }
      output->SetTimestamp(output_timestamp_helper_->GetTimestamp());
      output->SetDuration(
          output_timestamp_helper_->GetDuration(decoded_audio_size));
      output_timestamp_helper_->AddBytes(decoded_audio_size);
    } else if (IsEndOfStream(result, decoded_audio_size, input) &&
               !skip_eos_append) {
      DCHECK_EQ(packet.size, 0);
      // Create an end of stream output buffer.
      output = ShellBuffer::CreateEOSBuffer(
          output_timestamp_helper_->GetTimestamp());
    }

    if (output) {
      QueuedAudioBuffer queue_entry = { output };
      queued_audio_.push_back(queue_entry);
    }

    // TODO(xiaomings) : update statistics.
  } while (packet.size > 0);
}

}  // namespace

namespace media {

ShellRawAudioDecoder* ShellRawAudioDecoder::Create() {
  return new ShellRawAudioDecoderLinux;
}

}  // namespace media
