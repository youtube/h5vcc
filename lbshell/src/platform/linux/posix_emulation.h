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
#ifndef SRC_PLATFORM_LINUX_POSIX_EMULATION_H_
#define SRC_PLATFORM_LINUX_POSIX_EMULATION_H_
#include "lb_base_export.h"

#if defined(__cplusplus)
// Copied from chromium: base/basictypes.h
#if !defined(COMPILE_ASSERT)
template<bool> struct LBCompileAssert { };

#define COMPILE_ASSERT(expr, msg)                    \
  typedef LBCompileAssert<(static_cast<bool>(expr))> \
          msg[static_cast<bool>(expr) ? 1 : -1]
#endif
#endif  // defined (__cplusplus)

#endif  // SRC_PLATFORM_LINUX_POSIX_EMULATION_H_
