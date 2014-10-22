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

#ifndef SRC_LB_HTTP_USER_AGENT_SETTINGS_H_
#define SRC_LB_HTTP_USER_AGENT_SETTINGS_H_

#include <string>

#include "external/chromium/net/url_request/http_user_agent_settings.h"
#include "external/chromium/webkit/user_agent/user_agent.h"

class LBHttpUserAgentSettings : public net::HttpUserAgentSettings {
 public:
  explicit LBHttpUserAgentSettings(const std::string& accept_language)
      : accept_language_(accept_language) {
  }

  LBHttpUserAgentSettings()
      : accept_language_("") {
  }

  virtual ~LBHttpUserAgentSettings() { }

  // net::HttpUserAgentSettings methods
  virtual std::string GetAcceptLanguage() const {
    return accept_language_;
  }

  virtual std::string GetAcceptCharset() const {
    return std::string("utf-8");
  }

  virtual std::string GetUserAgent(const GURL& url) const {
    return webkit_glue::GetUserAgent(url);
  }

  void set_accept_language(const std::string& lang) {
    accept_language_ = lang;
  }

 private:
  std::string accept_language_;

  DISALLOW_COPY_AND_ASSIGN(LBHttpUserAgentSettings);
};

#endif  // SRC_LB_HTTP_USER_AGENT_SETTINGS_H_

