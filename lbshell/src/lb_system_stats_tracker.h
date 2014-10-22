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

#ifndef SRC_LB_SYSTEM_STATS_TRACKER_H_
#define SRC_LB_SYSTEM_STATS_TRACKER_H_

#include "base/message_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "base/time.h"
#include "base/timer.h"

#include "lb_console_values.h"
#include "lb_memory_manager.h"

#if !defined(__LB_SHELL__FOR_RELEASE__)

namespace LB {

// TODO(aabtop): We should be able to get rid of this once all LBWebViewHosts
//               have their own message loops, then they can be the homes.
// Provides a home for a stats tracking thread that we can run repeating timer
// events upon.
class StatsUpdateThread {
 public:
  StatsUpdateThread();
  ~StatsUpdateThread();

  static StatsUpdateThread* GetPtr() { return instance_; }
  MessageLoop* GetMessageLoop() { return stats_update_thread_.message_loop(); }

 private:
  static StatsUpdateThread* instance_;
  base::Thread stats_update_thread_;
};

// Utility function to extend the functionality of base::RepeatingTimer.
// base::RepeatingTimer automatically assumes that we would like to associate
// the timer with the thread's current message loop.  This class generalizes
// the orginal by removing the message loop assumption and taking it as a
// parameter.
class AnyMessageLoopRepeatingTimer {
 public:
  AnyMessageLoopRepeatingTimer(const tracked_objects::Location& posted_from,
                               MessageLoop* message_loop,
                               const base::Closure& task,
                               const base::TimeDelta& frequency);
  ~AnyMessageLoopRepeatingTimer();

 private:
  scoped_ptr<base::Timer> timer_;
  MessageLoop* message_loop_;
};

// This class will reach out to Steel/Chrome/WebKit systems and query for
// interesting information.  It will populate CVals with that information,
// opening it up to be displayed on the HUD or logged.
class SystemStatsTracker {
 public:
  explicit SystemStatsTracker(MessageLoop* message_loop);
  ~SystemStatsTracker();

  // Queries the systems to update the tracked values
  void Update();

 private:
  LB::CVal<size_t> javascript_memory_;

  LB::CVal<size_t> used_app_memory_;
  LB::CVal<size_t> used_sys_memory_;
  LB::CVal<size_t> free_memory_;
  LB::CVal<size_t> exe_memory_;
  LB::CVal<size_t> hidden_memory_;

  LB::CVal<int> dom_nodes_;

  LB::CVal<std::string> build_id_;
  LB::CVal<std::string> chrome_version_;
  LB::CVal<std::string> lbshell_version_;

  LB::CVal<size_t> skia_memory_;

  LB::CVal<uint32_t> app_lifetime_;

#if !defined(__LB_XB1__)
  // How much time has been elapsed in the playback of the current video.
  LB::CVal<double> media_time_elapsed_;
  // Time spent decrypting / time elapsed.
  LB::CVal<double> media_decrypt_load_;
  // The minimum value of the largest free shell buffer block over time.
  LB::CVal<size_t> minimum_largest_free_shell_buffer_;
  // The size of the largest shell buffer block ever allocated.
  LB::CVal<size_t> largest_shell_buffer_allocation_;

  LB::CVal<int> media_audio_codec_;
  LB::CVal<int> media_audio_channels_;
  LB::CVal<int> media_audio_sample_rate_;
  LB::CVal<int> media_audio_underflow_;

  LB::CVal<int> media_video_codec_;
  LB::CVal<int> media_video_width_;
  LB::CVal<int> media_video_height_;
  // Time spent decoding / time elapsed.
  LB::CVal<double> media_video_decoder_load_;
  // The max time it takes us to decode a frame.
  LB::CVal<double> media_video_decoder_max_;
  LB::CVal<double> media_dropped_frames_;
  // The min frames cached in the video renderer.
  LB::CVal<double> media_video_decoder_min_backlog_;
  // The frame that arrives late to the video renderer.
  LB::CVal<double> media_late_frames_;
#endif  // !defined(__LB_XB1__)

  double last_mem_update_time_;

  base::Time start_time_;

  scoped_ptr<AnyMessageLoopRepeatingTimer> timer_;
};

}  // namespace LB

#endif  // !defined(__LB_SHELL__FOR_RELEASE__)

#endif  // SRC_LB_SYSTEM_STATS_TRACKER_H_
