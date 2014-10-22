/*
 * Copyright (C) 2014 Google Inc.  All rights reserved.
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

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_COMMON_CROSSTHREADEVENTDISPATCHER_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_COMMON_CROSSTHREADEVENTDISPATCHER_H_

#include <wtf/Assertions.h>
#include <wtf/MainThread.h>

#include "CrossThreadTask.h"

// -----------------------------------------------------------------------------
// Introduction
// -----------------------------------------------------------------------------
//
// Use this file to create a ScriptExecutionContext::Task to issue a callback
// into Javascript. The ScriptExecutionContext maintains a light weight
// Javascript event queue, which is where you should ideally insert a call
// to an asynchronous callback.
// This is generally done in C++ via a special callback IDL interface, using
// its handleEvent() method.
//
// The functions below can be used to simply fire off an event into the queue
// with specific parameters. It is thread-safe when used correctly.
// The following describes a sample application:
//
// Suppose we have a callback IDL file:
// [ Callback ] DOMWindowClickEvent {
//   boolean handleEvent(in boolean leftButton, in DOMClickPosition position);
// }
// The corresponding header file required will have the following signature:
// virtual DOMWindowClickEvent::handleEvent(bool leftButton,
//                                          DOMClickPosition* position);
//
// To fire this particular event from another thread, just invoke
//
// ScriptExecutionContext context;
// RefPtr<DOMWindowClickEvent> handler;
// bool leftButton;
// PassRefPtr<DOMClickPosition> position;
// context->postTask(createEventDispatcherTask(handler, leftButton, position));
//
// Note: Complex objects which are expensive to copy or should not be copied
// are usually RefCounted. Intrinsic values (like boolean, int, etc) can be
// copied directly.
//
// Note: This file only handles callbacks upto 2 arguments. If required, please
// do extend it to add more parameters.
//
// Enjoy!
// -----------------------------------------------------------------------------


// If the calling code is running on the same thread as WebKit main thread
// and some of your values must inherit from RefCounted instead of
// ThreadSafeRefCounted, for example if this is a WebKit class like
// WebCore::File, you can still use them if you define
//   CROSS_THREAD_ALLOW_REFCOUNTED(WebCore::File)
// in your source file.
// Make sure that this statement is never placed in a header file!

#define CROSS_THREAD_ALLOW_REFCOUNTED(T)                \
  template<> struct WebCore::CrossThreadCopierBase<     \
      false, false, WTF::PassRefPtr<T> > {              \
    typedef WTF::PassRefPtr<T> Type;                    \
    static Type copy(Type ptr)                          \
    {                                                   \
      ASSERT(WTF::isMainThread());                      \
      return ptr;                                       \
    }                                                   \
  };

namespace WebCore {

namespace internal {

// Callbacks usually accept the raw pointer value, but we should use a managed
// pointer as much as possible. This helper class gets the raw pointer value
// from a PassRefPtr<>, or the same intrinsic value from the original item,
// whichever be the case. Note that for the below method to work, all complex
// objects passed via RefPtr<> or PassRefPtr<> must inherit from
// ThreadSafeRefCounted or it will throw a compiler error.
//
// Note: One might think that this lets us get away with passing a raw pointer
// around, but thanks to the CrossThreadCopier (a CrossThreadCopier::copy() is
// is invoked on every argument in createCallbackTask), we ensure that the
// only values that get through are either derived from RefCountedThreadSafe
// or match WTF::IsConvertibleToInteger<>)
//

struct argument_conversion_helper {
  template<typename T>
  static T* get(const PassRefPtr<T>& ptr) { return ptr.get(); }

  template<typename U>
  static U get(U value) { return value; }
};

// |ScheduledCallbackHandler| is the function accepted by the CrossThreadTask.
// Given the difference in semantics in handleEvent() and the function pointer
// acceptable by CrossThreadTask, we define this helper function that is passed
// into |createEventDispatcherTask|. Please do not use this directly.
template<typename C, typename P1>
void ScheduledCallbackHandler(ScriptExecutionContext* context,
                              PassRefPtr<C> callback, P1 arg1) {
    callback->handleEvent(argument_conversion_helper::get(arg1));
}

template<typename C, typename P1, typename P2>
void ScheduledCallbackHandler(ScriptExecutionContext* context,
                              PassRefPtr<C> callback, P1 arg1, P2 arg2) {
    callback->handleEvent(argument_conversion_helper::get(arg1),
                          argument_conversion_helper::get(arg2));
}

}  // namespace internal

// Create a ScriptExecutionContext::Task for a callback that accepts 1 param.
template<typename C, typename P1>
PassOwnPtr<ScriptExecutionContext::Task> createEventDispatcherTask(
    const RefPtr<C>& callback, P1 arg1) {
    return createCallbackTask<PassRefPtr<C>, PassRefPtr<C>, P1, P1>(
          &internal::ScheduledCallbackHandler,
          callback, arg1);
}

// Create a ScriptExecutionContext::Task for a callback that accepts 2 params.
template<typename C, typename P1, typename P2>
PassOwnPtr<ScriptExecutionContext::Task> createEventDispatcherTask(
    const RefPtr<C>& callback, P1 arg1, P2 arg2) {
    return createCallbackTask<PassRefPtr<C>, PassRefPtr<C>, P1, P1, P2, P2>(
          &internal::ScheduledCallbackHandler,
          callback, arg1, arg2);
}

}  // namespace WebCore

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_COMMON_CROSSTHREADEVENTDISPATCHER_H_
