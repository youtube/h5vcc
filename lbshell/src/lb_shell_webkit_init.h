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
// Provides accessor functions for webkit_glue::WebKitClientImpl
// to provide access to various lb_shell implementations of WebKit
// client functions.

#ifndef _SRC_LB_SHELL_WEBKIT_INIT_H_
#define _SRC_LB_SHELL_WEBKIT_INIT_H_

#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/base/threading/thread.h"
#include "external/chromium/webkit/glue/simple_webmimeregistry_impl.h"
#include "external/chromium/webkit/glue/webkit_glue.h"
#include "external/chromium/webkit/glue/webkitplatformsupport_impl.h"
#include "external/chromium/webkit/support/simple_database_system.h"
#include "external/chromium/webkit/tools/test_shell/simple_dom_storage_system.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebAudioDevice.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebStorageNamespace.h"
#include "webkit/glue/webfileutilities_impl.h"

#include "lb_shell.h"
#include "lb_webcookiejar_impl.h"

// named in honor of TestShellWebKitInit which lives in
// external/chromium/webkit/tools/test_shell/test_shell_webkit_init.h
class LBShellWebKitInit : public webkit_glue::WebKitPlatformSupportImpl {
 public:
  LBShellWebKitInit();
  ~LBShellWebKitInit();
  virtual WebKit::WebMimeRegistry* mimeRegistry() OVERRIDE;
  virtual WebKit::WebFileUtilities* fileUtilities() OVERRIDE;
  virtual WebKit::WebCookieJar* cookieJar() OVERRIDE;
  virtual WebKit::WebString defaultLocale() OVERRIDE;
  virtual WebKit::WebStorageNamespace* createLocalStorageNamespace(
      const WebKit::WebString& path, unsigned quota) OVERRIDE;

  virtual string16 GetLocalizedString(int message_id) OVERRIDE {
    return string16();
  }

  virtual base::StringPiece GetDataResource(int resource_id,
                                            ui::ScaleFactor scale_factor) OVERRIDE;

  virtual void GetPlugins(bool refresh,
      std::vector<webkit::WebPluginInfo>* plugins) OVERRIDE {
    // return no plugins.
  }

  virtual webkit_glue::ResourceLoaderBridge* CreateResourceLoader(
      const webkit_glue::ResourceLoaderBridge::RequestInfo& request_info) OVERRIDE;
  virtual webkit_glue::WebSocketStreamHandleBridge* CreateWebSocketBridge(
      WebKit::WebSocketStreamHandle* handle,
      webkit_glue::WebSocketStreamHandleDelegate* delegate) OVERRIDE;

  void SetThemeEngine(WebKit::WebThemeEngine *engine) {
    theme_engine_ = engine;
  }

  virtual WebKit::WebThemeEngine * themeEngine() {
    return theme_engine_;
  }

  virtual WebKit::WebGraphicsContext3D* createOffscreenGraphicsContext3D(
    const WebKit::WebGraphicsContext3D::Attributes&) OVERRIDE;

  // WebAudio support, overriding Platform.h directly
  virtual double audioHardwareSampleRate() OVERRIDE;
  virtual size_t audioHardwareBufferSize() OVERRIDE;
  virtual WebKit::WebAudioDevice* createAudioDevice(
      size_t bufferSize,
      unsigned numberOfChannels,
      double sampleRate,
      WebKit::WebAudioDevice::RenderCallback* callback) OVERRIDE;

 private:
  void InitializeCompositor();

  scoped_ptr<webkit_glue::SimpleWebMimeRegistryImpl> mime_registry_;
  WebKit::WebThemeEngine* theme_engine_;
  webkit_glue::WebFileUtilitiesImpl file_utilities_;
  SimpleDomStorageSystem dom_storage_system_;
  LBWebCookieJarImpl cookie_jar_;
  scoped_ptr<base::Thread> compositor_thread_;
};

#endif  // _SRC_LB_SHELL_WEBKIT_INIT_H_
