/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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

// This file is based on:
// webkit/tools/test_shell/test_shell_webblobregistry_impl.h

#ifndef SRC_LB_WEBBLOBREGISTRY_IMPL_H_
#define SRC_LB_WEBBLOBREGISTRY_IMPL_H_

#include "base/memory/ref_counted.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebBlobRegistry.h"

class GURL;

namespace base {
class MessageLoopProxy;
}

namespace webkit_blob {
class BlobData;
class BlobStorageController;
}  // namespace webkit_blob

class LBWebBlobRegistryImpl
    : public WebKit::WebBlobRegistry
    , public base::RefCountedThreadSafe<LBWebBlobRegistryImpl> {
 public:
  // Must be called from the IO thread Init method
  static void InitFromIOThread(
      webkit_blob::BlobStorageController* blob_storage_controller);
  // Must be called from the IO thread CleanUp method
  static void CleanUpFromIOThread();

  LBWebBlobRegistryImpl();

  // See WebBlobRegistry.h for documentation on these functions.
  virtual void registerBlobURL(const WebKit::WebURL& url,
                               WebKit::WebBlobData& data);
  virtual void registerBlobURL(const WebKit::WebURL& url,
                               const WebKit::WebURL& src_url);
  virtual void unregisterBlobURL(const WebKit::WebURL& url);

 protected:
  virtual ~LBWebBlobRegistryImpl();

 private:
  friend class base::RefCountedThreadSafe<LBWebBlobRegistryImpl>;

  bool IsInitialized() const;

  // Run on I/O thread.
  void AddFinishedBlob(const GURL& url, webkit_blob::BlobData* blob_data);
  void CloneBlob(const GURL& url, const GURL& src_url);
  void RemoveBlob(const GURL& url);

  static LBWebBlobRegistryImpl* s_self_;

  webkit_blob::BlobStorageController* blob_storage_controller_;
  scoped_refptr<base::MessageLoopProxy> io_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(LBWebBlobRegistryImpl);
};

#endif  // SRC_LB_WEBBLOBREGISTRY_IMPL_H_
