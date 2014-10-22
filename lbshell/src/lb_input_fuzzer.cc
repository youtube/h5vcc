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
#include "lb_input_fuzzer.h"

#include "base/rand_util.h"
#include "third_party/WebKit/Source/WebCore/platform/WindowsKeyboardCodes.h"

#include "lb_web_view_host.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
LBInputFuzzer::LBInputFuzzer(LBWebViewHost* view, float time_mean,
                             float time_std, FuzzedKeys fuzzed_keys)
    : LBUserInputDevice(view) {
  time_mean_ = time_mean;
  time_std_ = time_std;

  RegisterKeys(fuzzed_keys);

  next_poll_time_ = base::TimeTicks::Now() + RandWaitTime();
}

// Registers the arrow keys first, and then everything else.
void LBInputFuzzer::RegisterKeys(FuzzedKeys fuzzed_keys) {
  key_set_.reserve(256);
  key_set_.push_back(WebCore::VKEY_DOWN);
  key_set_.push_back(WebCore::VKEY_LEFT);
  key_set_.push_back(WebCore::VKEY_RIGHT);
  key_set_.push_back(WebCore::VKEY_UP);
  if (fuzzed_keys >= kMappedKeys) {
    // Register commonly mapped controller keys
    key_set_.push_back(WebCore::VKEY_RETURN);
    key_set_.push_back(WebCore::VKEY_ESCAPE);
    key_set_.push_back(WebCore::VKEY_BROWSER_HOME);
    key_set_.push_back(WebCore::VKEY_BROWSER_SEARCH);
    key_set_.push_back(WebCore::VKEY_MEDIA_PLAY_PAUSE);
    key_set_.push_back(WebCore::VKEY_F1);
    key_set_.push_back(WebCore::VKEY_F2);
    key_set_.push_back(WebCore::VKEY_F3);
    key_set_.push_back(WebCore::VKEY_F4);
    key_set_.push_back(WebCore::VKEY_F5);

    if (fuzzed_keys >= kAllKeys) {
      // The following sequence is always a larger subset than what we can ever
      // get. We start from 0x08 since before that are control codes.
      // See |WebKit/Source/WebCore/platform/WindowsKeyboardCodes.h| for more
      // info.
      // This will push mappings for arrows and common keys multiple times,
      // but that is fine. It just means they will be slightly more likely
      // to be sent as input
      for (unsigned char key = 0x08; key < 0xFF; ++key) {
        key_set_.push_back(key);
      }

      key_set_.push_back(WebCore::VKEY_UNKNOWN);
    }
  }
}

void LBInputFuzzer::Poll() {
  const base::TimeTicks& now = base::TimeTicks::Now();
  if (next_poll_time_ < now) {
    int key_code = key_set_[rand() % key_set_.size()];

    // quick down & up key-action
    view_->InjectKeystroke(key_code, 0,
      static_cast<WebKit::WebKeyboardEvent::Modifiers>(0), false);

    next_poll_time_ = now + RandWaitTime();
  }
}

base::TimeDelta LBInputFuzzer::RandWaitTime() const {
  return base::TimeDelta::FromMicroseconds(
      1000000 * RandGaussian(time_mean_, time_std_));
}

// Uses Box-Mueller Transform.
double LBInputFuzzer::RandGaussian(double mean, double stddev) const {
  double theta = 2 * M_PI * base::RandDouble();
  double rho = sqrt(-2 * log(1 - base::RandDouble()));
  double scale = stddev * rho;
  double x = mean + scale * cos(theta);
  return x;
}
#endif  // __LB_SHELL__ENABLE_CONSOLE__

