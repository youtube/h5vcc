// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dial/dial_system_config.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include "base/logging.h"

#include "lb_network_helpers.h"

namespace net {

const char* DialSystemConfig::kDefaultFriendlyName = "Dummy Steel Client";
const char* DialSystemConfig::kDefaultManufacturerName = "GNU";
const char* DialSystemConfig::kDefaultModelName = "Linux";

// static
std::string DialSystemConfig::GeneratePlatformUuid() {
  struct in_addr addr;
  int ip_address_error = LB::Platform::GetLocalIpAddress(&addr);
  DCHECK_EQ(0, ip_address_error);
  return std::string(inet_ntoa(addr));
}

}  // namespace net
