/*
 * Copyright (C) 2013 Google Inc.  All rights reserved.
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

#include "config.h"
#if ENABLE(LB_SHELL_REMOTE_CONTROL)

#include "JSRemoteMediaInfo.h"
#include "JSRemoteMediaScrollEventCallback.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

using namespace JSC;

// Callbacks in Javascript are bound to a scope, which contains the closure of
// referenced variables in it's path. For normal non-static attributes to an IDL
// generated file, the 'this' object is used to determine the globalObject.
// However, since this is a static function, there is no 'this' object.
// Hence, the generated code does not compile, as it starts assuming a 'this'
// object without defining it.
// To work around this mess, we [Custom] define this function. To get the scope
// where the callback is executed, we retrieve the ScriptExecutionContext from
// the ExecState, and use the ScriptExecutionContext's global scope.
//
// static
JSC::JSValue JSRemoteMediaInfo::setSeekCallback(JSC::ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return throwError(exec, createNotEnoughArgumentsError(exec));

    ScriptExecutionContext* scriptContext =
        jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject())->scriptExecutionContext();
    if (!scriptContext)
        return jsUndefined();

    if (exec->argumentCount() <= 0 || !exec->argument(0).isFunction())
        return throwTypeError(exec);

    // Get the global object from the scriptContext.
    JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(scriptContext, exec);
    RefPtr<RemoteMediaScrollEventCallback> callback =
        JSRemoteMediaScrollEventCallback::create(asObject(exec->argument(0)),
                                                 globalObject);
    RemoteMediaInfo::setSeekCallback(scriptContext, callback);

    // return void.
    return jsUndefined();
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_REMOTE_CONTROL)

