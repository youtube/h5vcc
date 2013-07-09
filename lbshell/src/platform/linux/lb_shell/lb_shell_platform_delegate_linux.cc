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
#include "lb_shell_platform_delegate.h"

#include <string>

#include "external/chromium/base/logging.h"
#include "external/chromium/base/file_util.h"
#include "lb_device_plugin.h"
#include "lb_graphics.h"
#include "lb_memory_internal.h"
#include "lb_memory_manager.h"

static bool exit_game_requested = false;

// global and externally visible.
std::string *global_dir_source_root = NULL;
std::string *global_game_content_path = NULL;
std::string *global_tmp_path = NULL;

// This is used from C code, so can't be a string.
const char *global_screenshot_output_path = NULL;

// static
void LBShellPlatformDelegate::PlatformInit() {
  char game_content_path[] = "content";

  // Set the content directory path based off of the path to this
  // executable
  char this_exe_path[PATH_MAX];
  size_t bytes_read = readlink("/proc/self/exe", this_exe_path, PATH_MAX);
  this_exe_path[bytes_read] = '\0';

  std::string exe_path = this_exe_path;

  std::string exe_dir = exe_path.substr(0, exe_path.rfind('/'));
  std::string content_dir = exe_dir + "/" + game_content_path;
  if (access(content_dir.c_str(), R_OK|X_OK) != 0) {
    printf("FATAL: Unable to access content directory '%s'.\n",
      content_dir.c_str());
    abort();
  }

  global_dir_source_root = LB_NEW std::string(content_dir +
                                              "/dir_source_root");
  global_game_content_path = LB_NEW std::string(content_dir);

  global_tmp_path = LB_NEW std::string("/tmp/steel");
  mkdir(global_tmp_path->c_str(), 0700);

#if defined (__LB_SHELL__ENABLE_SCREENSHOT__)
  global_screenshot_output_path = "/tmp/steel/screenshot";
  mkdir(global_screenshot_output_path, 0700);
#endif

  // Tell the JSC Heap to start collecting once it hits 8MB.
  // Same as on the other platforms.
  setenv("GCMaxHeapSize", "8M", 0);

  // setup graphics
  LBGraphics::SetupGraphics();
}

std::string LBShellPlatformDelegate::GetSystemLanguage() {
  return "en-US";
}

// static
void LBShellPlatformDelegate::PlatformUpdateDuringStartup() {
}

// static
void LBShellPlatformDelegate::PlatformTeardown() {
  // stop graphics
  LBGraphics::TeardownGraphics();
  delete global_tmp_path;
  global_tmp_path = NULL;
  delete global_game_content_path;
  global_game_content_path = NULL;
}

// static
bool LBShellPlatformDelegate::ExitGameRequested() {
  return exit_game_requested;
}

// static
const char *LBShellPlatformDelegate::GameTitleId() {
  // Unused on Linux.
  return "";
}

// static
void LBShellPlatformDelegate::PlatformMediaInit() {
}

// static
void LBShellPlatformDelegate::PlatformMediaTeardown() {
}

// static
void LBShellPlatformDelegate::CheckParentalControl() {
}

// static
bool LBShellPlatformDelegate::ProtectOutput() {
  // Output protection not supported.
  return false;
}
