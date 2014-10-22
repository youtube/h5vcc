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
#include "DvrManager.h"

#if ENABLE(LB_SHELL_DVR)

#include <public/Platform.h>

#include "CrossThreadEventDispatcher.h"
#include "DvrClipArray.h"
#include "DvrClipArrayCallback.h"
#include "DvrFileCallback.h"
#include "File.h"
#include "ScriptExecutionContext.h"
#include "BoolCallback.h"
#include "StringCallback.h"

namespace WebCore {

// Allow the following types to be used with CrossThreadEventDispatcher
// Normally only ThreadSafeRefCounted objects can be used with the dispatched
// however our code is guaranteed to run on a single thread
CROSS_THREAD_ALLOW_REFCOUNTED(StringCallback);

namespace {
// An outstanding operation is canceled because a newer operation was
// requested by JavaScript.
const char* kErrorCancelPrevious = "Canceled due to a newer operation";
}  // namespace

struct DvrManager::ClipArrayCallbackContainer {
  ClipArrayCallbackContainer(PassRefPtr<DvrClipArrayCallback> onSuccess,
                    PassRefPtr<StringCallback> onError)
      : onSuccess(onSuccess)
      , onError(onError) {}

  RefPtr<DvrClipArrayCallback> onSuccess;
  RefPtr<StringCallback> onError;
};

struct DvrManager::BoolCallbackContainer {
  BoolCallbackContainer(PassRefPtr<BoolCallback> onSuccess,
                        PassRefPtr<StringCallback> onError)
      : onSuccess(onSuccess)
      , onError(onError) {}

  RefPtr<BoolCallback> onSuccess;
  RefPtr<StringCallback> onError;
};

DvrManager::DvrManager(ScriptExecutionContext* context) {
  ASSERT(context);
  context_ = context;

  // The platform is required to provide an implementation for this interface
  // or make sure that LB_SHELL_DVR is not defined
  h5vcc_impl_ = WebKit::Platform::current()->h5vccDvrManager();
  ASSERT(h5vcc_impl_);

  h5vcc_impl_->AddWebKitInstance(this);
}

DvrManager::~DvrManager() {
  h5vcc_impl_->RemoveWebKitInstance(this);
}

// static
PassRefPtr<DvrManager> DvrManager::create(ScriptExecutionContext* context) {
  return adoptRef(new DvrManager(context));
}

void DvrManager::getClips(PassRefPtr<DvrClipArrayCallback> onSuccess,
                          PassRefPtr<StringCallback> onError) {
  if (get_clips_async_op_) {
    // The previous request hasn't finished yet. Cancel the request
    // and send an error message to JavaScript
    ClipArrayCallbackContainer* container =
        get_clips_async_op_->getContainer<ClipArrayCallbackContainer>();

    container->onError->scheduleCallback(context_, kErrorCancelPrevious);
  }

  // If not NULL this will disconnect the previous AsyncOpProxy
  get_clips_async_op_ = MakeAsyncOp(
      this,
      adoptPtr(new ClipArrayCallbackContainer(onSuccess, onError)),
      &DvrManager::handleGetClipsSuccess,
      &DvrManager::handleGetClipsError);

  h5vcc_impl_->GetClipsAsync(get_clips_async_op_->getProxy());
}

void DvrManager::checkPrivilege(unsigned int privilegeId,
                                bool attemptResolution,
                                const String& friendlyDisplay,
                                PassRefPtr<BoolCallback> onSuccess,
                                PassRefPtr<StringCallback> onError) {
  if (check_privilege_async_op_) {
    // The previous request hasn't finished yet. Cancel the request
    // and send an error message to JavaScript
    BoolCallbackContainer* container =
        check_privilege_async_op_->getContainer<BoolCallbackContainer>();

    container->onError->scheduleCallback(context_, kErrorCancelPrevious);
  }

  // If not NULL this will disconnect the previous AsyncOpProxy
  check_privilege_async_op_ = MakeAsyncOp(
      this,
      adoptPtr(new BoolCallbackContainer(onSuccess, onError)),
      &DvrManager::handleCheckPrivilegeSuccess,
      &DvrManager::handleCheckPrivilegeError);

  h5vcc_impl_->CheckPrivilegeAsync(check_privilege_async_op_->getProxy(),
                                   privilegeId, attemptResolution,
                                   friendlyDisplay);
}

void DvrManager::handleGetClipsSuccess(
    AsyncOpClipArrayType* async_op, AsyncOpClipArrayType::SuccessType clips) {
  ASSERT(async_op == get_clips_async_op_.get());

  ClipArrayCallbackContainer* container =
    async_op->getContainer<ClipArrayCallbackContainer>();

  context_->postTask(WebCore::createEventDispatcherTask(
      container->onSuccess, DvrClipArray::create(clips)));

  releaseReferences();
}

void DvrManager::handleGetClipsError(
    AsyncOpClipArrayType* async_op, AsyncOpClipArrayType::ErrorType reason) {
  ASSERT(async_op == get_clips_async_op_.get());

  ClipArrayCallbackContainer* container =
      async_op->getContainer<ClipArrayCallbackContainer>();

  context_->postTask(WebCore::createEventDispatcherTask(
      container->onError, reason));

  releaseReferences();
}

void DvrManager::handleCheckPrivilegeSuccess(
    AsyncOpBoolType* async_op, AsyncOpBoolType::SuccessType result) {
  ASSERT(async_op == check_privilege_async_op_.get());

  BoolCallbackContainer* container =
    async_op->getContainer<BoolCallbackContainer>();

  context_->postTask(WebCore::createEventDispatcherTask(
      container->onSuccess, result));

  releaseReferences();
}

void DvrManager::handleCheckPrivilegeError(
    AsyncOpBoolType* async_op, AsyncOpBoolType::ErrorType reason) {
  ASSERT(async_op == check_privilege_async_op_.get());

  BoolCallbackContainer* container =
      async_op->getContainer<BoolCallbackContainer>();

  context_->postTask(WebCore::createEventDispatcherTask(
      container->onError, reason));

  releaseReferences();
}

void DvrManager::releaseReferences() {
  get_clips_async_op_.clear();
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_DVR)
