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

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRCLIPCONTEXT_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRCLIPCONTEXT_H_

#include "config.h"
#if ENABLE(LB_SHELL_DVR)

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

#include "DvrManager.h"

namespace WebCore {
class DvrFileCallback;
class DvrProgressCallback;
class ScriptExecutionContext;
class StringCallback;

class DvrClipContext : public RefCounted<DvrClipContext> {
 public:
  virtual ~DvrClipContext();

  static PassRefPtr<DvrClipContext> create(ScriptExecutionContext* context,
                                           const String& clip_id);

  // IDL methods
  void getFile(PassRefPtr<DvrFileCallback> onDone,
               PassRefPtr<StringCallback> onError,
               PassRefPtr<DvrProgressCallback> onProgress);

  void getThumbnail(PassRefPtr<DvrFileCallback> onDone,
                    PassRefPtr<StringCallback> onError);

  void release();

 private:
  struct FileCallbackContainer;
  struct ThumbnailCallbackContainer;
  typedef H5vcc::AsyncOpProxyFile::AsyncOpType AsyncOpType;

  DvrClipContext(ScriptExecutionContext* context, const String& clip_id);

  void handleGetFileSuccess(AsyncOpType* async_op,
                            AsyncOpType::SuccessType file);
  void handleGetFileError(AsyncOpType* async_op, AsyncOpType::ErrorType reason);
  void handleGetFileProgress(AsyncOpType* async_op,
                             unsigned current, unsigned total);

  void handleGetThumbnailSuccess(AsyncOpType* async_op,
                                 AsyncOpType::SuccessType file);
  void handleGetThumbnailError(AsyncOpType* async_op,
                               AsyncOpType::ErrorType reason);

  void clearGetFile();
  void clearGetThumbnail();

  RefPtr<H5vcc::DvrFile> file_;
  OwnPtr<AsyncOpType> file_async_op_;

  RefPtr<H5vcc::DvrFile> thumbnail_;
  OwnPtr<AsyncOpType> thumbnail_async_op_;

  ScriptExecutionContext* context_;
  const String clip_id_;

  // Context was released and can no longer be used
  bool released_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_DVR)

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRCLIPCONTEXT_H_
