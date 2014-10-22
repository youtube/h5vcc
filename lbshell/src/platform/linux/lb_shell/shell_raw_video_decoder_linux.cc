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
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "lb_ffmpeg.h"
#include "media/base/video_util.h"
#include "media/filters/shell_video_decoder_impl.h"

using media::VideoFrame;
using media::VideoCodecProfile;

namespace {

VideoFrame::Format PixelFormatToVideoFormat(PixelFormat pixel_format) {
  switch (pixel_format) {
    case PIX_FMT_YUV422P:
      return VideoFrame::YV16;
    case PIX_FMT_YUV420P:
      return VideoFrame::YV12;
    case PIX_FMT_YUVJ420P:
      return VideoFrame::YV12;
    default:
      DVLOG(1) << "Unsupported PixelFormat: " << pixel_format;
  }
  return VideoFrame::INVALID;
}

int VideoCodecProfileToProfileID(VideoCodecProfile profile) {
  switch (profile) {
    case media::H264PROFILE_BASELINE:
      return FF_PROFILE_H264_BASELINE;
    case media::H264PROFILE_MAIN:
      return FF_PROFILE_H264_MAIN;
    case media::H264PROFILE_EXTENDED:
      return FF_PROFILE_H264_EXTENDED;
    case media::H264PROFILE_HIGH:
      return FF_PROFILE_H264_HIGH;
    case media::H264PROFILE_HIGH10PROFILE:
      return FF_PROFILE_H264_HIGH_10;
    case media::H264PROFILE_HIGH422PROFILE:
      return FF_PROFILE_H264_HIGH_422;
    case media::H264PROFILE_HIGH444PREDICTIVEPROFILE:
      return FF_PROFILE_H264_HIGH_444_PREDICTIVE;
    default:
      DVLOG(1) << "Unknown VideoCodecProfile: " << profile;
  }
  return FF_PROFILE_UNKNOWN;
}

PixelFormat VideoFormatToPixelFormat(VideoFrame::Format video_format) {
  switch (video_format) {
    case VideoFrame::YV16:
      return PIX_FMT_YUV422P;
    case VideoFrame::YV12:
      return PIX_FMT_YUV420P;
    default:
      DVLOG(1) << "Unsupported VideoFrame::Format: " << video_format;
  }
  return PIX_FMT_NONE;
}

// TODO(xiaomings) : Make this decoder handle decoder errors. Now it assumes
// that the input stream is always correct.
class ShellRawVideoDecoderLinux : public media::ShellRawVideoDecoder {
 public:
  ShellRawVideoDecoderLinux();
  ~ShellRawVideoDecoderLinux();

  virtual DecodeStatus Decode(
      const scoped_refptr<ShellBuffer>& buffer,
      scoped_refptr<VideoFrame>* frame) OVERRIDE;
  virtual bool Flush() OVERRIDE;
  virtual bool UpdateConfig(const VideoDecoderConfig& config) OVERRIDE;

 private:
  void ReleaseResource();
  static int GetVideoBuffer(AVCodecContext* codec_context, AVFrame* frame);
  static void ReleaseVideoBuffer(AVCodecContext*, AVFrame* frame);

  AVCodecContext* codec_context_;
  AVFrame* av_frame_;
  gfx::Size natural_size_;

  DISALLOW_COPY_AND_ASSIGN(ShellRawVideoDecoderLinux);
};

ShellRawVideoDecoderLinux::ShellRawVideoDecoderLinux()
    : codec_context_(NULL),
      av_frame_(NULL) {
  LB::EnsureFfmpegInitialized();
}

ShellRawVideoDecoderLinux::~ShellRawVideoDecoderLinux() {
  ReleaseResource();
}

ShellRawVideoDecoderLinux::DecodeStatus ShellRawVideoDecoderLinux::Decode(
    const scoped_refptr<ShellBuffer>& buffer,
    scoped_refptr<VideoFrame>* frame) {
  DCHECK(buffer);
  DCHECK(!frame->get());
  AVPacket packet;
  av_init_packet(&packet);
  avcodec_get_frame_defaults(av_frame_);
  packet.data = buffer->GetWritableData();
  packet.size = buffer->GetDataSize();
  packet.pts = buffer->GetTimestamp().InMilliseconds();

  int frame_decoded = 0;
  int result = avcodec_decode_video2(codec_context_,
                                     av_frame_,
                                     &frame_decoded,
                                     &packet);
  if (frame_decoded == 0)
    return NEED_MORE_DATA;

  // TODO(fbarchard): Work around for FFmpeg http://crbug.com/27675
  // The decoder is in a bad state and not decoding correctly.
  // Checking for NULL avoids a crash in CopyPlane().
  if (!av_frame_->data[VideoFrame::kYPlane] ||
      !av_frame_->data[VideoFrame::kUPlane] ||
      !av_frame_->data[VideoFrame::kVPlane]) {
    DLOG(ERROR) << "Video frame was produced yet has invalid frame data.";
    return FATAL_ERROR;
  }

  if (!av_frame_->opaque) {
    DLOG(ERROR) << "VideoFrame object associated with frame data not set.";
    return FATAL_ERROR;
  }
  *frame = static_cast<VideoFrame*>(av_frame_->opaque);

  if (frame->get()) {
    (*frame)->SetTimestamp(
        base::TimeDelta::FromMilliseconds(av_frame_->pkt_pts));
  }

  return FRAME_DECODED;
}

bool ShellRawVideoDecoderLinux::Flush() {
  avcodec_flush_buffers(codec_context_);

  return true;
}

bool ShellRawVideoDecoderLinux::UpdateConfig(const VideoDecoderConfig& config) {
  ReleaseResource();

  natural_size_ = config.natural_size();

  codec_context_ = avcodec_alloc_context3(NULL);
  DCHECK(codec_context_);
  codec_context_->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_context_->codec_id = CODEC_ID_H264;
  codec_context_->profile = VideoCodecProfileToProfileID(config.profile());
  codec_context_->coded_width = config.coded_size().width();
  codec_context_->coded_height = config.coded_size().height();
  codec_context_->pix_fmt = VideoFormatToPixelFormat(config.format());

  codec_context_->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  codec_context_->thread_count = 2;
  codec_context_->opaque = this;
  codec_context_->flags |= CODEC_FLAG_EMU_EDGE;
  codec_context_->get_buffer = GetVideoBuffer;
  codec_context_->release_buffer = ReleaseVideoBuffer;

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

  av_frame_ = avcodec_alloc_frame();
  DCHECK(av_frame_);

  return true;
}

void ShellRawVideoDecoderLinux::ReleaseResource() {
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

int ShellRawVideoDecoderLinux::GetVideoBuffer(AVCodecContext* codec_context,
                                        AVFrame* frame) {
  VideoFrame::Format format = PixelFormatToVideoFormat(
      codec_context->pix_fmt);
  if (format == VideoFrame::INVALID)
    return AVERROR(EINVAL);
  DCHECK(format == VideoFrame::YV12 || format == VideoFrame::YV16);

  gfx::Size size(codec_context->width, codec_context->height);
  int ret;
  if ((ret = av_image_check_size(size.width(), size.height(), 0, NULL)) < 0)
    return ret;

  gfx::Size natural_size;
  if (codec_context->sample_aspect_ratio.num > 0) {
    natural_size = media::GetNaturalSize(
        size, codec_context->sample_aspect_ratio.num,
        codec_context->sample_aspect_ratio.den);
  } else {
    ShellRawVideoDecoderLinux* vd = static_cast<ShellRawVideoDecoderLinux*>(
        codec_context->opaque);
    natural_size = vd->natural_size_;
  }

  if (!VideoFrame::IsValidConfig(format, size, gfx::Rect(size), natural_size))
    return AVERROR(EINVAL);

  scoped_refptr<VideoFrame> video_frame =
      VideoFrame::CreateFrame(format, size, gfx::Rect(size), natural_size,
                              media::kNoTimestamp());

  for (int i = 0; i < 3; i++) {
    frame->base[i] = video_frame->data(i);
    frame->data[i] = video_frame->data(i);
    frame->linesize[i] = video_frame->stride(i);
  }

  frame->opaque = NULL;
  video_frame.swap(reinterpret_cast<VideoFrame**>(&frame->opaque));
  frame->type = FF_BUFFER_TYPE_USER;
  frame->pkt_pts = codec_context->pkt ? codec_context->pkt->pts :
                                        AV_NOPTS_VALUE;
  frame->width = codec_context->width;
  frame->height = codec_context->height;
  frame->format = codec_context->pix_fmt;

  return 0;
}

void ShellRawVideoDecoderLinux::ReleaseVideoBuffer(AVCodecContext*,
                                             AVFrame* frame) {
  scoped_refptr<VideoFrame> video_frame;
  video_frame.swap(reinterpret_cast<VideoFrame**>(&frame->opaque));

  // The FFmpeg API expects us to zero the data pointers in
  // this callback
  memset(frame->data, 0, sizeof(frame->data));
  frame->opaque = NULL;
}

}  // namespace

namespace media {

ShellRawVideoDecoder* ShellRawVideoDecoder::Create(
    const VideoDecoderConfig& config,
    media::Decryptor* decryptor,
    bool was_encrypted) {
  DCHECK_EQ(config.codec(), media::kCodecH264)
      << "Video codec " << config.codec() << " is not supported.";
  scoped_ptr<ShellRawVideoDecoder> decoder(new ShellRawVideoDecoderLinux);
  if (decoder->UpdateConfig(config))
    return decoder.release();
  return NULL;
}

}  // namespace media

