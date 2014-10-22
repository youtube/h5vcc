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

#ifndef SRC_LB_TRACING_MANAGER_H_
#define SRC_LB_TRACING_MANAGER_H_

#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/base/memory/ref_counted_memory.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace LB {

class TracingManager {
 public:
  TracingManager();
  virtual ~TracingManager();

  void EnableTracing(bool enable);

  bool IsEnabled() const;
};

}  // namespace LB
#endif  // __LB_SHELL__ENABLE_CONSOLE__
#endif  // SRC_LB_TRACING_MANAGER_H_
