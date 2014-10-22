/*
 * Copyright 2013 Google Inc. All Rights Reserved.
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
#include "lb_storage_cleanup.h"

#if !defined(__LB_SHELL__FOR_RELEASE__)

#include <algorithm>
#include <string>
#include <vector>

#include "external/chromium/base/file_util.h"

#include "lb_globals.h"

using file_util::FileEnumerator;

namespace LB {

namespace {

// Maximum number of newest log files that will be kept
const size_t kNumLogsToKeep = 10;

// Maximum number of newest tracing files that will be kept
const size_t kNumTracesToKeep = 5;

// Maximum number of crash dump files that will be kept
const size_t kNumDumpsToKeep = 5;

typedef std::pair<base::Time, FilePath> FileEntry;

// Used to sort FileEntries by time (newer first)
struct FileEntryCompare {
  bool operator()(const FileEntry& e1, const FileEntry& e2) const {
    return e1.first > e2.first;
  }
};

void CleanFiles(const char* path, const char* ext, size_t max_files_to_keep,
                std::vector<FileEntry>* cleaned_files) {
  std::vector<FileEntry> files;

  FileEnumerator iter(FilePath(path), false, FileEnumerator::FILES);
  for (FilePath file = iter.Next(); !file.empty(); file = iter.Next()) {
    if (ext && !file.MatchesExtension(ext))
      continue;

    FileEnumerator::FindInfo info;
    iter.GetFindInfo(&info);
    files.push_back(std::make_pair(iter.GetLastModifiedTime(info), file));
  }

  if (files.size() <= max_files_to_keep)
    return;

  // Sort by creation time, newer come first
  std::sort(files.begin(), files.end(), FileEntryCompare());

  for (std::vector<FileEntry>::iterator it = files.begin() + max_files_to_keep;
       it != files.end(); ++it) {
    FilePath path = it->second;
    file_util::Delete(path, false);
  }

  if (cleaned_files) {
    cleaned_files->insert(cleaned_files->end(),
                          files.begin() + max_files_to_keep, files.end());
  }
}

}  // namespace

StorageCleanupThread::StorageCleanupThread()
    : SimpleThread("LB::StorageCleanupThread") {
}

StorageCleanupThread::~StorageCleanupThread() {
  Join();
}

void StorageCleanupThread::Run() {
  const char* logging_path = GetGlobalsPtr()->logging_output_path;

  // Clean log files
  CleanFiles(logging_path, ".log", kNumLogsToKeep, NULL);

  // Clean tracing files
  CleanFiles(logging_path, ".json", kNumTracesToKeep, NULL);

  // Clean crash dump files
  std::vector<FileEntry> cleaned_files;
  CleanFiles(logging_path, ".dmp", kNumDumpsToKeep, &cleaned_files);

  // Crash dumps have accompanying text files so we clean them too
  for (size_t i = 0; i < cleaned_files.size(); ++i) {
    const FileEntry& entry = cleaned_files[i];

    FilePath path = entry.second.ReplaceExtension(".txt");
    if (file_util::PathExists(path)) {
      file_util::Delete(path, false);
    }
  }
}

}  // namespace LB

#endif  // !defined(__LB_SHELL__FOR_RELEASE__)
