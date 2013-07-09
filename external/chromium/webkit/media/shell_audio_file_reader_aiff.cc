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

#include "webkit/media/shell_audio_file_reader_aiff.h"

#include <math.h>

#include "base/logging.h"
#include "lb_platform.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebAudioBus.h"

const uint32 kAIFFChunkID_FORM = 0x464f524d;  // 'FORM'
const uint32 kAIFFChunkID_AIFC = 0x41494643;  // 'AIFC'
const uint32 kAIFFChunkID_COMM = 0x434f4d4d;  // 'COMM'
const uint32 kAIFFChunkID_SSND = 0x53534e44;  // 'SSND'
const uint32 kAIFFChunkID_FVER = 0x46564552;  // 'FVER'

const uint32 kAIFFVersionTimestamp = 0xa2805140;

const uint32 kAIFFMinSize_COMM = 22;
const uint32 kAIFFCompressionType_fl32 = 0x666c3332;  // 'fl32'
const uint32 kAIFFCompressionType_FL32 = 0x464c3332;  // 'FL32'
// Number of bytes at the the FORM header where chunk parsing
// will commence.
const size_t kAIFFFormHeaderSize = 12;

namespace webkit_media {

// see:
// http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/AIFF/Docs/AIFF-C.9.26.91.pdf
// static
ShellAudioFileReader* ShellAudioFileReaderAIFF::Create(const uint8* data,
                                                       size_t data_size) {
  // need at least 12 bytes to describe an AIFF header
  if (!data || data_size < kAIFFFormHeaderSize) {
    return NULL;
  }
  // do some basic sanity-checking for AIFF format top-level container chunk
  uint32 form_chunk_id = LB::Platform::load_uint32_big_endian(data);
  if (form_chunk_id != kAIFFChunkID_FORM) {
    return NULL;
  }
  // sanity-check the size, it should be <= data_size, and large enough
  // to contain the AIFC value we check below.
  uint32 form_chunk_size = LB::Platform::load_uint32_big_endian(data + 4);
  if (form_chunk_size > data_size || form_chunk_size < 4) {
    return NULL;
  }
  // lastly the form type should be AIFC
  uint32 form_chunk_type = LB::Platform::load_uint32_big_endian(data + 8);
  if (form_chunk_type != kAIFFChunkID_AIFC) {
    return NULL;
  }

  // ok, this looks like a basically sane AIFF, construct a reader and return
  return new ShellAudioFileReaderAIFF(data + kAIFFFormHeaderSize,
                                      form_chunk_size);
}

ShellAudioFileReaderAIFF::ShellAudioFileReaderAIFF(const uint8* form_data,
                                                   uint32 form_chunk_size)
    : form_data_(form_data)
    , form_chunk_size_(form_chunk_size)
    , config_parsed_(false)
    , channels_(0)
    , sample_rate_(0.0)
    , number_of_frames_(0)
    , ssnd_samples_(NULL) {
}

ShellAudioFileReaderAIFF::~ShellAudioFileReaderAIFF() {
}

// AIFF files are structured in "chunks." Each chunk must be confined in
// the overall FORM chunk, which we validated the basic structure of in
// the Create() method. Each chunk is identified by a 4-letter code followed
// by a uint32 big-endian size value, and then chunk-dependent data. Chunks
// can appear in any order within the file. We are interested in finding the
// COMM chunk, which contains the config info, and the SSND chunk, which
// contains the raw sample data. This method finds both, parses the COMM
// chunk, and returns true on success.
bool ShellAudioFileReaderAIFF::parse_config() {
  uint32 chunk_offset = 0;

  // We will need at least 8 bytes to extract the ID and size of the next
  // chunk. If we have less than that we are done parsing.
  while (chunk_offset < form_chunk_size_ - 8) {
    uint32 chunk_id =
        LB::Platform::load_uint32_big_endian(form_data_ + chunk_offset);
    chunk_offset += 4;
    uint32 chunk_size =
        LB::Platform::load_uint32_big_endian(form_data_ + chunk_offset);
    chunk_offset += 4;

    switch(chunk_id) {
      case kAIFFChunkID_COMM:
        if (!ParseAIFF_COMM(chunk_offset, chunk_size)) {
          return false;
        }
        break;

      case kAIFFChunkID_SSND:
        if (!ParseAIFF_SSND(chunk_offset, chunk_size)) {
          return false;
        }
        break;

      case kAIFFChunkID_FVER:
        if (!ParseAIFF_FVER(chunk_offset, chunk_size)) {
          return false;
        }
        break;

      default:
        break;
    }

    chunk_offset += chunk_size;
  }

  // for this file to be valid and parsed we will need to have encountered
  // a valid SSND and a COMM chunk.
  return (config_parsed_ && ssnd_samples_);
}

bool ShellAudioFileReaderAIFF::extendedToDouble(const uint8* extended,
                                                double& out_double) {
  // load sign bit and 15-bit exponent from data
  uint16 sign_and_exponent = LB::Platform::load_uint16_big_endian(extended);
  // sign bit should not be set, a negative sample rate is not supported
  if (sign_and_exponent & 0x80) {
    return false;
  }
  // Remove the 80-bit exponent bias (16383) and add the 64-bit exponent
  // bias (1023)
  sign_and_exponent = sign_and_exponent - 16383 + 1023;
  // We want the least significant 12 bits of sign_and_exponent to be the
  // most significant 12 bits of ext_double, so therefore we shift it left
  // by 52 bits.
  uint64 ext_double = ((uint64)sign_and_exponent) << 52;
  // Now we want the most significant 52 bits of the 63 bit mantissa in
  // the rest of the double bytes. We are out of byte-alignment by a
  // nybble from the larger format, so we load the remaining 8 bytes in
  // extended into a uint64, check the MSb (it must be 1), shift the
  // value right by 12 bits, and OR it in to the double we're building.
  uint64 ext_mantissa = LB::Platform::load_uint64_big_endian(extended + 2);
  // MSb needs to be one, indicating this is a regular normalized number.
  if (!(ext_mantissa & 0x8000000000000000)) {
    return false;
  }
  // make room for the sign and exponent bits, then OR the remainder in
  // to our home-made double
  ext_double = ext_double | ((ext_mantissa & 0x7fffffffffffffff) >> 11);
  out_double = *(double*)(&ext_double);
  // check to see if we parsed a garbage number
  if (isnan(out_double)) {
    return false;
  }
  return true;
}

// COMM format:
// ID chunkID == 'COMM'
// long data_size
//
// uint16 number_of_channels
// uint32 number_of_sample_frames
// uint16 sample_size_in_bits
// extended (80-bit IEEE float) sample_rate
// ID compression_id_code
// pascal string compression_name (ignored)
bool ShellAudioFileReaderAIFF::ParseAIFF_COMM(uint32 offset, uint32 size) {
  // a COMM less than 22 bytes in size is incomplete and is a parse error
  if (size < kAIFFMinSize_COMM) {
    return false;
  }

  // load channel count
  uint16 number_of_channels = LB::Platform::load_uint16_big_endian(
      form_data_ + offset);
  channels_ = number_of_channels;

  // load number of frames in sample
  number_of_frames_ = LB::Platform::load_uint32_big_endian(
      form_data_ + offset + 2);

  // sample size should be 32-bit floats
  uint16 sample_size = LB::Platform::load_uint16_big_endian(
      form_data_ + offset + 6);
  if (sample_size != 32) {
    DLOG(ERROR) << "bad sample size on AIFF, we only support 32-bit floats!";
    return false;
  }

  // Convert the 80-bit IEEE floating point number to a 64-bit double.
  const uint8* extended = form_data_ + offset + 8;
  if (!extendedToDouble(extended, sample_rate_)) {
    return false;
  }

  // check compression code as uncompressed
  uint32 compression_id_code = LB::Platform::load_uint32_big_endian(
      form_data_ + offset + 18);
  if (compression_id_code != kAIFFCompressionType_FL32 &&
      compression_id_code != kAIFFCompressionType_fl32) {
    return false;
  }

  // OK, success!
  config_parsed_ = true;
  return true;
}

// SSND format:
// ID chunkID == 'SSND'
// long ckDataSize
//
// unsigned long offset
// unsigned long block_size
// char sound_data[]
bool ShellAudioFileReaderAIFF::ParseAIFF_SSND(uint32 offset, uint32 size) {
  // offset may be non-zero, to allow for byte-alignment of data
  uint32 data_offset = LB::Platform::load_uint32_big_endian(
      form_data_ + offset);
  // sanity-check the size against our frame count
  if (size < (number_of_frames_ * channels_ * 4) + data_offset) {
    return false;
  }
  // otherwise configure our pointer to point directly at the float data
  ssnd_samples_ = (float*)(form_data_ + offset + 8 + data_offset);
  return true;
}

// FVER format:
// ID chunkID = =='FVER'
// long data_size == 4, should only ever be the size of the next field
//
// unsigned long timestamp, always equal to 0xa2805140
bool ShellAudioFileReaderAIFF::ParseAIFF_FVER(uint32 offset, uint32 size) {
  // size is always 4, this chunk contains only a constant timestamp
  if (size != 4) {
    return false;
  }
  uint32 timestamp = LB::Platform::load_uint32_big_endian(form_data_ + offset);
  if (timestamp != kAIFFVersionTimestamp) {
    return false;
  }
  return true;
}

size_t ShellAudioFileReaderAIFF::channels() {
  DCHECK(config_parsed_);
  return channels_;
}

double ShellAudioFileReaderAIFF::sample_rate() {
  DCHECK(config_parsed_);
  return sample_rate_;
}

size_t ShellAudioFileReaderAIFF::number_of_frames() {
  DCHECK(config_parsed_);
  return number_of_frames_;
}

size_t ShellAudioFileReaderAIFF::read(WebKit::WebAudioBus* bus) {
  DCHECK(config_parsed_);
  DCHECK(ssnd_samples_);
  // de-interleave the data to the output bus channels
  for (int i = 0; i < channels_; ++i) {
    float* sample = ssnd_samples_ + i;
    float* dest = bus->channelData(i);
    for (int j = 0; j < number_of_frames_; ++j) {
      uint32 sample_as_uint32 =
          LB::Platform::load_uint32_big_endian((uint8*)sample);
      *dest = *((float*)&sample_as_uint32);
      ++dest;
      sample += channels_;
    }
  }
  return number_of_frames_;
}

}  // namespace webkit_media

