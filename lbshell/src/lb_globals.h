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
#ifndef SRC_LB_GLOBALS_H_
#define SRC_LB_GLOBALS_H_
#include <stdint.h>

typedef struct {
  const char* dir_source_root;
  const char* game_content_path;
  const char* screenshot_output_path;
  const char* logging_output_path;
  const char* tmp_path;
  uint32_t lifetime;  // In milliseconds
} global_values_t;

#ifdef __cplusplus
extern "C" {
#endif
LB_BASE_EXPORT global_values_t* GetGlobalsPtr();
#ifdef __cplusplus
}
#endif

#endif  // SRC_LB_GLOBALS_H_
