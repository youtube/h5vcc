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
#include "base/process_util.h"

#include <unistd.h>

#include "base/logging.h"
#include "base/process.h"

namespace base {

ProcessHandle GetCurrentProcessHandle() {
  return GetCurrentProcId();
}

ProcessId GetCurrentProcId() {
  return getpid();
}

ProcessMetrics* ProcessMetrics::CreateProcessMetrics(ProcessHandle process) {
  return NULL;
}

size_t ProcessMetrics::GetPagefileUsage() const {
  return 0;
}

bool ProcessMetrics::GetMemoryBytes(size_t* private_bytes,
                                    size_t* shared_bytes) {
  NOTIMPLEMENTED();
  return false;
}

ProcessMetrics::~ProcessMetrics() {
}

void EnableTerminationOnHeapCorruption() {
  // Nothing to do here
}

}  // namespace base

