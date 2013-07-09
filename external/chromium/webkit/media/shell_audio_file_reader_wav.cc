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

#include "webkit/media/shell_audio_file_reader_wav.h"

#include "base/logging.h"
#include "lb_platform.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebAudioBus.h"

const uint32 kWAVChunkID_RIFF = 0x52494646;  // 'RIFF'
const uint32 kWAVWaveID_WAVE = 0x57415645;   // 'WAVE'
const uint32 kWAVChunkID_fmt = 0x666d7420;   // 'fmt '
const uint32 kWAVChunkID_data = 0x64617461;  // 'data'

const uint16 kWAVFormatCodePCM = 0x0001;
const uint16 kWAVFormatCodeFloat = 0x0003;

const size_t kWAVRIFFChunkHeaderSize = 12;
const size_t kWAVfmtChunkHeaderSize = 16;

namespace webkit_media {

// see: http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
// static
ShellAudioFileReader* ShellAudioFileReaderWAV::Create(const uint8* data,
                                                      size_t data_size) {
  // need at least the RIFF chunk header for this to be a WAV
  if (!data || data_size < kWAVRIFFChunkHeaderSize) {
    return NULL;
  }
  // first four bytes need to be the RIFF chunkID
  uint32 riff_chunk_id = LB::Platform::load_uint32_big_endian(data);
  if (riff_chunk_id != kWAVChunkID_RIFF) {
    return NULL;
  }
  // sanity-check the size. It should be large enough to hold at least the chunk
  // header.
  uint32 riff_chunk_size = LB::Platform::load_uint32_little_endian(data + 4);
  if (riff_chunk_size > data_size || riff_chunk_size < 4) {
    return NULL;
  }
  // next 4 bytes need to be the WAVE id, check for that
  uint32 wave_id = LB::Platform::load_uint32_big_endian(data + 8);
  if (wave_id != kWAVWaveID_WAVE) {
    return NULL;
  }
  // ok, this looks to resemble a WAV file, construct and return a reader
  return new ShellAudioFileReaderWAV(data + kWAVRIFFChunkHeaderSize,
                                     riff_chunk_size);
}

ShellAudioFileReaderWAV::ShellAudioFileReaderWAV(const uint8* riff_data,
                                                 uint32 riff_chunk_size)
    : riff_data_(riff_data)
    , riff_chunk_size_(riff_chunk_size)
    , config_parsed_(false)
    , data_samples_(NULL)
    , channels_(0)
    , sample_rate_(0.0)
    , number_of_frames_(0)
    , is_float_(false) {
}

ShellAudioFileReaderWAV::~ShellAudioFileReaderWAV() {
}

bool ShellAudioFileReaderWAV::parse_config() {
  uint32 chunk_offset = 0;

  while (chunk_offset < riff_chunk_size_ - 8) {
    uint32 chunk_id =
      LB::Platform::load_uint32_big_endian(riff_data_ + chunk_offset);
    chunk_offset += 4;
    uint32 chunk_size =
      LB::Platform::load_uint32_little_endian(riff_data_ + chunk_offset);
    chunk_offset += 4;

    switch(chunk_id) {
      case kWAVChunkID_fmt:
        if (!ParseWAV_fmt(chunk_offset, chunk_size)) {
          return false;
        }
        break;

      case kWAVChunkID_data:
        if (!ParseWAV_data(chunk_offset, chunk_size)) {
          return false;
        }
        break;

      default:
        break;
    }

    chunk_offset += chunk_size;
  }

  return (config_parsed_ && data_samples_);
}

bool ShellAudioFileReaderWAV::ParseWAV_fmt(uint32 offset, uint32 size) {
  // check size for complete header
  if (size < kWAVfmtChunkHeaderSize) {
    return false;
  }

  // check format code is float
  uint16 format_code =
      LB::Platform::load_uint16_little_endian(riff_data_ + offset);
  if (format_code != kWAVFormatCodeFloat &&
      format_code != kWAVFormatCodePCM) {
    DLOG(ERROR) << "bad format on WAV, we only support uncompressed samples!";
    return false;
  }

  is_float_ = format_code == kWAVFormatCodeFloat;

  // load channel count
  uint16 number_of_channels =
      LB::Platform::load_uint16_little_endian(riff_data_ + offset + 2);
  channels_ = number_of_channels;

  // load sample rate
  uint32 samples_per_second =
      LB::Platform::load_uint32_little_endian(riff_data_ + offset + 4);
  sample_rate_ = samples_per_second;

  // skip over:
  // uint32 average byterate
  // uint16 block alignment

  // check sample size, we only support 32 bit floats or 16 bit PCM
  uint16 bits_per_sample =
      LB::Platform::load_uint32_little_endian(riff_data_ + offset + 14);
  if ((is_float_ && bits_per_sample != 32) ||
      (!is_float_ && bits_per_sample != 16)) {
    DLOG(ERROR) << "bad bits per sample on WAV";
    return false;
  }

  config_parsed_ = true;
  return true;
}

bool ShellAudioFileReaderWAV::ParseWAV_data(uint32 offset, uint32 size) {
  // set number of frames based on size of data chunk
  if (is_float_) {
    number_of_frames_ = size / (4 * channels_);
  } else {
    number_of_frames_ = size / (2 * channels_);
  }

  data_samples_ = riff_data_ + offset;
  return true;
}

size_t ShellAudioFileReaderWAV::channels() {
  DCHECK(config_parsed_);
  return channels_;
}

double ShellAudioFileReaderWAV::sample_rate() {
  DCHECK(config_parsed_);
  return sample_rate_;
}

size_t ShellAudioFileReaderWAV::number_of_frames() {
  DCHECK(config_parsed_);
  return number_of_frames_;
}

size_t ShellAudioFileReaderWAV::read(WebKit::WebAudioBus* bus) {
  DCHECK(config_parsed_);
  DCHECK(data_samples_);

  // de-interleave the data to the output bus channels
  if (is_float_) {
    for (int i = 0; i < channels_; ++i) {
      float* sample = (float*)data_samples_ + i;
      float* dest = bus->channelData(i);
      for (int j = 0; j < number_of_frames_; ++j) {
        uint32 sample_as_uint32 =
            LB::Platform::load_uint32_little_endian((uint8*)sample);
        *dest = *((float*)&sample_as_uint32);
        ++dest;
        sample += channels_;
      }
    }
  } else {
    // wav PCM data are channel-interleaved 16-bit signed ints, little-endian
    for (int i = 0; i < channels_; ++i) {
      int16* sample = (int16*)data_samples_ + i;
      float* dest = bus->channelData(i);
      for (int j = 0; j < number_of_frames_; ++j) {
        uint16 sample_pcm_unsigned =
            LB::Platform::load_uint16_little_endian((uint8*)sample);
        int16 sample_pcm = *((int16*)&sample_pcm_unsigned);
        *dest = (float)sample_pcm / 32768.0f;
        ++dest;
        sample += channels_;
      }
    }
  }

  return number_of_frames_;
}

}  // namespace webkit_media

