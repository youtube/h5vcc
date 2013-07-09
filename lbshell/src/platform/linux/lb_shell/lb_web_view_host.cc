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
// Owns the primary run loop for LBPS3.  Updates I/O, sets up graphics, and in
// general is the primary contact object between ps3 system SDK and WebKit.

#include "lb_web_view_host.h"

#include "external/chromium/base/bind.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/base/utf_string_conversions.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebSize.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebScriptSource.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebSettings.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "external/chromium/ui/base/keycodes/keyboard_code_conversion_x.h"

#include "external/chromium/webkit/glue/webkit_glue.h"

// This head file has a weird dependency and it has to be here after the others
#include "external/chromium/third_party/WebKit/Source/WebCore/platform/chromium/KeyboardCodes.h"

#include "lb_debug_console.h"
#include "lb_http_handler.h"
#include "lb_memory_manager.h"
#include "lb_network_console.h"
#include "lb_platform.h"
#include "lb_graphics_linux.h"
#include "lb_shell.h"
#include "lb_shell_constants.h"
#include "lb_shell_platform_delegate.h"
#include "lb_web_media_player_delegate.h"
#include "lb_web_view_delegate.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>

using base::StringPrintf;
using namespace WebKit;
using webkit_media::LBWebMediaPlayerDelegate;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
// for open()
#include <fcntl.h>
extern const char *global_screenshot_output_path;
#endif

static void NavTaskStartupURL(LBWebViewHost *view_host, LBShell *shell);
static void NavTask(LBWebViewHost *view_host, LBShell *shell, const GURL& url);

LBWebViewHost* LBWebViewHost::web_view_host_ = NULL;

// static
LBWebViewHost* LBWebViewHost::Create(LBShell* shell,
                                     LBWebViewDelegate* delegate,
                                     const webkit_glue::WebPreferences& prefs) {
  DCHECK(!web_view_host_);
  LBWebViewHost* host = LB_NEW LBWebViewHost(shell);

  host->webwidget_ = WebView::create(delegate);
  prefs.Apply(host->webview());
  host->webview()->initializeMainFrame(delegate);
  host->webview()->setFocus(true);
  // resize WebKit to device dimensions
  host->webwidget()->resize(WebSize(
    LBGraphics::GetPtr()->GetDeviceWidth(),
    LBGraphics::GetPtr()->GetDeviceHeight()));
  // force accelerated rendering in all cases
  host->webview()->settings()->setForceCompositingMode(true);
  // we'd like to composite directly to the screen, please
  // TODO: the function is not available any more. we need to find it and
  // double check we need to reimplement or remove this line.
  //host->webview()->settings()->setCompositeToTextureEnabled(false);
  host->webview()->settings()->setAccelerated2dCanvasEnabled(true);
  host->webview()->settings()->setAcceleratedCompositingEnabled(true);
  host->webview()->settings()->setAcceleratedCompositingFor3DTransformsEnabled(
      true);
  host->webview()->settings()->setAcceleratedCompositingForAnimationEnabled(
      true);
  host->webview()->settings()->setAcceleratedCompositingForCanvasEnabled(true);
  host->webview()->settings()->setAcceleratedCompositingForVideoEnabled(true);
  host->webview()->setCompositorSurfaceReady();

  LBGraphics::GetPtr()->SetWebViewHost(host);

  web_view_host_ = host;
  return host;
}

void LBWebViewHost::Destroy() {
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  console_->Shutdown();
  delete console_;
#endif
  if (webwidget_) {
    webwidget_->close();
    webwidget_ = NULL;
  }
  web_view_host_ = NULL;
}

LBWebViewHost::LBWebViewHost(LBShell* shell)
    : base::SimpleThread("LBWebViewHost Thread",
        base::SimpleThread::Options(kViewHostThreadStackSize,
                                     kViewHostThreadPriority))
    , exit_(false)
    , shell_(shell)
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    , console_(NULL)
    , telnet_connection_(NULL)
    , webkit_wedged_(false)
#endif
    {
  main_message_loop_ = MessageLoop::current();
  DCHECK(main_message_loop_);

#if SHUTDOWN_APPLICATION_AFTER
  NOTIMPLEMENTED();
#endif
}

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
void LBWebViewHost::output(std::string data) {
  // trim trailing newlines, if present.
  // the console library will start a new line for every output.
  while (data.length() && data[data.length() - 1] == '\n') {
    data.erase(data.length() - 1);
  }

  if (telnet_connection_) {
    data.append("\n");
    telnet_connection_->Output(data);
  } else {
      DLOG(INFO) << data;
  }
}

void LBWebViewHost::output_popup(std::string data) {
  output(data);
}

void LBWebViewHost::clearInput() {
  console_buffer_.clear();
}

bool LBWebViewHost::toggleWebKitWedged() {
  webkit_wedged_ = !webkit_wedged_;
  SetWebKitPaused(webkit_wedged_);
  return webkit_wedged_;
}

// static
void LBWebViewHost::MouseEventTask(WebKit::WebInputEvent::Type type,
                                   WebKit::WebMouseEvent::Button button,
                                   int x,
                                   int y,
                                   LBWebViewHost* host) {
  WebKit::WebMouseEvent mouse_event;
  mouse_event.type = type;
  mouse_event.button = button;
  mouse_event.x = x;
  mouse_event.y = y;
  if (host->IsExiting()) return;
  host->webwidget()->handleInputEvent(mouse_event);
}

void LBWebViewHost::SendMouseClick(WebKit::WebMouseEvent::Button button,
                                   int x,
                                   int y) {
  // post a MouseDown event followed by a MouseUp event 100 ms later.
  main_message_loop_->PostTask(FROM_HERE,
      base::Bind(MouseEventTask, WebKit::WebInputEvent::MouseDown, button, x, y, this));
  main_message_loop_->PostDelayedTask(FROM_HERE,
      base::Bind(MouseEventTask, WebKit::WebInputEvent::MouseUp, button, x, y, this),
                 base::TimeDelta::FromMilliseconds(100));
}

#endif

// The owner will be reponsible for deinit therefore Join() is not implemented
// in this class (the function from the base class will be called).
void LBWebViewHost::Start() {
  base::SimpleThread::Start();
}

// the main graphics thread runs here.  this is pretty much the
// "game loop" of LB_SHELL, that is the code responsible for calling
// the system callback, rendering and swapping buffers, polling and
// sending IO off, etc
void LBWebViewHost::Run() {

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  console_ = LB_NEW LBDebugConsole();
  console_->Init(shell_);

  LBNetworkConsole::Initialize(console_);

  // Create http server for WebInspector
  scoped_refptr<LBHttpHandler> http_handler = LB_NEW LBHttpHandler(this);
#endif

// Start loading the main page.
  main_message_loop_->PostTask(FROM_HERE,
    base::Bind(NavTaskStartupURL, this, shell_));

  while (!exit_) {

    // set up render of current frame
    LBGraphics::GetPtr()->UpdateAndDrawFrame();

    // update IO
    UpdateIO();
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    LBNetworkConsole::GetInstance()->Poll();
#endif
    // wait until frame we drew flips to be displayed
    LBGraphics::GetPtr()->BlockUntilFlip();
  }
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LBNetworkConsole::Teardown();
#endif
}

bool LBWebViewHost::FilterKeyEvent(
    WebKit::WebKeyboardEvent::Type type, int key_code,
    wchar_t char_code,
    WebKit::WebKeyboardEvent::Modifiers modifiers,
    bool is_console_eligible) {

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  if (is_console_eligible
      && type == WebKit::WebKeyboardEvent::KeyUp
      && modifiers == WebKit::WebKeyboardEvent::ControlKey) {
    // this is a release event for a control-key-combo.
    switch (key_code) {

      case 'C':  // control-C schedules a JS pause.
        console_->ParseAndExecuteCommand("js pause");
        break;

      case 'R':  // control-R reloads the page
        console_->ParseAndExecuteCommand("reload");
        break;

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
      case 'S': // control-S takes a screenshot
        LBGraphics::GetPtr()->TakeScreenshot("");
        break;
#endif

      case 'W':  // control-W pauses/unpauses ("wedges") the WebKit thread
        console_->ParseAndExecuteCommand("wedge");
        break;
    }
    // do not pass these intercepted keys to the browser.
    return false;
  }
#endif

  if (modifiers & ~WebKit::WebKeyboardEvent::ShiftKey) {
    // We do not pass control and alt combos to the browser so that we don't
    // activate some unsupported or non-functional (for us) chromium feature.
    return false;
  }

  // the browser may consume this event.
  return true;
}

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
void LBWebViewHost::RecordKeyInput(const char* file_name) {
  NOTIMPLEMENTED();
}

void LBWebViewHost::PlaybackKeyInput(const char* file_name, bool repeat) {
  NOTIMPLEMENTED();
}
#endif

static void KeyInputTask(WebKit::WebKeyboardEvent event, LBWebViewHost *host) {
  if (host->IsExiting()) return;
  host->webwidget()->handleInputEvent(event);
}

void LBWebViewHost::SendKeyEvent(
    WebKit::WebInputEvent::Type type,
    int key_code,
    wchar_t char_code,
    WebKit::WebInputEvent::Modifiers modifiers,
    bool is_console_eligible) {
#if defined(__LB_SHELL_DEBUG_TASKS__)
  // This keystroke info is extremely useful in debugging task-related issues.
  if (!char_code) printf("Sending keycode %d\n", key_code);
#endif

  if (FilterKeyEvent(type, key_code, char_code, modifiers,
                     is_console_eligible) == false) {
    // caught by filter, do not pass on.
    return;
  }

  WebKit::WebKeyboardEvent key_event;
  key_event.type = type;
  key_event.windowsKeyCode = key_code;
  key_event.text[0] = char_code;
  key_event.text[1] = 0;
  key_event.unmodifiedText[0] = char_code;
  key_event.unmodifiedText[1] = 0;
  key_event.modifiers = modifiers;
  key_event.setKeyIdentifierFromWindowsKeyCode();
  main_message_loop_->PostTask(FROM_HERE,
      base::Bind(KeyInputTask, key_event, this));
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

static void InjectJSTask(WebCString codeString, LBWebViewHost *host) {
  if (host->IsExiting()) return;
  WebScriptSource source(codeString.utf16());
  host->webview()->mainFrame()->executeScript(source);
}

// NOTE: This is NOT debug functionality!
// Invalid or exception-causing JavaScript is neither detected nor handled.
void LBWebViewHost::InjectJS(const char * code) {
  WebCString codeString(code);
  main_message_loop_->PostTask(FROM_HERE,
      base::Bind(InjectJSTask, codeString, this));
}

static void QuitTask(LBWebViewHost *host) {
  host->Join();
  MessageLoop::current()->Quit();
}

void LBWebViewHost::RequestQuit() {
  // disable WebKit compositing
  LBGraphics::GetPtr()->SetWebKitCompositeEnable(false);
  // setup exit flag. viewhost thread exits according to this flag.
  exit_ = true;
  // Sync webkit and viewhost threads.
  main_message_loop_->PostTask(FROM_HERE, base::Bind(QuitTask, this));
}

static int XKeyEventToWindowsKeyCode(XKeyEvent* event) {
  int windows_key_code =
      ui::KeyboardCodeFromXKeyEvent(reinterpret_cast<XEvent*>(event));
  if (windows_key_code == ui::VKEY_SHIFT ||
      windows_key_code == ui::VKEY_CONTROL ||
      windows_key_code == ui::VKEY_MENU) {
    // To support DOM3 'location' attribute, we need to lookup an X KeySym and
    // set ui::VKEY_[LR]XXX instead of ui::VKEY_XXX.
    KeySym keysym = XK_VoidSymbol;
    XLookupString(event, NULL, 0, &keysym, NULL);
    switch (keysym) {
      case XK_Shift_L:
        return ui::VKEY_LSHIFT;
      case XK_Shift_R:
        return ui::VKEY_RSHIFT;
      case XK_Control_L:
        return ui::VKEY_LCONTROL;
      case XK_Control_R:
        return ui::VKEY_RCONTROL;
      case XK_Meta_L:
      case XK_Alt_L:
        return ui::VKEY_LMENU;
      case XK_Meta_R:
      case XK_Alt_R:
        return ui::VKEY_RMENU;
    }
  }
  return windows_key_code;
}


// Converts a WebInputEvent::Modifiers bitfield into a
// corresponding X modifier state.
static WebKit::WebInputEvent::Modifiers
EventModifiersFromXState(XKeyEvent* keyEvent) {

  unsigned int modifiers = keyEvent->state;
  unsigned int webkit_modifiers = 0;
  if (modifiers & ControlMask) {
    webkit_modifiers |= WebKit::WebInputEvent::ControlKey;
  }
  if (modifiers & ShiftMask) {
    webkit_modifiers |= WebKit::WebInputEvent::ShiftKey;
  }
  if (modifiers & Mod1Mask) {
    webkit_modifiers |= WebKit::WebInputEvent::AltKey;
  }

  if (modifiers & Button1Mask) {
    webkit_modifiers |= WebKit::WebInputEvent::LeftButtonDown;
  }
  if (modifiers & Button2Mask) {
    webkit_modifiers |= WebKit::WebInputEvent::MiddleButtonDown;
  }
  if (modifiers & Button3Mask) {
    webkit_modifiers |= WebKit::WebInputEvent::RightButtonDown;
  }

  return static_cast<WebKit::WebInputEvent::Modifiers>(webkit_modifiers);
}

void LBWebViewHost::UpdateIO() {

  Display* display = LBGraphicsLinux::GetPtr()->GetXDisplay();
  while (XPending(display)) {
    XEvent event;
    XNextEvent(display, &event);
    switch (event.type) {
      case ButtonPress:
      {

      }
      break;
      case KeyPress:
      {
        WebKit::WebInputEvent::Modifiers modifiers =
          EventModifiersFromXState(reinterpret_cast<XKeyEvent*>(&event));
        int windows_key_code =
          XKeyEventToWindowsKeyCode(reinterpret_cast<XKeyEvent*>(&event));
        SendKeyEvent(WebKit::WebInputEvent::RawKeyDown,
                     windows_key_code, 0, modifiers, true);
      }
      break;
      case KeyRelease:
      {
        WebKit::WebInputEvent::Modifiers modifiers =
          EventModifiersFromXState(reinterpret_cast<XKeyEvent*>(&event));

        int windows_key_code =
          XKeyEventToWindowsKeyCode(reinterpret_cast<XKeyEvent*>(&event));
          SendKeyEvent(WebKit::WebInputEvent::KeyUp,
            windows_key_code, 0, modifiers, true);
      }
      break;

      default:
        break;
    }
  }
}

// TODO: Consider moving this method outside of platform-specific code.
// It's here because we create this type in the Create() method
// so we have the specific domain knowledge to know that
// we can cast this thing.  For a simple embedded setup
// like LB we may be able to move this into a centralized spot
// down the road.
WebView* LBWebViewHost::webview() const {
  return static_cast<WebView*>(webwidget_);
}

WebWidget* LBWebViewHost::webwidget() const {
  return webwidget_;
}

MessageLoop* LBWebViewHost::main_message_loop() const {
  return main_message_loop_;
}

static bool PauseApplicationTaskDone = false;
static void PauseApplicationTask(LBWebViewHost *host) {
  if (host->IsExiting()) return;

  // The purpose of this task is to pause the entire application,
  // including media players, WebKit, and JavaScriptCore.
  DCHECK(MessageLoop::current() == host->main_message_loop());

  DLOG(INFO) << StringPrintf("Pausing, main loop has %d items in queue.\n",
                             host->main_message_loop()->Size());

  // First we pause all media player instances.
  LBWebMediaPlayerDelegate::Instance()->PauseActivePlayers();

  // Poll for unpause or quit while blocking JavaScriptCore and
  // the rest of WebKit.
  while (!PauseApplicationTaskDone && !host->IsExiting()) {
    // sleep for 100 ms and then poll again
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(100));
  }

  // Finally, we unpause all media player instances
  // and return from the task so that WebKit can continue.
  if (!host->IsExiting()) {
    LBWebMediaPlayerDelegate::Instance()->ResumeActivePlayers();
  }
  DLOG(INFO) << StringPrintf(
      "Unpausing, main loop has %d items in queue.\n",
      host->main_message_loop()->Size());
}

void LBWebViewHost::SetWebKitPaused(bool paused) {
  if (paused) {
    PauseApplicationTaskDone = false;
    main_message_loop()->PostTask(FROM_HERE,
                                  base::Bind(PauseApplicationTask, this));
  } else {
    PauseApplicationTaskDone = true;
  }
}

void LBWebViewHost::SetPlayersPaused(bool paused) {
  if (paused) {
    main_message_loop()->PostTask(FROM_HERE,
        base::Bind(&LBWebMediaPlayerDelegate::PauseActivePlayers,
            base::Unretained(LBWebMediaPlayerDelegate::Instance())));
  } else {
    main_message_loop()->PostTask(FROM_HERE,
        base::Bind(&LBWebMediaPlayerDelegate::ResumeActivePlayers,
            base::Unretained(LBWebMediaPlayerDelegate::Instance())));
  }
}

void LBWebViewHost::TextInputFocusEvent() {
}

void LBWebViewHost::TextInputBlurEvent() {
}

static void NavTaskStartupURL(LBWebViewHost *view_host, LBShell *shell) {
  if (view_host->IsExiting()) {
    return;
  }

  shell->Navigate(GURL(shell->GetStartupURL()));
}

static void NavTask(LBWebViewHost *view_host, LBShell *shell, const GURL& url) {
  if (view_host->IsExiting()) {
    return;
  }

  shell->Navigate(url);
}

void LBWebViewHost::SendNavigateTask(const GURL& url) {
  main_message_loop_->PostTask(FROM_HERE,
      base::Bind(NavTask, this, shell_, url));
}

void LBWebViewHost::ShowNetworkFailure() {
  // TODO: Needs to setup Linux local://.
  SendNavigateTask(GURL("local://network_failure.html"));
}
