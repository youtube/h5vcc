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

// loosely modeled after test_shell_platform_delegate.h
// LBShellPlatformDelegate isolates a variety of platform-specific
// functions so that code can invoke them by purpose without resorting to
// ifdefs or runtime platform checks.  Each platform should define an
// implementation of this class.  In many cases, implementation of methods
// in this class will be stubs on platforms where those functions are
// unnecessary.

#ifndef SRC_LB_SHELL_PLATFORM_DELEGATE_H_
#define SRC_LB_SHELL_PLATFORM_DELEGATE_H_

#include <string>

#include <stdint.h>

class LBShellPlatformDelegate {
 public:
  // first method called by main()
  static void Init();
  static void PlatformInit();
  static void PlatformMediaInit();
  // gets the system langauge
  // NOTE: should be in the format described by bcp47.
  // http://www.rfc-editor.org/rfc/bcp/bcp47.txt
  // Example: "en-US" or "de"
  static std::string GetSystemLanguage();
  // last method called by main()
  static void Teardown();
  static void PlatformTeardown();
  static void PlatformMediaTeardown();
  // perform system update tasks at checkpoints during startup
  static void PlatformUpdateDuringStartup();
  // true if exit game was requested by the platform.
  static bool ExitGameRequested();
  // gets the title id of the game
  static const char *GameTitleId();
  // check parental control settings.
  static void CheckParentalControl();
  // enable output protection (HDCP or CGMS) and block until complete
  static bool ProtectOutput();
};

#endif  // SRC_LB_SHELL_PLATFORM_DELEGATE_H_
