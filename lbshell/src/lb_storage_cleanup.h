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
#ifndef SRC_LB_STORAGE_CLEANUP_H_
#define SRC_LB_STORAGE_CLEANUP_H_

#if !defined(__LB_SHELL__FOR_RELEASE__)

#include "external/chromium/base/threading/simple_thread.h"

#include "lb_shell_export.h"

namespace LB {

// Responsible for cleaning up old files that are no longer needed.
// For example, the application can produce multiple log, tracing or crash
// dump files over time and this thread will take care of deleting the older
// files as newer files are added.
class LB_SHELL_EXPORT StorageCleanupThread : public base::SimpleThread {
 public:
  StorageCleanupThread();
  virtual ~StorageCleanupThread();

 private:
  virtual void Run() OVERRIDE;
};

}  // namespace LB

#endif  // !defined(__LB_SHELL__FOR_RELEASE__)

#endif  // SRC_LB_STORAGE_CLEANUP_H_
