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

// The Steel version of W3C Speech Grammar significantly differs from the actual
// W3C Speech Grammar. Our grammar is passed in as a data url, and for now
// contains only one rule per grammar. Grammars are added / removed based on
// weight levels (0, 1].
// An example of the grammar we pass in is:
// <?xml version="1.0" encoding="ISO-8859-1"?>
// <!DOCTYPE grammar PUBLIC "-//W3C//DTD GRAMMAR 1.0//EN"
//     "http://www.w3.org/TR/speech-grammar/grammar.dtd">
// <grammar xml:lang="en_US" xmlns="http://www.w3.org/2001/06/grammar"
//     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
//     xsi:schemaLocation="http://www.w3.org/2001/06/grammar
//                         http://www.w3.org/TR/speech-grammar/grammar.xsd"
//     version="1.0">
//   <rule id="0">
//     <action>Go home</action>
//     <phrase>Go home</phrase>
//   </rule>
// </grammar>
// Note: This does not follow the DTD specified above, as our XML is a custom
// declaration.

#ifndef SRC_LB_SPEECH_GRAMMAR_H_
#define SRC_LB_SPEECH_GRAMMAR_H_

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"

#include "config.h"
#if ENABLE(SCRIPTED_SPEECH)
#include "third_party/WebKit/Source/WebCore/Modules/speech/SpeechGrammar.h"

namespace LB {

class SpeechRule {
 public:
  SpeechRule() : confidence(1.0f) {}
  std::string id;
  std::string phrase;
  std::string action;
  std::string pronunciation;
  float confidence;

 private:
  DISALLOW_COPY_AND_ASSIGN(SpeechRule);
};

class SpeechGrammar {
 public:
  SpeechGrammar() : valid_(false), weight_(0.0) {}
  const bool is_valid() { return valid_; }
  const ScopedVector<SpeechRule>& rules() const { return rules_; }
  const WTF::AtomicString& failure_reason() const { return failure_reason_; }
  const double weight() const { return weight_; }

  static scoped_ptr<SpeechGrammar> LoadFromGrammar(
      const WebCore::SpeechGrammar* grammar);
 private:
  static scoped_ptr<SpeechGrammar> LoadFromXml(const std::string& src);

  bool valid_;
  WTF::AtomicString failure_reason_;
  ScopedVector<SpeechRule> rules_;
  double weight_;

  DISALLOW_COPY_AND_ASSIGN(SpeechGrammar);
};

}  // namespace LB

#endif  // ENABLE(SCRIPTED_SPEECH)
#endif  // SRC_LB_SPEECH_GRAMMAR_H_
