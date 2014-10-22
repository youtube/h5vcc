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

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_DIAL_DIALNATIVESERVICEHANDLER_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_DIAL_DIALNATIVESERVICEHANDLER_H_

#include "config.h"
#include <wtf/OSAllocator.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class DialNativeServiceHandler;
class ScriptExecutionContext;
class StringCallback;
}  // namespace WebCore

namespace H5vcc {
class DialNativeServiceHandler {
 public:
  virtual ~DialNativeServiceHandler() {}

  // Called by WebKit whenever a new instance of
  // WebCore::DialNativeServiceHandler is created.
  // Can be called before a previous instance was garbage collected.
  virtual void Connect(WebCore::DialNativeServiceHandler* inst) = 0;

  // Called by WebKit whenever a instance of WebCore::DialNativeServiceHandler
  // is removed. Will be called after the instance was garbage collected.
  virtual void Disconnect(WebCore::DialNativeServiceHandler* inst) = 0;
};
}  // namespace H5vcc

#if ENABLE(LB_SHELL_DIAL_NATIVE_SERVICE)

namespace WebCore {

class DialNativeServiceHandler
    : public RefCounted<DialNativeServiceHandler> {
 public:
  virtual ~DialNativeServiceHandler();

  static PassRefPtr<DialNativeServiceHandler> create(
      ScriptExecutionContext* context);

  void registerCallback(WTF::PassRefPtr<WebCore::StringCallback> callback);

  WEBKIT_EXPORT void invokeStringCallback(const WTF::String& request);
  WEBKIT_EXPORT void releaseReferences();

 private:
  explicit DialNativeServiceHandler(ScriptExecutionContext* context);

  H5vcc::DialNativeServiceHandler* impl_;
  ScriptExecutionContext* context_;
  RefPtr<StringCallback> callback_;
};
}  // namespace WebCore
#endif  // ENABLE(LB_SHELL_DIAL_NATIVE_SERVICE)

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_DIAL_DIALNATIVESERVICEHANDLER_H_
