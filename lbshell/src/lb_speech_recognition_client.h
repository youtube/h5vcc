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

#ifndef SRC_LB_SPEECH_RECOGNITION_CLIENT_H_
#define SRC_LB_SPEECH_RECOGNITION_CLIENT_H_

#include "config.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "base/memory/scoped_ptr.h"
#include "base/message_loop_proxy.h"
#include "base/time.h"
#include "base/timer.h"
#include "base/values.h"
#include "media/audio/shell_wav_test_probe.h"
#include "third_party/WebKit/Source/WebCore/Modules/speech/SpeechRecognition.h"
#include "third_party/WebKit/Source/WebCore/Modules/speech/SpeechRecognitionAlternative.h"
#include "third_party/WebKit/Source/WebCore/Modules/speech/SpeechRecognitionClient.h"
#include "third_party/WebKit/Source/WebCore/Modules/speech/SpeechRecognitionError.h"
#include "third_party/WebKit/Source/WebCore/Modules/speech/SpeechRecognitionResult.h"

#include "lb_platform_mic.h"
#include "lb_speech_recognizer.h"

class LBSpeechRecognitionClient : public WebCore::SpeechRecognitionClient,
                                  public LBPlatformMic::Callback,
                                  public LBSpeechRecognizer::Callback {
 public:
  LBSpeechRecognitionClient(scoped_ptr<LBPlatformMic> microphone,
                            scoped_ptr<LBSpeechRecognizer> recognizer);
  virtual ~LBSpeechRecognitionClient();

  // Overriden from SpeechRecognitionClient; called on the WebKit thread
  virtual void start(WebCore::SpeechRecognition* recognition,
                     const WebCore::SpeechGrammarList* grammarList,
                     const WTF::String& lang,
                     bool continuous,
                     bool interimResults,
                     unsigned long maxAlternatives) OVERRIDE;
  virtual void stop(WebCore::SpeechRecognition*) OVERRIDE;
  virtual void abort(WebCore::SpeechRecognition*) OVERRIDE;
  virtual void setRecognitionInstance(WebCore::SpeechRecognition*) OVERRIDE;
  virtual void removeRecognitionInstance(WebCore::SpeechRecognition*) OVERRIDE;

  // Callbacks, overriden from LBPlatformMic::Callback; these methods may
  // be called from an AudioIn thread, depending on the platform-specific
  // implementation of the microphone.
  virtual void StartedAudio() OVERRIDE;
  virtual void EndedAudio() OVERRIDE;
  virtual void SentData(int16_t* buffer, int size,
                        bool is_last_chunk) OVERRIDE;
  virtual void ReceivedError(LBPlatformMic::Callback::ErrorCode error_code,
                             const char* error_message) OVERRIDE;

  // Callbacks, overriden from LBSpeechRecognizer::Callback; these methods
  // will be called from the IO thread.
  virtual void ReceivedResult(scoped_array<char> result,
                              int result_length) OVERRIDE;
  virtual void ReceivedNetworkError(const char* error_message) OVERRIDE;
  virtual void ReceivedNoSpeechError(const char* error_message) OVERRIDE;

  // Accessors
  bool is_continuous() const { return continuous_; }
  unsigned int max_alternatives() const { return max_alternatives_; }
  bool should_send_interim_results() const { return send_interim_results_; }

 private:
  typedef WebCore::SpeechRecognitionError::ErrorCode SpeechRecognitionErrorCode;
  typedef LBPlatformMic::Callback::ErrorCode CallbackErrorCode;
  typedef WTF::Vector<WTF::RefPtr<WebCore::SpeechRecognitionResult>>
      SpeechRecognitionResults;
  typedef WTF::Vector<WTF::RefPtr<WebCore::SpeechRecognitionAlternative>>
      SpeechRecognitionAlternatives;

  // These methods are called on the WebKit Thread
  void SendErrorToRecognition(SpeechRecognitionErrorCode error_code,
                              const char* error_message);
  void SendResultToRecognition(scoped_array<char> result, int result_length);

  // These methods are called on the IO Thread
  void StartRecognizing();
  void SendDataToRecognizer(scoped_array<int16_t> buffer,
                            int size,
                            bool is_last_chunk);

  // Helper functions
  bool ParseResult(scoped_array<char> result,
                   int result_length,
                   SpeechRecognitionAlternatives* alternatives_out,
                   bool* is_final_out);
  bool ConstructAlternatives(const scoped_ptr<base::Value> parse_tree,
                             SpeechRecognitionAlternatives* alternatives_out,
                             bool* is_final_out) const;
  SpeechRecognitionErrorCode ConvertErrorCode(CallbackErrorCode error_code);
  void TimeOut();

  bool started_;
  bool aborted_;

  WTF::String lang_;
  bool continuous_;
  unsigned int max_alternatives_;
  bool send_interim_results_;

  WebCore::SpeechRecognition* recognition_;
  scoped_ptr<LBPlatformMic> microphone_;
  scoped_ptr<LBSpeechRecognizer> recognizer_;

  scoped_ptr<base::Timer> timer_;

  scoped_refptr<base::MessageLoopProxy> webkit_message_loop_;
  scoped_refptr<base::MessageLoopProxy> io_message_loop_;

// For writing to WAV file
#if !defined(__LB_SHELL__FOR_RELEASE__)
  scoped_ptr<media::ShellWavTestProbe> test_probe_;
#endif
};

#endif  // ENABLE(SCRIPTED_SPEECH)

#endif  // SRC_LB_SPEECH_RECOGNITION_CLIENT_H_
