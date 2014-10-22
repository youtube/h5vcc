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
#include "System.h"

#include <public/Platform.h>

#ifdef __LB_SHELL_USE_JSC__
#include "GCController.h"
#else
#include "V8GCController.h"
#endif
#include "steel_build_id.h"
#include "steel_version.h"

namespace WebCore {

// static
H5vcc::System *System::impl_ = NULL;

// static
void System::init() {
  if (!impl_) {
    impl_ = WebKit::Platform::current()->h5vccSystem();
  }
}

// static
void System::collectGarbage() {
#ifdef __LB_SHELL_USE_JSC__
  gcController().garbageCollectNow();
#else
  V8GCController().collectGarbage();
#endif
}

// static
String System::getLocalizedString(const String& key) {
  init();

  // String::String() constructs a 'null' string. This is used to represent a
  // 'null' or 'undefined' value from JavaScript in a string-typed parameter.
  // The IDL specification from the W3C states that when an interface returns a
  // DOMString type, and the class returns a null string, the specified behavior
  // is that the bindings convert this null string into the literal string
  // "null". To get an undefined value instead, you have to add an attribute:
  // [TreatReturnedNullStringAs=Undefined]
  // static DOMString getLocalizedString(DOMString key);
  // http://trac.webkit.org/wiki/WebKitIDL#TreatReturnedNullStringAs
  return impl_ ? impl_->GetLocalizedString(key) : String();
}

// static
void System::minimize() {
  init();
  if (impl_) {
    impl_->Minimize();
  }
}

// static
void System::startPlatformAuthentication() {
  init();
  if (impl_) impl_->StartPlatformAuthentication();
}

// static
bool System::triggerHelp() {
  init();
  return impl_ ? impl_->TriggerHelp() : false;
}

// static
String System::getViewState() {
  init();
  return impl_ ? impl_->GetViewState() : "";
}

// static
bool System::tryUnsnapToFullscreen() {
  init();
  return impl_ ? impl_->TryUnsnapToFullscreen() : false;
}

// static
bool System::hideSplashScreen() {
  init();
  return impl_ ? impl_->HideSplashScreen() : false;
}

// static
void System::registerExperiments(const String& ids) {
  init();
  if (impl_) impl_->RegisterExperiments(ids);
}

// static
String System::getVideoContainerSizeOverride() {
  init();
  return impl_ ? impl_->GetVideoContainerSizeOverride() : "";
}

// static
bool System::areKeysReversed() {
  init();
  return impl_ ? impl_->AreKeysReversed() : false;
}

// static
String System::buildId() {
  return STEEL_BUILD_ID;
}

// static
String System::platform() {
#if defined(__LB_ANDROID__)
  return "Android";
#elif defined(__LB_LINUX__)
  return "Linux";
#elif defined(__LB_PS3__)
  return "PS3";
#elif defined(__LB_PS4__)
  return "PS4";
#elif defined(__LB_WIIU__)
  return "WiiU";
#elif defined(__LB_XB1__)
  return "XB1";
#elif defined(__LB_XB360__)
  return "XB360";
#else
# error Platform constant not defined!
#endif
}

// static
String System::region() {
  init();
  return impl_ ? impl_->GetRegion() : "";
}

// static
String System::version() {
  return STEEL_VERSION;
}

}  // namespace WebCore
