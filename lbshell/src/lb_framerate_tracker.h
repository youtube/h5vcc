/*
 * Copyright 2013 Google Inc. All Rights Reserved.
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

#ifndef SRC_LB_FRAMERATE_TRACKER_H_
#define SRC_LB_FRAMERATE_TRACKER_H_

#include "external/chromium/base/synchronization/lock.h"

// A simple class to deal with the logic of tracking the framerate, requiring
// client code to call its Tick() method every frame at the same time.
// The object works by collecting samples of consecutive frames and then
// when enough have been acquired, computes frame statistics and begins
// a new sample.
class LBFramerateTracker {
 public:
  LBFramerateTracker();

  // Indicate that a frame has occurred.
  void Tick();

  int GetFrameCount() const {
    base::AutoLock auto_lock(monitor_lock_);
    return total_frames_;
  }

  struct Stats {
    Stats();

    // How many frames were used in these calculations
    int num_frames;
    // The average FPS
    double average_fps;
    // The minimum FPS
    double minimum_fps;
  };

  Stats GetStats() const {
    base::AutoLock auto_lock(monitor_lock_);
    return prev_sample_set_stats_;
  }

 private:
  // For thread safety
  mutable base::Lock monitor_lock_;

  // The total number of frames seen so far by this tracker
  int total_frames_;

  // The time that the last frame tick occurred
  double last_frame_time_;

  // The time that the first frame of this sample set occurred
  double sample_set_start_time_;

  // The longest time between frames in the current sample set
  double longest_frame_time_;

  // How many frame samples will be used in the stats calculations for the
  // current sample set
  int num_frames_for_sample_set_;

  // How many frames have been acquired so far for the current sample set
  int num_sample_set_frames_;

  // Computed statistics from the previous sample set
  Stats prev_sample_set_stats_;
};

#endif  // SRC_LB_FRAMERATE_TRACKER_H_
