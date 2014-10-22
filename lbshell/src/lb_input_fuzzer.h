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
#ifndef SRC_LB_INPUT_FUZZER_H_
#define SRC_LB_INPUT_FUZZER_H_

#include <vector>

#include "base/time.h"

#include "lb_user_input_device.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
class LBInputFuzzer : public LBUserInputDevice {
 public:
  enum FuzzedKeys {
    kArrowKeys,
    kMappedKeys,
    kAllKeys,
  };
  // time-mean, time-std are in seconds
  LBInputFuzzer(LBWebViewHost* view, float time_mean, float time_std,
                FuzzedKeys fuzzed_keys);

  virtual void Poll() OVERRIDE;

 private:
  void RegisterKeys(FuzzedKeys fuzzed_keys);
  base::TimeDelta RandWaitTime() const;

  double RandGaussian(double mean, double stddev) const;

  double time_mean_;
  double time_std_;
  base::TimeTicks next_poll_time_;
  std::vector<int> key_set_;
};
#endif  // __LB_SHELL__ENABLE_CONSOLE__

#endif  // SRC_LB_INPUT_FUZZER_H_
