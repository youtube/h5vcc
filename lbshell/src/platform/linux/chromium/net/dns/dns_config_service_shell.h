/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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
#ifndef SRC_PLATFORM_LINUX_CHROMIUM_NET_DNS_DNS_CONFIG_SERVICE_SHELL_H_
#define SRC_PLATFORM_LINUX_CHROMIUM_NET_DNS_DNS_CONFIG_SERVICE_SHELL_H_

#include "net/dns/dns_config_service.h"

namespace net {

namespace internal {

class NET_EXPORT_PRIVATE DnsConfigServiceShell : public DnsConfigService {
 public:
  DnsConfigServiceShell();
  virtual ~DnsConfigServiceShell();

 private:
  // Immediately attempts to read the current configuration.
  virtual void ReadNow() OVERRIDE;

  // Registers system watchers. Returns true iff succeeds.
  virtual bool StartWatching() OVERRIDE;
};

}  // namespace internal

}  // namespace net

#endif  // SRC_PLATFORM_LINUX_CHROMIUM_NET_DNS_DNS_CONFIG_SERVICE_SHELL_H_
