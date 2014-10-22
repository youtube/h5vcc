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
#include "net/dns/dns_config_service_shell.h"

#include "base/basictypes.h"

namespace net {

namespace internal {

DnsConfigServiceShell::DnsConfigServiceShell() {
  NOTIMPLEMENTED();
}

DnsConfigServiceShell::~DnsConfigServiceShell() {
  NOTIMPLEMENTED();
}

void DnsConfigServiceShell::ReadNow() {
  NOTIMPLEMENTED();
}

bool DnsConfigServiceShell::StartWatching() {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace internal

// static
scoped_ptr<DnsConfigService> DnsConfigService::CreateSystemService() {
  return scoped_ptr<DnsConfigService>(new internal::DnsConfigServiceShell());
}

}  // namespace net

