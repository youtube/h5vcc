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

#ifndef MEDIA_BASE_SHELL_FILTER_GRAPH_LOG_H_
#define MEDIA_BASE_SHELL_FILTER_GRAPH_LOG_H_

#include "base/memory/ref_counted.h"
#include "base/platform_file.h"
#include "base/threading/thread.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT ShellFilterGraphLog
    : public base::RefCountedThreadSafe<ShellFilterGraphLog> {
 public:
  ShellFilterGraphLog();
  virtual ~ShellFilterGraphLog();
  // globally disable or enable graph logging
  static void SetGraphLoggingEnabled(bool enabled);
  // update the URL of the current page, used for log file nameing
  static void SetGraphLoggingPageURL(const char* url);

  // Stop logging, flushes and closes the writing file, drops all further
  // log entries.
  virtual void Stop();

  virtual void LogEvent(const uint32 object_id,
                        const uint32 event_id,
                        const int64 media_timestamp = 0,
                        const uint64 optional_state = 0);

 protected:
  friend class base::RefCountedThreadSafe<ShellFilterGraphLog>;

  // writing_thread work task
  void FlushDataToDisk();

  base::PlatformFile log_file_;
  uint32* log_buffer_;
  // unwritten_ and written_ must be accessed atomically.
  base::subtle::Atomic32 unwritten_;
  base::subtle::Atomic32 written_;
  // saved_ is only ever accessed by writing_thread_, safe to access normally.
  base::subtle::Atomic32 saved_;
  base::Thread writing_thread_;
  bool stopped_;
  base::TimeTicks start_time_ticks_;
};

}  // namespace media

#endif  // MEDIA_BASE_SHELL_FILTER_GRAPH_LOG_

