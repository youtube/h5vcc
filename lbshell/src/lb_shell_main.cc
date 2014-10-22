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

#if defined(__LB_XB360__) && !defined(__LB_SHELL__FOR_RELEASE__)
#include <windows.h>  // For GetCommandLine
#endif

#include "external/chromium/base/at_exit.h"
#include "external/chromium/base/base_switches.h"
#include "external/chromium/base/basictypes.h"
#include "external/chromium/base/command_line.h"
#include "external/chromium/base/debug/stack_trace.h"
#include "external/chromium/base/i18n/icu_util.h"
#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/base/message_loop.h"
#include "external/chromium/base/metrics/histogram.h"
#include "external/chromium/base/metrics/statistics_recorder.h"
#include "external/chromium/base/string_split.h"
#include "external/chromium/base/threading/platform_thread.h"
#include "external/chromium/base/threading/thread.h"
#include "external/chromium/media/base/shell_buffer_factory.h"
#include "external/chromium/net/base/net_util.h"
#include "external/chromium/net/dial/dial_service.h"
#include "external/chromium/net/http/http_cache.h"
#include "external/chromium/skia/ext/SkMemory_new_handler.h"
#include "external/chromium/third_party/icu/public/common/unicode/locid.h"
#ifdef __LB_SHELL_USE_JSC__
// required before InitializeThreading.h
#include "external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime/JSExportMacros.h"
#include "external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime/InitializeThreading.h"
#endif
#include "lb_console_values.h"
#include "lb_cookie_store.h"
#include "lb_globals.h"
#include "lb_memory_manager.h"
#include "lb_resource_loader_bridge.h"
#include "lb_savegame_syncer.h"
#include "lb_shell/lb_shell_constants.h"
#include "lb_shell.h"
#include "lb_shell_console_values_hooks.h"
#include "lb_shell_layout_test_runner.h"
#include "lb_shell_platform_delegate.h"
#include "lb_shell_switches.h"
#include "lb_storage_cleanup.h"

#include "lb_web_media_player_delegate.h"
#include "steel_build_id.h"
#include "steel_version.h"

#if defined(__LB_ANDROID__)
# include "lb_shell/lb_shell_webkit_init_android.h"
# define WEBKIT_INIT_CLASS LBShellWebKitInitAndroid
# include "lb_shell/lb_crash_dump_manager_android.h"
#elif defined(__LB_LINUX__)
# include "lb_shell/lb_shell_webkit_init_linux.h"
# define WEBKIT_INIT_CLASS LBShellWebKitInitLinux
#elif defined(__LB_PS3__)
# include "lb_shell/lb_shell_webkit_init_ps3.h"
# define WEBKIT_INIT_CLASS LBShellWebKitInitPS3
#elif defined(__LB_PS4__)
# include "lb_shell/lb_shell_webkit_init_ps4.h"
# define WEBKIT_INIT_CLASS LBShellWebKitInitPS4
#elif defined(__LB_WIIU__)
# include "lb_shell/lb_shell_webkit_init_wiiu.h"
# define WEBKIT_INIT_CLASS LBShellWebKitInitWiiU
#elif defined(__LB_XB1__)
# include "lb_shell/lb_shell_webkit_init_xb1.h"
# define WEBKIT_INIT_CLASS LBShellWebKitInitXB1
#elif defined(__LB_XB360__)
# include "lb_shell/lb_shell_webkit_init_xb360.h"
# define WEBKIT_INIT_CLASS LBShellWebKitInitXB360
#else
# error Platform not handled!
#endif

#if defined(__LB_WIIU__)
# include "lb_shell/lb_error_viewer.h"
#endif

// Ensure we don't accidentally release with these features:
#if defined(__LB_SHELL__FOR_RELEASE__)
# if defined(__LB_SHELL__FORCE_LOGGING__)
#  error Logging should not be enabled in a release build!
# endif
# if defined(__LB_SHELL__ENABLE_CONSOLE__)
#  error The debugging console should not be enabled in a release build!
# endif
# if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
#  error Screenshot functionality should not be enabled in a release build!
# endif
# if ENABLE_INSPECTOR
#  error Web inspector should not be enabled in a release build!
# endif
#endif

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

namespace {
class WebKitInstance {
 public:
  WebKitInstance();
  virtual ~WebKitInstance();

  MessageLoop* message_loop() { return webkit_thread_.message_loop(); }

 private:
  void InitializeOnWebKitThread();
  void ShutdownOnWebKitThread();

  base::Thread webkit_thread_;

  scoped_ptr<LBShellWebKitInit> webkit_init_;
  scoped_ptr<webkit_glue::WebThemeEngineImpl> engine_;
};

WebKitInstance::WebKitInstance()
    : webkit_thread_("Webkit") {

  webkit_thread_.StartWithOptions(base::Thread::Options(
      MessageLoop::TYPE_DEFAULT,
      kWebKitThreadStackSize,
      kWebKitThreadPriority,
      kWebKitThreadAffinity));

  message_loop()->PostTask(FROM_HERE,
      base::Bind(&WebKitInstance::InitializeOnWebKitThread,
                 base::Unretained(this)));
}

void WebKitInstance::InitializeOnWebKitThread() {
  // Initialize WebKit.
  webkit_init_.reset(new WEBKIT_INIT_CLASS);

#ifdef __LB_SHELL_USE_JSC__
  // Initialize the JavaScriptCore threading model.
  // Must be called from main thread and AFTER WebKit init.
  JSC::initializeThreading();
#endif

  engine_.reset(new webkit_glue::WebThemeEngineImpl());
  webkit_init_->SetThemeEngine(engine_.get());

  LBShellPlatformDelegate::PlatformUpdateDuringStartup();
}

WebKitInstance::~WebKitInstance() {
  // Any in-progress requests may post tasks to the WebKit thread. Wait for
  // these requests to complete and post their tasks to WebKit before
  // posting the task to shut down WebKit.
  LBResourceLoaderBridge::WaitForAllRequests();

  message_loop()->PostTask(FROM_HERE,
      base::Bind(&WebKitInstance::ShutdownOnWebKitThread,
                 base::Unretained(this)));

  webkit_thread_.Stop();
}

void WebKitInstance::ShutdownOnWebKitThread() {
  engine_.reset(NULL);
  webkit_init_.reset(NULL);
}

}  // namespace

#if defined(__LB_LAYOUT_TESTS__)
static void RunLayoutTests(const CommandLine& command_line,
                           WebKitInstance* webkit_instance) {
  LBShell shell("about:blank", webkit_instance->message_loop());
  shell.Show(WebKit::WebNavigationPolicyNewWindow);

  // Extract the tests to run from the command line.
  CommandLine::StringVector args = command_line.GetArgs();
  std::string work_dir(args[0]);
  std::vector<std::string> tests(args.begin()+1, args.end());

  scoped_refptr<LBShellLayoutTestRunner> test_runner(
      new LBShellLayoutTestRunner(
          &shell,
          work_dir,
          tests,
          webkit_instance->message_loop()));

  shell.RunLoop();
}

#else

static void RunLBShell(WebKitInstance* webkit_instance) {
  LBShell shell(GetUrlToLoad(), webkit_instance->message_loop());
  shell.Show(WebKit::WebNavigationPolicyNewWindow);

#if defined(__LB_WIIU__) && !defined(__LB_SHELL__FOR_RELEASE__)
  CommandLine *cl = CommandLine::ForCurrentProcess();
  std::string test_id = cl->GetSwitchValueASCII(LB::switches::kErrorTest);
  if (!test_id.empty()) {
    // Launch the error test.
    LBErrViewer::TestError(test_id);
  }
#endif

  // Start the main LB Shell app and then immediately wait for it to
  // quit/finish.
  shell.RunLoop();
}

#endif  // defined(__LB_LAYOUT_TESTS__)

int main(int argc, char **argv) {
  base::PlatformThread::SetName("LBShell Main");

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
  if (cl->HasSwitch(LB::switches::kHelp)) {
    printf("Steel %s build %s\n", STEEL_VERSION, STEEL_BUILD_ID);
    printf("Options:\n");
    printf("\n");
    printf("  --disable-save    Load the savegame at startup, but never\n");
    printf("      write to it for any reason.\n");
    printf("\n");
    printf("  --filter-graph-log    Enable media filter graph logging.\n");
    printf("\n");
    printf("  --help    Print a list of options and exit.\n");
    printf("\n");
    printf("  --lang=LANG    Override the system language.  LANG is a\n");
    printf("      two-letter language code with an option country code,\n");
    printf("      such as \"de\" or \"pt-BR\".\n");
    printf("\n");
    printf("  --load-savegame=PATH    Load an alternate savegame database\n");
    printf("      at startup.  (Default: ~/.steel.db)\n");
    printf("\n");
    printf("  --url=URL    Start the application with a non-standard URL.\n");
    printf("      Also puts perimeter checks into warning mode.\n");
    printf("\n");
    printf("  --proxy=HOST:PORT    Start the application with a proxy.\n");
    printf("      Overrides the system's proxy settings.\n");
    printf("      Also puts perimeter checks into warning mode.\n");
    printf("\n");
    printf("  --user-agent=USER_AGENT    Override the User-Agent string.\n");
    printf("\n");
    printf("  --version    Print the version number and exit.\n");
    printf("\n");
    printf("  --webcore-log-channels=CHANNELS    Additional channels\n");
    printf("      for WebKit logging.  Separate channels by commas.\n");
    printf("\n");
    return 1;
  }

  if (cl->HasSwitch(LB::switches::kVersion)) {
    printf("Steel %s build %s\n", STEEL_VERSION, STEEL_BUILD_ID);
    return 1;
  }
#endif

#if defined(__LB_ANDROID__)
  if (cl->HasSwitch(switches::kWaitForDebugger)) {
    DLOG(INFO) << "Waiting for debugger.";
    base::debug::WaitForDebugger(24 * 60 * 60, false);
  }
#endif

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // Setup console values system very early, so that it is setup before most
  // CVals get created.
  LB::ConsoleValueManager console_value_manager;
#endif

  // AtExitManagers can be nested, and our graphics library (spawned in
  // LBShellPlatformDelegate::Init() may create a base::Thread/message
  // loop, so this manager permits those actions, and will be torn down
  // after the inner at_exit_manager.
#if !defined(__LB_ANDROID__)
  // TODO: On Android, AtExitManager is created earlier during the
  // initialization process to support JNI binding registration. Check if
  // the code paths can be unified between different platforms.
  base::AtExitManager platform_at_exit_manager;
#endif
  LBShellPlatformDelegate::Init();

#if LB_ENABLE_MEMORY_DEBUGGING
  if (LB::Memory::IsContinuousLogEnabled()) {
    // Initialize the writer (we should already be recording to memory) now that
    // the filesystem and threading system are initialized.
    LB::Memory::InitLogWriter();
  }
#endif

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // And now attach LBShell specific hooks to the console value system.  This
  // is done later than the creation of the ConsoleValueManager because some
  // of the LBShell hooks require some systems to be initialized first.
  LB::ShellConsoleValueHooks shell_console_value_hooks;
#endif

  {  // scope for at_exit_manager
    // Initialize an AtExitManager to deal with all lazy instance objects
    // like base::Singleton objects, or base::Threads that should be shut
    // down before we call LBShellPlatformDelegate::Teardown().
    // We explicitly indicate that we wish to shadow the outer AtExitManager.
    base::ShadowingAtExitManager at_exit_manager;

  // Applications compiled with __LB_SHELL_NO_CHROME__ don't link
  // against chromium/base
#if !defined(__LB_SHELL_NO_CHROME__) && \
    !defined(__LB_SHELL__FOR_RELEASE__) && defined(__LB_XB1__)
    if (!base::debug::BeingDebugged())
      // Enabled MiniDump generation in case of a crash
      base::debug::EnableInProcessStackDumping();
#endif

#if defined(__LB_SHELL__FOR_RELEASE__) && defined(__LB_ANDROID__)
    InitCrashDump();
#endif

#if !defined(__LB_SHELL__FOR_RELEASE__)
    if (cl->HasSwitch(LB::switches::kDisableSave)) {
      LBSavegameSyncer::DisableSave();
    }
    if (cl->HasSwitch(LB::switches::kLoadSavegame)) {
      LBSavegameSyncer::LoadFromFile(cl->GetSwitchValuePath(
          LB::switches::kLoadSavegame));
    }
    if (cl->HasSwitch(LB::switches::kLang)) {
      LBShell::SetPreferredLanguage(cl->GetSwitchValueASCII(
          LB::switches::kLang));
    }

    // Spawn a thread that deletes old files to save disk space
    LB::StorageCleanupThread cleanup_thread;
    cleanup_thread.Start();
#endif

    // Start the savegame init ASAP, since the async load could take a while.
    LBSavegameSyncer::Init();

    LBShell::InitStrings();

    base::StatisticsRecorder::Initialize();

    LBShellPlatformDelegate::CheckParentalControl();
    LBShellPlatformDelegate::PlatformUpdateDuringStartup();

    if (!LBShellPlatformDelegate::ExitGameRequested()) {
#if !defined(__LB_XB1__)
      // Scope to hold the main_message_loop. For the Xbox One, we create
      // a different SingleThreadTaskRunner later by attaching it to the system
      // event dispatcher.
      MessageLoopForUI main_message_loop;
      main_message_loop.set_thread_name("main");
#endif

      // good time to turn on logging
      LBShell::InitLogging();

      // allocate working pool for media stack
      media::ShellBufferFactory::Initialize();
      webkit_media::LBWebMediaPlayerDelegate::Initialize();

      LBResourceLoaderBridge::Init(
          new LBCookieStore(),
          LBShell::PreferredLanguage(),
          false);

      LBResourceLoaderBridge::SetAcceptAllCookies(true);

      LBShellPlatformDelegate::PlatformUpdateDuringStartup();
      if (!LBShellPlatformDelegate::ExitGameRequested()) {
        // load ICU data tables
        if (!icu_util::Initialize()) {
          DLOG(FATAL) << "icu_util::Intialize() failed.";
        }

        LBShell::InitializeLBShell();

        // set the default locale
        icu_46::Locale default_locale(LBShell::PreferredLocale().c_str());
        UErrorCode error_code;  // ignored
        icu_46::Locale::setDefault(default_locale, error_code);

        if (!LBShellPlatformDelegate::ExitGameRequested()) {
          WebKitInstance webkit_instance;

#if defined(__LB_LAYOUT_TESTS__)
          RunLayoutTests(*cl, &webkit_instance);
#else
          RunLBShell(&webkit_instance);
#endif
        }

        // Localstorage data was flushed when WebKit was torn down.
        LBShell::ShutdownLBShell();
      }

#if ENABLE(DIAL_SERVER)
      // Terminate the Dial Service.
      net::DialService::GetInstance()->Terminate();
#endif

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
    LBSavegameSyncer::WaitForShutdown();
  }  // tear down at_exit_manager

  LBShellPlatformDelegate::Teardown();

  return 0;
}
