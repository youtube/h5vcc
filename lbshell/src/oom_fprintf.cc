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
// OOM-safe print.

#include "lb_memory_manager.h"
#if !defined(__LB_SHELL__FOR_RELEASE__)

#include "oom_fprintf.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
// an OOM-safe version of fprintf which does not use the heap.
// can even be used before malloc_init.
void oom_fprintf(int fd, const char *fmt, ...) {
  char buf[128];
  va_list ap;
  va_start(ap, fmt);
  size_t n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  write(fd, buf, n);
}

#endif  // #if !defined(__LB_SHELL__FOR_RELEASE__)
