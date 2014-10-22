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

#include "lb_download_manager.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/platform_file.h"
#include "base/time.h"
#include "media/base/bind_to_loop.h"
#include "net/base/file_stream.h"
#include "net/http/http_response_headers.h"
#include "net/http/shell/http_stream_shell.h"

#include "lb_resource_loader_bridge.h"

namespace LB {
namespace {
// Defines a minimal time interval that has to pass between two consecutive
// progress updates
const int64 kProgressUpdateIntervalMilliseconds = 33;
}  // namespace

class DownloadItem {
 public:
  typedef base::Callback<void(int)> CompleteCallback;

  DownloadItem(const DownloadManager::Request& request,
               scoped_ptr<net::FileStream> file_stream);

  void StartDownload(const CompleteCallback& complete_cb);

  const DownloadManager::Request& download_request() const {
    return download_request_;
  }

 private:
  enum RequestState {
    kInvalid,
    kSendRequest,
    kReadResponseHeaders,
    kReadResponseBody,
    kAppendResponse,
    kDone,
  };

  // completion_status is typically a byte count or a network error code
  // See net::CompletionCallback
  void TaskCompleteCallback(int completion_status);
  void UpdateProgress();

  void CreateStreamTask();
  void SendRequestTask();
  void ReadResponseHeadersTask();
  void ReadResponseBodyTask();
  void RequestComplete(int completion_status);

  DownloadManager::Request download_request_;

  net::HttpRequestInfo request_info_;
  net::HttpResponseInfo response_info_;

  scoped_ptr<net::HttpStreamShell> http_stream_;
  scoped_refptr<net::IOBufferWithSize> io_buffer_;

  RequestState next_state_;

  scoped_refptr<base::MessageLoopProxy> message_loop_proxy_;
  CompleteCallback complete_cb_;

  uint64 bytes_downloaded_;
  base::Time last_progress_update_;

  scoped_ptr<net::FileStream> file_stream_;
};

DownloadItem::DownloadItem(const DownloadManager::Request& request,
                           scoped_ptr<net::FileStream> file_stream)
    : download_request_(request)
    , next_state_(kInvalid)
    , bytes_downloaded_(0)
    , file_stream_(file_stream.Pass()) {
  message_loop_proxy_ = LBResourceLoaderBridge::GetIoThread();
  DCHECK(message_loop_proxy_);
}

void DownloadItem::StartDownload(const CompleteCallback& complete_cb) {
  complete_cb_ = complete_cb;

  // This buffer will be used to hold partial download results before
  // writing them to a file
  const int kResponseBufferSize = 100 * 1024;
  io_buffer_ = new net::IOBufferWithSize(kResponseBufferSize);

  request_info_.url = download_request_.url;
  request_info_.method = "GET";

  next_state_ = kSendRequest;
  message_loop_proxy_->PostTask(
      FROM_HERE,
      base::Bind(
          &DownloadItem::CreateStreamTask,
          base::Unretained(this)));
}

void DownloadItem::CreateStreamTask() {
  http_stream_.reset(
      new net::HttpStreamShell(net::CreateHttpStreamShellLoader()));

  int result = http_stream_->InitializeStream(
      &request_info_,
      net::BoundNetLog(),
      base::Bind(&DownloadItem::TaskCompleteCallback,
                 base::Unretained(this)));

  if (result != net::ERR_IO_PENDING) {
    TaskCompleteCallback(result);
  };
}

void DownloadItem::SendRequestTask() {
  int result = http_stream_->SendRequest(
      net::HttpRequestHeaders(),
      &response_info_,
      base::Bind(&DownloadItem::TaskCompleteCallback,
                 base::Unretained(this)));

  if (result != net::ERR_IO_PENDING) {
    TaskCompleteCallback(result);
  };
}

void DownloadItem::ReadResponseHeadersTask() {
  int result = http_stream_->ReadResponseHeaders(
      base::Bind(&DownloadItem::TaskCompleteCallback,
                 base::Unretained(this)));

  if (result != net::ERR_IO_PENDING) {
    TaskCompleteCallback(result);
  };
}

void DownloadItem::ReadResponseBodyTask() {
  int result = http_stream_->ReadResponseBody(
      io_buffer_,
      io_buffer_->size(),
      base::Bind(&DownloadItem::TaskCompleteCallback,
                 base::Unretained(this)));

  if (result != net::ERR_IO_PENDING) {
    TaskCompleteCallback(result);
  };
}

void DownloadItem::TaskCompleteCallback(int completion_status) {
  if (completion_status < 0) {
    // Got an error, abort operation
    RequestComplete(completion_status);
    return;
  }

  base::Closure next_task;
  // Proceed to next state
  switch (next_state_) {
    case kSendRequest:
      next_task = base::Bind(&DownloadItem::SendRequestTask,
                             base::Unretained(this));
      next_state_ = kReadResponseHeaders;
      break;
    case kReadResponseHeaders:
      next_task = base::Bind(&DownloadItem::ReadResponseHeadersTask,
                             base::Unretained(this));
      next_state_ = kReadResponseBody;
      break;
    case kReadResponseBody:
      next_task = base::Bind(&DownloadItem::ReadResponseBodyTask,
                             base::Unretained(this));
      next_state_ = kAppendResponse;
      break;
    case kAppendResponse:
      // In this case, completion_status is the number of bytes read
      if (completion_status > 0) {
        file_stream_->WriteSync(io_buffer_->data(), completion_status);
        bytes_downloaded_ += completion_status;

        UpdateProgress();
      }
      if (http_stream_->IsResponseBodyComplete()) {
        RequestComplete(net::OK);
      } else {
        // Read some more
        next_task = base::Bind(&DownloadItem::ReadResponseBodyTask,
                               base::Unretained(this));
      }
      break;
    default:
      NOTREACHED();
  }

  if (!next_task.is_null())
    message_loop_proxy_->PostTask(FROM_HERE, next_task);
}

void DownloadItem::UpdateProgress() {
  if (download_request_.progress_cb.is_null()) {
    return;
  }

  const base::Time now = base::Time::Now();
  const base::TimeDelta interval =
      base::TimeDelta::FromMilliseconds(kProgressUpdateIntervalMilliseconds);

  if (now - last_progress_update_ < interval) {
    return;
  }

  last_progress_update_ = now;

  DownloadManager::ProgressInfo progress;
  progress.bytes_downloaded = bytes_downloaded_;
  progress.bytes_total = response_info_.headers->GetContentLength();

  download_request_.progress_cb.Run(progress);
}

void DownloadItem::RequestComplete(int completion_status) {
  next_state_ = kDone;

  if (http_stream_) {
    http_stream_->Close(true /* not_reusable */);
    http_stream_.reset();
  }
  if (file_stream_) {
    // Close the stream before calling the completion callback to make sure
    // that the underlying file gets closed
    file_stream_.reset();
  }

  complete_cb_.Run(completion_status);
}

// static
const char* DownloadManager::ErrorToString(Error error) {
  return net::ErrorToString(error);
}

void DownloadManager::StartDownloadAsync(const Request& request) {
  DCHECK(!request.success_cb.is_null());
  DCHECK(!request.error_cb.is_null());

  if (file_util::PathExists(request.destination)) {
    if (request.use_existing) {
      request.success_cb.Run(request.destination);
      return;
    }

    // Delete the file before we start to prevent old or partial files
    // from being read in case of an error during the download process.
    file_util::Delete(request.destination, false /* recursive */);
  }

  if (!file_util::CreateDirectory(request.destination.DirName())) {
    request.error_cb.Run(net::ERR_FAILED);
    return;
  }

  // The file will be downloaded to a temp file first and then renamed to
  // the actual destination file to prevent partially downloaded files from
  // being accessible by the user in case of an error.
  FilePath temp_path;
  if (!file_util::CreateTemporaryFile(&temp_path)) {
    request.error_cb.Run(net::ERR_FAILED);
    return;
  }

  scoped_ptr<net::FileStream> file_stream =
      make_scoped_ptr(new net::FileStream(NULL /* netlog*/));

  int flags = base::PLATFORM_FILE_CREATE_ALWAYS |
              base::PLATFORM_FILE_WRITE;

  int error = file_stream->OpenSync(temp_path, flags);
  if (error != base::PLATFORM_FILE_OK) {
    request.error_cb.Run(net::ERR_FAILED);
    return;
  }

  DownloadItem* download_item = new DownloadItem(request, file_stream.Pass());

  // The callback is going to maintain the lifetime of DownloadItem
  // by holding a scoped_ptr to download_item. As soon as the callback
  // invocation is done, DownloadItem will be destroyed.
  //
  // DownloadComplete is bound to the DownloadManager via a weak pointer.
  // This is done support cases where a DownloadItem still exists after
  // DownloadManager was already destroyed. The callback will be executed
  // on the thread that is used the DownloadManager to make sure that
  // DownloadManager, as well as the completion callbacks, are in a valid
  // state.
  DownloadItem::CompleteCallback complete_cb = media::BindToLoop(
      base::MessageLoopProxy::current(),
      base::Bind(&DownloadManager::DownloadComplete,
                 AsWeakPtr(),
                 base::Passed(make_scoped_ptr(download_item)),
                 temp_path));

  // The completion callback will own the download item
  download_item->StartDownload(complete_cb);
}

void DownloadManager::DownloadComplete(scoped_ptr<DownloadItem> item,
                                       FilePath temp_path,
                                       int result) {
  const Request& request = item->download_request();

  if (!file_util::Move(temp_path, request.destination)) {
    request.error_cb.Run(net::ERR_FAILED);
  }

  if (result == net::OK) {
    request.success_cb.Run(request.destination);
  } else {
    request.error_cb.Run(result);
  }
}

}  // namespace LB
