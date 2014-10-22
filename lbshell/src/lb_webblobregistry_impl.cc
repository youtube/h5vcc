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
// webkit/tools/test_shell/test_shell_webblobregistry_impl.cc

#include "lb_webblobregistry_impl.h"

#include "base/bind.h"
#include "base/message_loop.h"
#include "googleurl/src/gurl.h"
#include "lb_resource_loader_bridge.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebBlobData.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebURL.h"
#include "webkit/base/file_path_string_conversions.h"
#include "webkit/blob/blob_data.h"
#include "webkit/blob/blob_storage_controller.h"

using WebKit::WebBlobData;
using WebKit::WebURL;
using webkit_blob::BlobData;

namespace {

std::string ToStdString(const WebKit::WebString& str) {
  WebKit::WebCString cstr = str.utf8();
  return std::string(cstr.data(), cstr.length());
}

// Creates a new BlobData from WebBlobData.
scoped_refptr<BlobData> NewBlobData(const WebBlobData& data) {
  scoped_refptr<BlobData> blob(new BlobData());
  WebBlobData::Item item;
  for (size_t i = 0; data.itemAt(i, item); ++i) {
    switch (item.type) {
      case WebBlobData::Item::TypeData:
        if (!item.data.isEmpty()) {
          // WebBlobData does not allow partial data.
          DCHECK(!item.offset && item.length == -1);
          blob->AppendData(item.data);
        }
        break;
      case WebBlobData::Item::TypeFile:
        if (item.length) {
          blob->AppendFile(
              webkit_base::WebStringToFilePath(item.filePath),
              static_cast<uint64>(item.offset),
              static_cast<uint64>(item.length),
              base::Time::FromDoubleT(item.expectedModificationTime));
        }
        break;
      case WebBlobData::Item::TypeBlob:
        if (item.length) {
          blob->AppendBlob(
              item.blobURL,
              static_cast<uint64>(item.offset),
              static_cast<uint64>(item.length));
        }
        break;
      default:
        NOTREACHED();
    }
  }
  blob->set_content_type(ToStdString(data.contentType()));
  blob->set_content_disposition(ToStdString(data.contentDisposition()));
  return blob;
}

}  // namespace

// static
LBWebBlobRegistryImpl* LBWebBlobRegistryImpl::s_self_;

// static
void LBWebBlobRegistryImpl::InitFromIOThread(
    webkit_blob::BlobStorageController* blob_storage_controller) {
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
  DCHECK(blob_storage_controller);

  s_self_->AddRef();
  s_self_->io_message_loop_ = LBResourceLoaderBridge::GetIoThread();
  s_self_->blob_storage_controller_ = blob_storage_controller;
}

// static
void LBWebBlobRegistryImpl::CleanUpFromIOThread() {
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
  s_self_->io_message_loop_ = NULL;
  s_self_->blob_storage_controller_ = NULL;
  s_self_->Release();
}

void LBWebBlobRegistryImpl::registerBlobURL(const WebURL& url,
                                            WebBlobData& data) {
  DCHECK(IsInitialized());
  GURL thread_safe_url = url;  // WebURL uses refcounted strings.
  io_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBWebBlobRegistryImpl::AddFinishedBlob, this,
      thread_safe_url, NewBlobData(data)));
}

void LBWebBlobRegistryImpl::registerBlobURL(const WebURL& url,
                                            const WebURL& src_url) {
  DCHECK(IsInitialized());
  GURL thread_safe_url = url;
  GURL thread_safe_src_url = src_url;
  io_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBWebBlobRegistryImpl::CloneBlob, this,
      thread_safe_url, thread_safe_src_url));
}

void LBWebBlobRegistryImpl::unregisterBlobURL(const WebURL& url) {
  DCHECK(IsInitialized());
  GURL thread_safe_url = url;
  io_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBWebBlobRegistryImpl::RemoveBlob, this,
      thread_safe_url));
}

LBWebBlobRegistryImpl::LBWebBlobRegistryImpl() {
  DCHECK(!s_self_);
  s_self_ = this;
  blob_storage_controller_ = NULL;
}

LBWebBlobRegistryImpl::~LBWebBlobRegistryImpl() {
  DCHECK(!IsInitialized());
  s_self_ = NULL;
}

bool LBWebBlobRegistryImpl::IsInitialized() const {
  return blob_storage_controller_ && io_message_loop_;
}

void LBWebBlobRegistryImpl::AddFinishedBlob(
    const GURL& url, BlobData* blob_data) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  blob_storage_controller_->AddFinishedBlob(url, blob_data);
}

void LBWebBlobRegistryImpl::CloneBlob(const GURL& url, const GURL& src_url) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  blob_storage_controller_->CloneBlob(url, src_url);
}

void LBWebBlobRegistryImpl::RemoveBlob(const GURL& url) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  blob_storage_controller_->RemoveBlob(url);
}
