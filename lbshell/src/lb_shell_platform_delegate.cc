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

#include "lb_shell_platform_delegate.h"

#include "lb_local_storage_database_adapter.h"
#include "lb_shell.h"
#include "media/base/shell_media_platform.h"
#include "object_watcher_shell.h"

// static
void LBShellPlatformDelegate::Init() {
  PlatformInit();

#if !defined(__LB_SHELL_NO_CHROME__)
  media::ShellMediaPlatform::Initialize();
#endif

  LBLocalStorageDatabaseAdapter::Register();

#if !defined(__LB_XB1__) && !defined(__LB_XB360__) && !defined(__LB_ANDROID__)
  // the objectwatcher makes a thread for polling sockets, in a lame
  // emulation of callback io.  start that thread now.  it will block
  // until it has something to watch.
  base::steel::ObjectWatcher::InitializeObjectWatcherSystem();
#endif
}

// static
void LBShellPlatformDelegate::Teardown() {
#if !defined(__LB_XB1__) && !defined(__LB_XB360__) && !defined(__LB_ANDROID__)
  // shutdown object polling thread
  base::steel::ObjectWatcher::TeardownObjectWatcherSystem();
#endif

#if !defined(__LB_SHELL_NO_CHROME__)
  media::ShellMediaPlatform::Terminate();
#endif

  PlatformTeardown();
}
