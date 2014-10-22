/*
 * Copyright (C) 2014, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_ACCOUNTMANAGER_ACCESSTOKENCALLBACK_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_ACCOUNTMANAGER_ACCESSTOKENCALLBACK_H_


#if ENABLE(LB_SHELL_ACCOUNT_MANAGER)

#include "config.h"

#include <wtf/Forward.h>
#include <wtf/ThreadSafeRefCounted.h>

#include "JSValue.h"

namespace WebCore {

class JSDOMGlobalObject;
class ScriptExecutionContext;

class AccessTokenCallback : public ThreadSafeRefCounted<AccessTokenCallback> {
 public:
  virtual ~AccessTokenCallback() {}
  virtual bool handleEvent(const String& accessToken,
                           const unsigned int expirationSec) = 0;
};

// This is need to let WebKit know how to convert "unsigned int" into
// a JS object. "unsigned int" is required due to the type in idl.
inline JSC::JSValue toJS(
    JSC::ExecState*, JSDOMGlobalObject*, unsigned int number) {
  return JSC::jsNumber(number);
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_ACCOUNT_MANAGER)

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_ACCOUNTMANAGER_ACCESSTOKENCALLBACK_H_
