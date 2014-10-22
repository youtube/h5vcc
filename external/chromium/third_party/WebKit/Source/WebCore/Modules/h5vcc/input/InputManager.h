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

#ifndef InputManager_h
#define InputManager_h

#include "config.h"

#include <wtf/RefCounted.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

#include "ScriptExecutionContext.h"

namespace WebCore {
class InputManager;
class StringCallback;
}  // namespace WebCore

namespace H5vcc {

class InputManager {
 public:
  virtual ~InputManager() {}

  // Use the platform specific virtual keyboard to query for text input from
  // the user. Upon completion, H5vcc implementation should communicate the
  // results back by calling WebCore::InputManager::handleQueryVirtualKeyboard.
  //
  // Arguments:
  //   type: platform specific type of the input keyboard (ex. "Default")
  //   title, description: will be shown in keyboard dialog
  //   defaultText: Used to populate the input field of the keyboard
  virtual void QueryVirtualKeyboard(const String& type,
                                    const String& title,
                                    const String& description,
                                    const String& defaultText) = 0;

  // Called by WebKit whenever a new instance of WebCore::InputManager
  // is created. Can be called before a previous instance was garbage collected.
  virtual void AddWebKitInstance(WebCore::InputManager* inst) = 0;

  // Called by WebKit whenever a new instance of WebCore::InputManager
  // is removed. Will be called after the instance was garbage collected.
  virtual void RemoveWebKitInstance(WebCore::InputManager* inst) = 0;
};

}  // namespace H5vcc

#if ENABLE(LB_SHELL_INPUT)

namespace WebCore {

class InputManager : public RefCounted<InputManager> {
 public:
  virtual ~InputManager();

  static PassRefPtr<InputManager> create(ScriptExecutionContext* context);

  // IDL methods
  void queryVirtualKeyboard(const String& type,
                            PassRefPtr<StringCallback> onDone,
                            PassRefPtr<StringCallback> onAbort,
                            const String& title,
                            const String& description,
                            const String& defaultText);

  // Communicate the results of the platform virtual keyboard back to WebKit.
  // "success" should be set to False to indicate that the input operation
  // was aborted and result should contain the reason.
  WEBKIT_EXPORT void handleQueryVirtualKeyboard(bool success,
                                                const String& result);

  // Call to release all references to WebKit objects that may be preventing
  // this class from being garbage collected.
  WEBKIT_EXPORT void releaseReferences();

 private:
  explicit InputManager(ScriptExecutionContext* context);

  H5vcc::InputManager* impl_;
  ScriptExecutionContext* context_;

  RefPtr<StringCallback> queryVirtualKeyboardCallbackDone_;
  RefPtr<StringCallback> queryVirtualKeyboardCallbackAbort_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_INPUT)

#endif  // InputManager_h
