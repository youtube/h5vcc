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
// Implements the WebKit plugin interface requirements.  As the sole function
// of this plugin is to provide the scriptable object most functions return
// defaults.

#ifndef _SRC_LBPS3_LBPS3_LB_DEVICE_PLUGIN_H_
#define _SRC_LBPS3_LBPS3_LB_DEVICE_PLUGIN_H_

#include "external/chromium/third_party/WebKit/Source/WebCore/plugins/npapi.h"
#include "external/chromium/third_party/WebKit/Source/WebCore/plugins/npruntime.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebPlugin.h"

namespace WebKit {
  class WebFrame;
}

class LBDevicePlugin : public WebKit::WebPlugin {
 public:
  LBDevicePlugin(WebKit::WebFrame* frame);
  virtual ~LBDevicePlugin();

  // WebKit::WebPlugin Methods
  virtual bool initialize(WebKit::WebPluginContainer*) { return true; }
  virtual void destroy() {}

  virtual NPObject* scriptableObject();

  virtual void paint(WebKit::WebCanvas*, const WebKit::WebRect&) {}

  // Coordinates are relative to the containing window.
  virtual void updateGeometry(
      const WebKit::WebRect& frameRect, const WebKit::WebRect& clipRect,
      const WebKit::WebVector<WebKit::WebRect>& cutOutsRects, bool isVisible) {}

  virtual void updateFocus(bool update) {}
  virtual void updateVisibility(bool update) {}

  virtual bool acceptsInputEvents() { return false; }
  virtual bool handleInputEvent(const WebKit::WebInputEvent&,
      WebKit::WebCursorInfo&) { return false; }

  virtual void didReceiveResponse(const WebKit::WebURLResponse&) {}
  virtual void didReceiveData(const char* data, int dataLength) {}
  virtual void didFinishLoading() {}
  virtual void didFailLoading(const WebKit::WebURLError&) {}

  // Called in response to WebPluginContainer::loadFrameRequest
  virtual void didFinishLoadingFrameRequest(
      const WebKit::WebURL&, void* notifyData) {}
  virtual void didFailLoadingFrameRequest(
      const WebKit::WebURL&, void* notifyData, const WebKit::WebURLError&) {}

 protected:
  NPObject * npObject_;
  NPP npp_;
  NPClass scriptableNPClass_;
};

#endif  // _SRC_LBPS3_LBPS3_LB_DEVICE_PLUGIN_H_
