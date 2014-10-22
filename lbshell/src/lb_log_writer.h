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

#ifndef SRC_LB_LOG_WRITER_H_
#define SRC_LB_LOG_WRITER_H_

#include "lb_memory_manager.h"

#if LB_ENABLE_MEMORY_DEBUGGING

// Callers are responsible for ensuring multiple threads do not access the log
// writer at the same time.
namespace LB {
namespace Memory {
void LogWriterStart();
void LogWriterStop();
void LogWriterAppend(const char* data, size_t num_bytes);
}  // namespace Memory
}  // namespace LB

#endif  // LB_ENABLE_MEMORY_DEBUGGING

#endif  // SRC_LB_LOG_WRITER_H_
