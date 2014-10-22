/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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
#ifndef SRC_PLATFORM_LINUX_POSIX_EMULATION_LB_SHELL_LB_RAND_H_
#define SRC_PLATFORM_LINUX_POSIX_EMULATION_LB_SHELL_LB_RAND_H_

#ifdef __cplusplus
namespace LB {
namespace Platform {
#endif

void get_random_bytes(char *buf, int num_bytes);

#ifdef __cplusplus
}  // namespace Platform
}  // namespace LB
#endif

#endif  // SRC_PLATFORM_LINUX_POSIX_EMULATION_LB_SHELL_LB_RAND_H_
