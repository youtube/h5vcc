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
// Provides a callback-driven interface to WebKit/Chromium for diverse
// functions related to loading and displaying web pages.

#include "lb_web_view_delegate.h"

#include "external/chromium/base/callback_helpers.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebDataSource.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebHistoryItem.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebInputElement.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebURLError.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebURLRequest.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebURLResponse.h"
#include "external/chromium/webkit/tools/test_shell/simple_dom_storage_system.h"
#include "lb_device_plugin.h"
#include "lb_graphics.h"
#include "lb_memory_manager.h"
#include "lb_shell.h"
#include "lb_web_graphics_context_proxy.h"
#include "lb_web_media_player_delegate.h"
#include "external/chromium/media/base/filter_collection.h"
#include "external/chromium/media/base/media_log.h"
#include "external/chromium/media/base/message_loop_factory.h"
#include "external/chromium/webkit/media/webmediaplayer_delegate.h"
#include "external/chromium/webkit/media/webmediaplayer_impl.h"

LBOutputSurface::LBOutputSurface(LBGraphics* graphics) {
  graphics_ = graphics;
  context_ = graphics->GetCompositorContext();
}

LBWebViewDelegate::LBWebViewDelegate(LBShell* shell)
    : shell_(shell) {
}

LBWebViewDelegate::~LBWebViewDelegate() {
}

WebKit::WebStorageNamespace* LBWebViewDelegate::createSessionStorageNamespace(
    unsigned quota) {
  // Enforce quota, ignoring the parameter from WebCore as in Chrome.
  // Match what is in lb_shell_webkit_init
  return SimpleDomStorageSystem::instance().CreateSessionStorageNamespace();
}

void LBWebViewDelegate::SetNavCompletedClosure(base::Closure closure) {
  nav_completed_closure_ = closure;
}

// signal deciding when to hide the splash screen
void LBWebViewDelegate::didFirstVisuallyNonEmptyLayout(WebKit::WebFrame *) {
#if defined(__LB_SHELL__ENABLE_CONSOLE__) && defined(_DEBUG)
  shell_->webViewHost()->output("Non-empty layout.");
#endif
  LBGraphics::GetPtr()->HideSpinner();

  if (!nav_completed_closure_.is_null())
    base::ResetAndReturn(&nav_completed_closure_).Run();
}

void LBWebViewDelegate::show(WebKit::WebNavigationPolicy policy) {
  // test_shell has
  // something along the lines of calls to the OS to make sure
  // that the window returned by GetWidgetHost() is visible.
}

WebKit::WebCompositorOutputSurface* LBWebViewDelegate::createOutputSurface() {

  LBGraphics* graphics = LBGraphics::GetPtr();
  DCHECK(graphics);

  // TODO: Does this belong here?
  graphics->SetWebKitCompositeEnable(true);

  return new LBOutputSurface(graphics);
}

WebKit::WebPlugin* LBWebViewDelegate::createPlugin(
    WebKit::WebFrame* frame, const WebKit::WebPluginParams& params) {
  // Check mime type for "x-youtube/x-device" which indicates we should
  // construct our LBDevicePlugin object.
  if (strcmp(params.mimeType.utf8().data(), "x-youtube/x-device") == 0) {
    return LB_NEW LBDevicePlugin(frame);
  }
  return NULL;
}


WebKit::WebMediaPlayer* LBWebViewDelegate::createMediaPlayer(
      WebKit::WebFrame* frame,
      const WebKit::WebURL& url,
      WebKit::WebMediaPlayerClient* client) {
  scoped_ptr<media::MessageLoopFactory> message_loop_factory(
      new media::MessageLoopFactory());

  scoped_ptr<media::FilterCollection> collection(
      new media::FilterCollection());

    return LB_NEW webkit_media::WebMediaPlayerImpl(
        frame,
        client,
        webkit_media::LBWebMediaPlayerDelegate::WeakInstance(),
        collection.release(),
        NULL,
        NULL,
        message_loop_factory.release(),
        NULL,
        new media::MediaLog());
}


void LBWebViewDelegate::navigateBackForwardSoon(int offset) {
  shell_->navigation_controller()->GoToOffset(offset);
}

int LBWebViewDelegate::historyBackListCount() {
  return shell_->navigation_controller()->GetCurrentEntryIndex();
}

int LBWebViewDelegate::historyForwardListCount() {
  LBNavigationController *nav = shell_->navigation_controller();
  return nav->GetEntryCount() - nav->GetCurrentEntryIndex() - 1;
}


void LBWebViewDelegate::didStartProvisionalLoad(WebKit::WebFrame* frame) {
  shell_->StartedProvisionalLoad(frame);

  WebKit::WebCString name = frame->uniqueName().utf8();
  if (name.isEmpty()) {
    // main frame.

    // During this callback, the frame's current history item refers to what's
    // already on-screen.  Let's make a new one from the URL we're about to
    // start loading.
    WebKit::WebURL web_url = frame->provisionalDataSource()->request().url();
    WebKit::WebHistoryItem item;
    item.initialize();
    item.setURLString(web_url.spec().utf16());

    LBGraphics::GetPtr()->ShowSpinner();

    shell_->navigation_controller()->Pending(item);
  } else {
    // ignored.  we don't track history for iframes.
  }
}

void LBWebViewDelegate::didReceiveServerRedirectForProvisionalLoad(
    WebKit::WebFrame* frame) {
  // a redirect can be treated as a new provisional load for us.
  didStartProvisionalLoad(frame);
}

void LBWebViewDelegate::didCommitProvisionalLoad(
    WebKit::WebFrame* frame, bool is_new_navigation) {
  WebKit::WebHistoryItem item = frame->currentHistoryItem();
  // the docs say always check isNull(), but only an unparseable URL
  // has triggered this so far.
  if (item.isNull()) return;

  WebKit::WebCString name = frame->uniqueName().utf8();
  if (name.isEmpty()) {
    // main frame.
    shell_->navigation_controller()->CommitPending(item);
  } else {
    // ignored.  we don't track history for iframes.
  }
}

void LBWebViewDelegate::didNavigateWithinPage(
    WebKit::WebFrame* frame, bool is_new_navigation) {
  WebKit::WebHistoryItem item = frame->currentHistoryItem();
  // the docs say always check isNull(), but for us it should always be false.
  DCHECK(item.isNull() == false);
  if (item.isNull()) return;

  WebKit::WebCString name = frame->uniqueName().utf8();
  if (name.isEmpty()) {
    // main frame.
    if (is_new_navigation) {
      // the controller didn't get a load request for this.
      shell_->navigation_controller()->AddEntry(item);
    } else {
      // will commit an in-history navigation.
      shell_->navigation_controller()->CommitPending(item);
    }
  } else {
    // ignored.  we don't track history for iframes.
  }
}

void LBWebViewDelegate::didFailProvisionalLoad(
      WebKit::WebFrame* frame,
      const WebKit::WebURLError& error) {
  // Right now, we treat all failures the same.
  didFailLoad(frame, error);
}

void LBWebViewDelegate::didFailLoad(
      WebKit::WebFrame* frame,
      const WebKit::WebURLError& error) {
  WebKit::WebCString name = frame->uniqueName().utf8();
  if (name.isEmpty()) {
    // main frame.

    // This is a load failure that did not result in an error document.
    WebKit::WebCString domain_webcstring = error.domain.utf8();
    std::string domain(domain_webcstring.data(), domain_webcstring.length());
    DLOG(ERROR) << base::StringPrintf("didFailLoad, error %s:%d",
                                      domain.c_str(), error.reason);
    shell_->HandleNetworkFailure(frame);
  }
}

void LBWebViewDelegate::didFinishLoad(WebKit::WebFrame* frame) {
  WebKit::WebCString name = frame->uniqueName().utf8();
  if (name.isEmpty()) {
    // main frame.

    WebKit::WebDataSource *source = frame->dataSource();
    int status_code = source->response().httpStatusCode();
    if (status_code >= 400) {
      // This is a load failure that _did_ result in an error document.
      DLOG(ERROR) << base::StringPrintf("didFinishLoad, status %d",
                                        status_code);
      shell_->HandleNetworkFailure(frame);
    } else {
      shell_->HandleNetworkSuccess(frame);
    }
  }
}

void LBWebViewDelegate::focusedNodeChanged(const WebKit::WebNode& node) {
  if (!node.isNull() && node.isElementNode() && node.nodeName() == "INPUT" &&
      !node.toConst<WebKit::WebInputElement>().isReadOnly()) {
    shell_->webViewHost()->TextInputFocusEvent();
  } else {
    shell_->webViewHost()->TextInputBlurEvent();
  }
}

#if !defined(__LB_SHELL__FOR_RELEASE__)
void LBWebViewDelegate::didAddMessageToConsole(
    const WebKit::WebConsoleMessage& message,
    const WebKit::WebString& source_name, unsigned source_line) {
  // trim the newline from the message
  std::string message_text(message.text.utf8().data());
  if (message_text.length() &&
      message_text[message_text.length() - 1] == '\n') {
    message_text.erase(message_text.length() - 1);
  }

  std::string js_log = StringPrintf("JS [%s:%d] %s",
      source_name.utf8().data(), source_line, message_text.c_str());

  if (message.level == WebKit::WebConsoleMessage::LevelTip ||
      message.level == WebKit::WebConsoleMessage::LevelLog ||
      message.level == WebKit::WebConsoleMessage::LevelWarning) {
    DLOG(INFO) << js_log;
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    shell_->webViewHost()->output(js_log);
#endif
  } else {
    DLOG(ERROR) << js_log;
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    shell_->webViewHost()->output_popup(js_log);
#endif
  }
}
#endif  // !defined(__LB_SHELL__FOR_RELEASE__)
