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
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) H./OWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "AccountManager.h"

#include <public/Platform.h>

#include "CrossThreadEventDispatcher.h"
#include "ScriptExecutionContext.h"

#if ENABLE(LB_SHELL_ACCOUNT_MANAGER)

namespace WebCore {

AccountManager::AccountManager(ScriptExecutionContext* context)
    :  context_(context)
    ,  token_callbacks_(H5vcc::AccountManager::kNumOfTypes) {
  account_manager_impl_ = WebKit::Platform::current()->h5vccAccountManager();
  ASSERT(account_manager_impl_);
  account_manager_impl_->AddWebKitInstance(this);
}

AccountManager::~AccountManager() {
  account_manager_impl_->RemoveWebKitInstance(this);
}

// static
PassRefPtr<AccountManager> AccountManager::create(
  ScriptExecutionContext* context) {
  return adoptRef(new AccountManager(context));
}

void AccountManager::chooseAccount() {
  account_manager_impl_->ChooseAccount();
}

void AccountManager::getAuthToken(
    PassRefPtr<AccessTokenCallback> callback) {
  token_callbacks_[H5vcc::AccountManager::kGetAccessToken] = callback;
  account_manager_impl_->GetAuthToken();
}

void AccountManager::requestPairing(PassRefPtr<AccessTokenCallback> callback) {
  token_callbacks_[H5vcc::AccountManager::kRequestPairing] = callback;
  account_manager_impl_->RequestPairing();
}

void AccountManager::invalidateAuthToken(String authToken) {
  account_manager_impl_->InvalidateAuthToken(authToken);
}

void AccountManager::requestUnpairing(
    PassRefPtr<AccessTokenCallback> callback) {
  token_callbacks_[H5vcc::AccountManager::kRequestUnpairing] = callback;
  account_manager_impl_->RequestUnpairing();
}

void AccountManager::signOut() {
  account_manager_impl_->SignOut();
}

void AccountManager::invokeAccessTokenCallback(
    const H5vcc::AccountManager::CallbackType& method,
    const WTF::String& accessToken,
    const uint64_t expirationSec = 0) {
  RefPtr<AccessTokenCallback> result = token_callbacks_[method];

  ASSERT(context_);
  ASSERT(result.get());

  context_->postTask(createEventDispatcherTask(
      result, accessToken, expirationSec));

  // make sure that we release our reference to the callback
  token_callbacks_[method] = NULL;
}

void AccountManager::releaseReferences() {
  token_callbacks_.clear();
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_ACCOUNT_MANAGER)
