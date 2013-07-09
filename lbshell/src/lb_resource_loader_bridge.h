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

#ifndef _LB_RESOURCE_LOADER_BRIDGE_H_
#define _LB_RESOURCE_LOADER_BRIDGE_H_

#include <string>
#include "external/chromium/base/message_loop_proxy.h"
#include "external/chromium/net/cookies/cookie_monster.h"
#include "external/chromium/net/http/http_cache.h"
#include "external/chromium/webkit/glue/resource_loader_bridge.h"

class FilePath;
class GURL;
namespace webkit_glue { class ResourceResponseInfo; }

class LBResourceLoaderBridge {
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

  // May only be called after Init.
  static void SetCookie(const GURL& url,
                        const GURL& first_party_for_cookies,
                        const std::string& cookie);
  static std::string GetCookies(const GURL& url,
                                const GURL& first_party_for_cookies);
  static void ClearCookies();
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

  // Steel has it's own set of rules on what URL should be allowed and what not.
  // Do this check here, and return true if everything is ok and the URL should
  // be loaded.
  static bool DoesHttpResponsePassSecurityCheck(
      const GURL& url, const webkit_glue::ResourceResponseInfo& info);

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // Enabled/Disable error message output when perimeter checking fails.
  static void SetPerimeterCheckLogging(bool enabled);

  // Enabled/Disable perimeter checking. When disabled, the checks still occur
  // to allow  for logging of perimeter violations, but the request will not fail.
  static void SetPerimeterCheckEnabled(bool enabled);
#endif
};

#endif  // _LB_RESOURCE_LOADER_BRIDGE_H_
