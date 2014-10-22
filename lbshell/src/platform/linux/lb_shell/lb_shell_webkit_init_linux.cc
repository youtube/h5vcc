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
#include "lb_shell_webkit_init_linux.h"

#include "base/stringprintf.h"
#include "third_party/WebKit/Source/WTF/wtf/text/CString.h"

class H5vccSystemImpl : public H5vccSystemCommonImpl {
 public:
  virtual ~H5vccSystemImpl() {}

  virtual void RegisterExperiments(const WTF::String& ids) OVERRIDE {
    WTF::CString utf8_ids = ids.utf8();
    DLOG(INFO) << base::StringPrintf("registered experiment ids: %s",
                                     utf8_ids.data());
    NOTIMPLEMENTED();
  }

  virtual bool AreKeysReversed() OVERRIDE {
    return false;  // no such concept on this platform.
  }

  virtual WTF::String GetRegion() OVERRIDE {
    return "";  // hardware is not regioned.
  }

  virtual void StartPlatformAuthentication() OVERRIDE {
    // no such concept on this platform.
  }

  virtual bool TriggerHelp() OVERRIDE {
    // not supported on this platform.
    return false;
  }
};

LBShellWebKitInitLinux::LBShellWebKitInitLinux() {
  system_impl_.reset(new H5vccSystemImpl);
}
