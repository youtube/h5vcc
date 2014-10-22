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

#ifndef SRC_LB_SPEECH_RECOGNIZER_IMPL_H_
#define SRC_LB_SPEECH_RECOGNIZER_IMPL_H_

#include "lb_speech_recognizer.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_request.h"

#include "lb_request_context.h"

class SpeechRequestDelegate;
class DownSpeechRequestDelegate;
class UpSpeechRequestDelegate;

class LBSpeechRecognizerImpl : public LBSpeechRecognizer {
  // All of SpeechRequestDelegate's methods are called on the IO Thread
  friend class SpeechRequestDelegate;
  friend class DownSpeechRequestDelegate;
  friend class UpSpeechRequestDelegate;

 public:
  LBSpeechRecognizerImpl();
  virtual ~LBSpeechRecognizerImpl();

  // These methods are called on the IO Thread
  virtual void Initialize(const char* lang,
                          bool continuous,
                          bool interim_results,
                          int sampling_frequency) OVERRIDE;
  virtual void AddData(scoped_array<int16_t> buffer,
                       int num_samples,
                       bool is_last_chunk) OVERRIDE;

  virtual void StopRecognizing() OVERRIDE;
  virtual void AbortRecognizing() OVERRIDE;

  // Accessors
  bool is_stopped() const { return !started_; }
  bool is_aborted() const { return aborted_; }
  bool has_sent_a_result() const { return sent_a_result_; }
  bool is_continuous() const { return continuous_; }

 private:
  // These methods are called on the IO Thread
  void InitDelegate();
  void ReadServerResponse(net::URLRequest* https_request);
  // TODO(arwar): Make this a static method
  bool IsEmptyResult(const char* result, int result_length);

  bool started_;
  bool aborted_;
  bool sent_a_result_;

  bool continuous_;
  bool send_interim_results_;

  scoped_ptr<SpeechRequestDelegate> up_request_delegate_;
  scoped_ptr<SpeechRequestDelegate> down_request_delegate_;
  scoped_ptr<LBRequestContext> request_context_;
  scoped_ptr<net::URLRequest> up_https_request_;
  scoped_ptr<net::URLRequest> down_https_request_;
  scoped_refptr<net::IOBuffer> https_response_;

  scoped_refptr<base::MessageLoopProxy> io_message_loop_;
};

#endif  // ENABLE(SCRIPTED_SPEECH)

#endif  // SRC_LB_SPEECH_RECOGNIZER_IMPL_H_
