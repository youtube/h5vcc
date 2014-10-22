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

#ifndef SRC_LB_SPEECH_RECOGNIZER_H_
#define SRC_LB_SPEECH_RECOGNIZER_H_

#include "config.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"

class LBSpeechRecognizer {
 public:
  // These callback functions are implemented in LBSpeechRecognitionClient,
  // and will be executed on the WebKit thread.
  class Callback {
   public:
    virtual ~Callback() {}
    virtual void ReceivedResult(scoped_array<char> result,
                                int result_length) = 0;
    virtual void ReceivedNetworkError(const char* error_message) = 0;
    virtual void ReceivedNoSpeechError(const char* error_message) = 0;
  };

  virtual ~LBSpeechRecognizer() {}

  void SetClientInstance(LBSpeechRecognizer::Callback* client) {
    DCHECK(client);
    client_ = client;
  }

  void RemoveClientInstance(LBSpeechRecognizer::Callback* client) {
    DCHECK(client_);
    if (client_ == client) {
      client_ = NULL;
    } else {
      NOTREACHED();
    }
  }

  // These methods are called on the IO Thread
  virtual void Initialize(const char* lang,
                          bool continuous,
                          bool interim_results,
                          int sampling_frequency) = 0;
  virtual void AddData(scoped_array<int16_t> buffer,
                       int num_samples,
                       bool is_last_chunk) = 0;

  virtual void StopRecognizing() = 0;
  virtual void AbortRecognizing() = 0;

 protected:
  LBSpeechRecognizer::Callback* client_;
};

#endif  // ENABLE(SCRIPTED_SPEECH)

#endif  // SRC_LB_SPEECH_RECOGNIZER_H_
