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
// Provides a glue layer between Chromium's CookieMonster and WebKit's
// WebCookieJar classes.

#include "lb_resource_loader_bridge.h"
#include "lb_webcookiejar_impl.h"

using WebKit::WebString;
using WebKit::WebURL;

void LBWebCookieJarImpl::setCookie(const WebURL& url,
                                   const WebURL& first_party_for_cookies,
                                   const WebString& cookie) {
  LBResourceLoaderBridge::SetCookie(url, first_party_for_cookies, cookie.utf8());
}

WebString LBWebCookieJarImpl::cookies(const WebURL& url,
                                      const WebURL& first_party_for_cookies) {
  return WebString::fromUTF8(
    LBResourceLoaderBridge::GetCookies(url, first_party_for_cookies));
}
