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

#ifndef MINIDUMP_WRITER_H_
#define MINIDUMP_WRITER_H_

#include "external/chromium/breakpad/src/client/minidump_file_writer-inl.h"
#include "core_dumper.h"
#include "symbol_info.h"

namespace crash_report {

class MinidumpWriter {
 public:
  // The following kLimit* constants are for when minidump_size_limit_ is set
  // and the minidump size might exceed it.
  //
  // Estimate for how big each thread's stack will be (in bytes).
  static const unsigned kLimitAverageThreadStackLength = 8 * 1024;
  // Number of threads whose stack size we don't want to limit.  These base
  // threads will simply be the first N threads returned by the dumper (although
  // the crashing thread will never be limited).  Threads beyond this count are
  // the extra threads.
  static const unsigned kLimitBaseThreadCount = 20;
  // Maximum stack size to dump for any extra thread (in bytes).
  static const unsigned kLimitMaxExtraThreadStackLen = 2 * 1024;
  // Make sure this number of additional bytes can fit in the minidump

  MinidumpWriter(const char* minidump_path,
                 CoreDumper* dumper,
                 SymbolInfo* symbol_info);

  ~MinidumpWriter();

  bool Init();

  bool Dump();

 private:
  bool WriteThreadListStream(MDRawDirectory* dirent);

  bool WriteModuleListStream(MDRawDirectory* dirent);

  bool WriteMemoryListStream(MDRawDirectory* dirent);

  bool WriteExceptionStream(MDRawDirectory* dirent);

  bool WriteSystemInfoStream(MDRawDirectory* dirent);

  void* Alloc(unsigned bytes) {
    return dumper_->allocator()->Alloc(bytes);
  }

  const char* path_;
  CoreDumper* dumper_;
  SymbolInfo* symbol_info_;

  google_breakpad::MinidumpFileWriter minidump_writer_;
  MDLocationDescriptor crashing_thread_context_;
  google_breakpad::wasteful_vector<MDMemoryDescriptor> memory_blocks_;
};

bool WriteMinidump(const char* filename,
                   CoreDumper* dumper,
                   SymbolInfo* symbol_info);

} // namespace crash_report

#endif //MINIDUMP_WRITER_H_
