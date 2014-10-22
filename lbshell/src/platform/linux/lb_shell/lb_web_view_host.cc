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
// Owns the primary run loop.  Updates I/O, sets up graphics, and in
// general is the primary contact object between system and WebKit.

#include "lb_web_view_host_impl.h"

#ifdef __LB_SHELL_USE_V8__
// Include v8.h before any X headers.
#include <v8.h>
// Some of X11's defines clash with those defined in v8.h, undef them before
// including X11 headers.
#undef True
#undef False
#undef None
#endif

#include "external/chromium/base/bind.h"
#include "external/chromium/base/debug/trace_event.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/base/utf_string_conversions.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebSize.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebScriptSource.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebSettings.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "external/chromium/third_party/WebKit/Source/WTF/wtf/StackBounds.h"
#include "external/chromium/ui/base/keycodes/keyboard_code_conversion_x.h"

#include "external/chromium/webkit/glue/webkit_glue.h"

// This head file has a weird dependency and it has to be here after the others
#include "external/chromium/third_party/WebKit/Source/WebCore/platform/chromium/KeyboardCodes.h"

#include "lb_debug_console.h"
#include "lb_http_handler.h"
#include "lb_memory_manager.h"
#include "lb_network_console.h"
#include "lb_platform.h"
#include "lb_shell.h"
#include "lb_shell_constants.h"
#include "lb_shell_platform_delegate.h"
#include "lb_web_media_player_delegate.h"
#include "lb_web_view_delegate.h"

// X11 Headers included in lb_graphics_linux.h define macros that cause problems
// compiling chromium headers included by the other lb_*.h headers, so include
// it out of order here.
#include "lb_graphics_linux.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>

using base::StringPrintf;
using webkit_media::LBWebMediaPlayerDelegate;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
// for open()
#include <fcntl.h>
#endif

static void NavTaskStartupURL(LBWebViewHost *view_host, LBShell *shell) {
  if (view_host->IsExiting()) {
    return;
  }

  shell->Navigate(GURL(shell->GetStartupURL()));
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

LBWebViewHostImpl::LBWebViewHostImpl(LBShell* shell)
    : LBWebViewHost(shell) {
}

void LBWebViewHostImpl::UpdateIO() {
  TRACE_EVENT0("lb_shell", "LBWebViewHost::UpdateIO");

  Display* display = LBGraphicsLinux::GetPtr()->GetXDisplay();
  Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", True);
  while (XPending(display)) {
    XEvent event;
    XNextEvent(display, &event);
    switch (event.type) {
      case ClientMessage:
        if (event.xclient.data.l[0] == wm_delete) {
          // This is the window manager's close event.
          RequestQuit();
        }
        break;

      case KeyPress: {
        XKeyEvent* key_event = reinterpret_cast<XKeyEvent*>(&event);
        WebKit::WebInputEvent::Modifiers modifiers =
          EventModifiersFromXState(key_event);
        int windows_key_code = XKeyEventToWindowsKeyCode(key_event);
        SendKeyEvent(WebKit::WebInputEvent::RawKeyDown,
                     windows_key_code, 0, modifiers, true);
        char ascii_key;
        XLookupString(key_event, &ascii_key, 1, NULL, NULL);
        if (ascii_key) {
          SendKeyEvent(WebKit::WebInputEvent::Char,
                       windows_key_code, ascii_key, modifiers, true);
        }
        break;
      }

      case KeyRelease: {
        XKeyEvent* key_event = reinterpret_cast<XKeyEvent*>(&event);
        WebKit::WebInputEvent::Modifiers modifiers =
          EventModifiersFromXState(key_event);

        int windows_key_code = XKeyEventToWindowsKeyCode(key_event);
        SendKeyEvent(WebKit::WebInputEvent::KeyUp,
            windows_key_code, 0, modifiers, true);
        break;
      }
    }
  }

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  if (fuzzer_.get()) {
    fuzzer_->Poll();
  }
#endif  // __LB_SHELL__ENABLE_CONSOLE__
}

// this is pretty much the "game loop" of LB_SHELL, that is the code
// responsible for calling the system callback, rendering and swapping buffers,
// polling and sending IO off, etc
void LBWebViewHostImpl::RunLoop() {
  // We don't explicitly set the main thread stack size in Linux, however
  // it should default to 8MB, and here we ensure that it is at least
  // as large as we want it to be.
  DCHECK_GE(WTF::StackBounds::currentThreadStackBounds().size(),
            kViewHostThreadStackSize);

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  console_ = new LBDebugConsole();
  console_->Init(shell_);

  LBNetworkConsole::Initialize(console_);
#endif

#if ENABLE_INSPECTOR
  // Create http server for WebInspector
  scoped_ptr<LBHttpHandler> http_handler(new LBHttpHandler(this));
#endif

  // Start loading the main page.
  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(NavTaskStartupURL, this, shell_));

  while (!exit_) {
    TRACE_EVENT0("lb_shell", "LBWebViewHost Main Loop Iteration");

    // set up render of current frame
    LBGraphics::GetPtr()->UpdateAndDrawFrame();

    // update IO
    UpdateIO();

    // wait until frame we drew flips to be displayed
    LBGraphics::GetPtr()->BlockUntilFlip();
  }

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LBNetworkConsole::Teardown();
#endif
}
