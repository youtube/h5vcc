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

#ifndef CORE_DUMPER_H_
#define CORE_DUMPER_H_

#include <elf.h>
#include <linux/limits.h>
#include <map>
#include <stdint.h>
#include <sys/types.h>
#include <sys/user.h>
#include <vector>

#include "common/coredump_format.h"
#include "common/elf_core_dump.h"
#include "external/chromium/breakpad/src/common/memory.h"
#include "external/chromium/breakpad/src/common/linux/memory_mapped_file.h"
#include "external/chromium/breakpad/src/google_breakpad/common/minidump_format.h"

namespace crash_report {

class CoreDumper {
 public:
  // Constructs a dumper for extracting information from the core dump file at
  // |core_path|
  CoreDumper(const char* core_path);

  ~CoreDumper();

  bool Init();

  google_breakpad::wasteful_vector<PPU_Thr_Info>* GetPPUThrInfo() {
    return &ppu_thr_info_;
  }

  google_breakpad::wasteful_vector<SPU_Thr_Info>* GetSPUThrInfo() {
    return &spu_thr_info_;
  }

  PPU_Reg_Info* GetPPURegInfo(Elf_Off thread_id);

  SPU_Reg_Info* GetSPURegInfo(Elf_Word thread_id);

  System_Info* GetSystemInfo() { return &system_info_; }

  google_breakpad::wasteful_vector<PPU_Dump_Excep>* GetPPUDumpExcep() {
    return &ppu_dump_excep_;
  }

  google_breakpad::wasteful_vector<SPU_Dump_Excep>* GetSPUDumpExcep() {
    return &spu_dump_excep_;
  }

  google_breakpad::wasteful_vector<RSX_Dump_Excep>* GetRSXDumpExcep() {
    return &rsx_dump_excep_;
  }

  unsigned char* GetAppVersion() {
    return game_app_info_.app_version;
  }

  u_int32_t GetCrashThread();

  bool GetThrgrpInfo(u_int32_t thrgrp_id, SPU_Thrgrp_Info* thrgrp_info);

  google_breakpad::PageAllocator* allocator() { return &allocator_; }

  void CopyFromProcess(void* dest, u_int64_t virtual_address, size_t length);

  std::vector<u_int32_t>* GetCallStack(u_int64_t ppu_thr_id);

  size_t GetMemorySize(u_int64_t ppu_thr_id);

  void GetMemory(void* dest,
                 u_int64_t virtual_address,
                 u_int64_t ppu_thr_id);

  bool GetSrr0(u_int64_t ppu_thr_id, u_int64_t* srr0);

 private:
  bool Setup();
  void ReadDumpCauseInfo(google_breakpad::MemoryRange* data,
                         u_int32_t unit_num,
                         u_int32_t unit_size);

  template <typename T>
  void ReadData(google_breakpad::MemoryRange* data,
                size_t offset,
                google_breakpad::wasteful_vector<T>* infos);

  template <typename T>
  void ReadArray(google_breakpad::MemoryRange* data,
                 u_int32_t unit_num,
                 google_breakpad::wasteful_vector<T>* infos);

  // Path of the core dump file.
  const char* core_path_;

  mutable google_breakpad::PageAllocator allocator_;

  // Memory-mapped core dump file at |core_path_|
  google_breakpad::MemoryMappedFile mapped_core_file_;

  // Content of the core dump file.
  ElfCoreDump core_;

  // System info found in the core dump file.
  System_Info system_info_;

  // PPU exception info found in the core dump file.
  PPU_Excep_Info ppu_excep_info_;

  // Process info found in the core dump file.
  Process_Info process_info_;

  // PPU register info found in the core dump file.
  google_breakpad::wasteful_vector<PPU_Reg_Info> ppu_reg_info_;

  // SPU register info found in the core dump file.
  google_breakpad::wasteful_vector<SPU_Reg_Info> spu_reg_info_;

  // PPU thread info found in the core dump file.
  google_breakpad::wasteful_vector<PPU_Thr_Info> ppu_thr_info_;

  // Dump cause info found in the core dump file.
  google_breakpad::wasteful_vector<Dump_Cause_Info> dump_cause_info_;

  // PPU dump excep found in the core dump file.
  google_breakpad::wasteful_vector<PPU_Dump_Excep> ppu_dump_excep_;

  // SPU dump excep found in the core dump file.
  google_breakpad::wasteful_vector<SPU_Dump_Excep> spu_dump_excep_;

  // RSX dump excep found in the core dump file.
  google_breakpad::wasteful_vector<RSX_Dump_Excep> rsx_dump_excep_;

  // User defined dump cause found in the core dump file.
  google_breakpad::wasteful_vector<User_Defined_Excep> user_defined_excep_;

  // SPU thread group info found in the core dump file.
  google_breakpad::wasteful_vector<SPU_Thrgrp_Info> spu_thrgrp_info_;

  // SPU thread info found in the core dump file.
  google_breakpad::wasteful_vector<SPU_Thr_Info> spu_thr_info_;

  // PRX info found in the core dump file.
  google_breakpad::wasteful_vector<PRX_Info> prx_info_;

  // Page attribute info found in the core dump file.
  google_breakpad::wasteful_vector<Page_Attr_Info> page_attr_info_;

  // Game app info found in the core dump file.
  Game_App_Info game_app_info_;

  // PPU thread call stack info found in the core dump file.
  std::map<u_int64_t, std::vector<u_int32_t> > call_stack_;
};

} // namespace crash_report

#endif // CORE_DUMPER_H_
