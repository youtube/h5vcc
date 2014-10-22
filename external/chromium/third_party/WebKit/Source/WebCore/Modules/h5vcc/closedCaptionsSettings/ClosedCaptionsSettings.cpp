/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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
#include "ClosedCaptionsSettings.h"

#include <public/Platform.h>

#if ENABLE(LB_SHELL_GLOBAL_CC_SETTINGS)

namespace WebCore {

// static
H5vcc::ClosedCaptionsSettings *ClosedCaptionsSettings::impl_ = NULL;

// static
void ClosedCaptionsSettings::init() {
  if (!impl_) {
    impl_ = WebKit::Platform::current()->h5vccClosedCaptionsSettings();
  }
}

// static
String ClosedCaptionsSettings::backgroundColor() {
  init();
  return impl_ ? impl_->BackgroundColor() : "";
}

// static
float ClosedCaptionsSettings::backgroundOpacity() {
  init();
  return impl_ ? impl_->BackgroundOpacity() : 0;
}

// static
String ClosedCaptionsSettings::captionColor() {
  init();
  return impl_ ? impl_->CaptionColor() : "";
}

// static
float ClosedCaptionsSettings::captionOpacity() {
  init();
  return impl_ ? impl_->CaptionOpacity() : 0;
}

// static
String ClosedCaptionsSettings::edgeStyle() {
  init();
  return impl_ ? impl_->EdgeStyle() : "";
}

// static
short ClosedCaptionsSettings::captionSize() {
  init();
  return impl_ ? impl_->CaptionSize() : 0;
}

// static
String ClosedCaptionsSettings::fontFamily() {
  init();
  return impl_ ? impl_->FontFamily() : "";
}

// static
bool ClosedCaptionsSettings::isEnabled() {
  init();
  return impl_ ? impl_->IsEnabled() : false;
}

// static
bool ClosedCaptionsSettings::useDefaultOptions() {
  init();
  return impl_ ? impl_->UseDefaultOptions() : false;
}

// static
String ClosedCaptionsSettings::windowColor() {
  init();
  return impl_ ? impl_->WindowColor() : "";
}

// static
float ClosedCaptionsSettings::windowOpacity() {
  init();
  return impl_ ? impl_->WindowOpacity() : 0;
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_GLOBAL_CC_SETTINGS)
