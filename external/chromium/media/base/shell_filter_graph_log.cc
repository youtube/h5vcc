/*
 * Copyright 2012 Google Inc. All Rights Reserved.
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

#include "media/base/shell_filter_graph_log.h"

#include <inttypes.h>
#include <string>

#include "base/bind.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "base/time.h"
#include "media/base/shell_filter_graph_log_constants.h"

#if !defined(__LB_SHELL__FOR_RELEASE__)
extern const char* global_screenshot_output_path;
#endif

namespace media {

// 32 bytes per log entry:
//
// offset | size | type   | item
// -------+------+--------+-----------
//      0 |    4 | uint32 | object id (see shell_filter_graph_log_constants.h)
//      4 |    4 | uint32 | event id
//      8 |    8 | int64  | system clock reading in milliseconds
//     16 |    8 | int64  | media timestamp in milliseconds
//     24 |    8 | varies | state-specific info
//
static const base::subtle::Atomic32 kGraphLogEntrySize = 32;
// The log buffer size must remain a multiple of log entry size.
static const int kGraphLogBufferSize = kGraphLogEntrySize * 16 * 1024;

static const int64 kWriteDelayMilliseconds = 1000;

static bool g_graph_logging_enabled = false;
static std::string g_graph_logging_page_url;

// static
void ShellFilterGraphLog::SetGraphLoggingEnabled(bool enabled) {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  g_graph_logging_enabled = enabled;
#endif
}

// static
void ShellFilterGraphLog::SetGraphLoggingPageURL(const char* url) {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (g_graph_logging_enabled) {
    g_graph_logging_page_url = url;
  }
#endif
}

ShellFilterGraphLog::ShellFilterGraphLog()
    : log_buffer_(NULL)
    , unwritten_(0)
    , written_(0)
    , saved_(0)
    , writing_thread_("FilterGraphLogWriter")
    , stopped_(false) {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (g_graph_logging_enabled) {
    DCHECK(!g_graph_logging_page_url.empty());
    // use timestamp as file name
    start_time_ticks_ = base::TimeTicks::HighResNow();
    std::string file_path_str = base::StringPrintf("%s/%"PRId64".log",
        global_screenshot_output_path, start_time_ticks_.ToInternalValue());
    FilePath log_file_path = FilePath(file_path_str);
    log_file_ = base::CreatePlatformFile(log_file_path,
                                         base::PLATFORM_FILE_CREATE_ALWAYS |
                                         base::PLATFORM_FILE_WRITE,
                                         NULL,
                                         NULL);
    DCHECK_GE(log_file_, 0);
    log_buffer_ = (uint32*)malloc(kGraphLogBufferSize);
    DLOG(INFO) << base::StringPrintf(
        "logging media graph state for url %s into file: %s",
        g_graph_logging_page_url.c_str(),
        file_path_str.c_str());
    // copy the page URL into the buffer first
    memcpy(log_buffer_,
           g_graph_logging_page_url.c_str(),
           g_graph_logging_page_url.length());
    // write at least one '\0' to terminate string, and align
    // rest of log with kGraphLogEntrySize
    int url_aligned_size =
        ((g_graph_logging_page_url.length() + kGraphLogEntrySize) /
            kGraphLogEntrySize) * kGraphLogEntrySize;
    // need at least one additional byte of zeros, to terminate string here
    DCHECK_GT(url_aligned_size, g_graph_logging_page_url.length());
    // write zeros to term url string and align buffer
    memset(log_buffer_ + g_graph_logging_page_url.length(), 0,
        url_aligned_size - g_graph_logging_page_url.length());
    // bump unwritten and written past the url string
    unwritten_ = url_aligned_size;
    written_ = url_aligned_size;

    // now start the writing thread
    writing_thread_.Start();
    // post first writing thread task, it will self-post after that
    writing_thread_.message_loop()->PostDelayedTask(FROM_HERE,
        base::Bind(&ShellFilterGraphLog::FlushDataToDisk, this),
        base::TimeDelta::FromMilliseconds(kWriteDelayMilliseconds));
  }
#endif
}

ShellFilterGraphLog::~ShellFilterGraphLog() {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (g_graph_logging_enabled) {
    // should have already stopped the filter graph log
    DCHECK(!writing_thread_.IsRunning());
    free(log_buffer_);
    log_buffer_ = NULL;
    base::ClosePlatformFile(log_file_);
  }
#endif
}

void ShellFilterGraphLog::Stop() {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (g_graph_logging_enabled) {
    // set flag indicating we've been stopped
    stopped_ = true;
    // post a flush for right now
    if (writing_thread_.IsRunning()) {
      writing_thread_.message_loop()->PostTask(FROM_HERE,
          base::Bind(&ShellFilterGraphLog::FlushDataToDisk, this));
    }
    // will execute any pending tasks before stopping
    writing_thread_.Stop();
  }
#endif
}


void ShellFilterGraphLog::FlushDataToDisk() {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (g_graph_logging_enabled) {
    // sample a current copy of written_
    base::subtle::Atomic32 written = base::subtle::NoBarrier_CompareAndSwap(
        &written_, 0, 0);
    DCHECK_GE(written, saved_);
    // saved_ points at first unsaved word in the stream :). There are
    // potentially (unwritten_ - written_) number of inconsistent bytes at the
    // leading edge of the log. We write everything from saved_ to that point,
    // or the end of the buffer, whichever comes first.
    int bytes_to_write = written - saved_ -
        (base::subtle::NoBarrier_CompareAndSwap(&unwritten_, 0, 0) - written);
    // i don't get out of bed for less than 1K
    if (bytes_to_write >= 1024 || stopped_) {
      int32 saved_buffer_pos = saved_ % kGraphLogBufferSize;
      char* bytes = (char*)log_buffer_ + saved_buffer_pos;
      // don't read past the end of the circular buffer
      if (saved_buffer_pos + bytes_to_write > kGraphLogBufferSize) {
        bytes_to_write = kGraphLogBufferSize - saved_buffer_pos;
      }
      int bytes_written = base::WritePlatformFileAtCurrentPos(log_file_,
          bytes, bytes_to_write);
      DCHECK_EQ(bytes_written, bytes_to_write);
      base::FlushPlatformFile(log_file_);
      saved_ += bytes_written;
    }
    if (!stopped_) {
      writing_thread_.message_loop()->PostDelayedTask(FROM_HERE,
          base::Bind(&ShellFilterGraphLog::FlushDataToDisk, this),
          base::TimeDelta::FromMilliseconds(kWriteDelayMilliseconds));
    }
  }
#endif
}

void ShellFilterGraphLog::LogEvent(const uint32 object_id,
                                   const uint32 event_id,
                                   const int64 media_timestamp,
                                   const uint64 optional_state) {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (g_graph_logging_enabled && !stopped_) {
    // first thing is collect the clock time
    base::TimeDelta t = base::TimeTicks::HighResNow() - start_time_ticks_;
    int64 timestamp = t.InMicroseconds();
    // bump unwritten_ counter forward to claim a space in the buffer
    base::subtle::Atomic32 offset = base::subtle::NoBarrier_AtomicIncrement(
        &unwritten_, kGraphLogEntrySize);
    DCHECK(offset >= 0);
    // make offset a uint32 pointer into the log buffer
    offset = (offset % kGraphLogBufferSize) / sizeof(uint32);
    // first log object id
    log_buffer_[offset++] = object_id;
    // then log event id
    log_buffer_[offset++] = event_id;
    // log event time
    *(int64*)(log_buffer_ + offset) = timestamp;
    offset += 2;
    // save media timestamp
    *(int64*)(log_buffer_ + offset) = media_timestamp;
    offset += 2;
    *(uint64*)(log_buffer_ + offset) = optional_state;
    // increment written_ to track what log data is final
    base::subtle::NoBarrier_AtomicIncrement(&written_, kGraphLogEntrySize);
  }
#endif
}

}  // namespace media

