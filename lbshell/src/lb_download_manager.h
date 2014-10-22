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

#ifndef SRC_LB_DOWNLOAD_MANAGER_H_
#define SRC_LB_DOWNLOAD_MANAGER_H_

#include "base/callback.h"
#include "base/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "googleurl/src/gurl.h"

namespace LB {
class DownloadItem;

class DownloadManager
    : public base::SupportsWeakPtr<DownloadManager> {
 public:
  typedef int Error;

  struct ProgressInfo {
    uint64 bytes_downloaded;
    uint64 bytes_total;
  };

  typedef base::Callback<void(FilePath)> SuccessCallback;
  typedef base::Callback<void(ProgressInfo)> ProgressCallback;
  typedef base::Callback<void(Error)> ErrorCallback;

  struct Request {
    // Source URL
    GURL url;
    // Destination file path
    FilePath destination;
    // Doesn't download a new file if a file already exists at the given
    // destination. Please note that the manager will not check if the existing
    // file is different from the file at the given url however it will not
    // use a partially downloaded file
    bool use_existing;
    // Called after the file was successfully downloaded, guaranteed to be
    // called on the thread that initiated the download
    SuccessCallback success_cb;
    // Called in case of an error, guaranteed to be called on the thread that
    // initiated the download
    ErrorCallback error_cb;
    // Optional progress callback, will be called from a worker thread
    ProgressCallback progress_cb;
  };

  void StartDownloadAsync(const Request& request);
  static const char* ErrorToString(Error error);

 private:
  void DownloadComplete(scoped_ptr<DownloadItem> item, FilePath temp_path,
                        int result);
};

}  // namespace LB

#endif  // SRC_LB_DOWNLOAD_MANAGER_H_
