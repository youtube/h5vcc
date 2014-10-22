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
// This file is a fork of:
// external/chromium/webkit/tools/test_shell/simple_resource_loader_bridge.h

#ifndef SRC_LB_RESOURCE_LOADER_BRIDGE_H_
#define SRC_LB_RESOURCE_LOADER_BRIDGE_H_

#include <string>

#include "base/message_loop_proxy.h"
#include "lb_shell_export.h"
#include "net/cookies/cookie_monster.h"
#include "net/http/http_cache.h"
#include "webkit/glue/resource_loader_bridge.h"

class FilePath;
class GURL;
namespace webkit_glue { struct ResourceResponseInfo; }

class LB_SHELL_EXPORT LBResourceLoaderBridge {
 public:
  // Call this function to initialize the simple resource loader bridge.
  // It is safe to call this function multiple times.
  //
  // NOTE: If this function is not called, then a default request context will
  // be initialized lazily.
  //
  static void Init(net::CookieMonster::PersistentCookieStore
                       *persistent_cookie_store,
                   std::string preferred_language,
                   bool no_proxy);

  // Call this function to shutdown the simple resource loader bridge.
  static void Shutdown();

  // Call this during app termination before tearing down WebKit to ensure that
  // all requests have been completed. Must not be called from the IO thread.
  static void WaitForAllRequests();

  // May only be called after Init.
  static void SetCookie(const GURL& url,
                        const GURL& first_party_for_cookies,
                        const std::string& cookie);
  static std::string GetCookies(const GURL& url,
                                const GURL& first_party_for_cookies);

  static void ClearCookies();
  static bool GetCookiesEnabled();
  static void SetCookiesEnabled(bool value);

  // Destroy and re-create the net::CookieMonster, ensuring any cookie state
  // has been cleared. Does not delete cookies saved to disk.
  static void PurgeCookies(const base::Closure& cookies_cleared_cb);

  static bool EnsureIOThread();
  static void SetAcceptAllCookies(bool accept_all_cookies);

  // If called before Init, does nothing.
  static void ChangeLanguage(const std::string& lang);

  // These methods should only be called after Init(), and before
  // Shutdown(). The MessageLoops get replaced upon each call to
  // Init(), and destroyed upon a call to ShutDown().
  static scoped_refptr<base::MessageLoopProxy> GetCacheThread();
  static scoped_refptr<base::MessageLoopProxy> GetIoThread();

  // Call this function to set up whether using file-over-http feature.
  // |file_over_http| indicates whether using file-over-http or not.
  // If yes, when the request url uses file scheme and matches sub string
  // |file_path_template|, SimpleResourceLoaderBridge will use |http_prefix|
  // plus string of after |file_path_template| in original request URl to
  // generate a new http URL to get the data and send back to peer.
  // That is how we implement file-over-http feature.
  static void AllowFileOverHTTP(const std::string& file_path_template,
                                const GURL& http_prefix);

  // Creates a ResourceLoaderBridge instance.
  static webkit_glue::ResourceLoaderBridge* Create(
    const webkit_glue::ResourceLoaderBridge::RequestInfo& request_info);

  static void OutputError(const std::string& message);

#if !defined(__LB_SHELL__FOR_RELEASE__)
  // Enable/Disable error message output when perimeter checking fails.
  static void SetPerimeterCheckLogging(bool enabled) {
    perimeter_log_enabled_ = enabled;
  }

  // Enable/Disable perimeter checking. When disabled, the checks still occur
  // to allow for logging of perimeter violations,
  // but the request will not fail.
  static void SetPerimeterCheckEnabled(bool enabled) {
    perimeter_check_enabled_ = enabled;
  }

  static bool PerimeterLogEnabled() { return perimeter_log_enabled_; }
  static bool PerimeterCheckEnabled() { return perimeter_check_enabled_; }
#endif

 private:
  static bool cookies_enabled_;
#if !defined(__LB_SHELL__FOR_RELEASE__)
  static bool perimeter_log_enabled_;
  static bool perimeter_check_enabled_;
#endif
};

#endif  // SRC_LB_RESOURCE_LOADER_BRIDGE_H_
