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
#include "DvrClipContext.h"

#if ENABLE(LB_SHELL_DVR)

#include <public/Platform.h>

#include "AsyncOp.h"
#include "CrossThreadEventDispatcher.h"
#include "DvrFileCallback.h"
#include "DvrProgressCallback.h"
#include "File.h"
#include "ScriptExecutionContext.h"
#include "StringCallback.h"

namespace WebCore {

// Allow the following types to be used with CrossThreadEventDispatcher
// Normally only ThreadSafeRefCounted objects can be used with the dispatched
// however our code is guaranteed to run on a single thread
CROSS_THREAD_ALLOW_REFCOUNTED(File);
CROSS_THREAD_ALLOW_REFCOUNTED(StringCallback);

namespace {
// An outstanding operation is canceled because a newer operation was
// requested by JavaScript.
const char* kErrorCancelPrevious = "Canceled due to a newer operation";
// The context is no longer valid since it was already released
const char* kErrorContextReleased = "Context was already released";
}  // namespace

struct DvrClipContext::FileCallbackContainer {
  FileCallbackContainer(PassRefPtr<DvrFileCallback> onSuccess,
                        PassRefPtr<StringCallback> onError,
                        PassRefPtr<DvrProgressCallback> onProgress)
      : onSuccess(onSuccess)
      , onError(onError)
      , onProgress(onProgress) {}

  RefPtr<DvrFileCallback> onSuccess;
  RefPtr<StringCallback> onError;
  RefPtr<DvrProgressCallback> onProgress;
};

struct DvrClipContext::ThumbnailCallbackContainer {
  ThumbnailCallbackContainer(PassRefPtr<DvrFileCallback> onSuccess,
                             PassRefPtr<StringCallback> onError)
      : onSuccess(onSuccess)
      , onError(onError) {}

  RefPtr<DvrFileCallback> onSuccess;
  RefPtr<StringCallback> onError;
};

// static
PassRefPtr<DvrClipContext> DvrClipContext::create(
    ScriptExecutionContext* context, const String& clip_id) {
  return adoptRef(new DvrClipContext(context, clip_id));
}

DvrClipContext::~DvrClipContext() {
  release();
}

DvrClipContext::DvrClipContext(ScriptExecutionContext* context,
                               const String& clip_id)
    : context_(context)
    , clip_id_(clip_id)
    , released_(false) {}

void DvrClipContext::getFile(PassRefPtr<DvrFileCallback> onSuccess,
                             PassRefPtr<StringCallback> onError,
                             PassRefPtr<DvrProgressCallback> onProgress) {
  if (released_) {
    onError->scheduleCallback(context_, kErrorContextReleased);
    return;
  }

  if (file_async_op_) {
    // The previous request hasn't finished yet; send an error message.
    FileCallbackContainer* container =
        file_async_op_->getContainer<FileCallbackContainer>();

    container->onError->scheduleCallback(
        context_, kErrorCancelPrevious);
  }

  // If not NULL this will disconnect the previous AsyncOpProxy
  file_async_op_ = MakeAsyncOp(
      this,
      adoptPtr(new FileCallbackContainer(onSuccess, onError, onProgress)),
      &DvrClipContext::handleGetFileSuccess,
      &DvrClipContext::handleGetFileError,
      &DvrClipContext::handleGetFileProgress);

  H5vcc::DvrManager* manager = WebKit::Platform::current()->h5vccDvrManager();
  ASSERT(manager);

  manager->GetFileAsync(file_async_op_->getProxy(), clip_id_);
}

void DvrClipContext::getThumbnail(
    PassRefPtr<DvrFileCallback> onSuccess,
    PassRefPtr<StringCallback> onError) {
  if (released_) {
    onError->scheduleCallback(context_, kErrorContextReleased);
    return;
  }

  if (thumbnail_async_op_) {
    // The previous request hasn't finished yet; send an error message.
    ThumbnailCallbackContainer* container =
        thumbnail_async_op_->getContainer<ThumbnailCallbackContainer>();

    container->onError->scheduleCallback(
        context_, kErrorCancelPrevious);
  }

  // If not NULL this will disconnect the previous AsyncOpProxy
  thumbnail_async_op_ = MakeAsyncOp(
      this,
      adoptPtr(new ThumbnailCallbackContainer(onSuccess, onError)),
      &DvrClipContext::handleGetThumbnailSuccess,
      &DvrClipContext::handleGetThumbnailError);

  H5vcc::DvrManager* manager = WebKit::Platform::current()->h5vccDvrManager();
  ASSERT(manager);

  manager->GetThumbnailAsync(thumbnail_async_op_->getProxy(), clip_id_);
}

void DvrClipContext::handleGetFileSuccess(AsyncOpType* async_op,
                                          AsyncOpType::SuccessType file) {
  if (released_) {
    ASSERT_NOT_REACHED();
    return;
  }

  ASSERT(async_op == file_async_op_.get());

  // Hold a reference to the file until the context is released
  file_ = file;

  FileCallbackContainer* container =
      async_op->getContainer<FileCallbackContainer>();

  context_->postTask(WebCore::createEventDispatcherTask(
      container->onSuccess, File::create(file_->path())));

  clearGetFile();
}

void DvrClipContext::handleGetFileError(AsyncOpType* async_op,
                                        AsyncOpType::ErrorType reason) {
  if (released_) {
    ASSERT_NOT_REACHED();
    return;
  }

  ASSERT(async_op == file_async_op_.get());

  FileCallbackContainer* container =
      async_op->getContainer<FileCallbackContainer>();

  context_->postTask(WebCore::createEventDispatcherTask(
      container->onError, reason));

  clearGetFile();
}

void DvrClipContext::handleGetFileProgress(AsyncOpType* async_op,
                                           unsigned current, unsigned total) {
  if (released_) {
    ASSERT_NOT_REACHED();
    return;
  }

  ASSERT(async_op == file_async_op_.get());

  FileCallbackContainer* container =
      async_op->getContainer<FileCallbackContainer>();

  if (container->onProgress) {
    context_->postTask(WebCore::createEventDispatcherTask(
        container->onProgress, current, total));
  }
}

void DvrClipContext::handleGetThumbnailSuccess(AsyncOpType* async_op,
                                               AsyncOpType::SuccessType file) {
  if (released_) {
    ASSERT_NOT_REACHED();
    return;
  }

  ASSERT(async_op == thumbnail_async_op_.get());

  // Hold a reference to the file until the context is released
  thumbnail_ = file;

  ThumbnailCallbackContainer* container =
      async_op->getContainer<ThumbnailCallbackContainer>();

  context_->postTask(WebCore::createEventDispatcherTask(
      container->onSuccess, File::create(thumbnail_->path())));

  clearGetThumbnail();
}

void DvrClipContext::handleGetThumbnailError(AsyncOpType* async_op,
                                             AsyncOpType::ErrorType reason) {
  if (released_) {
    ASSERT_NOT_REACHED();
    return;
  }

  ASSERT(async_op == thumbnail_async_op_.get());

  ThumbnailCallbackContainer* container =
      async_op->getContainer<ThumbnailCallbackContainer>();

  context_->postTask(WebCore::createEventDispatcherTask(
      container->onError, reason));

  clearGetThumbnail();
}

void DvrClipContext::clearGetFile() {
  file_async_op_.clear();
}

void DvrClipContext::clearGetThumbnail() {
  thumbnail_async_op_.clear();
}

void DvrClipContext::release() {
  if (!released_) {
    // Release all resource references
    file_ = NULL;
    thumbnail_ = NULL;

    clearGetFile();
    clearGetThumbnail();

    released_ = true;
  }
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_DVR)
