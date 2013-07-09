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

#ifndef _LBPS3_LB_WEBCOOKIEJAR_IMPL_H_
#define _LBPS3_LB_WEBCOOKIEJAR_IMPL_H_

#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebCookieJar.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebString.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebURL.h"

// strongly inspired by test_shell/simple_webcookiejar_impl.h
class LBWebCookieJarImpl : public WebKit::WebCookieJar {
 public:
  // WebKit::WebCookieJar methods:
  virtual void setCookie(
      const WebKit::WebURL& url, const WebKit::WebURL& first_party_for_cookies,
      const WebKit::WebString& cookie);
  virtual WebKit::WebString cookies(
      const WebKit::WebURL& url, const WebKit::WebURL& first_party_for_cookies);
};

#endif // _LBPS3_LB_WEBCOOKIEJAR_IMPL_H_
