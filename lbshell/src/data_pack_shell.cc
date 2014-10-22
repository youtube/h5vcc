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
// This file makes resources available in a pack file. This is a
// re-implementation of data_pack.cc because that file uses memory mapped IO.
// The format of the (version 4) file is this:
//
// |--byte--|--byte--|--byte--|--byte--|
// |===================================|
// |--------------version--------------|
// |--------number-of-resources--------|
// |encoding|   ==>
//
// |--byte--|--byte--|--byte--|--byte--|--byte--|--byte--|
// |=====================================================|
// |--resource-id-1--|-----------file-offset-1-----------|
// |--resource-id-2--|-----------file-offset-2-----------|
// |--resource-id-3--|-----------file-offset-3-----------|
// |--resource-id-4--|-----------file-offset-4-----------|
//       ...
//       ... resource data
//
// All of these fields (except for the actual resource data) are in
// little-endian format.
//
// It is assumed that all the resources are in the same order in the file that
// their metadata is at the beginning of the file, and that all the resources
// are packed in the file. Therefore, if you want to know the length of a
// resource, subtract that resource's file offset from the file offset of the
// next resource listed in the metadata. I'm assuming that real chrome puts a
// bogus entry at the very end, so you can find the length of the last real
// resource.
//
// Because the real implementation of this file uses memory mapped IO, and the
// accessor functions return pointers that point inside the memory mapped range,
// the actual resource bytes have to be "static" with respect to the rest of the
// program. Therefore, the bytes can't be realloced at any point after the
// accessor functions get called. I'm relying on std::map to not do any internal
// copying/moving of data if it has to rebalance its internal tree.

#include "ui/base/resource/data_pack.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/metrics/histogram.h"
#include "base/platform_file.h"
#include "base/string_piece.h"
#include "base/stringprintf.h"
#include "base/sys_byteorder.h"
#include "lb_memory_manager.h"

namespace {
static const uint32 kFileFormatVersion = 4;
}

namespace ui {

bool DataPack::LoadFromPath(const FilePath& path) {
  pack_file_ = path;

  base::PlatformFileError e;
  base::PlatformFile f(
      base::CreatePlatformFile(path, base::PLATFORM_FILE_OPEN |
                                     base::PLATFORM_FILE_READ, NULL, &e));
  if (e != base::PLATFORM_FILE_OK) {
    DLOG(ERROR) << "Could not open resources pack";
    return false;
  }
  bool ok = LoadFromFile(f);
  base::ClosePlatformFile(f);
  return ok;
}

// Loads a pack file from |file|, returning false on error.
bool DataPack::LoadFromFile(base::PlatformFile file) {
  DataPackHeader file_header;

  if (base::ReadPlatformFile(file, 0,
          reinterpret_cast<char*>(&file_header), sizeof(DataPackHeader)) !=
      sizeof(DataPackHeader)) {
    DLOG(ERROR) << "Could not read resources pack header";
    return false;
  }
  if (base::ByteSwapToLE32(file_header.version) != ::kFileFormatVersion) {
    DLOG(ERROR) << StringPrintf("%s %s %d", "Resources pack is wrong version!",
                                            "Expected version:",
                                            ::kFileFormatVersion);
    return false;
  }
  resource_count_ = base::ByteSwapToLE32(file_header.resource_count);
  text_encoding_type_ = static_cast<TextEncodingType>(file_header.encoding);
  if (text_encoding_type_ != UTF8 && text_encoding_type_ != UTF16 &&
      text_encoding_type_ != BINARY) {
    DLOG(ERROR) << "Resources pack has unknown encoding type!";
    return false;
  }

  metadata_.reset(new DataPackEntry[resource_count_]);

  // Read in all the metadata
  if (base::ReadPlatformFile(
          file, sizeof(DataPackHeader),
          reinterpret_cast<char*>(metadata_.get()),
          resource_count_ * sizeof(DataPackEntry))
      != resource_count_ * sizeof(DataPackEntry)) {
    DLOG(ERROR) << StringPrintf("Could not load resource metadata!");
    return false;
  }

  // Convert the endianness of the data we just read in
  for (int i(0); i < resource_count_; i++) {
    metadata_[i].resource_id = base::ByteSwapToLE16(metadata_[i].resource_id);
    metadata_[i].file_offset = base::ByteSwapToLE32(metadata_[i].file_offset);

    resource_id_to_metadata_index_.insert(std::make_pair(
        metadata_[i].resource_id, i));
  }

  // Try to avoid reading in the 500k of data. Instead, lazily load it.
  return true;
}

bool DataPack::GetStringPiece(uint16 resource_id,
                              base::StringPiece* data) const {
  // See if resource_id is in the cache
  std::map<uint16, std::string>::const_iterator i(
      static_cache_.find(resource_id));
  if (i != static_cache_.end()) {
    data->set(i->second.data(), i->second.size());
    return true;
  }

  // Look up resource_id and load it into the cache
  std::map<uint16, int>::const_iterator j(
      resource_id_to_metadata_index_.find(resource_id));
  if (j == resource_id_to_metadata_index_.end()) {
    return false;
  }
  int index = j->second;
  // Real chrome doesn't worry about i being the last index in the array, so I
  // can only assume there's a bogus entry at the end.
  int length = metadata_[index+1].file_offset - metadata_[index].file_offset;

  scoped_array<char> buffer(new char[length]);

  base::PlatformFileError e;
  base::PlatformFile f(
      base::CreatePlatformFile(pack_file_, base::PLATFORM_FILE_OPEN |
                                           base::PLATFORM_FILE_READ, NULL, &e));
  if (e != base::PLATFORM_FILE_OK) {
    DLOG(ERROR) << "Could not open resources pack";
    return false;
  }
  if (base::ReadPlatformFile(f,
                             metadata_[index].file_offset,
                             buffer.get(),
                             length) != length) {
    DLOG(ERROR) << StringPrintf(
        "Could not load resource metadata index: %d!", index);
    base::ClosePlatformFile(f);
    return false;
  }
  base::ClosePlatformFile(f);

  static_cache_.insert(std::make_pair(resource_id, std::string(
      buffer.get(), length)));

  data->set(static_cache_[resource_id].data(), length);
  return true;
}

}  // namespace ui
