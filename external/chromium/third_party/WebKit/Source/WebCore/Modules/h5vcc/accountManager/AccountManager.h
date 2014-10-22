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

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_ACCOUNTMANAGER_ACCOUNTMANAGER_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_ACCOUNTMANAGER_ACCOUNTMANAGER_H_

#include "config.h"

#include <wtf/Assertions.h>
#include <wtf/OSAllocator.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#include "AccessTokenCallback.h"

namespace WebCore {
class AccessTokenCallback;
class AccountManager;
class ScriptExecutionContext;
}  // namespace WebCore

namespace H5vcc {

class AccountManager {
 public:
  enum CallbackType {
    kRequestPairing,
    kGetAccessToken,
    kRequestUnpairing,
    kNumOfTypes
  };

  virtual ~AccountManager() {}

  virtual void ChooseAccount() {  ASSERT(false);  }  // deprecated.
  virtual void GetAuthToken() = 0;
  virtual void InvalidateAuthToken(const WTF::String& authToken) = 0;
  virtual void RequestPairing() = 0;
  virtual void RequestUnpairing() = 0;

  // SendAuthToken will be called by H5vcc implementation when receiving
  // authToken, then it calls invokeStringCallback to send authToken to JS.
  virtual void SendAuthToken(const CallbackType& method,
                             const WTF::String& authToken,
                             const uint64_t expirationSec) = 0;
  virtual void SignOut() {  ASSERT(false);  }  // deprecated.

  // Called by WebKit whenever a new instance of WebCore::AccountManager is
  // created. Can be called before a previous instance was garbage collected.
  virtual void AddWebKitInstance(WebCore::AccountManager* inst) = 0;

  // Called by WebKit whenever a instance of WebCore::AccountManager is removed.
  // Will be called after the instance was garbage collected.
  virtual void RemoveWebKitInstance(WebCore::AccountManager* inst) = 0;
};

}  // namespace H5vcc

#if ENABLE(LB_SHELL_ACCOUNT_MANAGER)

namespace WebCore {

class AccountManager : public RefCounted<AccountManager> {
 public:
  virtual ~AccountManager();

  static PassRefPtr<AccountManager> create(ScriptExecutionContext* context);

  void chooseAccount();
  void getAuthToken(PassRefPtr<AccessTokenCallback> callback);
  void requestPairing(PassRefPtr<AccessTokenCallback> callback);
  void invalidateAuthToken(String authToken);
  void requestUnpairing(PassRefPtr<AccessTokenCallback> callback);
  void signOut();

  WEBKIT_EXPORT void invokeAccessTokenCallback(
      const H5vcc::AccountManager::CallbackType& method,
      const WTF::String& accessToken,
      const uint64_t expirationSec);
  WEBKIT_EXPORT void releaseReferences();

 private:
  typedef WTF::Vector<RefPtr<AccessTokenCallback> > CallbackRegistry;

  explicit AccountManager(ScriptExecutionContext* context);

  H5vcc::AccountManager* account_manager_impl_;
  ScriptExecutionContext* context_;

  CallbackRegistry token_callbacks_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_ACCOUNT_MANAGER)

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_ACCOUNTMANAGER_ACCOUNTMANAGER_H_
