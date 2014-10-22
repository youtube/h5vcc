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

#ifndef SRC_LB_SHELL_SWITCHES_H_
#define SRC_LB_SHELL_SWITCHES_H_

#include "lb_shell_export.h"

namespace LB {
namespace switches {

LB_SHELL_EXTERN const char kUrl[];
LB_SHELL_EXTERN const char kQueryString[];
LB_SHELL_EXTERN const char kWebCoreLogChannels[];
LB_SHELL_EXTERN const char kDisableSave[];
LB_SHELL_EXTERN const char kLoadSavegame[];
LB_SHELL_EXTERN const char kUserAgent[];
LB_SHELL_EXTERN const char kLang[];
LB_SHELL_EXTERN const char kIgnorePlatformAuthentication[];
LB_SHELL_EXTERN const char kProxy[];
LB_SHELL_EXTERN const char kHideSplashScreenAtInit[];

#if defined(__LB_WIIU__)
LB_SHELL_EXTERN const char kErrorTest[];
#endif

#if defined(__LB_LINUX__)
LB_SHELL_EXTERN const char kVersion[];
LB_SHELL_EXTERN const char kHelp[];
#endif

#if defined(__LB_XB1__) || defined(__LB_XB360__)
LB_SHELL_EXTERN const char kDrawGestureRecognizerBorder[];
#endif

}  // namespace switches
}  // namespace LB

#endif  // SRC_LB_SHELL_SWITCHES_H_
