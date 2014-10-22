/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_AUDIOCONFIG_AUDIOCONFIG_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_AUDIOCONFIG_AUDIOCONFIG_H_

#include "config.h"

#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

#if ENABLE(AUDIO_CONFIGURATION_REPORTING)

namespace WebCore {

class AudioConfig : public RefCounted<AudioConfig> {
 public:
  virtual ~AudioConfig() {}
  static PassRefPtr<AudioConfig> create(
      const String& connector,
      const uint64_t latencyMs,
      const String& codingType,
      const uint64_t numberOfChannels,
      const uint64_t samplingFrequency) {
    return adoptRef(new AudioConfig(connector,
                                    latencyMs,
                                    codingType,
                                    numberOfChannels,
                                    samplingFrequency));
  }

  const String& connector() const { return m_connector; }
  const uint64_t latencyMs() const { return m_latencyMs; }
  const String& codingType() const { return m_codingType; }
  const uint64_t numberOfChannels() const { return m_numberOfChannels; }
  const uint64_t samplingFrequency() const { return m_samplingFrequency; }

 private:
  AudioConfig(const String& connector,
              const uint64_t latencyMs,
              const String& codingType,
              const uint64_t numberOfChannels,
              const uint64_t samplingFrequency)
      : m_connector(connector)
      , m_latencyMs(latencyMs)
      , m_codingType(codingType)
      , m_numberOfChannels(numberOfChannels)
      , m_samplingFrequency(samplingFrequency) {}
  const String m_connector;
  const uint64_t m_latencyMs;
  const String m_codingType;
  const uint64_t m_numberOfChannels;
  const uint64_t m_samplingFrequency;
};

}  // namespace WebCore

#else

namespace WebCore {

class AudioConfig;

}  // namespace WebCore

#endif  // ENABLE(AUDIO_CONFIGURATION_REPORTING)

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_AUDIOCONFIG_AUDIOCONFIG_H_
