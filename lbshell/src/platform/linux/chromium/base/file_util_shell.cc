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
// Most functions in this file are similar to the posix version. libc
// file API is used since PlatformFile API misses some necessary functions.

#include "base/file_util.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>

#include "base/logging.h"
#include "base/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "base/time.h"
#include "lb_globals.h"

namespace file_util {

void GetRandomString(int output_size, char *output) {
  snprintf(output, output_size, "%x%x%x%x",
    static_cast<unsigned int>(time(NULL)),
    static_cast<unsigned int>(clock()),
    rand(),
    rand());
}

bool CreateTemporaryFile(FilePath* path) {
  base::ThreadRestrictions::AssertIOAllowed();

  FilePath temp_file;

  if (!GetTempDir(path))
    return false;

  if (CreateTemporaryFileInDir(*path, &temp_file)) {
    *path = temp_file;
    return true;
  }

  return false;
}

FILE* CreateAndOpenTemporaryFileInDir(const FilePath& dir, FilePath* path) {
  base::ThreadRestrictions::AssertIOAllowed();

  for (int i = 0; i < 3; i++) {
    // Try create a new temporary file with random generated name. If
    // the one exists, keep trying another filename until we reach some limit.
    char random_string[48];
    GetRandomString(sizeof(random_string) - 4, random_string);
    strncat(random_string, ".tmp",
      sizeof(random_string) - strlen(random_string) - 1);

    FilePath new_path = dir.Append(random_string);

    // Make sure file doesn't already exist
    struct stat file_info;
    if (stat(new_path.value().c_str(), &file_info) == 0)
      continue;

    FILE *file = fopen(new_path.value().c_str(), "w+");
    if (file) {
      *path = new_path;
      return file;
    }
  }
  return NULL;
}

bool CreateTemporaryFileInDir(const FilePath& dir,
                              FilePath* temp_file) {
  FILE *file = CreateAndOpenTemporaryFileInDir(dir, temp_file);
  if (file) {
    fclose(file);
    return true;
  }
  return false;
}

bool GetFileInfo(const FilePath& file_path, base::PlatformFileInfo* results) {
  DCHECK(results);
  struct stat _stat;
  int rv = stat(file_path.value().c_str(), &_stat);
  if (rv == 0) {
    results->creation_time = base::Time::FromTimeT(_stat.st_ctime);
    results->is_directory = _stat.st_mode & S_IFDIR;
    results->is_symbolic_link = _stat.st_mode & S_IFLNK;
    results->last_accessed = base::Time::FromTimeT(_stat.st_atime);
    results->last_modified = base::Time::FromTimeT(_stat.st_mtime);
    results->size = _stat.st_size;
    return true;
  }
  return false;
}

bool GetTempDir(FilePath* path) {
  *path = FilePath(GetGlobalsPtr()->tmp_path).Append("tmp");

  // Ensure path exists
  int result = mkdir(path->value().c_str(), 0700);
  return (result == 0) || (errno == EEXIST);
}

bool GetShmemTempDir(FilePath* path, bool executable) {
  return GetTempDir(path);
}

bool CreateNewTempDirectory(const FilePath::StringType& prefix,
                            FilePath* new_temp_path) {
  FilePath tmpdir;
  if (!GetTempDir(&tmpdir))
    return false;

  return CreateTemporaryDirInDir(tmpdir, prefix, new_temp_path);
}

bool CreateTemporaryDirInDir(const FilePath& base_dir,
                             const FilePath::StringType& prefix,
                             FilePath* new_dir) {
  // based on version from file_util_win.cc
  base::ThreadRestrictions::AssertIOAllowed();

  FilePath path_to_create;

  for (int count = 0; count < 3; ++count) {
    // Try create a new temporary directory with random generated name. If
    // the one exists, keep trying another path name until we reach some limit.
    char random_string[48];
    GetRandomString(sizeof(random_string), random_string);

    path_to_create = base_dir.Append(random_string);
    if (mkdir(path_to_create.value().c_str(), 0700) == 0) {
      *new_dir = path_to_create;
      return true;
    }
  }

  return false;
}

bool PathExists(const FilePath& path) {
  return access(path.value().c_str(), F_OK) == 0;
}

bool AbsolutePath(FilePath* path) {
  // Some platforms may not have realpath(), so we just check to make sure
  // every path is absolute already.
  // see file_util_posix.cc for the original version.
  DCHECK_EQ(path->value().c_str()[0], '/');
  return true;
}

int CountFilesCreatedAfter(const FilePath& path,
                           const base::Time& comparison_time) {
  // Copied and modified from file_util_posix.cc
  base::ThreadRestrictions::AssertIOAllowed();
  int file_count = 0;

  DIR* dir = opendir(path.value().c_str());
  if (dir) {
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
      if ((strcmp(ent->d_name, ".") == 0) ||
          (strcmp(ent->d_name, "..") == 0))
        continue;

      struct stat st;
      int test = stat(path.Append(ent->d_name).value().c_str(), &st);
      if (test != 0) {
        DPLOG(ERROR) << "stat failed";
        continue;
      }
      // Here, we use Time::TimeT(), which discards microseconds. This
      // means that files which are newer than |comparison_time| may
      // be considered older. If we don't discard microseconds, it
      // introduces another issue. Suppose the following case:
      //
      // 1. Get |comparison_time| by Time::Now() and the value is 10.1 (secs).
      // 2. Create a file and the current time is 10.3 (secs).
      //
      // As POSIX doesn't have microsecond precision for |st_ctime|,
      // the creation time of the file created in the step 2 is 10 and
      // the file is considered older than |comparison_time|. After
      // all, we may have to accept either of the two issues: 1. files
      // which are older than |comparison_time| are considered newer
      // (current implementation) 2. files newer than
      // |comparison_time| are considered older.
      if (static_cast<time_t>(st.st_ctime) >= comparison_time.ToTimeT())
        ++file_count;
    }
    closedir(dir);
  }
  return file_count;
}

bool NormalizeFilePath(const FilePath& path, FilePath* normalized_path) {
  // We don't support parent references.
  if (path.ReferencesParent())
    return false;

  // We do support relative paths.

  // To be consistant with windows, fail if path_result is a directory.
  struct stat file_info;
  if (stat(path.value().c_str(), &file_info) != 0 ||
      S_ISDIR(file_info.st_mode)) {
    return false;
  }

  *normalized_path = path;
  return true;
}

bool DirectoryExists(const FilePath& path) {
  base::PlatformFileInfo info;
  if (!GetFileInfo(path, &info)) {
    return false;
  }
  return info.is_directory;
}

bool GetPosixFilePermissions(const FilePath& path, int* mode) {
  base::ThreadRestrictions::AssertIOAllowed();
  DCHECK(mode);

  struct stat stat_buf;
  if (stat(path.value().c_str(), &stat_buf) != 0)
    return false;

  *mode = stat_buf.st_mode & FILE_PERMISSION_MASK;
  return true;
}

bool SetPosixFilePermissions(const FilePath& path,
                             int mode) {
  base::ThreadRestrictions::AssertIOAllowed();
  DCHECK_EQ(mode & ~FILE_PERMISSION_MASK, 0);

  struct stat stat_buf;
  if (stat(path.value().c_str(), &stat_buf) != 0)
    return false;

  // Clears the existing permission bits, and adds the new ones.
  mode_t updated_mode_bits = stat_buf.st_mode & ~FILE_PERMISSION_MASK;
  updated_mode_bits |= mode & FILE_PERMISSION_MASK;

  if (HANDLE_EINTR(chmod(path.value().c_str(), updated_mode_bits)) != 0)
    return false;

  return true;
}

FILE* OpenFile(const std::string& filename, const char* mode) {
  return OpenFile(FilePath(filename), mode);
}

FILE* OpenFile(const FilePath& filename, const char* mode) {
  base::ThreadRestrictions::AssertIOAllowed();
  FILE* result = NULL;
  do {
    result = fopen(filename.value().c_str(), mode);
  } while (!result && errno == EINTR);
  return result;
}

int WriteFile(const FilePath& filename, const char* data, int size) {
  base::ThreadRestrictions::AssertIOAllowed();
  int fd = HANDLE_EINTR(open(filename.value().c_str(),
                             O_CREAT|O_WRONLY|O_TRUNC,
                             0666));
  if (fd < 0)
    return -1;

  int bytes_written = WriteFileDescriptor(fd, data, size);
  if (int ret = HANDLE_EINTR(close(fd)) < 0)
    return ret;
  return bytes_written;
}

int WriteFileDescriptor(const int fd, const char* data, int size) {
  // Allow for partial writes.
  ssize_t bytes_written_total = 0;
  for (ssize_t bytes_written_partial = 0; bytes_written_total < size;
       bytes_written_total += bytes_written_partial) {
    bytes_written_partial =
        HANDLE_EINTR(write(fd, data + bytes_written_total,
                           size - bytes_written_total));
    if (bytes_written_partial < 0)
      return -1;
  }

  return bytes_written_total;
}

int AppendToFile(const FilePath& filename, const char* data, int size) {
  base::ThreadRestrictions::AssertIOAllowed();
  int fd = HANDLE_EINTR(open(filename.value().c_str(), O_WRONLY | O_APPEND));
  if (fd < 0)
    return -1;

  int bytes_written = WriteFileDescriptor(fd, data, size);
  if (int ret = HANDLE_EINTR(close(fd)) < 0)
    return ret;
  return bytes_written;
}

bool Move(const FilePath& from_path, const FilePath& to_path) {
  base::ThreadRestrictions::AssertIOAllowed();
  // Windows compatibility: if to_path exists, from_path and to_path
  // must be the same type, either both files, or both directories.
  struct stat to_file_info;
  if (stat(to_path.value().c_str(), &to_file_info) == 0) {
    struct stat from_file_info;
    if (stat(from_path.value().c_str(), &from_file_info) == 0) {
      if (S_ISDIR(to_file_info.st_mode) != S_ISDIR(from_file_info.st_mode))
        return false;
    } else {
      return false;
    }
  }

  if (rename(from_path.value().c_str(), to_path.value().c_str()) == 0)
    return true;

  if (!CopyDirectory(from_path, to_path, true))
    return false;

  Delete(from_path, true);
  return true;
}

bool CopyDirectory(const FilePath& from_path, const FilePath& to_path,
                   bool recursive) {
  // Some old callers of CopyDirectory want it to support wildcards.
  // After some discussion, we decided to fix those callers.
  // Break loudly here if anyone tries to do this.
  // TODO(evanm): remove this once we're sure it's ok.
  DCHECK(to_path.value().find('*') == std::string::npos);
  DCHECK(from_path.value().find('*') == std::string::npos);

  // This function does not properly handle destinations within the source
  FilePath real_to_path = to_path;
  if (PathExists(real_to_path)) {
    if (!AbsolutePath(&real_to_path))
      return false;
  } else {
    real_to_path = real_to_path.DirName();
    if (!AbsolutePath(&real_to_path))
      return false;
  }
  FilePath real_from_path = from_path;
  if (!AbsolutePath(&real_from_path))
    return false;
  if (real_to_path.value().size() >= real_from_path.value().size() &&
      real_to_path.value().compare(0, real_from_path.value().size(),
      real_from_path.value()) == 0)
    return false;

  bool success = true;
  int traverse_type = FileEnumerator::FILES | FileEnumerator::SHOW_SYM_LINKS;
  if (recursive)
    traverse_type |= FileEnumerator::DIRECTORIES;
  FileEnumerator traversal(from_path, recursive, traverse_type);

  // We have to mimic windows behavior here. |to_path| may not exist yet,
  // start the loop with |to_path|.
  FileEnumerator::FindInfo info;
  FilePath current = from_path;
  if (stat(from_path.value().c_str(), &info.stat) < 0) {
    DLOG(ERROR) << "CopyDirectory() couldn't stat source directory: "
                << from_path.value() << " errno = " << errno;
    success = false;
  }
  struct stat to_path_stat;
  FilePath from_path_base = from_path;
  if (recursive && stat(to_path.value().c_str(), &to_path_stat) == 0 &&
      S_ISDIR(to_path_stat.st_mode)) {
    // If the destination already exists and is a directory, then the
    // top level of source needs to be copied.
    from_path_base = from_path.DirName();
  }

  // The Windows version of this function assumes that non-recursive calls
  // will always have a directory for from_path.
  DCHECK(recursive || S_ISDIR(info.stat.st_mode));

  while (success && !current.empty()) {
    // current is the source path, including from_path, so append
    // the suffix after from_path to to_path to create the target_path.
    FilePath target_path(to_path);
    if (from_path_base != current) {
      if (!from_path_base.AppendRelativePath(current, &target_path)) {
        success = false;
        break;
      }
    }

    if (S_ISDIR(info.stat.st_mode)) {
      if (mkdir(target_path.value().c_str(), info.stat.st_mode & 01777) != 0 &&
          errno != EEXIST) {
        DLOG(ERROR) << "CopyDirectory() couldn't create directory: "
                    << target_path.value() << " errno = " << errno;
        success = false;
      }
    } else if (S_ISREG(info.stat.st_mode)) {
      if (!CopyFile(current, target_path)) {
        DLOG(ERROR) << "CopyDirectory() couldn't create file: "
                    << target_path.value();
        success = false;
      }
    } else {
      DLOG(WARNING) << "CopyDirectory() skipping non-regular file: "
                    << current.value();
    }

    current = traversal.Next();
    traversal.GetFindInfo(&info);
  }

  return success;
}

bool CreateDirectory(const FilePath& full_path) {
  std::vector<FilePath> subpaths;

  // Collect a list of all parent directories, but not including root.
  FilePath last_path = full_path;
  for (FilePath path = full_path.DirName();
       path.value() != last_path.value(); path = path.DirName()) {
    subpaths.push_back(last_path);
    last_path = path;
  }

  // Iterate through the parents and create the missing ones.
  for (std::vector<FilePath>::reverse_iterator i = subpaths.rbegin();
       i != subpaths.rend(); ++i) {
    if (DirectoryExists(*i))
      continue;
    if (mkdir(i->value().c_str(), 0700) == 0)
      continue;
    // Mkdir failed, but it might have failed with EEXIST, or some other error
    // due to the the directory appearing out of thin air. This can occur if
    // two processes are trying to create the same file system tree at the same
    // time. Check to see if it exists and make sure it is a directory.
    if (!DirectoryExists(*i))
      return false;
  }
  return true;
}

bool Delete(const FilePath& path, bool recursive) {
  const char* path_str = path.value().c_str();
  base::PlatformFileInfo info;
  // Return true if the path doesn't exist.
  if (!PathExists(path)) {
    return true;
  }
  if (!GetFileInfo(path, &info)) {
    return false;
  }

  if (!info.is_directory)
    return (unlink(path_str) == 0);
  if (!recursive)
    return (rmdir(path_str) == 0);

  bool success = true;
  std::stack<std::string> directories;
  directories.push(path.value());
  FileEnumerator traversal(path, true, static_cast<FileEnumerator::FileType>(
        FileEnumerator::FILES | FileEnumerator::DIRECTORIES |
        FileEnumerator::SHOW_SYM_LINKS));
  for (FilePath current = traversal.Next(); success && !current.empty();
       current = traversal.Next()) {
    FileEnumerator::FindInfo info;
    traversal.GetFindInfo(&info);

    if (S_ISDIR(info.stat.st_mode))
      directories.push(current.value());
    else
      success = (unlink(current.value().c_str()) == 0);
  }

  while (success && !directories.empty()) {
    FilePath dir = FilePath(directories.top());
    directories.pop();
    success = (rmdir(dir.value().c_str()) == 0);
  }
  return success;
}

int ReadFile(const FilePath& filename, char* data, int size) {
  int fd = open(filename.value().c_str(), O_RDONLY);
  if (fd < 0) {
    DLOG(ERROR) << "file_util::ReadFile failed on " << filename.value();
    return -1;
  }
  ssize_t bytes_read = read(fd, data, size);
  close(fd);
  return bytes_read;
}

bool GetCurrentDirectory(FilePath* dir) {
  *dir = FilePath(GetGlobalsPtr()->game_content_path);
  return true;
}

bool ReplaceFile(const FilePath& from_path, const FilePath& to_path) {
  int ret = rename(from_path.value().c_str(), to_path.value().c_str());

  return (ret == 0);
}

bool CopyFile(const FilePath& from_path, const FilePath& to_path) {
  int infile = open(from_path.value().c_str(), O_RDONLY);
  if (infile < 0)
    return false;

  // Get file info so we can set the same permissions on the copied file
  struct stat file_info;
  fstat(infile, &file_info);

  int outfile = open(to_path.value().c_str(), O_CREAT | O_WRONLY,
                     file_info.st_mode);
  if (outfile < 0) {
    close(infile);
    return false;
  }

  const size_t kBufferSize = 32768;
  std::vector<char> buffer(kBufferSize);
  bool result = true;

  int64 offset = 0;
  while (result) {
    ssize_t bytes_read = read(infile, &buffer[0], buffer.size());

    if (bytes_read <= 0) {
      result = bytes_read == 0;
      break;
    }

    // Allow for partial writes
    ssize_t bytes_written_per_read = 0;
    do {
      ssize_t bytes_written_partial = write(
          outfile,
          &buffer[bytes_written_per_read],
          bytes_read - bytes_written_per_read);
      if (bytes_written_partial < 0) {
        result = false;
        break;
      }
      bytes_written_per_read += bytes_written_partial;
    } while (bytes_written_per_read < bytes_read);
    offset += bytes_read;
  }

  if (HANDLE_EINTR(close(infile)) < 0)
    result = false;
  if (HANDLE_EINTR(close(outfile)) < 0)
    result = false;

  return result;
}

bool ReadFromFD(int fd, char* buffer, size_t bytes) {
  size_t total_read = 0;
  while (total_read < bytes) {
    ssize_t bytes_read =
        HANDLE_EINTR(read(fd, buffer + total_read, bytes - total_read));
    if (bytes_read <= 0)
      break;
    total_read += bytes_read;
  }
  return total_read == bytes;
}

///////////////////////////////////////////////
// FileEnumerator

FileEnumerator::FileEnumerator(const FilePath& root_path,
                               bool recursive,
                               int file_type)
    : current_directory_entry_(0)
    , root_path_(root_path)
    , recursive_(recursive)
    , file_type_(file_type) {
  DCHECK(!(recursive && (INCLUDE_DOT_DOT & file_type_)));
  pending_paths_.push(root_path);
}

FileEnumerator::FileEnumerator(const FilePath& root_path,
                               bool recursive,
                               int file_type,
                               const FilePath::StringType& pattern)
  : current_directory_entry_(0)
  , root_path_(root_path)
  , recursive_(recursive)
  , file_type_(file_type) {
  DCHECK(!(recursive && (INCLUDE_DOT_DOT & file_type_)));
  if (pattern.empty()) {
    pattern_ = FilePath::StringType();
  } else {
    pattern_ = root_path.Append(pattern).value();
  }

  // Allow a trailing asterisk, but no other wildcards.
  size_t pattern_len = pattern_.size();
  size_t asterisk_pos = pattern_.find('*');
  DCHECK((asterisk_pos == pattern_len - 1) ||
         (asterisk_pos == std::string::npos));
  DCHECK_EQ(pattern_.find_first_of("[]?"), std::string::npos);
  pending_paths_.push(root_path);
}

FileEnumerator::~FileEnumerator() {
}

// An overly simplified version of fnmatch() that only works if the
// asterisk is at the end of the pattern.
static bool MatchWildcardStrings(const std::string& pattern,
                                 const std::string& str) {
  const size_t pattern_len = pattern.size();
  DCHECK(pattern_len);

  if (pattern[pattern_len - 1] != '*') {
    // No asterisk found at all. Do a regular string compare.
    return pattern == str;
  } else {
    // If the strings are the same up to the asterisk, then the pattern matches.
    return pattern.compare(0,
                           pattern_len - 1,
                           str,
                           0,
                           pattern_len - 1) == 0;
  }
}

FilePath FileEnumerator::Next() {
  ++current_directory_entry_;

  // While we've exhausted the entries in the current directory, do the next
  while (current_directory_entry_ >= directory_entries_.size()) {
    if (pending_paths_.empty())
      return FilePath();

    root_path_ = pending_paths_.top();
    root_path_ = root_path_.StripTrailingSeparators();
    pending_paths_.pop();

    std::vector<DirectoryEntryInfo> entries;
    if (!ReadDirectory(&entries, root_path_, file_type_ & SHOW_SYM_LINKS))
      continue;

    directory_entries_.clear();
    current_directory_entry_ = 0;
    for (std::vector<DirectoryEntryInfo>::const_iterator
        i = entries.begin(); i != entries.end(); ++i) {
      FilePath full_path = root_path_.Append(i->filename);
      if (ShouldSkip(full_path))
        continue;

      // We may not have fnmatch() function, so use a simplified version
      // to check strings that have only an asterisk at the end.  Anything else
      // should assert in the constructor.
      if (pattern_.size() &&
          !MatchWildcardStrings(pattern_, full_path.value())) {
        continue;
      }

      if (recursive_ && S_ISDIR(i->stat.st_mode))
        pending_paths_.push(full_path);

      if ((S_ISDIR(i->stat.st_mode) && (file_type_ & DIRECTORIES)) ||
          (!S_ISDIR(i->stat.st_mode) && (file_type_ & FILES)))
        directory_entries_.push_back(*i);
    }
  }

  return root_path_.Append(
    directory_entries_[current_directory_entry_].filename);
}

void FileEnumerator::GetFindInfo(FindInfo* info) {
  DCHECK(info);

  if (current_directory_entry_ >= directory_entries_.size())
    return;

  DirectoryEntryInfo* cur_entry = &directory_entries_[current_directory_entry_];
  memcpy(&(info->stat), &(cur_entry->stat), sizeof(info->stat));
  info->filename.assign(cur_entry->filename.value());
}

bool FileEnumerator::IsDirectory(const FindInfo& info) {
  return S_ISDIR(info.stat.st_mode);
}

// static
FilePath FileEnumerator::GetFilename(const FindInfo& find_info) {
  return FilePath(find_info.filename);
}

// static
int64 FileEnumerator::GetFilesize(const FindInfo& find_info) {
  return find_info.stat.st_size;
}

// static
base::Time FileEnumerator::GetLastModifiedTime(const FindInfo& find_info) {
  return base::Time::FromTimeT(find_info.stat.st_mtime);
}

bool FileEnumerator::ReadDirectory(std::vector<DirectoryEntryInfo>* entries,
                                   const FilePath& source, bool show_links) {
  DIR* dir = opendir(source.value().c_str());
  if (!dir)
    return false;

  struct dirent* dent;
  while ((dent = readdir(dir))) {
    DirectoryEntryInfo info;
    info.filename = FilePath(dent->d_name);

    FilePath full_name = source.Append(dent->d_name);
    int ret = stat(full_name.value().c_str(), &info.stat);
    if (ret < 0) {
      // Print the stat() error message unless it was ENOENT and we're
      // following symlinks.
      if (!(errno == ENOENT && !show_links)) {
        DPLOG(ERROR) << "Couldn't stat "
                     << source.Append(dent->d_name).value();
      }
      memset(&info.stat, 0, sizeof(info.stat));
    }
    entries->push_back(info);
  }

  closedir(dir);
  return true;
}

}  // namespace file_util
