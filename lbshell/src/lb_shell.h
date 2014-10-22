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
#ifndef SRC_LB_SHELL_H_
#define SRC_LB_SHELL_H_

#include "external/chromium/base/bind.h"
#include "external/chromium/base/file_path.h"
#include "external/chromium/base/memory/weak_ptr.h"
#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/net/http/http_cache.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebNavigationPolicy.h"
#include "external/chromium/webkit/glue/webpreferences.h"
#include "lb_debug_console.h"
#include "lb_navigation_controller.h"
#include "lb_shell_export.h"
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
#include "lb_tracing_manager.h"
#endif
#include "lb_web_view_delegate.h"
#include "lb_web_view_host.h"

class GURL;

class LB_SHELL_EXPORT LBShell : public base::SupportsWeakPtr<LBShell> {
 public:
  LBShell(const std::string& startup_url, MessageLoop* webkit_message_loop);
  virtual ~LBShell();

  // platform-specific initialization and cleanup methods
  static void InitializeLBShell();
  static void ShutdownLBShell();

  static void InitLogging();
  static void CleanupLogging();

  static void InitStrings();

  static void SetPreferredLanguage(const std::string& lang);
  static std::string PreferredLanguage();

  static std::string PreferredLocale();

  // Load the string referred to by |id|, translated to the system language.
  // This function will return one of these (in preference order):
  // 1) The text that matches |id| and the system language.
  // 2) The text that matches |id| and the language "en-US".
  // 3) The string |fallback|.
  static std::string GetString(const char *id, const std::string& fallback);

  void Show(WebKit::WebNavigationPolicy policy);
  void HandleNetworkFailure(WebKit::WebFrame* frame);
  void HandleNetworkSuccess(WebKit::WebFrame* frame);

  void StartedProvisionalLoad(WebKit::WebFrame* frame);

  void LoadURL(const GURL& url);

  static void ResetWebPreferences();
  webkit_glue::WebPreferences* GetWebPreferences() { return web_prefs_; }

  bool Reload();  // reload last successful page
  bool Retry();  // retry a failed page, reload otherwise
  bool Navigate(const GURL& url);  // to a new url
  bool Navigate(const WebKit::WebHistoryItem& item);  // within history
  bool NavigateToAboutBlank(const base::Closure& closure);

  LBNavigationController * navigation_controller() const {
    return navigation_controller_.get();
  }

  WebKit::WebView* webView() {
    return web_view_host_.get() ? web_view_host_->webview() : NULL;
  }
  LBWebViewHost* webViewHost() { return web_view_host_.get(); }

  const std::string& GetStartupURL() const { return startup_url_; }

  MessageLoop* webkit_message_loop() const { return webkit_message_loop_; }

  // Starts the main LBShell run loop and does not return until the app
  // exits.
  void RunLoop();

  // This function will not return until all messages on the WebKit message
  // loop when this function is called have been processed.
  void SyncWithWebKit();

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // This will be called when a document is successfully loaded
  typedef base::Callback<void(WebKit::WebFrame*)> NetworkSuccessCallback;
  void SetOnNetworkSuccessCallback(const NetworkSuccessCallback& cb) {
    network_success_cb_ = cb;
  }
  void ResetOnNetworkSuccessCallback() {
    network_success_cb_.Reset();
  }

  typedef base::Callback<void(WebKit::WebFrame*)> NetworkFailureCallback;
  void SetOnNetworkFailureCallback(const NetworkFailureCallback& cb) {
    network_failure_cb_ = cb;
  }
  void ResetOnNetworkFailureCallback() {
    network_failure_cb_.Reset();
  }

  typedef base::Callback<void(WebKit::WebFrame*)>
          StartedProvisionalLoadCallback;
  void SetOnStartedProvisionalLoadCallback(
    const StartedProvisionalLoadCallback& cb) {
    started_provisional_load_cb_ = cb;
  }
  void ResetOnStartedProvisionalLoadCallback() {
    started_provisional_load_cb_.Reset();
  }
#endif

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LB::TracingManager* tracing_manager() { return &tracing_manager_; }
#endif

 protected:
  static void LoadStrings(const std::string& lang);

  static std::string preferred_language_;
  static std::string preferred_locale_;
  static std::map<std::string, std::string> strings_;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LB::TracingManager tracing_manager_;
#endif

  scoped_ptr<LBWebViewHost> web_view_host_;
  scoped_ptr<LBWebViewDelegate> delegate_;
  scoped_ptr<LBNavigationController> navigation_controller_;
  int num_retries_;
  int64_t total_retry_delay_ms_;
  bool should_display_network_error_;

  static webkit_glue::WebPreferences * web_prefs_;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  NetworkSuccessCallback network_success_cb_;
  NetworkFailureCallback network_failure_cb_;
  StartedProvisionalLoadCallback started_provisional_load_cb_;
#endif

  std::string startup_url_;

  MessageLoop* webkit_message_loop_;
};

#endif  // SRC_LB_SHELL_H_
