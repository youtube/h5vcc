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

#include "core_dumper.h"

#include <assert.h>
#include <elf.h>
#include <stdio.h>
#include <string.h>

#include "common/coredump_format.h"
#include "common/endian.h"
#include "external/chromium/breakpad/src/common/memory_range.h"

namespace crash_report {

CoreDumper::CoreDumper(const char* core_path)
    : core_path_(core_path),
      ppu_reg_info_(&allocator_, 8),
      spu_reg_info_(&allocator_, 8),
      ppu_thr_info_(&allocator_, 8),
      dump_cause_info_(&allocator_, 8),
      ppu_dump_excep_(&allocator_, 1),
      spu_dump_excep_(&allocator_, 1),
      rsx_dump_excep_(&allocator_, 1),
      user_defined_excep_(&allocator_, 1),
      spu_thrgrp_info_(&allocator_, 8),
      spu_thr_info_(&allocator_, 8),
      prx_info_(&allocator_, 8),
      page_attr_info_(&allocator_, 8) {
  assert(core_path_);
}

CoreDumper::~CoreDumper() {
}

bool CoreDumper::Init() {
  return Setup();
}

void CoreDumper::ReadDumpCauseInfo(google_breakpad::MemoryRange* data,
                                   u_int32_t unit_num,
                                   u_int32_t unit_size) {
  for (unsigned i = 0; i < unit_num; ++i) {
    ReadData<Dump_Cause_Info>(data, i * unit_size, &dump_cause_info_);
    switch (dump_cause_info_[i].cause_type) {
      case PPU_EXCEP: {
        ReadData<PPU_Dump_Excep>(data, i * unit_size +
            sizeof(Dump_Cause_Info), &ppu_dump_excep_);
        break;
      }
      case SPU_EXCEP: {
        ReadData<SPU_Dump_Excep>(data, i * unit_size +
            sizeof(Dump_Cause_Info), &spu_dump_excep_);
        break;
      }
      case RSX_EXCEP: {
        ReadData<RSX_Dump_Excep>(data, i * unit_size +
            sizeof(Dump_Cause_Info), &rsx_dump_excep_);
        break;
      }
      case USER_DEFINED: {
        ReadData<User_Defined_Excep>(data, i * unit_size +
            sizeof(Dump_Cause_Info), &user_defined_excep_);
        break;
      }
    }
  }
}

bool CoreDumper::Setup() {
  if (!mapped_core_file_.Map(core_path_)) {
    fprintf(stderr, "Could not map core dump file into memory\n");
    return false;
  }

  core_.SetContent(mapped_core_file_.content());
  if (!core_.IsValid()) {
    fprintf(stderr, "Invalid core dump file\n");
    return false;
  }

  google_breakpad::wasteful_vector<ElfCoreDump::Note> notes = core_.GetNotes();
  if (notes.size() == 0) {
    fprintf(stderr, "PT_NOTE section not found\n");
    return false;
  }

  for (unsigned i = 0; i < notes.size(); i++) {
    ElfCoreDump::Note note = notes[i];
    Elf_Word type = note.GetType();
    google_breakpad::MemoryRange name = note.GetName();
    const Desc_Hdr* header = note.GetDescHdr();
    const u_int32_t unit_size = header->unit_size;
    const u_int32_t unit_num = header->unit_num;
    google_breakpad::MemoryRange data = note.GetData();

    // Read useful information from coredump notes.
    switch (type) {
      case SYSTEM_INFO: {
        Swap(data.GetData<System_Info>(0), &system_info_);
        break;
      }
      case PPU_EXCEP_INFO: {
        Swap(data.GetData<PPU_Excep_Info>(0), &ppu_excep_info_);
        break;
      }
      case PROCESS_INFO: {
        Swap(data.GetData<Process_Info>(0), &process_info_);
        break;
      }
      case PPU_REG_INFO: {
        ReadArray<PPU_Reg_Info>(&data, unit_num, &ppu_reg_info_);
        break;
      }
      case SPU_REG_INFO: {
        ReadArray<SPU_Reg_Info>(&data, unit_num, &spu_reg_info_);
        break;
      }
      case PPU_THR_INFO: {
        ReadArray<PPU_Thr_Info>(&data, unit_num, &ppu_thr_info_);
        break;
      }
      case DUMP_CAUSE_INFO: {
        ReadDumpCauseInfo(&data, unit_num, unit_size);
        break;
      }
      case SPU_THRGRP_INFO: {
        size_t offset = 0;
        for (unsigned i = 0; i < unit_num; ++i) {
          ReadData<SPU_Thrgrp_Info>(&data, offset, &spu_thrgrp_info_);
          offset += sizeof(SPU_Thrgrp_Info);
          offset += sizeof(u_int32_t) * spu_thrgrp_info_[i].num_spu_thr;
        }
        break;
      }
      case SPU_THR_INFO: {
        ReadArray<SPU_Thr_Info>(&data, unit_num, &spu_thr_info_);
        break;
      }
      case PRX_INFO: {
        size_t offset = 0;
        for (unsigned i = 0; i < unit_num; ++i) {
          ReadData<PRX_Info>(&data, offset, &prx_info_);
          offset += sizeof(PRX_Info);
          offset += sizeof(PRX_Seg) * prx_info_[i].number_of_segments;
        }
        break;
      }
      case PAGE_ATTR_INFO: {
        ReadArray<Page_Attr_Info>(&data, unit_num, &page_attr_info_);
        break;
      }
      case GAME_APP_INFO: {
        Swap(data.GetData<Game_App_Info>(0), &game_app_info_);
        break;
      }
      case PPU_CALL_INFO: {
        size_t offset = 0;
        for (unsigned i = 0; i < unit_num; ++i) {
          PPU_Call_Info call_info;
          Swap(data.GetData<PPU_Call_Info>(offset), &call_info);
          offset += sizeof(PPU_Call_Info);
          u_int64_t thr_id = ElfOffTo64(call_info.ppu_thr_id);
          u_int32_t num_frames = call_info.num_frames;
          call_stack_[thr_id] = std::vector<u_int32_t>();
          for (unsigned j = 0; j < num_frames; ++j) {
            Elf_Word value;
            LoadBE(data.GetArrayElement<u_int32_t>(offset, j), &value);
            call_stack_[thr_id].push_back(value);
          } // for (unsigned j = 0; j < num_frames; ++j)
          offset += sizeof(u_int32_t) * num_frames;
        } // for (unsigned i = 0; i < unit_num; ++i)
        break;
      } // case PPU_CALL_INFO
    }
  }
  return true;
}

u_int32_t CoreDumper::GetCrashThread() {
  if (ppu_dump_excep_.size() > 0) {
    // PPU thread id has 64 bits in coredump files, but minidump file only
    // accept 32 bits thread id. From limited test, the high 32 bits are always
    // 0. So here use low 32 bits for the thread id and assert if the high 32
    // bits are always 0.
    assert(ppu_dump_excep_[0].ppu_thr_id.high == 0);
    return ppu_dump_excep_[0].ppu_thr_id.low;
  } else if (spu_dump_excep_.size() > 0) {
    return spu_dump_excep_[0].spu_thr_id;
  }
  return 0;
}

bool CoreDumper::GetThrgrpInfo(u_int32_t thrgrp_id,
                               SPU_Thrgrp_Info* thrgrp_info) {
  for (unsigned i = 0, n = spu_thrgrp_info_.size(); i < n; ++i) {
    if (spu_thrgrp_info_[i].spu_thrgrp_id == thrgrp_id) {
      thrgrp_info = &spu_thrgrp_info_[i];
      return true;
    }
  }
  return false;
}

PPU_Reg_Info* CoreDumper::GetPPURegInfo(Elf_Off thread_id) {
  for (unsigned i = 0; i < ppu_reg_info_.size(); ++i) {
    if (ppu_reg_info_[i].ppu_thr_id.low == thread_id.low &&
        ppu_reg_info_[i].ppu_thr_id.high == thread_id.high) {
      return &ppu_reg_info_[i];
    }
  }
  return NULL;
}

SPU_Reg_Info* CoreDumper::GetSPURegInfo(Elf_Word thread_id) {
  for (unsigned i = 0; i < spu_reg_info_.size(); ++i) {
    if (spu_reg_info_[i].spu_thr_id == thread_id) {
      return &spu_reg_info_[i];
    }
  }
  return NULL;
}

void CoreDumper::CopyFromProcess(void* dest,
                                 u_int64_t virtual_address,
                                 size_t length) {
  if (!core_.CopyData(dest, virtual_address, length)) {
    memset(dest, 0xab, length);
  }
}

std::vector<u_int32_t>* CoreDumper:: GetCallStack(
    u_int64_t ppu_thr_id) {
  return &call_stack_[ppu_thr_id];
}

size_t CoreDumper::GetMemorySize(u_int64_t ppu_thr_id) {
  // Get the fake memory size for ppu thread with thread id |ppu_thr_id|.
  // Give each callstack 32 bytes of memory, so the total memory size for a
  // thread is the callstack size * 32 (4 * sizeof(u_int64_t)).
  std::vector<u_int32_t>* callstack = GetCallStack(ppu_thr_id);
  return callstack->size() * 4 * sizeof(u_int64_t);
}

void CoreDumper::GetMemory(void* dest,
                           u_int64_t virtual_address,
                           u_int64_t ppu_thr_id) {
  // Make fake memory based on callstacks info from coredump to make breakpad
  // processor be able to reconstruct the callstack from this fake memory.
  // Give each callstack 32 bytes of memory. First 8 bytes is stack pointer,
  // third 8 bytes stores instruction address, second and forth 8 bytes are
  // not used.
  std::vector<u_int32_t> *callstack = GetCallStack(ppu_thr_id);
  size_t num_frames = callstack->size();
  u_int64_t* buffer = (u_int64_t*)dest;
  *buffer = virtual_address + 32;
  buffer = buffer + 4;
  for (unsigned i = 1; i < num_frames; ++i) {
    // Set stack pointer to the address of next callstack memory address.
    *buffer = virtual_address + 32 * (i + 1);
    buffer = buffer + 2;
    // Set the callstack address read from coredump to instruction address.
    // Since breakpad processor -8 for the instruction address, so +8 here.
    *buffer = (*callstack)[i] + 8;
    buffer = buffer + 2;
  }
}

bool CoreDumper::GetSrr0(u_int64_t ppu_thr_id, u_int64_t* srr0) {
  std::vector<u_int32_t> *callstack = GetCallStack(ppu_thr_id);
  if (callstack->size() > 0) {
    *srr0 = (*callstack)[0];
    return true;
  }
  return false;
}

template <typename T>
void CoreDumper::ReadData(google_breakpad::MemoryRange* data,
                          size_t offset,
                          google_breakpad::wasteful_vector<T>* infos) {
  T value;
  Swap(data->GetData<T>(offset), &value);
  infos->push_back(value);
}

template <typename T>
void CoreDumper::ReadArray(google_breakpad::MemoryRange* data,
                           u_int32_t unit_num,
                           google_breakpad::wasteful_vector<T>* infos) {
  for (unsigned i = 0; i < unit_num; ++i) {
    T value;
    Swap(data->GetArrayElement<T>(0, i), &value);
    infos->push_back(value);
  }
}

// TODO: Functions to get minidump needed info.

} // namespace crash_report
