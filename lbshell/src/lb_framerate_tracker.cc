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

#include "lb_framerate_tracker.h"

#include "external/chromium/base/logging.h"
#include "external/chromium/base/time.h"

LBFramerateTracker::Stats::Stats()
    : num_frames(0)
    , average_fps(0.0)
    , minimum_fps(0.0) {
}

LBFramerateTracker::LBFramerateTracker()
    : total_frames_(0)
    , first_frame_time_(0.0)
    , last_frame_time_(0.0)
    , sample_set_start_time_(0.0)
    , longest_frame_time_(0.0)
    , num_frames_for_sample_set_(10)
    , num_sample_set_frames_(0) {
}

namespace {
// Given the data collected from our frame samples, compute statistics
LBFramerateTracker::Stats ComputeStatsFromSampleSet(
    int num_frames, double longest_frame_time, double sample_set_time) {
  LBFramerateTracker::Stats stats;
  stats.num_frames = num_frames;

  DCHECK_GT(longest_frame_time, 0.0);
  stats.minimum_fps = 1.0 / longest_frame_time;

  stats.average_fps = num_frames / sample_set_time;

  return stats;
}
}

void LBFramerateTracker::Tick() {
  base::AutoLock auto_lock(monitor_lock_);

  ++total_frames_;
  double cur_time = base::Time::Now().ToDoubleT();

  // If this is the first frame, initialize the timers
  if (total_frames_ == 1) {
    first_frame_time_ = cur_time;
    last_frame_time_ = cur_time;
    sample_set_start_time_ = cur_time;
  } else {
    ++num_sample_set_frames_;

    // Keep track of how long the longest frame was
    double frame_time = cur_time - last_frame_time_;
    if (frame_time > longest_frame_time_) {
      longest_frame_time_ = frame_time;
    }

    // Did we meet our sample quota for the current sample set?
    if (num_sample_set_frames_ >= num_frames_for_sample_set_) {
      double sample_set_time = cur_time - sample_set_start_time_;

      // We have collected enough frames to compute stats for a new
      // sample set, so do that now.
      prev_sample_set_stats_ = ComputeStatsFromSampleSet(
          num_sample_set_frames_, longest_frame_time_, sample_set_time);

      // Reset for next sample set
      num_sample_set_frames_ = 0;
      sample_set_start_time_ = cur_time;
      longest_frame_time_ = 0;

      // Update the number of frames to take per sample set based on the
      // recently calculated average framerate, and then clamp it.
      num_frames_for_sample_set_ = prev_sample_set_stats_.average_fps;
      if (num_frames_for_sample_set_ < 4) num_frames_for_sample_set_ = 4;
      if (num_frames_for_sample_set_ > 100) num_frames_for_sample_set_ = 100;
    }

    last_frame_time_ = cur_time;
  }
}

int LBFramerateTracker::GetLifetime() const {
  base::AutoLock auto_lock(monitor_lock_);
  double cur_time = base::Time::Now().ToDoubleT();
  return cur_time - first_frame_time_;
}

