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

#ifndef MEDIA_BASE_MOCK_SHELL_FILTER_GRAPH_LOG_H_
#define MEDIA_BASE_MOCK_SHELL_FILTER_GRAPH_LOG_H_

#include "media/base/shell_filter_graph_log.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {

class MockShellFilterGraphLog : public ShellFilterGraphLog {
 public:
  MockShellFilterGraphLog() : ShellFilterGraphLog() { }

  // ShellFilterGraphLog implementation
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD4(LogEvent,
               void(const uint32,
                    const uint32,
                    const int64,
                    const uint64));
};

}  // namespace media

#endif // MEDIA_BASE_MOCK_SHELL_FILTER_GRAPH_LOG_H_
