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

#include "lb_system_stats_tracker.h"

#include "chrome_version_info.h"

#include "base/synchronization/waitable_event.h"
#if !defined(__LB_XB1__)
#include "media/base/shell_media_statistics.h"
#endif  // !defined(__LB_XB1__)
#include "skia/ext/SkMemory_new_handler.h"
#include "third_party/WebKit/Source/WTF/wtf/ExportMacros.h"  // needed before InspectorCounters.
#include "third_party/WebKit/Source/WebKit/chromium/src/WebInspectorExports.h"
#include "third_party/WebKit/Source/WTF/wtf/OSAllocator.h"

#include "lb_globals.h"
#include "steel_build_id.h"
#include "steel_version.h"

#if !defined(__LB_SHELL__FOR_RELEASE__)

namespace LB {

#if !defined(__LB_XB1__)
using media::ShellMediaStatistics;
#endif  // !defined(__LB_XB1__)

StatsUpdateThread* StatsUpdateThread::instance_ = NULL;

StatsUpdateThread::StatsUpdateThread()
    : stats_update_thread_("LB StatsUpdate Thread") {
  stats_update_thread_.Start();

  DCHECK(!instance_);
  instance_ = this;
}
StatsUpdateThread::~StatsUpdateThread() {
  DCHECK(instance_);
  instance_ = NULL;
}

AnyMessageLoopRepeatingTimer::AnyMessageLoopRepeatingTimer(
    const tracked_objects::Location& posted_from,
    MessageLoop* message_loop,
    const base::Closure& task,
    const base::TimeDelta& frequency) {
  message_loop_ = message_loop;

  timer_.reset(new base::Timer(true, true));
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &base::Timer::Start, base::Unretained(timer_.get()),
      posted_from, frequency, task));
}

AnyMessageLoopRepeatingTimer::~AnyMessageLoopRepeatingTimer() {
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &scoped_ptr<base::Timer>::reset, base::Unretained(&timer_),
      static_cast<base::Timer*>(NULL)));

  // Wait for the timer to actually be stopped.
  base::WaitableEvent timer_stopped(true, false);
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &base::WaitableEvent::Signal, base::Unretained(&timer_stopped)));
  timer_stopped.Wait();
}

namespace {
const double kMemUpdatePeriod = 2.0;
}  // namespace

SystemStatsTracker::SystemStatsTracker(MessageLoop* message_loop)
    : javascript_memory_("WebKit.JS.Memory", 0,
                         "Total memory used by JavaScript.")
    , used_app_memory_("Memory.App", 0,
                       "Total memory allocated via the app's allocators.")
    , used_sys_memory_("Memory.Sys", 0,
                       "Total memory allocated by the system, as opposed to "
                       "directly by the application.")
    , free_memory_("Memory.Free", 0,
                   "Total free application memory remaining.")
    , exe_memory_("Memory.Exe", 0,
                  "Total memory occupied by the size of the executable.")
    , hidden_memory_("Memory.Hidden", 0,
                     "Physical memory that is allocated but not mapped to a "
                     "virtual address.")
    , dom_nodes_("WebKit.DOM.NumNodes", 0,
                 "Number of nodes in the current DOM tree for the current "
                 "WebKit instance.")
    , skia_memory_("Skia.Memory", 0,
                   "Total memory used by the Skia system.")
    , build_id_("VersionInfo.LB.BuildId", STEEL_BUILD_ID,
                "The unique LBShell build ID based on the "
                "Git hash for the build and the time.")
    , chrome_version_("VersionInfo.Chrome.Version", CHROME_VERSION,
                      "The version of Chrome that LBShell is currently forked "
                      "from.")
    , lbshell_version_("VersionInfo.LB.Version", STEEL_VERSION,
                       "The manually specified version of LBShell for this "
                       "build.")
    , app_lifetime_("LB.Lifetime", 0,
                    "Application lifetime in milliseconds.")
#if !defined(__LB_XB1__)
    , media_time_elapsed_("Media.TimeElapsed", 0, "Playback Time Elapsed")
    , media_decrypt_load_("Media.DecryptLoad", 0,
                          "Total Time Spent In Deccrypting Media Data")
    , minimum_largest_free_shell_buffer_(
          "Media.MinimumLargestFreeShellBuffer", 0,
          "The minimum value of the largest free shell buffer block over time")
    , largest_shell_buffer_allocation_(
          "Media.LargestShellBufferAllocation", 0,
          "The size of the largest shell buffer block ever allocated")
    , media_audio_codec_("Media.Audio.Codec", 0, "Audio Codec")
    , media_audio_channels_("Media.Audio.Channels", 0, "Audio Channels")
    , media_audio_sample_rate_("Media.Audio.SampleRate", 0, "Sample Rate")
    , media_audio_underflow_("Media.Audio.Underflow", 0, "Underflow Count")
    , media_video_codec_("Media.Video.Codec", 0, "Video Codec")
    , media_video_width_("Media.Video.Size.Width", 0, "Video Width")
    , media_video_height_("Media.Video.Size.Height", 0, "Video Height")
    , media_video_decoder_load_("Media.Video.DecoderLoad", 0,
                                "Total Time Spent In Video Decoder")
    , media_video_decoder_max_("Media.Video.DecoderMax", 0,
                               "Max Time Spent In Decoding One Frame")
    , media_dropped_frames_("Media.Video.DroppedFrames", 0, "Dropped Frames")
    , media_video_decoder_min_backlog_("Media.Video.MinBacklog", 0,
                                       "Video Renderer Backlog")
    , media_late_frames_("Media.Video.LateFrames", 0, "Late Frames")
#endif  // !defined(__LB_XB1__)
    // Initialize this to minus one kMemUpdatePeriod in order to force an
    // immediate update.
    , last_mem_update_time_(-kMemUpdatePeriod)
    , start_time_(base::Time::Now()) {
  timer_.reset(new AnyMessageLoopRepeatingTimer(
      FROM_HERE, message_loop,
      base::Bind(&SystemStatsTracker::Update, base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(16)));
}

SystemStatsTracker::~SystemStatsTracker() {
  timer_.reset();
}

void SystemStatsTracker::Update() {
  javascript_memory_ = OSAllocator::getCurrentBytesAllocated();

  if (LB::Memory::IsCountEnabled()) {
    double cur_time = base::Time::Now().ToDoubleT();
    if (cur_time - last_mem_update_time_ > kMemUpdatePeriod) {
      // This can be expensive, so don't query it every frame.
      // Since these values are displayed in MB, 4 seconds is an acceptable
      // latency.
      LB::Memory::Info memory_info;
      LB::Memory::GetInfo(&memory_info);

      used_app_memory_ = memory_info.application_memory;
      used_sys_memory_ = memory_info.system_memory;
      free_memory_ = memory_info.free_memory;
      exe_memory_ = memory_info.executable_size;
      hidden_memory_ = memory_info.hidden_memory;

      last_mem_update_time_ = cur_time;
    }
  }

#if ENABLE(INSPECTOR)
  dom_nodes_ = WebKit::WebInspectorCounterValue(
      WebCore::InspectorCounters::NodeCounter);
#endif

  skia_memory_ = sk_get_bytes_allocated();

  base::TimeDelta lifetime = base::Time::Now() - start_time_;
  GetGlobalsPtr()->lifetime = static_cast<int>(lifetime.InMilliseconds());
  app_lifetime_ = GetGlobalsPtr()->lifetime;

#if !defined(__LB_XB1__)
  const ShellMediaStatistics& stat = ShellMediaStatistics::Instance();

  media_time_elapsed_ = stat.GetElapsedTime();
  double load = stat.GetTotalDuration(
      ShellMediaStatistics::STAT_TYPE_DECRYPT) / stat.GetElapsedTime();
  media_decrypt_load_ = static_cast<int>(load * 100) / 100.0;
  minimum_largest_free_shell_buffer_ = stat.GetMin(
      ShellMediaStatistics::STAT_TYPE_LARGEST_FREE_SHELL_BUFFER);
  largest_shell_buffer_allocation_ = stat.GetMax(
      ShellMediaStatistics::STAT_TYPE_ALLOCATED_SHELL_BUFFER_SIZE);

  media_audio_codec_ = stat.GetCurrent(
      ShellMediaStatistics::STAT_TYPE_AUDIO_CODEC);
  media_audio_channels_ = stat.GetCurrent(
      ShellMediaStatistics::STAT_TYPE_AUDIO_CHANNELS);
  media_audio_sample_rate_ = stat.GetCurrent(
      ShellMediaStatistics::STAT_TYPE_AUDIO_SAMPLE_PER_SECOND);
  media_audio_underflow_ = stat.GetCurrent(
      ShellMediaStatistics::STAT_TYPE_AUDIO_UNDERFLOW);

  media_video_codec_ = stat.GetCurrent(
      ShellMediaStatistics::STAT_TYPE_VIDEO_CODEC);
  media_video_width_ = stat.GetCurrent(
      ShellMediaStatistics::STAT_TYPE_VIDEO_WIDTH);
  media_video_height_ = stat.GetCurrent(
      ShellMediaStatistics::STAT_TYPE_VIDEO_HEIGHT);
  load = stat.GetTotalDuration(
      ShellMediaStatistics::STAT_TYPE_VIDEO_FRAME_DECODE) /
      stat.GetElapsedTime();
  media_video_decoder_load_ = static_cast<int>(load * 100) / 100.0;
  media_video_decoder_max_ = stat.GetMaxDuration(
      ShellMediaStatistics::STAT_TYPE_VIDEO_FRAME_DECODE);
  media_dropped_frames_ = stat.GetTimes(
      ShellMediaStatistics::STAT_TYPE_VIDEO_FRAME_DROP);
  media_video_decoder_min_backlog_ = stat.GetMin(
      ShellMediaStatistics::STAT_TYPE_VIDEO_RENDERER_BACKLOG);
  media_late_frames_ = stat.GetTimes(
      ShellMediaStatistics::STAT_TYPE_VIDEO_FRAME_LATE);
#endif  // !defined(__LB_XB1__)
}

}  // namespace LB

#endif  // !defined(__LB_SHELL__FOR_RELEASE__)
