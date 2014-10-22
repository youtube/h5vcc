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
#ifndef SRC_LB_WEB_VIEW_HOST_H_
#define SRC_LB_WEB_VIEW_HOST_H_

#include <string>

#include "Platform.h"

#include "base/base_export.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/message_pump_shell.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/simple_thread.h"
#include "base/time.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/WebKit/Source/WTF/config.h"
#include "third_party/WebKit/Source/WTF/wtf/Platform.h"
#include "webkit/glue/webpreferences.h"

#include "lb_debug_console.h"
#include "lb_input_fuzzer.h"
#include "lb_splash_screen.h"
#include "lb_system_stats_tracker.h"
#include "lb_user_input_device.h"

class LBWebViewDelegate;

namespace WebKit {
class WebWidget;
}

class LBNetworkConnection;
class LBDebugConsole;
class LBShell;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
class LBInputRecorder;
class LBPlaybackInputDevice;
#endif

class LBWebViewHost {
 public:
  // borrowing heavily from
  // external/chromium/webkit/tools/test_shell/webview_host.h
  // and webwidget_host.h, which here have been collapsed into
  // one class
  static LBWebViewHost* Create(LBShell* shell,
                               LBWebViewDelegate* delegate,
                               const webkit_glue::WebPreferences& prefs);
  static LBWebViewHost* Get() { return web_view_host_; }
  void Destroy();

  WebKit::WebView* webview() const;
  WebKit::WebWidget* webwidget() const;
  MessageLoop* webkit_message_loop() const;

  // sends a key event to the browser.
  // may be trapped by on-screen console if is_console_eligible.
  void SendKeyEvent(WebKit::WebKeyboardEvent::Type type,
                    int key_code,
                    wchar_t char_code,
                    WebKit::WebKeyboardEvent::Modifiers modifiers,
                    bool is_console_eligible);

  // sends a mouse event to the browser.
  void SendMouseEvent(const WebKit::WebMouseEvent& event);

  // this is a simple wrapper around SendKeyEvent
  // that simulates press & release in one call.
  // may be trapped by on-screen console if is_console_eligible.
  void InjectKeystroke(int key_code, wchar_t char_code,
                       WebKit::WebKeyboardEvent::Modifiers modifiers,
                       bool is_console_eligible);

#if ENABLE(TOUCH_EVENTS)
  // sends a touch event to the browser.
  void SendTouchEvent(WebKit::WebTouchEvent::Type type, int id, int x, int y);
#endif

  // this injects JavaScript code to be executed in a global context.
  void InjectJS(const char * code);

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // toggle WebKit paused state. Returns true if WK was paused by this call.
  bool toggleWebKitWedged();
  // call me with NULL to start/stop record/playback
  void RecordKeyInput(const char* file_name);
  void PlaybackKeyInput(const char* file_name, bool repeat);

  void FuzzerStart(const std::string& file_name, float time_mean,
                   float time_std, LBInputFuzzer::FuzzedKeys fuzzed_keys);
  void FuzzerStop();
  bool FuzzerActive() const;
#endif  // __LB_SHELL__ENABLE_CONSOLE__

  bool IsExiting() { return exit_; }

  void InputEventTask(const WebKit::WebInputEvent& event);

  // will pause the WebKit thread if paused is true
  void SetWebKitPaused(bool paused);
  static void PauseApplicationTask(LBWebViewHost* web_view_host);

  // Called when the WebKit thread enters and leaves a paused state due to
  // PauseApplicationTask
  virtual void OnApplicationPaused() {}
  virtual void OnApplicationUnpaused() {}
  virtual void OnKeySentEvent() {}

  // will pause/resume any active video players
  void SetPlayersPaused(bool paused);

  void SendNavigateTask(const GURL& url);
  void SendNavigateTask(const GURL& url, const base::Closure& nav_complete);
  void SendNavigateToAboutBlankTask();
  void SendNavigateToAboutBlankTask(const base::Closure& nav_complete);

  virtual void ShowNetworkFailure();

  // Called when a non-read-only text input gains/loses focus
  virtual void TextInputFocusEvent() {}
  virtual void TextInputBlurEvent() {}

  LBShell* shell() const { return shell_; }

  // a polite request to quit the application.
  // tells the view host to stop and spawns a task that tells the
  // message loop to stop as well.  when the message loop stops,
  // the application cleans up and shuts down.
  void RequestQuit();

  static void CreateWebWidget(LBWebViewDelegate* delegate,
                              const webkit_glue::WebPreferences& prefs,
                              WebKit::WebWidget** out_web_widget);
  static void DestroyWebWidget(WebKit::WebWidget* web_widget);

  // UI thread run loop
  virtual void RunLoop() = 0;

  // Called when the user moves the input cursor
  virtual void CursorChangedEvent() {}

  // Clear cookies and contents of localStorage from memory, and reload
  // from disk.
  void ClearAndReloadUserData(const base::Closure& userdata_cleared_cb);

  // Call to hide the splash screen. Returns true if possible and splash screen
  // was hidden.
  bool HideSplashScreen();
  void DrawSplashScreen();

  virtual ~LBWebViewHost();

 protected:
  static LBWebViewHost* web_view_host_;

  explicit LBWebViewHost(LBShell* shell);

  WebKit::WebWidget* webwidget_;

  // we store a weak reference to the main message loop, which it is
  // assumed is running on the same thread as what called Create()
  // (and therefore the ctor)
  MessageLoop* webkit_message_loop_;

  // Called after a key is sent to WebKit through SendKeyEvent
  virtual void OnSentKeyEvent() {}

  virtual bool FilterKeyEvent(WebKit::WebKeyboardEvent::Type type,
                      int key_code,
                      wchar_t char_code,
                      WebKit::WebKeyboardEvent::Modifiers modifiers,
                      bool is_console_eligible);

  bool exit_;

  // not owned:
  LBShell* shell_;

  bool pause_application_task_done_;

  scoped_ptr<LB::SplashScreen> splash_screen_;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  void clearInput();
  void UpdatePlaybackController();

  // owned by us:
  LBDebugConsole *console_;
  std::string console_buffer_;
  // toggle behavior
  bool webkit_wedged_;

  scoped_ptr<LBPlaybackInputDevice> playback_;
  scoped_ptr<LBInputRecorder> input_recorder_;
  scoped_ptr<LBInputFuzzer> fuzzer_;
#endif

#if !defined(__LB_SHELL__FOR_RELEASE__)
  scoped_ptr<LB::StatsUpdateThread> stats_update_thread_;
  scoped_ptr<LB::SystemStatsTracker> system_stats_tracker_;
#endif

 private:
  static void NavTask(LBWebViewHost *view_host,
                      LBShell *shell,
                      const GURL& url,
                      const base::Closure& nav_complete);

  static void NavToBlankTask(LBWebViewHost *view_host,
                             LBShell *shell,
                             const base::Closure& nav_complete);
  void ClearDomStorage(const base::Closure& storage_cleared_cb);
  void ReloadDatabase(const base::Closure& reloaded_cb);
};

#endif  // SRC_LB_WEB_VIEW_HOST_H_
