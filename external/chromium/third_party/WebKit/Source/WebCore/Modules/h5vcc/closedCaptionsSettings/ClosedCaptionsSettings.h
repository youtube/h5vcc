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

#ifndef ClosedCaptionsSettings_h
#define ClosedCaptionsSettings_h

#include "config.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace H5vcc {

class ClosedCaptionsSettings {
 public:
  virtual ~ClosedCaptionsSettings() {}

  virtual String BackgroundColor() = 0;
  virtual float BackgroundOpacity() = 0;
  virtual String CaptionColor() = 0;
  virtual float CaptionOpacity() = 0;
  virtual String EdgeStyle() = 0;
  virtual short CaptionSize() = 0;
  virtual String FontFamily() = 0;
  virtual bool IsEnabled() = 0;
  virtual bool UseDefaultOptions() = 0;
  virtual String WindowColor() = 0;
  virtual float WindowOpacity() = 0;
};

}  // namespace H5vcc

#if ENABLE(LB_SHELL_GLOBAL_CC_SETTINGS)

namespace WebCore {

class ClosedCaptionsSettings : public RefCounted<ClosedCaptionsSettings> {
 public:
  ClosedCaptionsSettings() {}
  virtual ~ClosedCaptionsSettings() {}

  static String backgroundColor();
  static float backgroundOpacity();
  static String captionColor();
  static float captionOpacity();
  static String edgeStyle();
  static short captionSize();
  static String fontFamily();
  static bool isEnabled();
  static bool useDefaultOptions();
  static String windowColor();
  static float windowOpacity();

 private:
  static void init();

  static H5vcc::ClosedCaptionsSettings *impl_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_GLOBAL_CC_SETTINGS)

#endif  // ClosedCaptionsSettings_h
