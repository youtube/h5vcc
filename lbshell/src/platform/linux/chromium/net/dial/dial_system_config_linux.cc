// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external/chromium/net/dial/dial_system_config.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include "external/chromium/base/logging.h"

#include "lb_platform.h"

namespace net {

const char* DialSystemConfig::kDefaultFriendlyName = "Dummy Steel Client";
const char* DialSystemConfig::kDefaultManufacturerName = "GNU";
const char* DialSystemConfig::kDefaultModelName = "Linux";

// static
std::string DialSystemConfig::GeneratePlatformUuid() {
  struct in_addr addr;
  DCHECK_EQ(0, LB::Platform::GetLocalIpAddress(&addr));
  return std::string(inet_ntoa(addr));
}

} // namespace net
