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

#ifndef SRC_LB_VIRTUAL_FILE_SYSTEM_H_
#define SRC_LB_VIRTUAL_FILE_SYSTEM_H_

// These classes implement a simple virtual filesystem, primarily intended to
// be used for simulating a filesystem that SQLite can write to, and allowing
// that filesystem to be saved out into a single memory buffer.

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"

#define MAX_VFS_PATHNAME 128

class LBVirtualFileSystem;

class LBVirtualFile {
 public:
  int Read(void *out, const size_t bytes, const int offset) const;
  int Write(const void *data, const size_t bytes, const int offset);

  int Truncate(const size_t size);

  const size_t Size() const { return size_; }

 private:
  explicit LBVirtualFile(const std::string &name);
  ~LBVirtualFile() { }

  // Returns the number of bytes written
  int Serialize(void *buffer, const bool dry_run);
  int Deserialize(const void *buffer);

  void WriteBuffer(void *buffer, const void *src, size_t size, bool dry_run);
  void ReadBuffer(void *dst, const void *buffer, size_t size);

  std::vector<char> buffer_;
  size_t size_;

  std::string name_;

  int serialize_position_;

  friend class LBVirtualFileSystem;
  DISALLOW_COPY_AND_ASSIGN(LBVirtualFile);
};

// Contains a mapping from paths to their corresponding memory blocks.
class LBVirtualFileSystem {
 public:
  struct SerializedHeader {
    unsigned int version;
    unsigned int file_size;
    unsigned int file_count;
  };

  LBVirtualFileSystem();
  ~LBVirtualFileSystem();

  // Serializes the file system out to a single contiguous buffer.
  // A dry run only calculates the number of bytes needed.
  // Returns the number of bytes written.
  int Serialize(char *buffer, const bool dry_run);

  // Deserializes a file system from a memory buffer
  void Deserialize(const char *buffer);

  // Simple file open. Will create a file if it does not exist, and files are
  // always readable and writable.
  LBVirtualFile* Open(std::string filename);

  void Delete(std::string filename);

 private:
  unsigned int GetVersion() const;

  typedef std::map<std::string, LBVirtualFile *> FileTable;
  FileTable table_;
};

#endif  // SRC_LB_VIRTUAL_FILE_SYSTEM_H_
