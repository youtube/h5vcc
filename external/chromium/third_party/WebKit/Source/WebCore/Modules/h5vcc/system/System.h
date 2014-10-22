/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_SYSTEM_SYSTEM_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_SYSTEM_SYSTEM_H_

#include "config.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace H5vcc {

class System {
 public:
  virtual ~System() {}
  virtual void RegisterExperiments(const String& ids) = 0;
  virtual String GetVideoContainerSizeOverride() = 0;
  virtual String GetViewState() = 0;
  virtual bool AreKeysReversed() = 0;
  virtual String GetLocalizedString(const String& key) = 0;
  virtual String GetRegion() = 0;
  virtual void Minimize() = 0;
  virtual void StartPlatformAuthentication() = 0;
  virtual bool TriggerHelp() = 0;
  virtual bool TryUnsnapToFullscreen() = 0;
  virtual bool HideSplashScreen() = 0;
};

}  // namespace H5vcc

namespace WebCore {

class System : public RefCounted<System> {
 public:
  System() {}
  virtual ~System() {}

  static void collectGarbage();
  static String getLocalizedString(const String& key);
  static String getVideoContainerSizeOverride();
  static String getViewState();
  static void minimize();
  static void registerExperiments(const String& ids);
  static void startPlatformAuthentication();
  static bool triggerHelp();
  static bool tryUnsnapToFullscreen();
  static bool hideSplashScreen();

  static bool areKeysReversed();
  static String buildId();
  static String platform();
  static String region();
  static String version();

 private:
  static void init();

  static H5vcc::System *impl_;
};

}  // namespace WebCore

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_SYSTEM_SYSTEM_H_
