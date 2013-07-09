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

#ifndef SRC_LB_SHELL_SWITCHES_H
#define SRC_LB_SHELL_SWITCHES_H_

namespace LB {
namespace switches {

extern const char kUrl[];
extern const char kQueryString[];
extern const char kWebCoreLogChannels[];
extern const char kDisableSave[];
extern const char kLoadSavegame[];
extern const char kUserAgent[];
extern const char kFilterGraphLog[];
#if defined(__LB_WIIU__)
extern const char kErrorTest[];
#endif
extern const char kLang[];
extern const char kIgnorePlatformAuthentication[];
#if defined(__LB_LINUX__)
extern const char kVersion[];
#endif

}  // namespace switches
}  // namespace LB

#endif  // SRC_LB_SHELL_SWITCHES_H_

