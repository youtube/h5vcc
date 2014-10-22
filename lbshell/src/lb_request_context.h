/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// This file is a fork of:
// external/chromium/webkit/tools/test_shell/test_shell_request_context.h

#ifndef SRC_LB_REQUEST_CONTEXT_H_
#define SRC_LB_REQUEST_CONTEXT_H_

#include "base/threading/thread.h"
#include "net/cookies/cookie_monster.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_storage.h"

namespace webkit_blob {
class BlobStorageController;
}

// A basic net::URLRequestContext that only provides an in-memory cookie store.
class LBRequestContext : public net::URLRequestContext {
 public:
  // Use an in-memory cache
  LBRequestContext();

  virtual ~LBRequestContext();

  // Use an on-disk cache at the specified location.  Optionally, use the cache
  // in playback or record mode.
  LBRequestContext(net::CookieMonster::PersistentCookieStore*
                      persistent_cookie_store,
                   bool no_proxy);

  webkit_blob::BlobStorageController* blob_storage_controller() const {
    return blob_storage_controller_.get();
  }

  // Delete and re-initialize the net::CookieMonster. This has the effect of
  // clearing all cookies in memory, but has no effect on cookies already
  // flushed to storage.
  void ResetCookieMonster();

 private:
  void Init(net::CookieMonster::PersistentCookieStore *persistent_cookie_store,
            bool no_proxy);

  net::URLRequestContextStorage storage_;
  scoped_ptr<webkit_blob::BlobStorageController> blob_storage_controller_;
  scoped_refptr<net::CookieMonster::PersistentCookieStore>
      persistent_cookie_store_;
};

#endif  // SRC_LB_REQUEST_CONTEXT_H_
