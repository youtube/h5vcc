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
// Provides the entry point to the Leanback Shell.

#include "external/chromium/base/at_exit.h"
#include "external/chromium/base/basictypes.h"
#include "external/chromium/base/command_line.h"
#include "external/chromium/base/i18n/icu_util.h"
#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/base/message_loop.h"
#include "external/chromium/base/metrics/histogram.h"
#include "external/chromium/base/metrics/statistics_recorder.h"
#include "external/chromium/base/string_split.h"
#include "external/chromium/media/base/shell_buffer_factory.h"
#include "external/chromium/media/base/shell_filter_graph_log.h"
#include "external/chromium/net/base/net_util.h"
#include "external/chromium/net/http/http_cache.h"
#include "external/chromium/skia/ext/SkMemory_new_handler.h"
#include "external/chromium/third_party/icu/public/common/unicode/locid.h"
// required before InitializeThreading.h
#include "external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime/JSExportMacros.h"
#include "external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime/InitializeThreading.h"
#include "lb_app_counters.h"
#include "lb_cookie_store.h"
#include "lb_memory_manager.h"
#include "lb_resource_loader_bridge.h"
#include "lb_savegame_syncer.h"
#include "lb_shell.h"
#include "lb_shell_layout_test_runner.h"
#include "lb_shell_platform_delegate.h"
#include "lb_shell_switches.h"
#include "lb_shell_webkit_init.h"
#include "lb_stack.h"
#include "lb_web_media_player_delegate.h"
#if defined(__LB_WIIU__)
#include "lb_shell/lb_error_viewer.h"
#endif
#include "steel_build_id.h"
#include "steel_version.h"

static const char* LB_URL = "https://www.youtube.com/tv";

// Get the URL to load. If it's not present, default to LB_URL.
static std::string GetUrlToLoad() {
#if defined(__LB_SHELL__FOR_RELEASE__)
  return std::string(LB_URL);
#else
  CommandLine *cl = CommandLine::ForCurrentProcess();

#if defined(__LB_WIIU__)
  if (cl->HasSwitch(LB::switches::kErrorTest)) {
    return std::string("about:blank");
  }
#endif

  std::string url = cl->GetSwitchValueASCII(LB::switches::kUrl);
  if (url.empty()) {
    return std::string(LB_URL);
  }

  LBResourceLoaderBridge::SetPerimeterCheckEnabled(false);
  std::string query = cl->GetSwitchValueASCII(LB::switches::kQueryString);
  if (!query.empty()) {
    for (size_t i = 0; i < query.length(); ++i) {
      if (query[i] == ';') query[i] = '&';
    }
    url += "?" + query;
  }

  printf("Overriding URL: %s\n", url.c_str());
  return url;
#endif
}

static void RunWebKit(int argc, char **argv) {
  // Begins a new scope for WebKit.
  // Initialize WebKit.
  LBShellWebKitInit lb_shell_webkit_init;

  // Initialize the JavaScriptCore threading model.
  // Must be called from main thread and AFTER WebKit init.
  JSC::initializeThreading();

  webkit_glue::WebThemeEngineImpl engine;
  lb_shell_webkit_init.SetThemeEngine(&engine);

  LBShellPlatformDelegate::PlatformUpdateDuringStartup();
  if (!LBShellPlatformDelegate::ExitGameRequested()) {
#if defined(__LB_LAYOUT_TESTS__)
    // Extract the tests to run from the command line.
    std::string work_dir(argv[1]);
    std::vector<std::string> tests;
    for (int i = 2; i < argc; ++i) {
      tests.push_back(std::string(argv[i]));
    }

    scoped_refptr<LBShellLayoutTestRunner> test_runner(
        LB_NEW LBShellLayoutTestRunner(
            work_dir,
            tests,
            MessageLoop::current()));

    MessageLoop::current()->Run();

    test_runner->Join();
#else
    LBShell *shell = LB_NEW LBShell(GetUrlToLoad());
    shell->Show(WebKit::WebNavigationPolicyNewWindow);

#if defined(__LB_WIIU__) && !defined(__LB_SHELL__FOR_RELEASE__)
    CommandLine *cl = CommandLine::ForCurrentProcess();
    std::string test_id = cl->GetSwitchValueASCII(LB::switches::kErrorTest);
    if (!test_id.empty()) {
      // Launch the error test.
      LBErrViewer::TestError(test_id);
    }
#endif

    // Run the main loop.
    MessageLoop::current()->Run();
    // The main loop has ended.
    delete shell;
#endif
  }
  // Tear down WebKit when leaving scope.
}

int main(int argc, char **argv) {
  LB::SetStackSize();

#if defined(__LB_SHELL__FOR_RELEASE__) && !defined(__LB_LINUX__)
  // We shall have no more arguments.
  argc = 0;
  argv = NULL;
#endif

#if LB_ENABLE_MEMORY_DEBUGGING
  // Tell Skia how to get the size of allocated blocks.
  sk_set_block_size_func(lb_memory_requested_size);
#endif

  // Parse the command line and setup chromium flags.
  CommandLine::Init(argc, argv);
  CommandLine* cl = CommandLine::ForCurrentProcess();

#if defined(__LB_LINUX__)
  if (cl->HasSwitch(LB::switches::kVersion)) {
    printf("Steel %s build %s\n", STEEL_VERSION, STEEL_BUILD_ID);
    return 1;
  }
#endif

  LBShellPlatformDelegate::Init();
  {  // scope for at_exit_manager
    // We need an AtExitManager before we access any base::LazyInstance or
    // base::Singleton objects, or create any base::Threads.
    base::AtExitManager at_exit_manager;

#if !defined(__LB_SHELL__FOR_RELEASE__)
    if (cl->HasSwitch(LB::switches::kDisableSave)) {
      LBSavegameSyncer::DisableSave();
    }
    if (cl->HasSwitch(LB::switches::kLoadSavegame)) {
      LBSavegameSyncer::LoadFromFile(cl->GetSwitchValuePath(
          LB::switches::kLoadSavegame));
    }
    if (cl->HasSwitch(LB::switches::kFilterGraphLog)) {
      media::ShellFilterGraphLog::SetGraphLoggingEnabled(true);
    }
    if (cl->HasSwitch(LB::switches::kLang)) {
      LBShell::SetPreferredLanguage(cl->GetSwitchValueASCII(
          LB::switches::kLang));
    }
#endif

    // Start the savegame init ASAP, since the async load could take a while.
    LBSavegameSyncer::Init();

    LBShell::InitStrings();

    base::StatisticsRecorder::Initialize();

    LBShellPlatformDelegate::CheckParentalControl();
    LBShellPlatformDelegate::PlatformUpdateDuringStartup();

    if (!LBShellPlatformDelegate::ExitGameRequested()) {
      // scope to hold the main_message_loop.
      MessageLoopForUI main_message_loop;
      main_message_loop.set_thread_name("main");

      // good time to turn on logging
      LBShell::InitLogging();

      // allocate working pool for media stack
      media::ShellBufferFactory::Initialize();
      webkit_media::LBWebMediaPlayerDelegate::Initialize();

      LBResourceLoaderBridge::Init(
          LB_NEW LBCookieStore(),
          LBShell::PreferredLanguage(),
          false);

      LBResourceLoaderBridge::SetAcceptAllCookies(true);

      LBShellPlatformDelegate::PlatformUpdateDuringStartup();
      if (!LBShellPlatformDelegate::ExitGameRequested()) {
        // load ICU data tables
        if (!icu_util::Initialize()) {
          DLOG(FATAL) << "icu_util::Intialize() failed.";
        }

        LBAppCounters app_counters;

        LBShell::InitializeLBShell();

        // set the default locale
        icu_46::Locale default_locale(LBShell::PreferredLocale().c_str());
        UErrorCode error_code;  // ignored
        icu_46::Locale::setDefault(default_locale, error_code);

        RunWebKit(argc, argv);
        // Localstorage data was flushed when WebKit was torn down.

        LBShell::ShutdownLBShell();
      }

      // LBResourceLoaderBridge::Shutdown() writes the last cookies.
      LBResourceLoaderBridge::Shutdown();

      // Now that localstorage and cookies are both shut down, start shutting
      // down the syncer.  This shutdown will proceed asynchronously.
      LBSavegameSyncer::Shutdown();

      webkit_media::LBWebMediaPlayerDelegate::Terminate();
      media::ShellBufferFactory::Terminate();

      LBShell::CleanupLogging();
    }  // tear down main_message_loop

    // The syncer uses a base::Thread, which needs the exit manager.
    // Make sure the syncer is completely done.
    LBSavegameSyncer::WaitForFlush();
  }  // tear down at_exit_manager

  LBShellPlatformDelegate::Teardown();
  return 0;
}
