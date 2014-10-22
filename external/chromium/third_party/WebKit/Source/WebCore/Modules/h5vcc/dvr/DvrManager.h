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

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRMANAGER_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRMANAGER_H_

#include "config.h"

#include <wtf/RefCounted.h>
#include <wtf/PassRefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/text/WTFString.h>

#include "../common/AsyncOp.h"
#include "ScriptExecutionContext.h"

namespace WebCore {
class DvrClipArrayCallback;
class DvrManager;
class BoolCallback;
class StringCallback;
}  // namespace WebCore

namespace H5vcc {

// All resource that will be created by the H5vcc layer are reference
// counted to allow WebKit to hold a reference to a resource as long
// as it's needed. References will be released as soon as the WebKit
// objects are garbage collected or explicitly released. We are using
// ThreadSafeRefCounted here since a resource can be used on different
// threads.
class DvrResource : public WTF::ThreadSafeRefCounted<DvrResource> {
 protected:
  DvrResource() {}
  virtual ~DvrResource() {}

  friend class WTF::ThreadSafeRefCounted<DvrResource>;
};

// Clip metadata interface
class DvrClip : public DvrResource {
 public:
  // Unique clip ID
  virtual const WTF::String& clip_id() const = 0;

  // Clip name
  virtual const WTF::String& clip_name() const = 0;

  // Clip default visibility [Default, Public, Owner, Title]
  virtual const WTF::String& clip_visibility() const = 0;

  // Recording date using a JavaScript timestamp defined by number of
  // milliseconds since 1 January 1970 00:00:00 UTC.
  virtual double date_recorded() const = 0;

  // Duration of the clip in seconds
  virtual unsigned int duration_sec() const = 0;

  // True is a local copy is not available
  virtual bool is_remote() const = 0;

  // Game title name
  virtual const WTF::String& title_name() const = 0;

  // Candidates for removal in the final version
  virtual const WTF::String& user_caption() const = 0;
  virtual const WTF::String& title_data() const = 0;
};

// Lightweight file handle
class DvrFile : public DvrResource {
 public:
  virtual const WTF::String& path() const = 0;
};

// This object will be used by the H5vcc layer to notify WebKit when an
// asynchronous clip enumeration operation is done
typedef WTF::Vector<RefPtr<DvrClip> > DvrClipArray;
typedef WebCore::AsyncOpProxy<const DvrClipArray&> AsyncOpProxyClipArray;

// This object will be used by the H5vcc layer to notify WebKit when an
// asynchronous file operation is done
typedef WebCore::AsyncOpProxy<PassRefPtr<DvrFile> > AsyncOpProxyFile;

// This object will be used by the H5vcc layer to notify WebKit when an
// asynchronous operation that returns a boolean is done
typedef WebCore::AsyncOpProxy<bool> AsyncOpProxyBool;

// Clip manager interface
class DvrManager {
 public:
  virtual ~DvrManager() {}

  // Retrieve a list of available dvr clips
  virtual void GetClipsAsync(PassRefPtr<AsyncOpProxyClipArray> async) = 0;

  // Retrieve a file path for the given clip
  virtual void GetFileAsync(PassRefPtr<AsyncOpProxyFile> async,
                            const String& clipId) = 0;

  // Retrieve a thumbnail file path for the given clip
  virtual void GetThumbnailAsync(PassRefPtr<AsyncOpProxyFile> async,
                                 const String& clipId) = 0;

  // Check if the primary user has the given privilege
  // privilegeId - platform specific privilege ID
  // attemptResolution - will instruct the system to attempt to resolve
  //   the problem by showing a message to the user
  // friendlyDisplay - if not null, will customize the user message shown
  //   if attemptResolution is set to true
  virtual void CheckPrivilegeAsync(PassRefPtr<AsyncOpProxyBool> async,
                                   unsigned int privilegeId,
                                   bool attemptResolution,
                                   const String& friendlyDisplay) = 0;

  // Called by WebKit whenever a new instance of WebCore::DvrManager
  // is created. Can be called before a previous instance was garbage collected.
  virtual void AddWebKitInstance(WebCore::DvrManager* inst) = 0;

  // Called by WebKit whenever a new instance of WebCore::DvrManager
  // is removed. Will be called after the instance was garbage collected.
  virtual void RemoveWebKitInstance(WebCore::DvrManager* inst) = 0;
};

}  // namespace H5vcc

#if ENABLE(LB_SHELL_DVR)

namespace WebCore {

class DvrManager : public RefCounted<DvrManager> {
 public:
  virtual ~DvrManager();

  static PassRefPtr<DvrManager> create(ScriptExecutionContext* context);

  // IDL methods
  void getClips(PassRefPtr<DvrClipArrayCallback> onSuccess,
                PassRefPtr<StringCallback> onError);

  void checkPrivilege(unsigned int privelegeId,
                      bool attemptResolution,
                      const String& friendlyDisplay,
                      PassRefPtr<BoolCallback> onSuccess,
                      PassRefPtr<StringCallback> onError);

  // Call to release all references to WebKit objects that may be preventing
  // this class from being garbage collected.
  WEBKIT_EXPORT void releaseReferences();

 private:
  struct ClipArrayCallbackContainer;
  struct BoolCallbackContainer;

  typedef H5vcc::AsyncOpProxyClipArray::AsyncOpType AsyncOpClipArrayType;
  typedef H5vcc::AsyncOpProxyBool::AsyncOpType AsyncOpBoolType;

  explicit DvrManager(ScriptExecutionContext* context);

  void handleGetClipsSuccess(AsyncOpClipArrayType* async_op,
                             AsyncOpClipArrayType::SuccessType clip_array);
  void handleGetClipsError(AsyncOpClipArrayType* async_op,
                           AsyncOpClipArrayType::ErrorType reason);

  void handleCheckPrivilegeSuccess(AsyncOpBoolType* async_op,
                                   AsyncOpBoolType::SuccessType result);
  void handleCheckPrivilegeError(AsyncOpBoolType* async_op,
                                 AsyncOpBoolType::ErrorType reason);

  OwnPtr<AsyncOpClipArrayType> get_clips_async_op_;
  OwnPtr<AsyncOpBoolType> check_privilege_async_op_;

  H5vcc::DvrManager* h5vcc_impl_;
  ScriptExecutionContext* context_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_DVR)

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRMANAGER_H_
