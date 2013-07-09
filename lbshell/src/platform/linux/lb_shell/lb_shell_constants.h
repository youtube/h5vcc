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
// Constants useful for LB_Shell project.

#ifndef SRC_LB_SHELL_CONSTANTS_H_
#define SRC_LB_SHELL_CONSTANTS_H_

#include <stdint.h>

// Thread settings.
// Settings for all lb_shell threads are here.

// Main Thread/WebKit
const int kWebKitThreadStackSize = 1024 * 1024;
const int kWebKitThreadPriority = 3;

// For IO
const int kIOThreadStackSize = 256 * 1024;
const int kIOThreadPriority = 10;
const int kCacheThreadStackSize = 32 * 1024;

// For ObjectWatcher
const int kObjectWatcherThreadStackSize = 4 * 1024;
const int kObjectWatcherThreadPriority = 1;

// LBWebViewHost thread
const int kViewHostThreadStackSize = 32 * 1024;
const int kViewHostThreadPriority = 4;

// For game data thread
const int kSaveGameDaemonStackSize = 128 * 1024;
const int kSaveGameDaemonPriority = 5;

const int kAppCountersStackSize = 64 * 1024;

// Chrome Compositor Thread
const int kCompositorThreadAffinity = 0; // Unused dummy.
const int kCompositorThreadStackSize = 256 * 1024;
const int kCompositorThreadPriority = 2;

// Network I/O threads
const int kNetworkIOThreadAffinity = -1; // Unused

// Alignment and size for socket receive buffer.
static const uint32_t kNetworkIOBufferAlign = 16;
static const uint32_t kNetworkReceiveBufferSize = 16 * 1024;

#endif  // SRC_LB_SHELL_CONSTANTS_H_
