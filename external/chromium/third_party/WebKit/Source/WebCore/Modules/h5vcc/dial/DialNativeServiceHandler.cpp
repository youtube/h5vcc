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

#include "DialNativeServiceHandler.h"

#include <public/Platform.h>
#include "ScriptExecutionContext.h"
#include "StringCallback.h"

#if ENABLE(LB_SHELL_DIAL_NATIVE_SERVICE)

namespace WebCore {

DialNativeServiceHandler::DialNativeServiceHandler(
    ScriptExecutionContext* context) {
  context_ = context;
  impl_ = WebKit::Platform::current()->h5vccDialNativeServiceHandler();
  ASSERT(impl_);
}

DialNativeServiceHandler::~DialNativeServiceHandler() {
  impl_->Disconnect(this);
}

// static
PassRefPtr<DialNativeServiceHandler> DialNativeServiceHandler::create(
    ScriptExecutionContext* context) {
  return adoptRef(new DialNativeServiceHandler(context));
}

void DialNativeServiceHandler::registerCallback(
    PassRefPtr<StringCallback> callback) {
  callback_ = callback;
  impl_->Connect(this);
}

void DialNativeServiceHandler::invokeStringCallback(const WTF::String& req) {
  if (!context_ || !callback_) {
    return;
  }
  callback_->scheduleCallback(context_, req);
}

void DialNativeServiceHandler::releaseReferences() {
  callback_ = NULL;
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_DIAL_NATIVE_SERVICE)
