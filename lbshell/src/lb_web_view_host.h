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

#include "external/chromium/base/base_export.h"
#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/base/message_loop.h"
#include "external/chromium/base/message_pump_shell.h"
#include "external/chromium/base/synchronization/lock.h"
#include "external/chromium/base/synchronization/waitable_event.h"
#include "external/chromium/base/threading/simple_thread.h"
#include "external/chromium/third_party/WebKit/Source/JavaScriptCore/debugger/DebuggerShell.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "external/chromium/webkit/glue/webpreferences.h"
#include "lb_shell/lb_web_view_host_impl.h"

class LBWebViewDelegate;

namespace WebKit {
  class WebWidget;
}

class LBNetworkConnection;
class LBDebugConsole;
class LBShell;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
const int kInputRecordBufferSize = 50;
#endif

class LBWebViewHost
    : public base::SimpleThread,
      public LBWebViewHostImpl
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    , public JSC::DebuggerTTYInterface
#endif
{
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

  virtual void Start() OVERRIDE;

  WebKit::WebView* webview() const;
  WebKit::WebWidget* webwidget() const;
  MessageLoop* main_message_loop() const;

  // sends a key event to the browser.
  // may be trapped by on-screen console if is_console_eligible.
  void SendKeyEvent(WebKit::WebKeyboardEvent::Type type,
                    int key_code,
                    wchar_t char_code,
                    WebKit::WebKeyboardEvent::Modifiers modifiers,
                    bool is_console_eligible);

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
  // from JSC::DebuggerTTYInterface:
  virtual void output(std::string data) OVERRIDE;
  virtual void output_popup(std::string data) OVERRIDE;
  // toggle WebKit paused state. Returns true if WK was paused by this call.
  bool toggleWebKitWedged();
  // call me with NULL to start/stop record/playback
  void RecordKeyInput(const char* file_name);
  void PlaybackKeyInput(const char* file_name, bool repeat);
  inline void SetTelnetConnection(LBNetworkConnection *connection) {
    telnet_connection_ = connection;
  }
  inline LBNetworkConnection* GetTelnetConnection() const {
    return telnet_connection_;
  }
  // send a mouse down event followed by a mouse up to webkit.
  // x and y are pixel coordinates.
  void SendMouseClick(WebKit::WebMouseEvent::Button button,
                      int x, int y);
#endif

  bool IsExiting() { return exit_; }

  // will pause the WebKit thread if paused is true
  void SetWebKitPaused(bool paused);

  // will pause/resume any active video players
  void SetPlayersPaused(bool paused);

  void StartApp();

  void SendNavigateTask(const GURL& url);

  void ShowNetworkFailure();

  // Called when a non-read-only text input gains/loses focus
  void TextInputFocusEvent();
  void TextInputBlurEvent();
  LBShell* shell() const { return shell_; }

  // a polite request to quit the application.
  // tells the view host to stop and spawns a task that tells the
  // message loop to stop as well.  when the message loop stops,
  // the application cleans up and shuts down.
  void RequestQuit();

 protected:
  static LBWebViewHost* web_view_host_;

  LBWebViewHost(LBShell* shell);

  WebKit::WebWidget* webwidget_;

  // we store a weak reference to the main message loop, which it is
  // assumed is running on the same thread as what called Create()
  // (and therefore the ctor)
  MessageLoop * main_message_loop_;

  // main thread run loop
  virtual void Run() OVERRIDE;
  void UpdateIO();

 private:
  bool FilterKeyEvent(WebKit::WebKeyboardEvent::Type type,
                      int key_code,
                      wchar_t char_code,
                      WebKit::WebKeyboardEvent::Modifiers modifiers,
                      bool is_console_eligible);

  bool exit_;

  // not owned:
  LBShell* shell_;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  void clearInput();
  static void MouseEventTask(WebKit::WebInputEvent::Type type,
                             WebKit::WebMouseEvent::Button button,
                             int x,
                             int y,
                             LBWebViewHost* host);

  // owned by us:
  LBDebugConsole * console_;
  LBNetworkConnection *telnet_connection_;
  std::string console_buffer_;
  // toggle behavior
  bool webkit_wedged_;
  int input_record_fd_;
  int64_t input_record_start_time_;
  char input_record_buffer_[kInputRecordBufferSize];
#endif
};

#endif  // SRC_LB_WEB_VIEW_HOST_H_
