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

#include <signal.h>

#include <string>

#include "external/chromium/base/logging.h"
#include "external/chromium/base/file_util.h"
#include "lb_globals.h"
#include "lb_graphics.h"
#include "lb_memory_manager.h"

static bool exit_game_requested = false;

// static
void LBShellPlatformDelegate::PlatformInit() {
  // ignore SIGPIPE when a socket is written to after the other end hangs up:
  signal(SIGPIPE, SIG_IGN);

  char game_content_path[] = "content";
  global_values_t* global_values = GetGlobalsPtr();

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

  global_values->dir_source_root =
      strdup((content_dir + "/dir_source_root").c_str());
  global_values->game_content_path = strdup(content_dir.c_str());

  global_values->tmp_path = strdup("/tmp/steel");
  mkdir(global_values->tmp_path, 0700);

#if defined (__LB_SHELL__ENABLE_SCREENSHOT__)
  global_values->screenshot_output_path = strdup("/tmp/steel/screenshot");
  mkdir(global_values->screenshot_output_path, 0700);
#endif

#if defined(__LB_SHELL__FORCE_LOGGING__) || \
    defined(__LB_SHELL__ENABLE_CONSOLE__)
  global_values->logging_output_path = strdup("/tmp/steel/logging");
  mkdir(global_values->logging_output_path, 0700);
#endif

  // Tell the JSC Heap to start collecting once it hits 8MB.
  // Same as on the other platforms.
  setenv("GCMaxHeapSize", "8M", 0);

#if !defined(__LB_SHELL_NO_CHROME__)
  // setup graphics
  LBGraphics::SetupGraphics();
#endif
}

std::string LBShellPlatformDelegate::GetSystemLanguage() {
  return "en-US";
}

// static
void LBShellPlatformDelegate::PlatformUpdateDuringStartup() {
}

// static
void LBShellPlatformDelegate::PlatformTeardown() {
#if !defined(__LB_SHELL_NO_CHROME__)
  // stop graphics
  LBGraphics::TeardownGraphics();
#endif

  global_values_t* global_values = GetGlobalsPtr();
  free(const_cast<char*>(global_values->dir_source_root));
  free(const_cast<char*>(global_values->game_content_path));
  free(const_cast<char*>(global_values->screenshot_output_path));
  free(const_cast<char*>(global_values->logging_output_path));
  free(const_cast<char*>(global_values->tmp_path));
  global_values->dir_source_root = NULL;
  global_values->game_content_path = NULL;
  global_values->screenshot_output_path = NULL;
  global_values->logging_output_path = NULL;
  global_values->tmp_path = NULL;
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
void LBShellPlatformDelegate::CheckParentalControl() {
}

// static
bool LBShellPlatformDelegate::ProtectOutput() {
  // Output protection not supported.
  return false;
}
