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

#include "lb_network_helpers.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>

namespace LB {
namespace Platform {

int GetLocalIpAddress(struct in_addr* addr) {
  struct ifaddrs* ifaddr;
  int ret = getifaddrs(&ifaddr);
  if (ret == -1) return ret;

  for (struct ifaddrs* it = ifaddr; it != NULL; it = it->ifa_next) {
    if (it->ifa_addr == NULL) continue;

    // don't support non IPv4.
    if (it->ifa_addr->sa_family != AF_INET) continue;

    struct sockaddr_in* if_addr = (struct sockaddr_in*)(it->ifa_addr);

    // skip if loopback adapter.
    if (if_addr->sin_addr.s_addr == htonl(INADDR_LOOPBACK)) continue;

    // copy over the address
    *addr = if_addr->sin_addr;
    freeifaddrs(ifaddr);
    return 0;
  }

  freeifaddrs(ifaddr);
  return -1;
}

int GetSocketError(int socket) {
  int os_error = 0;
  socklen_t len = sizeof(os_error);
  if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &os_error, &len) < 0)
    os_error = net_errno();
  return os_error;
}

}  // namespace Platform
}  // namespace LB

