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
// OOM-safe libpng wrappers.
#ifndef SRC_OOM_PNG_H_
#define SRC_OOM_PNG_H_

#include "external/chromium/third_party/libpng/png.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LB_ENABLE_MEMORY_DEBUGGING
void oom_fprintf(int fd, const char *fmt, ...);
#if LB_MEMORY_DUMP_GRAPH
png_structp oom_png_create(const char *file_name, int width, int height,
                           int bit_depth, int color_type);
void oom_png_destroy(png_structp png);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif  // SRC_OOM_PNG_H_
