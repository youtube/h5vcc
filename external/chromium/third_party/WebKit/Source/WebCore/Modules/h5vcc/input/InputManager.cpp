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

#include "config.h"
#if ENABLE(LB_SHELL_INPUT)

#include "InputManager.h"

#include <public/Platform.h>

#include "ScriptExecutionContext.h"
#include "StringCallback.h"

namespace WebCore {

InputManager::InputManager(ScriptExecutionContext* context) {
  context_ = context;
  impl_ = WebKit::Platform::current()->h5vccInputManager();

  // The platform is required to provide an implementation for this interface
  // or make sure that LB_SHELL_INPUT is not defined
  ASSERT(impl_);
  impl_->AddWebKitInstance(this);
}

InputManager::~InputManager() {
  impl_->RemoveWebKitInstance(this);
}

void InputManager::queryVirtualKeyboard(const String& type,
                                        PassRefPtr<StringCallback> onDone,
                                        PassRefPtr<StringCallback> onAbort,
                                        const String& title,
                                        const String& description,
                                        const String& defaultText) {
  // Make sure that we don't overwrite callbacks that belong to an existing
  // operation
  if (queryVirtualKeyboardCallbackDone_ ||
      queryVirtualKeyboardCallbackAbort_) {
    if (onAbort) {
      onAbort->scheduleCallback(context_, "Virtual keyboard already in use");
    }
    return;
  }

  queryVirtualKeyboardCallbackDone_ = onDone;
  queryVirtualKeyboardCallbackAbort_ = onAbort;

  impl_->QueryVirtualKeyboard(type, title, description, defaultText);
}

// static
PassRefPtr<InputManager> InputManager::create(ScriptExecutionContext* context) {
  return adoptRef(new InputManager(context));
}

void InputManager::handleQueryVirtualKeyboard(bool success,
                                              const String& result) {
  ASSERT(context_);

  if (success) {
    if (queryVirtualKeyboardCallbackDone_) {
      queryVirtualKeyboardCallbackDone_->scheduleCallback(context_, result);
    }
  } else {
    if (queryVirtualKeyboardCallbackAbort_) {
      queryVirtualKeyboardCallbackAbort_->scheduleCallback(context_, result);
    }
  }

  // Make sure that we release our reference to the callback
  queryVirtualKeyboardCallbackDone_ = NULL;
  queryVirtualKeyboardCallbackAbort_ = NULL;
}

void InputManager::releaseReferences() {
  queryVirtualKeyboardCallbackDone_ = NULL;
  queryVirtualKeyboardCallbackAbort_ = NULL;
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_INPUT)
