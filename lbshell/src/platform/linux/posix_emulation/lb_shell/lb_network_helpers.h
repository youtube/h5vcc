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
#ifndef SRC_PLATFORM_LINUX_POSIX_EMULATION_LB_SHELL_LB_NETWORK_HELPERS_H_
#define SRC_PLATFORM_LINUX_POSIX_EMULATION_LB_SHELL_LB_NETWORK_HELPERS_H_

#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

namespace LB {
namespace Platform {

static inline int close_socket(int s) {
  return close(s);
}

static inline int NetWouldBlock() {
  return errno == EWOULDBLOCK || errno == EAGAIN;
}

static inline int net_errno() {
  return errno;
}

LB_BASE_EXPORT int GetSocketError(int socket);
LB_BASE_EXPORT int GetLocalIpAddress(struct in_addr* addr);

}  // namespace Platform
}  // namespace LB

#define LB_NET_EADDRNOTAVAIL EADDRNOTAVAIL
#define LB_NET_EBADF EBADF
#define LB_NET_EINPROGRESS EINPROGRESS
#define LB_NET_EINVAL EINVAL
#define LB_NET_EMFILE EMFILE
#define LB_NET_ENOPROTOOPT ENOPROTOOPT
#define LB_NET_ERROR_ECONNRESET ECONNRESET
#define LB_NET_EWOULDBLOCK EWOULDBLOCK

#endif  // SRC_PLATFORM_LINUX_POSIX_EMULATION_LB_SHELL_LB_NETWORK_HELPERS_H_
