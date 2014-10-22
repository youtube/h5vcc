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
#ifndef SRC_PLATFORM_LINUX_LB_SHELL_LB_PULSE_AUDIO_H_
#define SRC_PLATFORM_LINUX_LB_SHELL_LB_PULSE_AUDIO_H_

#include <pulse/pulseaudio.h>

#include <set>

#include "base/synchronization/lock.h"
#include "base/threading/thread.h"

// TODO(xiaomings) : put these decoder/streamer classes into namespace LB

class LBPulseAudioContext;

class LBPulseAudioStream {
 public:
  class Host {
   public:
    typedef base::Callback<void(const uint8*, size_t)> WriteFunc;
    virtual ~Host() {}
    virtual void RequestFrame(size_t length, WriteFunc write) = 0;
  };

  LBPulseAudioStream();
  ~LBPulseAudioStream();

  bool Initialize(LBPulseAudioContext* context, Host* host, int rate,
                  int channel);
  bool Play();
  bool Pause();
  uint64 GetPlaybackCursorInMicroSeconds();
  void RequestFrame();

 private:
  enum {
    kInitial = 0,
    kSuccess,
    kFailure
  };

  static const int kMinLatency = 1000000;
  static const int kMaxLatency = 4000000;

  static void RequestCallback(pa_stream* s, size_t length, void* userdata);
  void HandleRequest(size_t length);
  static void UnderflowCallback(pa_stream* s, void* userdata);
  void HandleUnderflow();
  static void SuccessCallback(pa_stream* s, int success, void* userdata);
  bool Cork(bool pause);

  void WriteFrame(const uint8* data, size_t size);

  LBPulseAudioContext* context_;
  int latency_;
  pa_buffer_attr buf_attr_;
  pa_sample_spec sample_spec_;
  pa_stream* stream_;
  size_t last_request_size_;
  Host* host_;

  DISALLOW_COPY_AND_ASSIGN(LBPulseAudioStream);
};

class LBPulseAudioContext {
 public:
  LBPulseAudioContext();
  ~LBPulseAudioContext();

  bool Initialize();
  pa_context* GetContext();
  void Iterate();

  LBPulseAudioStream* CreateStream(LBPulseAudioStream::Host* host,
                                   int rate, int channel);
  void DestroyStream(LBPulseAudioStream* stream);

  base::Lock& lock() { return lock_; }

 private:
  typedef std::set<LBPulseAudioStream*> Streams;
  enum {
    kInitial = 0,
    kReady,
    kError
  };

  static void StateCallback(pa_context* c, void* userdata);

  pa_mainloop* mainloop_;
  pa_context* context_;
  base::Thread pulse_thread_;

  Streams streams_;
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(LBPulseAudioContext);
};

#endif  // SRC_PLATFORM_LINUX_LB_SHELL_LB_PULSE_AUDIO_H_

