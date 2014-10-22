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

#ifndef SRC_LB_PLATFORM_MIC_H_
#define SRC_LB_PLATFORM_MIC_H_

#include "config.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "base/logging.h"
#include "base/time.h"

// Platform-specific microphone implementations should derive from this class,
// which is a component of LBSpeechRecognitionClient.
class LBPlatformMic {
 public:
  // These callback functions are implemented in LBSpeechRecognitionClient,
  // and will be executed on the WebKit thread.
  class Callback {
   public:
    enum ErrorCode {
      ErrorOpening = 1,   // Error initializing microphone
      ErrorClosing = 2,   // Error shutting down microphone
      ErrorSilence = 3,   // Mic is silent -- user can fix
      ErrorListening = 4  // Error while capturing audio
    };
    virtual ~Callback() {}
    virtual void StartedAudio() = 0;
    virtual void EndedAudio() = 0;
    virtual void SentData(int16_t* buffer, int size, bool is_last_chunk) = 0;
    virtual void ReceivedError(LBPlatformMic::Callback::ErrorCode error_code,
                               const char* error_message) = 0;
  };

  LBPlatformMic() : init_successful_(false), client_(NULL), stopped_(false) {}
  virtual ~LBPlatformMic() {}

  void SetClientInstance(LBPlatformMic::Callback* client) {
    DCHECK(client);
    client_ = client;
  }

  void RemoveClientInstance(LBPlatformMic::Callback* client) {
    DCHECK(client_);
    if (client_ == client) {
      client_ = NULL;
    } else {
      NOTREACHED();
    }
  }

  virtual void StartListening() = 0;
  virtual void StopListening() = 0;

  // Returns the sampling frequency in Hz; should be between 8000-48000
  virtual int sampling_frequency() const = 0;
  bool init_successful() const { return init_successful_; }
  static base::TimeDelta MaxSpeechTime() {
    return base::TimeDelta::FromSeconds(30);
  }

 protected:
  bool stopped_;
  bool init_successful_;
  LBPlatformMic::Callback* client_;
};

#endif  // ENABLE(SCRIPTED_SPEECH)

#endif  // SRC_LB_PLATFORM_MIC_H_
