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

#include "lb_speech_recognition_client.h"

#if ENABLE(SCRIPTED_SPEECH)

#include <algorithm>
#include <string>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/string_piece.h"

#include "lb_resource_loader_bridge.h"

namespace {
const base::TimeDelta kWaitBetweenResults = base::TimeDelta::FromSeconds(5);
}  // namespace

LBSpeechRecognitionClient::LBSpeechRecognitionClient(
    scoped_ptr<LBPlatformMic> microphone,
    scoped_ptr<LBSpeechRecognizer> recognizer)
    : microphone_(microphone.Pass())
    , recognizer_(recognizer.Pass())
    , continuous_(false)
    , send_interim_results_(false)
    , started_(false)
    , aborted_(false) {
  io_message_loop_ = LBResourceLoaderBridge::GetIoThread();

  DCHECK(microphone_);
  microphone_->SetClientInstance(this);

  DCHECK(recognizer_);
  recognizer_->SetClientInstance(this);

  timer_.reset(new base::Timer(
        FROM_HERE,
        kWaitBetweenResults,
        base::Bind(&LBSpeechRecognitionClient::TimeOut, base::Unretained(this)),
        true /* is_repeating */));
}

LBSpeechRecognitionClient::~LBSpeechRecognitionClient() {
  DCHECK(microphone_);
  microphone_->RemoveClientInstance(this);

  DCHECK(recognizer_);
  recognizer_->RemoveClientInstance(this);
}

void LBSpeechRecognitionClient::start(
    WebCore::SpeechRecognition* recognition,
    const WebCore::SpeechGrammarList* grammars,
    const WTF::String& lang,
    bool continuous,
    bool interimResults,
    unsigned long maxAlternatives) {
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());

  DCHECK_EQ(recognition_, recognition);
  recognition_->didStart();

  aborted_ = false;
  lang_ = lang;
  continuous_ = continuous;
  send_interim_results_ = interimResults;
  max_alternatives_ = static_cast<unsigned int>(maxAlternatives);

  DCHECK(microphone_);
  // This may create another thread for audio input processing,
  // depending on the platform-specific implementation of the microphone.
  microphone_->StartListening();

  started_ = true;
}

void LBSpeechRecognitionClient::stop(WebCore::SpeechRecognition* recognition) {
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());
  DCHECK_EQ(recognition_, recognition);

  // stop() may be called multiple times (from within this class, and/or from
  // the microphone (when it times out), and/or from the webpage)
  if (started_) {
    DCHECK(microphone_);
    microphone_->StopListening();

    DCHECK(recognizer_);
    recognizer_->StopRecognizing();

    timer_->Stop();

    started_ = false;
  }
}

void LBSpeechRecognitionClient::abort(
    WebCore::SpeechRecognition* recognition) {
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());
  DCHECK_EQ(recognition_, recognition);

  // The Web Speech API specification states: "If the abort method is called on
  // an object which is already stopped or aborting (that is, start was never
  // called on it, the end or error event has fired on it, or abort was
  // previously called on it), the user agent must ignore the call."
  if (started_) {
    aborted_ = true;

    // Report the error
    SendErrorToRecognition(SpeechRecognitionErrorCode::ErrorCodeAborted,
                           "Speech recognition was aborted.");

    DCHECK(recognizer_);
    recognizer_->AbortRecognizing();

    stop(recognition);
    recognition_->didEnd();
  }
}

void LBSpeechRecognitionClient::setRecognitionInstance(
    WebCore::SpeechRecognition* recognition) {
  if (!webkit_message_loop_) {
    webkit_message_loop_ = base::MessageLoopProxy::current();
  }
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());

  DCHECK(recognition);
  recognition_ = recognition;
}

void LBSpeechRecognitionClient::removeRecognitionInstance(
    WebCore::SpeechRecognition* recognition) {
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());

  if (recognition_ == recognition) {
    recognition_ = NULL;
  } else {
    NOTREACHED();
  }
}

//-----------------------------------------------------------------------------
// LBPlatformMic::Callback

void LBSpeechRecognitionClient::StartedAudio() {
  webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
      &WebCore::SpeechRecognition::didStartAudio,
      base::Unretained(recognition_)));

  io_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBSpeechRecognitionClient::StartRecognizing,
      base::Unretained(this)));

// For writing to WAV file
#if !defined(__LB_SHELL__FOR_RELEASE__)
  test_probe_.reset(new media::ShellWavTestProbe);
  test_probe_->Initialize("audiointest.wav" /* file_name */,
                          1 /* channel_count (mono = 1 channel) */,
                          16000 /* samples_per_second (aka frequency) */,
                          16 /* bits_per_sample (2-byte samples * 8 bits) */,
                          false /* use_floats */);
  test_probe_->CloseAfter(LBPlatformMic::MaxSpeechTime().InMilliseconds());
#endif
}

void LBSpeechRecognitionClient::EndedAudio() {
  webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
      &WebCore::SpeechRecognition::didEndAudio,
      base::Unretained(recognition_)));

  // This calls stop() in the case that voice recognition times out,
  // as opposed to being stopped by pressing the 'stop' button on the webpage
  // (in that case, the webpage tells SpeechRecognition to call stop() here).
  webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBSpeechRecognitionClient::stop,
      base::Unretained(this),
      base::Unretained(recognition_)));


// For writing to WAV file
#if !defined(__LB_SHELL__FOR_RELEASE__)
    test_probe_->Close();
#endif
}

void LBSpeechRecognitionClient::SentData(int16_t* buffer,
                                         int num_elements,
                                         bool is_last_chunk) {
  // Package up the data before passing it to a different thread
  scoped_array<int16_t> data;
  data.reset(new int16_t[num_elements]);
  memcpy(data.get(), buffer, num_elements*sizeof(int16_t));

  io_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBSpeechRecognitionClient::SendDataToRecognizer,
      base::Unretained(this),
      base::Passed(&data), num_elements, is_last_chunk));

// For writing to WAV file
#if !defined(__LB_SHELL__FOR_RELEASE__)
  test_probe_->AddData(reinterpret_cast<const uint8*>(buffer),
                       num_elements * (sizeof(int16_t) / sizeof(char)),
                       0 /* timestamp */);
#endif
}

void LBSpeechRecognitionClient::ReceivedError(CallbackErrorCode error_code,
                                              const char* error_message) {
  webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBSpeechRecognitionClient::SendErrorToRecognition,
      base::Unretained(this),
      ConvertErrorCode(error_code), error_message));
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LBSpeechRecognizer::Callback

void LBSpeechRecognitionClient::ReceivedResult(scoped_array<char> result,
                                               int result_length) {
  webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBSpeechRecognitionClient::SendResultToRecognition,
      base::Unretained(this),
      base::Passed(&result), result_length));
}

void LBSpeechRecognitionClient::ReceivedNetworkError(
    const char* error_message) {
  webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBSpeechRecognitionClient::SendErrorToRecognition,
      base::Unretained(this),
      SpeechRecognitionErrorCode::ErrorCodeNetwork, error_message));
}

void LBSpeechRecognitionClient::ReceivedNoSpeechError(
    const char* error_message) {
  webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBSpeechRecognitionClient::SendErrorToRecognition,
      base::Unretained(this),
      SpeechRecognitionErrorCode::ErrorCodeNoSpeech, error_message));
}
//-----------------------------------------------------------------------------

void LBSpeechRecognitionClient::SendErrorToRecognition(
    SpeechRecognitionErrorCode error_code,
    const char* error_message) {
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());

  recognition_->didReceiveError(WebCore::SpeechRecognitionError::create(
      error_code, error_message));

  stop(recognition_);
  recognition_->didEnd();
}

void LBSpeechRecognitionClient::SendResultToRecognition(
    scoped_array<char> result,
    int result_length) {
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());

  SpeechRecognitionResults interim_results;
  SpeechRecognitionResults final_results;

  SpeechRecognitionAlternatives alternatives;
  bool is_final = false;
  bool success = ParseResult(result.Pass(), result_length,
                             &alternatives, &is_final);
  if (!success)
    return;  // An error has already been sent to recognition_

  if (is_final) {
    final_results.append(WebCore::SpeechRecognitionResult::create(
        alternatives, is_final));
  } else if (send_interim_results_) {
    interim_results.append(WebCore::SpeechRecognitionResult::create(
        alternatives, is_final));
  }  // else don't do anything with the interim results; it'll be empty

  recognition_->didReceiveResults(final_results /* newFinalResults */,
                                  interim_results /* currentInterimResults */);

  // If the timer is not already started, Reset() will start it; and if it is,
  // it defers the action (calling TimeOut()) again by the delay (5 seconds).
  timer_->Reset();

  // TODO(arwar): We are running under the assumption here that recognition
  // must be continuous to have interim results. Documentation says the two are
  // unrelated, but I don't see how that makes sense. Dig further into this.

  // If we're not doing continuous recognition, don't send more than one result
  if (!continuous_) {
    stop(recognition_);
    // We don't want to receive anything else after this
    recognizer_->AbortRecognizing();
    recognition_->didEnd();
  }
}

void LBSpeechRecognitionClient::StartRecognizing() {
  DCHECK(io_message_loop_->BelongsToCurrentThread());

  recognizer_->Initialize(lang_.utf8().data(),
                          continuous_,
                          send_interim_results_,
                          microphone_->sampling_frequency());
}


void LBSpeechRecognitionClient::SendDataToRecognizer(
    scoped_array<int16_t> buffer,
    int num_samples,
    bool is_last_chunk) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());

  recognizer_->AddData(buffer.Pass(), num_samples, is_last_chunk);
}

bool LBSpeechRecognitionClient::ParseResult(
    scoped_array<char> result,
    int result_length,
    SpeechRecognitionAlternatives* alternatives_out,
    bool* is_final_out) {
  base::StringPiece json_string(result.get(), result_length);
  DLOG(INFO) << "Result received: " << json_string;

  int error_code = 0;
  std::string error_message;
  scoped_ptr<base::Value> parse_tree;
  parse_tree.reset(base::JSONReader::ReadAndReturnError(
      json_string,
      base::JSONParserOptions::JSON_PARSE_RFC,
      &error_code,
      &error_message));

  if (!parse_tree || error_code > 0) {
    // TODO(arwar): Investigate encountering this error because of a bad
    // Internet connection, which may cause URLRequest::Read() to read
    // pieces of json results rather than an entire one. To solve that,
    // we'd have to create a new IOBuffer for every call to Read() and have
    // some sort of streaming json reader to which OnReadCompleted() sends
    // each buffer, that tells us when it's received a complete json,
    // which we can then try to parse.

    // Report the error
    webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
        &LBSpeechRecognitionClient::SendErrorToRecognition,
        base::Unretained(this),
        SpeechRecognitionErrorCode::ErrorCodeNetwork,
        "There was an error trying to parse the JSON."));

    DLOG(WARNING) << "Error parsing JSON result. Message from json_reader: "
                  << error_message;
    return false;
  }

  DCHECK(alternatives_out);
  DCHECK(is_final_out);
  bool success = ConstructAlternatives(parse_tree.Pass(),
                                       alternatives_out,
                                       is_final_out);
  if (!success) {
    // Report the error
    webkit_message_loop_->PostTask(FROM_HERE, base::Bind(
        &LBSpeechRecognitionClient::SendErrorToRecognition,
        base::Unretained(this),
        SpeechRecognitionErrorCode::ErrorCodeNetwork,
        "Could not parse json response."));

    DLOG(WARNING) << "Unable to parse json response from Speech API.";
    return false;
  }

  return true;
}

bool LBSpeechRecognitionClient::ConstructAlternatives(
    const scoped_ptr<base::Value> parse_tree,
    SpeechRecognitionAlternatives* alternatives_out,
    bool* is_final_out) const {
  // EXAMPLE JSON RESPONSE:
  // {                                          /* root */
  //   "result":
  //   [                                        /* result */
  //     {                                      /* dict */
  //       "alternative":
  //       [                                    /* alternative */
  //         {                                  /* recognition_result */
  //           "transcript":"cats and dogs",    /* transcript */
  //           "confidence" : 0.98762912        /* confidence */
  //         },
  //         {                                  /* recognition_result */
  //           "transcript":"Katz and Dogz"     /* transcript */
  //         }
  //       ],
  //       "final":true                         /* is_final_out */
  //     }
  //   ],
  //   "result_index":0                         /* result_index */
  // }

  // CORRESPONDING PARSE TREE:
  //                                        root
  //                                       /    \
  //                                 result      result_index
  //                                 root[0]        root[1]
  //                                   |
  //                                  dict
  //                                result[0]
  //                                /       \
  //                     alternative         is_final_out
  //                       dict[0]            dict[1]
  //                    /         \
  //          rec_result           rec_result
  //        alternative[0]       alternative[1]
  //          /       \                   |
  //  transcript      confidence       transcript
  // rec_result[0]   rec_result[1]    rec_result[0]

  bool success = false;
  const DictionaryValue* root;
  success = parse_tree->GetAsDictionary(&root);
  if (!success) return false;

  const ListValue* result;            // root[0] = result
  success = root->GetList("result", &result);
  if (!success) return false;

  int result_index;             // root[1] = result_index
  success = root->GetInteger("result_index", &result_index);
  if (!success) return false;

  const DictionaryValue* dict;        // result[0] = dict
  success = result->GetDictionary(0, &dict);
  if (!success) return false;

  const ListValue* alternative;       // dict[0] = alternative
  success = dict->GetList("alternative", &alternative);
  if (!success) return false;

  if (dict->HasKey("final")) {  // dict[1] = is_final_out
    success = dict->GetBoolean("final", is_final_out);
    if (!success) return false;
  } else {
    *is_final_out = false;
  }

  unsigned int num_alternatives = std::min(
      static_cast<unsigned int>(alternative->GetSize()), max_alternatives_);
  for (int index = 0; index < num_alternatives; index++) {
    const DictionaryValue* recognition_result;
    string transcript;
    double confidence;
                                // alternative[index] = recognition_result
    success = alternative->GetDictionary(index, &recognition_result);
    if (!success) continue;  // skip this alternative
                                // recognition_result[0] = transcript
    success = recognition_result->GetString("transcript", &transcript);
    if (!success) continue;  // skip this alternative

    if (recognition_result->HasKey("confidence")) {
                                // recognition_result[1] = confidence
      success = recognition_result->GetDouble("confidence", &confidence);
      if (!success) continue;  // skip this alternative
    } else {
      confidence = 0.0;
    }

    alternatives_out->append(WebCore::SpeechRecognitionAlternative::create(
        WTF::String::fromUTF8(transcript.c_str()), confidence));
  }  // for

  if (alternatives_out->size() == 0 && alternative->GetSize() > 0) {
    // Couldn't parse any of the alternatives received
    return false;
  }

  return true;
}

LBSpeechRecognitionClient::SpeechRecognitionErrorCode
    LBSpeechRecognitionClient::ConvertErrorCode(CallbackErrorCode error_code) {
  switch (error_code) {
    case CallbackErrorCode::ErrorOpening:
    case CallbackErrorCode::ErrorClosing:
      return SpeechRecognitionErrorCode::ErrorCodeAborted;
    case CallbackErrorCode::ErrorSilence:
      return SpeechRecognitionErrorCode::ErrorCodeNotAllowed;
    case CallbackErrorCode::ErrorListening:
      return SpeechRecognitionErrorCode::ErrorCodeAudioCapture;
  }
}

void LBSpeechRecognitionClient::TimeOut() {
  stop(recognition_);
  // We don't want to receive anything else after this
  recognizer_->AbortRecognizing();
  recognition_->didEnd();
}

#endif  // ENABLE(SCRIPTED_SPEECH)
