/*
 * Copyright 2013 Google Inc. All Rights Reserved.
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

#include "lb_web_view_host.h"

#include "base/bind.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "media/base/bind_to_loop.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebSize.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebSettings.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/tools/test_shell/simple_dom_storage_system.h"

// This header file has a weird dependency and it has to be here after the
// others
#include "third_party/WebKit/Source/WebCore/platform/chromium/KeyboardCodes.h"

#include "lb_cookie_store.h"
#include "lb_globals.h"
#include "lb_graphics.h"
#include "lb_local_storage_database_adapter.h"
#include "lb_on_screen_display.h"
#include "lb_resource_loader_bridge.h"
#include "lb_savegame_syncer.h"
#include "lb_shell.h"
#include "lb_shell/lb_web_view_host_impl.h"
#include "lb_system_stats_tracker.h"
#include "lb_user_playback_input_device.h"
#include "lb_web_view_delegate.h"
#include "lb_web_media_player_delegate.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
// for open()
#include <fcntl.h>
#endif

using webkit_media::LBWebMediaPlayerDelegate;

LBWebViewHost* LBWebViewHost::web_view_host_ = NULL;

// static
LBWebViewHost* LBWebViewHost::Create(LBShell* shell,
                                     LBWebViewDelegate* delegate,
                                     const webkit_glue::WebPreferences& prefs) {
  DCHECK(!web_view_host_);
  web_view_host_ = new LBWebViewHostImpl(shell);

  shell->webkit_message_loop()->PostTask(FROM_HERE,
      base::Bind(&LBWebViewHost::CreateWebWidget,
                 delegate, prefs, &web_view_host_->webwidget_));

  // Wait for the above initialization to complete on the webkit thread before
  // proceeding.
  shell->SyncWithWebKit();

  LBGraphics::GetPtr()->SetWebViewHost(web_view_host_);

  return web_view_host_;
}

void LBWebViewHost::Destroy() {
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  console_->Shutdown();
  delete console_;
#endif
  if (webwidget_) {
    // Close the web widget from the webkit thread
    shell_->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(&LBWebViewHost::DestroyWebWidget, webwidget_));

    // Wait for the webkit thread to close down the web widget
    shell_->SyncWithWebKit();

    webwidget_ = NULL;
  }
  web_view_host_ = NULL;
}

LBWebViewHost::LBWebViewHost(LBShell* shell)
    : exit_(false)
    , shell_(shell)
    , pause_application_task_done_(false)
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    , console_(NULL)
    , webkit_wedged_(false)
#endif
{
  webkit_message_loop_ = shell->webkit_message_loop();
  DCHECK(webkit_message_loop_);

#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (LB::Memory::ShutdownApplicationMinutes() > 0) {
    webkit_message_loop_->PostDelayedTask(FROM_HERE,
        base::Bind(&LBWebViewHost::RequestQuit, base::Unretained(this)),
        base::TimeDelta::FromMinutes(LB::Memory::ShutdownApplicationMinutes()));
  }

  // Create a stats update thread so that stats update objects have a thread
  // to attach to and run their updates on
  stats_update_thread_.reset(new LB::StatsUpdateThread());

  // Start up our stats tracking object
  system_stats_tracker_.reset(new LB::SystemStatsTracker(
      LB::StatsUpdateThread::GetPtr()->GetMessageLoop()));
#endif
}

LBWebViewHost::~LBWebViewHost() {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  system_stats_tracker_.reset();
#endif
}

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
bool LBWebViewHost::toggleWebKitWedged() {
  webkit_wedged_ = !webkit_wedged_;
  SetWebKitPaused(webkit_wedged_);
  return webkit_wedged_;
}

void LBWebViewHost::clearInput() {
  console_buffer_.clear();
  LB::OnScreenDisplay::GetPtr()->GetConsole()->ClearInput();
}
#endif  // __LB_SHELL__ENABLE_CONSOLE__

bool LBWebViewHost::FilterKeyEvent(
    WebKit::WebKeyboardEvent::Type type, int key_code,
    wchar_t char_code,
    WebKit::WebKeyboardEvent::Modifiers modifiers,
    bool is_console_eligible) {

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LB::OnScreenDisplay* osd = LB::OnScreenDisplay::GetPtr();
  LB::OnScreenConsole* osc = osd->GetConsole();

  if (is_console_eligible &&
      type == WebKit::WebKeyboardEvent::RawKeyDown &&
      modifiers == WebKit::WebKeyboardEvent::ControlKey) {
    // this is a release event for a control-key-combo.
    switch (key_code) {
      case 'L':  // control-L clears the screen
        osc->ClearOutput();
        break;

      case 'O':  // control-O toggles the console display
        osd->ToggleConsole();
        break;

      case 'R':  // control-R reloads the page
        console_->ParseAndExecuteCommand(osc, "reload");
        break;

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
      case 'S':  // control-S takes a screenshot
        LBGraphics::GetPtr()->TakeScreenshot("");
        break;
#endif

      case 'W':  // control-W pauses/unpauses ("wedges") the WebKit thread
        console_->ParseAndExecuteCommand(osc, "wedge");
        break;
    }
    // do not pass these intercepted keys to the browser.
    return false;
  }

  if (osd->ConsoleVisible() && is_console_eligible) {
    // the console is visible, so let's pass all normal chars to the console.
    if (type == WebKit::WebKeyboardEvent::Char
        && (modifiers & ~WebKit::WebKeyboardEvent::ShiftKey) == 0) {
      switch (char_code) {
        case 10:  // treat a new line char code as an enter
        case 13:  // enter
          if (console_buffer_.length()) {
            // move the command to the output console
            osc->OutputPuts(base::StringPrintf(
                "\n> %s\n", console_buffer_.c_str()));

            console_->ParseAndExecuteCommand(osc, console_buffer_);
            // clear the buffer
            console_buffer_.clear();
          }
          break;

        case 8:  // backspace
          if (console_buffer_.size()) {
            console_buffer_.erase(console_buffer_.size() - 1);
          }
          break;

        case 27:  // escape
          // don't stuff these into the input buffer, please.
          // it's confusing to have mysterious non-printable characters
          // mucking up the command-line when you try to quit a video.
          break;

        default:
          console_buffer_.push_back(char_code);
          break;
      }

      // update the input console
      osc->InputClearAndPuts(
          base::StringPrintf("> %s", console_buffer_.c_str()));
    }

    // do not let this keystroke fall through to the browser.
    return false;
  }
#endif

  if (modifiers & ~WebKit::WebKeyboardEvent::ShiftKey) {
    // We do not pass control and alt combos to the browser so that we don't
    // activate some unsupported or non-functional (for us) chromium feature.
    return false;
  }

  if (key_code == WebCore::VKEY_TAB) {
    // Don't pass through the TAB key, to prevent TAB navigation
    return false;
  }

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  // trap screenshot key if sent
  if (key_code == WebCore::VKEY_SNAPSHOT) {
    if (type == WebKit::WebKeyboardEvent::KeyUp) {
      // signal screenshot
     LBGraphics::GetPtr()->TakeScreenshot("");
    }
    // do not send to webkit
    return false;
  }
#endif

  // the browser may consume this event.
  return true;
}

void LBWebViewHost::SendKeyEvent(
    WebKit::WebInputEvent::Type type,
    int key_code,
    wchar_t char_code,
    WebKit::WebInputEvent::Modifiers modifiers,
    bool is_console_eligible) {
#if defined(__LB_XB1__)
  // TODO: Do this check on all platforms once all platforms have a message loop
  // b/12080903
  DCHECK(static_cast<LBWebViewHostImpl*>(this)->
         message_loop_proxy()->BelongsToCurrentThread());
#endif

#if defined(__LB_SHELL_DEBUG_TASKS__)
  // This keystroke info is extremely useful in debugging task-related issues.
  if (!char_code) printf("Sending keycode %d\n", key_code);
#endif
  TRACE_EVENT_INSTANT1("lb_shell", "SendKeyEvent", "Key Code", key_code);

  if (FilterKeyEvent(type, key_code, char_code, modifiers,
                     is_console_eligible) == false) {
    // caught by filter, do not pass on.
    return;
  }

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // only unfiltered events can be recorded, if we're recording.
  if (input_recorder_) {
    input_recorder_->WriteEvent(type, key_code, char_code, modifiers);
  }
#endif

  WebKit::WebKeyboardEvent key_event;
  key_event.type = type;
  key_event.windowsKeyCode = key_code;
  key_event.text[0] = char_code;
  key_event.text[1] = 0;
  key_event.unmodifiedText[0] = char_code;
  key_event.unmodifiedText[1] = 0;
  key_event.modifiers = modifiers;
  key_event.setKeyIdentifierFromWindowsKeyCode();
  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBWebViewHost::InputEventTask, base::Unretained(this),
                 key_event));

  OnSentKeyEvent();
}

void LBWebViewHost::InjectKeystroke(
    int key_code,
    wchar_t char_code,
    WebKit::WebKeyboardEvent::Modifiers modifiers,
    bool is_console_eligible) {
  SendKeyEvent(WebKit::WebInputEvent::RawKeyDown, key_code, 0,
               modifiers, is_console_eligible);
  if (char_code != 0) {
    SendKeyEvent(WebKit::WebInputEvent::Char, char_code, char_code,
                 modifiers, is_console_eligible);
  }
  SendKeyEvent(WebKit::WebInputEvent::KeyUp, key_code, 0, modifiers,
               is_console_eligible);
}

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
void LBWebViewHost::RecordKeyInput(const char* file_name) {
  input_recorder_.reset();
  if (file_name) {
    std::string file_path =
        std::string(GetGlobalsPtr()->screenshot_output_path) + "/" + file_name;
    input_recorder_.reset(new LBInputRecorder(file_path));
  }
}

void LBWebViewHost::PlaybackKeyInput(const char* file_name, bool repeat) {
  playback_.reset();
  if (file_name) {
    std::string file_path =
        std::string(GetGlobalsPtr()->screenshot_output_path) + "/" + file_name;
    playback_.reset(new LBPlaybackInputDevice(this, file_path, repeat));
  }
}

void LBWebViewHost::UpdatePlaybackController() {
  if (playback_.get()) {
    playback_->Poll();
    if (playback_->PlaybackComplete()) {
      DLOG(INFO) << "input playback complete.";
      playback_.reset();
    }
  }
}

void LBWebViewHost::FuzzerStart(const std::string& file_name, float time_mean,
                                float time_std,
                                LBInputFuzzer::FuzzedKeys fuzzed_keys) {
  DLOG(INFO) << "*** Starting fuzzer";
  fuzzer_.reset(new LBInputFuzzer(this, time_mean, time_std, fuzzed_keys));
  RecordKeyInput(file_name.c_str());
}

void LBWebViewHost::FuzzerStop() {
  DLOG(INFO) << "*** Stopping fuzzer";
  RecordKeyInput(NULL);
  fuzzer_.reset();
}

bool LBWebViewHost::FuzzerActive() const {
  return fuzzer_.get() != NULL;
}
#endif  // __LB_SHELL__ENABLE_CONSOLE__

WebView* LBWebViewHost::webview() const {
  return static_cast<WebView*>(webwidget_);
}

WebKit::WebWidget* LBWebViewHost::webwidget() const {
  return webwidget_;
}

MessageLoop* LBWebViewHost::webkit_message_loop() const {
  return webkit_message_loop_;
}

void LBWebViewHost::PauseApplicationTask(LBWebViewHost *host) {
  if (host->IsExiting()) return;

  // The purpose of this task is to pause the entire application,
  // including media players, WebKit, and JavaScriptCore.
  DCHECK(MessageLoop::current() == host->webkit_message_loop());

#if !defined(__LB_ANDROID__)
  DLOG(INFO) << StringPrintf("Pausing, main loop has %d items in queue.\n",
                             host->webkit_message_loop()->Size());
#endif

  LBWebMediaPlayerDelegate::Instance()->PauseActivePlayers();

  host->OnApplicationPaused();

  // Poll for unpause or quit while blocking JavaScriptCore and
  // the rest of WebKit.
  while (!host->pause_application_task_done_ && !host->IsExiting()) {
    // sleep for 100 ms and then poll again
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(100));
  }

  // Finally, we unpause all media player instances
  // and return from the task so that WebKit can continue.
  if (!host->IsExiting()) {
    LBWebMediaPlayerDelegate::Instance()->ResumeActivePlayers();
  }

  host->OnApplicationUnpaused();

#if !defined(__LB_ANDROID__)
  DLOG(INFO) << StringPrintf(
      "Unpausing, main loop has %d items in queue.\n",
      host->webkit_message_loop()->Size());
#endif
}

void LBWebViewHost::SetWebKitPaused(bool paused) {
  if (paused) {
    pause_application_task_done_ = false;
    webkit_message_loop()->PostTask(FROM_HERE,
                                    base::Bind(PauseApplicationTask, this));
  } else {
    pause_application_task_done_ = true;
  }
}

void LBWebViewHost::SetPlayersPaused(bool paused) {
  if (paused) {
    webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(&LBWebMediaPlayerDelegate::PauseActivePlayers,
            base::Unretained(LBWebMediaPlayerDelegate::Instance())));
  } else {
    webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(&LBWebMediaPlayerDelegate::ResumeActivePlayers,
            base::Unretained(LBWebMediaPlayerDelegate::Instance())));
  }
}

// static
void LBWebViewHost::NavTask(LBWebViewHost *view_host,
                            LBShell *shell,
                            const GURL& url,
                            const base::Closure& nav_complete) {
  if (view_host->IsExiting()) {
    return;
  }

  shell->Navigate(url);

  if (!nav_complete.is_null()) {
    nav_complete.Run();
  }
}

// static
void LBWebViewHost::NavToBlankTask(LBWebViewHost *view_host,
                                   LBShell *shell,
                                   const base::Closure& nav_complete) {
  shell->NavigateToAboutBlank(nav_complete);
}

void LBWebViewHost::SendNavigateTask(const GURL& url) {
  SendNavigateTask(url, base::Closure());
}

void LBWebViewHost::SendNavigateTask(const GURL& url,
                                     const base::Closure& nav_complete) {
  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(&NavTask, this, shell_, url, nav_complete));
}

void LBWebViewHost::SendNavigateToAboutBlankTask() {
  SendNavigateToAboutBlankTask(base::Closure());
}

void LBWebViewHost::SendNavigateToAboutBlankTask(
    const base::Closure& nav_complete) {
  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(&NavToBlankTask, this, shell_, nav_complete));
}

void LBWebViewHost::RequestQuit() {
  if (exit_)
    return;

  // setup exit flag. viewhost thread exits according to this flag.
  exit_ = true;

  SendNavigateToAboutBlankTask();
}

void LBWebViewHost::ShowNetworkFailure() {
  SendNavigateTask(GURL("local:///network_failure.html"));
}

void LBWebViewHost::ClearAndReloadUserData(
    const base::Closure& userdata_cleared_cb) {
  // Chain of callbacks to be called which will
  // 1) Purge Cookies from memory
  // 2) Clear DOM storage
  // 3) Reload the SaveGame database from disk
  base::Closure reload_database_closure = base::Bind(
      &LBWebViewHost::ReloadDatabase,
      base::Unretained(this),
      userdata_cleared_cb);
  base::Closure clear_dom_storage_closure = base::Bind(
      &LBWebViewHost::ClearDomStorage,
      base::Unretained(this),
      reload_database_closure);

  // Cookies will be purged on the I/O thread
  LBResourceLoaderBridge::PurgeCookies(
      media::BindToLoop(
          webkit_message_loop_->message_loop_proxy(),
          clear_dom_storage_closure));
}

bool LBWebViewHost::HideSplashScreen() {
  DLOG_IF(WARNING, !splash_screen_) << "No extended splash screen available";
  return (splash_screen_ != NULL) && splash_screen_->Hide();
}

void LBWebViewHost::DrawSplashScreen() {
  DCHECK(splash_screen_.get());
  splash_screen_->Draw();
}

void LBWebViewHost::ClearDomStorage(const base::Closure& storage_cleared_cb) {
  // This should be done on the webkit message loop, to ensure that webkit
  // does not attempt to access localStore or cookies while this occurs
  DCHECK_EQ(MessageLoop::current(), webkit_message_loop_);

  // Release cached localStorage data
  SimpleDomStorageSystem::instance().Reset();

  if (!storage_cleared_cb.is_null()) {
    storage_cleared_cb.Run();
  }
}


void LBWebViewHost::ReloadDatabase(const base::Closure& reloaded_cb) {
  // This should be done on the webkit message loop, to ensure that webkit
  // does not attempt to access localStore or cookies while this occurs
  DCHECK_EQ(MessageLoop::current(), webkit_message_loop_);

  // Flush the current savegame to disk
  LBSavegameSyncer::ForceSync(false /* blocking */);

  // Shutdown will block until the flush has completed
  // Dump the savegame from memory, and reload from disk
  LBSavegameSyncer::Shutdown();
  LBSavegameSyncer::Init();

  // TODO: Clean up how these classes are initialized. See b/12821854
  LBLocalStorageDatabaseAdapter::Reinitialize();
  LBCookieStore::Reinitialize();

  if (!reloaded_cb.is_null()) {
    reloaded_cb.Run();
  }
}

void LBWebViewHost::CreateWebWidget(LBWebViewDelegate* delegate,
                                    const webkit_glue::WebPreferences& prefs,
                                    WebKit::WebWidget** out_web_widget) {
  WebKit::WebView* web_view = WebView::create(delegate);

  prefs.Apply(web_view);
  web_view->initializeMainFrame(delegate);
  web_view->setFocus(true);

  // make sure that graphics was already created by this point
  DCHECK(LBGraphics::GetPtr());

  // resize WebKit to device dimensions
  web_view->resize(
      WebKit::WebSize(LBGraphics::GetPtr()->GetDeviceWidth(),
                      LBGraphics::GetPtr()->GetDeviceHeight()));

  // force accelerated rendering in all cases
  web_view->settings()->setForceCompositingMode(true);

  // we'd like to composite directly to the screen, please
  web_view->settings()->setAccelerated2dCanvasEnabled(true);
  web_view->settings()->setAcceleratedCompositingEnabled(true);
  web_view->settings()->setAcceleratedCompositingFor3DTransformsEnabled(
      true);
  web_view->settings()->setAcceleratedCompositingForAnimationEnabled(
      true);
  web_view->settings()->setAcceleratedCompositingForCanvasEnabled(true);
  web_view->settings()->setAcceleratedCompositingForVideoEnabled(true);
  web_view->setCompositorSurfaceReady();

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  web_view->settings()->setShowFPSCounter(true);
#endif

#if defined(__LB_PS4__) || defined(__LB_WIIU__) || defined(__LB_XB1__) || \
    defined(__LB_XB360__)
  // Disable tiling for LBShell, we will never scroll, and thus never benefit
  // from tiling.  By disabling it, we do not pay extra memory cost for border
  // tiles that overlap the boundary of the true surface.
  // TODO(aabtop): Enable this line of code on all platforms.  The only reason
  //               this is not currently done is because the change hasn't been
  //               tested yet on all platforms.
  web_view->settings()->setMaxUntiledLayerSize(WebKit::WebSize(2048, 2048));
#endif

  // Chrome has various conditions for when window.close() is permitted, such
  // that sometimes it works and sometimes it doesn't.  To avoid work-arounds
  // in JavaScript, this settings explicitly allows window.close().
  web_view->settings()->setAllowScriptsToCloseWindows(true);

  // Enable V8's gc JavaScript method.
  webkit_glue::SetJavaScriptFlags("--expose-gc");

  *out_web_widget = web_view;
}

void LBWebViewHost::InputEventTask(const WebKit::WebInputEvent& event) {
  DCHECK_EQ(MessageLoop::current(), webkit_message_loop());
  if (IsExiting())
    return;
  webwidget()->handleInputEvent(event);
}

void LBWebViewHost::SendMouseEvent(const WebKit::WebMouseEvent& event) {
  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBWebViewHost::InputEventTask,
      base::Unretained(this), event));
}

void LBWebViewHost::DestroyWebWidget(WebKit::WebWidget* web_widget) {
  web_widget->close();
}

#if ENABLE(TOUCH_EVENTS)
void LBWebViewHost::SendTouchEvent(WebKit::WebTouchEvent::Type type,
                                   int id, int x, int y) {
  // A touch event can contain multiple touch points, but lbshell
  // does not currently support multi-touch.
  WebKit::WebTouchPoint touch_point;
  touch_point.id = id;
  touch_point.screenPosition = WebKit::WebPoint(x, y);
  touch_point.position = touch_point.screenPosition;
  touch_point.force = 1.0;

  if (type == WebKit::WebTouchEvent::TouchStart)
    touch_point.state = WebKit::WebTouchPoint::StatePressed;
  else if (type == WebKit::WebTouchEvent::TouchMove)
    touch_point.state = WebKit::WebTouchPoint::StateMoved;
  else
    touch_point.state = WebKit::WebTouchPoint::StateReleased;

  // Set up and send a WebTouchEvent
  WebKit::WebTouchEvent event;
  event.type = type;
  event.touchesLength = 1;
  event.touches[0] = touch_point;
  // changedTouches and targetTouches appear to be populated automatically

  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBWebViewHost::InputEventTask,
                 base::Unretained(this), event));
}
#endif
