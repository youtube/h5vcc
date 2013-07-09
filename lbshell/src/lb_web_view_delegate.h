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

#ifndef _SRC_LB_WEB_VIEW_DELEGATE_H_
#define _SRC_LB_WEB_VIEW_DELEGATE_H_

#include "external/chromium/base/basictypes.h"
#include "external/chromium/base/bind.h"
#include "external/chromium/base/memory/weak_ptr.h"
#include "external/chromium/cc/output_surface.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebConsoleMessage.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrameClient.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebPluginParams.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebViewClient.h"

#include "lb_web_graphics_context_3d.h"

class LBShell;
class LBGraphics;

class LBOutputSurface : public cc::OutputSurface {
 public:
  LBOutputSurface(LBGraphics* graphics);

  bool BindToClient(cc::OutputSurfaceClient*) OVERRIDE { return true;}

  const struct Capabilities& Capabilities() const OVERRIDE {
    return capabilities_;
  }

  WebKit::WebGraphicsContext3D* Context3D() const OVERRIDE {
    return context_;
  }

  cc::SoftwareOutputDevice* SoftwareDevice() const OVERRIDE {
    return 0;
  }

 private:
  LBGraphics* graphics_;
  LBWebGraphicsContext3D* context_;

  struct Capabilities capabilities_;
};


class LBWebViewDelegate : public WebKit::WebViewClient,
                          public WebKit::WebFrameClient,
                          public base::SupportsWeakPtr<LBWebViewDelegate> {
 public:
  explicit LBWebViewDelegate(LBShell* shell);
  ~LBWebViewDelegate();
/*
  // WebKit::WebViewClient
  virtual WebKit::WebView* createView(
      WebKit::WebFrame* creator,
      const WebKit::WebURLRequest& request,
      const WebKit::WebWindowFeatures& features,
      const WebKit::WebString& frame_name);
  virtual WebKit::WebWidget* createPopupMenu(WebKit::WebPopupType popup_type);
  virtual WebKit::WebWidget* createPopupMenu(
      const WebKit::WebPopupMenuInfo& info);
*/
  virtual WebKit::WebStorageNamespace* createSessionStorageNamespace(
      unsigned quota);

  // The frame's document finished the initial non-empty layout of a page
  virtual void didFirstVisuallyNonEmptyLayout(WebKit::WebFrame*);

#if !defined(__LB_SHELL__FOR_RELEASE__)
  virtual void didAddMessageToConsole(
      const WebKit::WebConsoleMessage& message,
      const WebKit::WebString& source_name, unsigned source_line);
#endif

    // Creates the output surface that renders to the client's WebView.
  virtual WebKit::WebCompositorOutputSurface* createOutputSurface() OVERRIDE;


/*
  virtual void didStartLoading();
  virtual void didStopLoading();
  virtual bool shouldBeginEditing(const WebKit::WebRange& range);
  virtual bool shouldEndEditing(const WebKit::WebRange& range);
  virtual bool shouldInsertNode(
      const WebKit::WebNode& node, const WebKit::WebRange& range,
      WebKit::WebEditingAction action);
  virtual bool shouldInsertText(
      const WebKit::WebString& text, const WebKit::WebRange& range,
      WebKit::WebEditingAction action);
  virtual bool shouldChangeSelectedRange(
      const WebKit::WebRange& from, const WebKit::WebRange& to,
      WebKit::WebTextAffinity affinity, bool still_selecting);
  virtual bool shouldDeleteRange(const WebKit::WebRange& range);
  virtual bool shouldApplyStyle(
      const WebKit::WebString& style, const WebKit::WebRange& range);
  virtual bool isSmartInsertDeleteEnabled();
  virtual bool isSelectTrailingWhitespaceEnabled();
  virtual void didBeginEditing();
  virtual void didChangeSelection(bool is_selection_empty);
  virtual void didChangeContents();
  virtual void didEndEditing();
  virtual bool handleCurrentKeyboardEvent();
  virtual void spellCheck(
      const WebKit::WebString& text, int& misspelledOffset,
      int& misspelledLength);
  virtual WebKit::WebString autoCorrectWord(
      const WebKit::WebString& misspelled_word);
  virtual void runModalAlertDialog(
      WebKit::WebFrame* frame, const WebKit::WebString& message);
  virtual bool runModalConfirmDialog(
      WebKit::WebFrame* frame, const WebKit::WebString& message);
  virtual bool runModalPromptDialog(
      WebKit::WebFrame* frame, const WebKit::WebString& message,
      const WebKit::WebString& default_value, WebKit::WebString* actual_value);
  virtual bool runModalBeforeUnloadDialog(
      WebKit::WebFrame* frame, const WebKit::WebString& message);
  virtual void showContextMenu(
      WebKit::WebFrame* frame, const WebKit::WebContextMenuData& data);
  virtual void setStatusText(const WebKit::WebString& text);
  virtual void startDragging(
      const WebKit::WebDragData& data, WebKit::WebDragOperationsMask mask,
      const WebKit::WebImage& image, const WebKit::WebPoint& offset);
*/
  virtual void focusedNodeChanged(const WebKit::WebNode& node) OVERRIDE;

  virtual void navigateBackForwardSoon(int offset);
  virtual int historyBackListCount();
  virtual int historyForwardListCount();
/*
  virtual WebKit::WebNotificationPresenter* notificationPresenter();
  virtual WebKit::WebGeolocationClient* geolocationClient();
  virtual WebKit::WebDeviceOrientationClient* deviceOrientationClient();
  virtual WebKit::WebSpeechInputController* speechInputController(
      WebKit::WebSpeechInputListener*);

  // WebKit::WebWidgetClient
  virtual void didInvalidateRect(const WebKit::WebRect& rect);
  virtual void didScrollRect(int dx, int dy,
                             const WebKit::WebRect& clip_rect);
  virtual void scheduleComposite();
  virtual void scheduleAnimation();
  virtual void didFocus();
  virtual void didBlur();
  virtual void didChangeCursor(const WebKit::WebCursorInfo& cursor);
  virtual void closeWidgetSoon();
*/
  virtual void show(WebKit::WebNavigationPolicy policy);
/*
  virtual void runModal();
  virtual WebKit::WebRect windowRect();
  virtual void setWindowRect(const WebKit::WebRect& rect);
  virtual WebKit::WebRect rootWindowRect();
  virtual WebKit::WebRect windowResizerRect();
  virtual WebKit::WebScreenInfo screenInfo();
*/

  // WebKit::WebFrameClient
  virtual WebKit::WebPlugin* createPlugin(
      WebKit::WebFrame*, const WebKit::WebPluginParams&);
/*
  virtual WebKit::WebWorker* createWorker(
      WebKit::WebFrame*, WebKit::WebWorkerClient*);
*/
  virtual WebKit::WebMediaPlayer* createMediaPlayer(
      WebKit::WebFrame*,
      const WebKit::WebURL&,
      WebKit::WebMediaPlayerClient*) OVERRIDE;
/*
  virtual bool allowPlugins(WebKit::WebFrame* frame, bool enabled_per_settings);
  virtual bool allowImages(WebKit::WebFrame* frame, bool enabled_per_settings);
  virtual void loadURLExternally(
      WebKit::WebFrame*, const WebKit::WebURLRequest&,
      WebKit::WebNavigationPolicy);
  virtual WebKit::WebNavigationPolicy decidePolicyForNavigation(
      WebKit::WebFrame*, const WebKit::WebURLRequest&,
      WebKit::WebNavigationType, const WebKit::WebNode&,
      WebKit::WebNavigationPolicy default_policy, bool isRedirect);
  virtual bool canHandleRequest(
      WebKit::WebFrame*, const WebKit::WebURLRequest&);
  virtual WebKit::WebURLError cannotHandleRequestError(
      WebKit::WebFrame*, const WebKit::WebURLRequest& request);
  virtual WebKit::WebURLError cancelledError(
      WebKit::WebFrame*, const WebKit::WebURLRequest& request);
  virtual void unableToImplementPolicyWithError(
      WebKit::WebFrame*, const WebKit::WebURLError&);
  virtual void willPerformClientRedirect(
      WebKit::WebFrame*, const WebKit::WebURL& from, const WebKit::WebURL& to,
      double interval, double fire_time);
  virtual void didCancelClientRedirect(WebKit::WebFrame*);
  virtual void didCreateDataSource(
      WebKit::WebFrame*, WebKit::WebDataSource*);
  */
  virtual void didStartProvisionalLoad(WebKit::WebFrame*);
  virtual void didReceiveServerRedirectForProvisionalLoad(WebKit::WebFrame*);
  virtual void didFailProvisionalLoad(
      WebKit::WebFrame*, const WebKit::WebURLError&);
  virtual void didCommitProvisionalLoad(
      WebKit::WebFrame*, bool is_new_navigation);
  /*
  virtual void didClearWindowObject(WebKit::WebFrame*);
  virtual void didReceiveTitle(
      WebKit::WebFrame*, const WebKit::WebString& title,
      WebKit::WebTextDirection direction);
  virtual void didFinishDocumentLoad(WebKit::WebFrame*);
  virtual void didHandleOnloadEvents(WebKit::WebFrame*);
  */
  virtual void didFailLoad(
      WebKit::WebFrame*, const WebKit::WebURLError&);
  virtual void didFinishLoad(WebKit::WebFrame*);
  virtual void didNavigateWithinPage(
      WebKit::WebFrame*, bool is_new_navigation);
  /*
  virtual void didChangeLocationWithinPage(
      WebKit::WebFrame*);
  virtual void assignIdentifierToRequest(
      WebKit::WebFrame*, unsigned identifier, const WebKit::WebURLRequest&);
  virtual void willSendRequest(
      WebKit::WebFrame*, unsigned identifier, WebKit::WebURLRequest&,
      const WebKit::WebURLResponse& redirectResponse);
  virtual void didReceiveResponse(
      WebKit::WebFrame*, unsigned identifier, const WebKit::WebURLResponse&);
  virtual void didFinishResourceLoad(
      WebKit::WebFrame*, unsigned identifier);
  virtual void didFailResourceLoad(
      WebKit::WebFrame*, unsigned identifier, const WebKit::WebURLError&);
  virtual void didDisplayInsecureContent(WebKit::WebFrame* frame);
  virtual void didRunInsecureContent(
      WebKit::WebFrame* frame,
      const WebKit::WebSecurityOrigin& origin,
      const WebKit::WebURL& target_url);
  virtual bool allowScript(WebKit::WebFrame* frame, bool enabled_per_settings);
  virtual void openFileSystem(
      WebKit::WebFrame* frame,
      WebKit::WebFileSystem::Type type,
      long long size,
      bool create,
      WebKit::WebFileSystemCallbacks* callbacks);
*/
  void SetNavCompletedClosure(base::Closure closure);

 private:
  // non-owning pointer.  Delegate is owned by the host.
  LBShell * shell_;

  base::Closure nav_completed_closure_;

  DISALLOW_COPY_AND_ASSIGN(LBWebViewDelegate);
};

#endif  // _SRC_LB_WEB_VIEW_DELEGATE_H_
