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
#if !defined(__LB_XB1__) && !defined(__LB_XB360__) && !defined(__LB_PS3__)
// TODO(linguo): PS3 will be unified later

#include "lb_web_audio_device.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/compiler_specific.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "lb_platform.h"
#include "media/audio/audio_parameters.h"
#include "media/base/audio_bus.h"
#include "media/base/shell_buffer_factory.h"
#include "media/audio/shell_audio_streamer.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebVector.h"

using media::ShellAudioStreamer;

namespace {

const size_t kRenderBufferSizeFrames = 1024;
const size_t kFramesPerChannel = kRenderBufferSizeFrames * 4;
const double kStandardOutputSampleRate = 48000.0f;

}  // namespace

class LBWebAudioDeviceImpl
    : public LBWebAudioDevice
    , public media::ShellAudioStream {
 public:
  LBWebAudioDeviceImpl(
      size_t buffer,
      unsigned int numberOfChannels,
      double sampleRate,
      WebKit::WebAudioDevice::RenderCallback* callback);
  virtual ~LBWebAudioDeviceImpl();

  // WebAudioDevice implementation
  virtual void start() OVERRIDE;
  virtual void stop() OVERRIDE;
  virtual double sampleRate() OVERRIDE;

  // ShellAudioStream implementation
  virtual bool PauseRequested() const OVERRIDE;
  virtual bool PullFrames(uint32_t* offset_in_frame,
                          uint32_t* total_frames) OVERRIDE;
  void ConsumeFrames(uint32_t frame_played);
  virtual const media::AudioParameters& GetAudioParameters() const OVERRIDE;
  virtual media::AudioBus* GetAudioBus() OVERRIDE;

 private:
  void DoStart();
  void DoStop();

  bool IsAudioInterleaved() const;
  uint32_t BytesPerSample() const;

  void InitializeAudioBusInterleaved(
      const uint32_t bytes_per_sample, const unsigned num_output_channels);
  void InitializeAudioBusPlanar(
      const uint32_t bytes_per_sample, const unsigned num_output_channels);

  void Pull4BytesInterleaved(const uint64_t channel_offset);
  void Pull2BytesPlanar(const uint64_t channel_offset);

  // thread that will run on the same core as the Audio hardware
  base::Thread helper_thread_;

  WebKit::WebVector<float*> render_buffers_;
  media::AudioParameters audio_parameters_;
  scoped_ptr<media::AudioBus> output_audio_bus_;
  scoped_refptr<media::ShellBufferFactory> buffer_factory_;

  uint64_t rendered_frame_cursor_;
  uint64_t buffered_frame_cursor_;
  uint32_t bytes_per_sample_;
  bool needs_data_;
  bool interleaved_;

  WebKit::WebAudioDevice::RenderCallback* callback_;
};

LBWebAudioDeviceImpl::LBWebAudioDeviceImpl(
    size_t buffer,
    unsigned int numberOfChannels,
    double sampleRate,
    WebKit::WebAudioDevice::RenderCallback* callback)
    : helper_thread_("LBWebAudioContext")
    , render_buffers_(static_cast<size_t>(numberOfChannels))
    , rendered_frame_cursor_(0)
    , buffered_frame_cursor_(0)
    , needs_data_(true)
    , callback_(callback) {
  // These numbers should reflect the values returned in
  // LBWebAudioDevice::GetAudioHardwareXXX
  DCHECK_EQ(buffer, kRenderBufferSizeFrames);
  DCHECK_EQ(sampleRate, GetAudioHardwareSampleRate());
  buffer_factory_ = media::ShellBufferFactory::Instance();
  bytes_per_sample_ =
      ShellAudioStreamer::Instance()->GetConfig().bytes_per_sample();
  interleaved_ = ShellAudioStreamer::Instance()->GetConfig().interleaved();

  for (int i = 0; i < numberOfChannels; ++i) {
    render_buffers_[i] = reinterpret_cast<float*>(buffer_factory_->AllocateNow(
        kRenderBufferSizeFrames * sizeof(float)));
    DCHECK(render_buffers_[i]);
  }

  // We only support up to two stereo channels
  const unsigned int kMaxNumberOfChannels = 2u;
  unsigned int num_output_channels =
      std::max(numberOfChannels, kMaxNumberOfChannels);

  if (IsAudioInterleaved()) {
    InitializeAudioBusInterleaved(BytesPerSample(), num_output_channels);
  } else {
    InitializeAudioBusPlanar(BytesPerSample(), num_output_channels);
  }

  media::ChannelLayout channel_layout =
      num_output_channels == 1 ? media::CHANNEL_LAYOUT_MONO
                               : media::CHANNEL_LAYOUT_STEREO;

  audio_parameters_ = media::AudioParameters(
      media::AudioParameters::AUDIO_PCM_LINEAR,
      channel_layout,
      static_cast<int>(GetAudioHardwareSampleRate()),
      BytesPerSample() * 8,
      kRenderBufferSizeFrames);
#if defined(__LB_WIIU__)
  audio_parameters_.set_output_to_tv(false);
#endif
}

LBWebAudioDeviceImpl::~LBWebAudioDeviceImpl() {
  DCHECK(!ShellAudioStreamer::Instance()->HasStream(this));

  for (int i = 0; i < output_audio_bus_->channels(); ++i) {
    buffer_factory_->Reclaim(
        reinterpret_cast<uint8_t*>(output_audio_bus_->channel(i)));
  }

  int render_buffers_size = render_buffers_.size();
  for (int i = 0; i < render_buffers_size; ++i) {
    buffer_factory_->Reclaim(reinterpret_cast<uint8_t*>(render_buffers_[i]));
    render_buffers_[i] = NULL;
  }
}

void LBWebAudioDeviceImpl::start() {
  base::Thread::Options options;

#if defined(__LB_WIIU__)
  // Wii U Audio API must be accessed from the same core it was initialized on
  options.affinity = LB::Platform::kMediaPipelineThreadAffinity;
#endif

  helper_thread_.StartWithOptions(options);
  helper_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&LBWebAudioDeviceImpl::DoStart, base::Unretained(this)));
}

void LBWebAudioDeviceImpl::stop() {
  helper_thread_.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&LBWebAudioDeviceImpl::DoStop, base::Unretained(this)));
  helper_thread_.Stop();
}

double LBWebAudioDeviceImpl::sampleRate() {
  return GetAudioHardwareSampleRate();
}

bool LBWebAudioDeviceImpl::PauseRequested() const {
  return needs_data_;
}

bool LBWebAudioDeviceImpl::PullFrames(uint32_t* offset_in_frame,
                                      uint32_t* total_frames) {
  uint32_t dummy_offset_in_frame, dummy_total_frames;
  if (!offset_in_frame) offset_in_frame = &dummy_offset_in_frame;
  if (!total_frames) total_frames = &dummy_total_frames;

  // Assert that we never render more than has been buffered
  DCHECK_GE(buffered_frame_cursor_, rendered_frame_cursor_);
  *total_frames = buffered_frame_cursor_ - rendered_frame_cursor_;
  if ((kFramesPerChannel - *total_frames) >= kRenderBufferSizeFrames) {
    // Fill our temporary buffer with PCM float samples
    callback_->render(render_buffers_, kRenderBufferSizeFrames);

    // Determine the offset into the audio bus that represents the tail of
    // buffered data
    uint64_t channel_offset = buffered_frame_cursor_ % kFramesPerChannel;

    switch (BytesPerSample()) {
      case sizeof(float): {
        DCHECK(IsAudioInterleaved());
        if (IsAudioInterleaved()) {
          Pull4BytesInterleaved(channel_offset);
          break;
        }
        return false;
      }
      case sizeof(int16_t): {
        DCHECK(!IsAudioInterleaved());
        if (!IsAudioInterleaved()) {
          Pull2BytesPlanar(channel_offset);
          break;
        }
        return false;
      }
      default:
        NOTREACHED();
        return false;
    }

    buffered_frame_cursor_ += kRenderBufferSizeFrames;
    *total_frames += kRenderBufferSizeFrames;
  }
  needs_data_ = *total_frames < kRenderBufferSizeFrames;
  *offset_in_frame = rendered_frame_cursor_ % kFramesPerChannel;
  return !PauseRequested();
}

void LBWebAudioDeviceImpl::ConsumeFrames(uint32_t frame_played) {
  // Increment number of frames rendered by the hardware
  rendered_frame_cursor_ += frame_played;
}

const media::AudioParameters& LBWebAudioDeviceImpl::GetAudioParameters() const {
  return audio_parameters_;
}

media::AudioBus* LBWebAudioDeviceImpl::GetAudioBus() {
  return output_audio_bus_.get();
}

void LBWebAudioDeviceImpl::DoStart() {
  ShellAudioStreamer::Instance()->AddStream(this);
}

void LBWebAudioDeviceImpl::DoStop() {
  ShellAudioStreamer::Instance()->RemoveStream(this);
}

bool LBWebAudioDeviceImpl::IsAudioInterleaved() const {
  return interleaved_;
}

uint32_t LBWebAudioDeviceImpl::BytesPerSample() const {
  return bytes_per_sample_;
}

void LBWebAudioDeviceImpl::InitializeAudioBusInterleaved(
    const uint32_t bytes_per_sample, const unsigned num_output_channels) {
  std::vector<float*> channel_data;
  channel_data.push_back(reinterpret_cast<float*>(
      buffer_factory_->AllocateNow(bytes_per_sample * kFramesPerChannel *
                                   num_output_channels)));
  DCHECK(channel_data[0]);
  uint32_t frames = kFramesPerChannel * num_output_channels *
      bytes_per_sample / sizeof(float);
  output_audio_bus_ = media::AudioBus::WrapVector(
      frames, channel_data);
}

void LBWebAudioDeviceImpl::InitializeAudioBusPlanar(
    const uint32_t bytes_per_sample, const unsigned num_output_channels) {
  std::vector<float*> channel_data;
  for (int i = 0; i < num_output_channels; ++i) {
    channel_data.push_back(reinterpret_cast<float*>(
        buffer_factory_->AllocateNow(bytes_per_sample * kFramesPerChannel)));
    DCHECK(channel_data[i]);
  }
  uint32_t frames = kFramesPerChannel * bytes_per_sample / sizeof(float);
  output_audio_bus_ = media::AudioBus::WrapVector(
      frames, channel_data);
}

void LBWebAudioDeviceImpl::Pull4BytesInterleaved(
    const uint64_t channel_offset) {
  float* output_buffer = output_audio_bus_->channel(0);
  output_buffer += channel_offset * audio_parameters_.channels();

  for (int i = 0; i < kRenderBufferSizeFrames; ++i) {
    for (int c = 0; c < audio_parameters_.channels(); ++c) {
      float* input_buffer = render_buffers_[c];
      *output_buffer = input_buffer[i];
      ++output_buffer;
    }
  }
}

void LBWebAudioDeviceImpl::Pull2BytesPlanar(
    const uint64_t channel_offset) {
  // PCM Float is centered on 0, with a range of [-1.0, 1.0]
  // PCM16 has a range of [-32768, 32767]
  const int32_t kPCM16Max = 32767;
  const int32_t kPCM16Min = -32768;
  const float kConversionFactor = (kPCM16Max - kPCM16Min) / 2.f;

  for (int c = 0; c < output_audio_bus_->channels(); ++c) {
    float* input_buffer = render_buffers_[c];
    int16_t* output_buffer = reinterpret_cast<int16_t*>(
        output_audio_bus_->channel(c));
    output_buffer += channel_offset;
    for (int i = 0; i < kRenderBufferSizeFrames; ++i) {
      // convert to a larger integer and clamp, to account for rounding
      // and the fact that the range of PCM16 is not centered on 0, but
      // PCMFloat is
      int32_t converted = (int32_t)(input_buffer[i] * kConversionFactor);
      output_buffer[i] =
          (int16_t)(std::min(kPCM16Max, std::max(kPCM16Min, converted)));
    }
  }
}

// static
WebKit::WebAudioDevice* LBWebAudioDevice::Create(
    size_t buffer,
    unsigned numberOfChannels,
    double sampleRate,
    WebKit::WebAudioDevice::RenderCallback* callback) {
  return new LBWebAudioDeviceImpl(
      buffer, numberOfChannels, sampleRate, callback);
}

// static
double LBWebAudioDevice::GetAudioHardwareSampleRate() {
  uint32_t native_output_sample_rate = ShellAudioStreamer::Instance()->
      GetConfig().native_output_sample_rate();
  if (native_output_sample_rate !=
      ShellAudioStreamer::Config::kInvalidSampleRate) {
    return native_output_sample_rate;
  }
  return kStandardOutputSampleRate;
}

// static
size_t LBWebAudioDevice::GetAudioHardwareBufferSize() {
  return kRenderBufferSizeFrames;
}

#endif  // !defined(__LB_XB1__) && !defined(__LB_XB360__) && !defined(__LB_PS3__) NOLINT(whitespace/line_length)
