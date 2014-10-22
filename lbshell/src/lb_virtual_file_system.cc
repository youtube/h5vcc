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

#include "lb_virtual_file_system.h"

#include "base/logging.h"

namespace {
// Update this any time the serialization format changes.
const char *kVersion = "SAV0";
}

// ---------------- LBVirtualFile Methods -------------------

LBVirtualFile::LBVirtualFile(const std::string &name) : size_(0) {
  name_.assign(name);
}

int LBVirtualFile::Read(void *out, const size_t bytes, int offset) const {
  DCHECK_GE(offset, 0);
  if (offset > size_)
    return 0;
  size_t bytes_to_read = std::min(size_ - offset, bytes);
  if (bytes_to_read == 0)
    return 0;
  memcpy(out, &buffer_[offset], bytes_to_read);
  return bytes_to_read;
}

int LBVirtualFile::Write(const void *data, const size_t bytes,
                         const int offset) {
  if (buffer_.size() < offset + bytes)
    buffer_.resize(offset + bytes);

  memcpy(&buffer_[offset], data, bytes);
  size_ = std::max<int>(size_, offset + bytes);
  return bytes;
}

int LBVirtualFile::Truncate(const size_t size) {
  size_ = std::min(size_, size);
  return size_;
}

int LBVirtualFile::Serialize(void *buffer, const bool dry_run) {
  // TODO(wpwang): Use the pickle library to serialize.
  // TODO(wpwang): Nice to have: add the ability to Serialize/Deserialize
  // directly to/from a file, instead of an intermediate buffer.
  serialize_position_ = 0;

  // Save out filename length
  size_t name_length = name_.length();
  DCHECK_LT(name_length, MAX_VFS_PATHNAME);
  WriteBuffer(buffer, &name_length, sizeof(size_t), dry_run);

  // Save out the filename
  WriteBuffer(buffer, name_.data(), name_length, dry_run);

  // Save out the file contents size
  WriteBuffer(buffer, &size_, sizeof(size_t), dry_run);

  // Save out the file contents
  WriteBuffer(buffer, &buffer_[0], size_, dry_run);

  // Return the number of bytes written
  return serialize_position_;
}

int LBVirtualFile::Deserialize(const void *buffer) {
  serialize_position_ = 0;

  // Read in filename length
  size_t name_length;
  ReadBuffer(&name_length, buffer, sizeof(size_t));

  // Read in filename
  char name[MAX_VFS_PATHNAME];
  if (name_length >= sizeof(name)) {
    DLOG(ERROR) << "Filename was longer than the maximum allowed.";
    return -1;
  }
  ReadBuffer(name, buffer, name_length);
  name_.assign(name, name_length);

  // Read in file contents size
  ReadBuffer(&size_, buffer, sizeof(size_t));

  // Read in the file contents
  buffer_.resize(size_);
  ReadBuffer(&buffer_[0], buffer, size_);

  // Return the number of bytes read
  return serialize_position_;
}

void LBVirtualFile::WriteBuffer(void *buffer, const void *src, size_t size,
                                bool dry_run) {
  if (!dry_run)
    memcpy(reinterpret_cast<char*>(buffer) + serialize_position_, src, size);
  serialize_position_ += size;
}

void LBVirtualFile::ReadBuffer(void *dst, const void *buffer, size_t size) {
  memcpy(dst, reinterpret_cast<const char*>(buffer) + serialize_position_,
         size);
  serialize_position_ += size;
}

// ---------------- LBVirtualFileSystem Methods -------------------

LBVirtualFileSystem::LBVirtualFileSystem() { }

LBVirtualFileSystem::~LBVirtualFileSystem() {
  for (FileTable::iterator itr = table_.begin(); itr != table_.end(); ++itr) {
    delete itr->second;
  }
}

LBVirtualFile* LBVirtualFileSystem::Open(std::string filename) {
  LBVirtualFile *result = table_[filename];
  if (!result) {
    result = new LBVirtualFile(filename);
    table_[filename] = result;
  }
  return result;
}

void LBVirtualFileSystem::Delete(std::string filename) {
  if (table_.find(filename) != table_.end()) {
    delete table_[filename];
    table_.erase(filename);
  }
}

int LBVirtualFileSystem::Serialize(char *buffer, const bool dry_run) {
  char *original = buffer;

  // We don't know the total size of the file yet, so defer writing the header.
  buffer += sizeof(SerializedHeader);

  // Serialize each file
  for (FileTable::iterator itr = table_.begin(); itr != table_.end(); ++itr) {
    int bytes = itr->second->Serialize(buffer, dry_run);
    buffer += bytes;
  }

  if (!dry_run) {
    // Now we can write the header to the beginning of the buffer.
    SerializedHeader header;
    header.version = GetVersion();
    header.file_count = table_.size();
    header.file_size = buffer - original;
    memcpy(original, &header, sizeof(SerializedHeader));
  }

  return buffer - original;
}

void LBVirtualFileSystem::Deserialize(const char *buffer) {
  // TODO(wpwang): Use the pickle library to serialize.

  // Clear out any old files
  for (FileTable::iterator itr = table_.begin(); itr != table_.end(); ++itr) {
    delete itr->second;
  }
  table_.clear();

  // Read in expected number of files
  SerializedHeader header;
  memcpy(&header, buffer, sizeof(SerializedHeader));
  buffer += sizeof(SerializedHeader);

  // Do some basic validation on the header data.
  if (header.version != GetVersion()) {
    DLOG(INFO) << "Attempted to load a different version; operation aborted.";
    return;
  }
  if (header.file_size < sizeof(SerializedHeader)) {
    DLOG(INFO) << "Possible data corruption detected; operation aborted.";
    return;
  }

  for (int i = 0; i < header.file_count; i++) {
    LBVirtualFile *file = new LBVirtualFile("");
    int bytes = file->Deserialize(buffer);
    if (bytes < 0) {
      DLOG(ERROR) << "Failed to deserialize virtual file system.";

      // Something went wrong; the data in the table is probably corrupt, so
      // clear it out.
      delete file;
      for (FileTable::iterator itr = table_.begin(); itr != table_.end();
           ++itr) {
        delete itr->second;
      }
      table_.clear();
      break;
    }

    buffer += bytes;

    table_[file->name_] = file;
  }
}

unsigned int LBVirtualFileSystem::GetVersion() const {
  DCHECK_EQ(strlen(kVersion), 4);
  int version;
  memcpy(&version, kVersion, sizeof(version));
  return version;
}
