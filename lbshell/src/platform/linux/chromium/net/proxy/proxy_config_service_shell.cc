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
#include "base/logging.h"
#include "net/proxy/proxy_config.h"
#include "net/proxy/proxy_config_service_shell.h"

namespace net {

ProxyConfigServiceShell::ProxyConfigServiceShell()
  : PollingProxyConfigService(base::TimeDelta(),
        &ProxyConfigServiceShell::GetCurrentProxyConfig) {
}

ProxyConfigServiceShell::~ProxyConfigServiceShell() {
}

// static
void ProxyConfigServiceShell::GetCurrentProxyConfig(ProxyConfig* config) {
  const char *http_proxy = getenv("http_proxy");
  if (!http_proxy) {
    config->proxy_rules().type = ProxyConfig::ProxyRules::TYPE_NO_RULES;
  } else {
    config->proxy_rules().type = ProxyConfig::ProxyRules::TYPE_SINGLE_PROXY;
    config->proxy_rules().single_proxy = ProxyServer::FromURI(
        http_proxy, ProxyServer::SCHEME_HTTP);
  }
}

}  // namespace net
