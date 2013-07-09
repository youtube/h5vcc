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

#include "minidump_writer.h"

#include <assert.h>
#include <stdio.h>

#include "external/chromium/breakpad/src/client/linux/handler/minidump_descriptor.h"
#include "external/chromium/breakpad/src/common/linux/linux_libc_support.h"

namespace crash_report {

MinidumpWriter::MinidumpWriter(const char* minidump_path,
                               CoreDumper* dumper,
                               SymbolInfo* symbol_info)
    : path_(minidump_path),
      dumper_(dumper),
      symbol_info_(symbol_info),
      memory_blocks_(dumper_->allocator()) {
  assert(minidump_path);
}

MinidumpWriter::~MinidumpWriter() {
  minidump_writer_.Close();
}

bool MinidumpWriter::Init() {
  if (!dumper_->Init())
    return false;
  if (!symbol_info_->Init())
    return false;
  if (!minidump_writer_.Open(path_))
    return false;
  return true;
}

bool MinidumpWriter::Dump() {
  // A minidump file contains a number of tagged streams. This is the number
  // of stream which we write.
  unsigned kNumWriters = 5;

  google_breakpad::TypedMDRVA<MDRawHeader> header(&minidump_writer_);
  google_breakpad::TypedMDRVA<MDRawDirectory> dir(&minidump_writer_);
  if (!header.Allocate())
    return false;
  if (!dir.AllocateArray(kNumWriters))
    return false;
  my_memset(header.get(), 0, sizeof(MDRawHeader));

  header.get()->signature = MD_HEADER_SIGNATURE;
  header.get()->version = MD_HEADER_VERSION;
  header.get()->time_date_stamp = time(NULL);
  header.get()->stream_count = kNumWriters;
  header.get()->stream_directory_rva = dir.position();

  unsigned dir_index = 0;
  MDRawDirectory dirent;

  if (!WriteThreadListStream(&dirent))
    return false;
  dir.CopyIndex(dir_index++, &dirent);

  if (!WriteModuleListStream(&dirent))
    return false;
  dir.CopyIndex(dir_index++, &dirent);

  if (!WriteMemoryListStream(&dirent))
    return false;
  dir.CopyIndex(dir_index++, &dirent);

  if (!WriteExceptionStream(&dirent))
    return false;
  dir.CopyIndex(dir_index++, &dirent);

  if (!WriteSystemInfoStream(&dirent))
    return false;
  dir.CopyIndex(dir_index++, &dirent);

  return true;
}

// First step: only consider ppu.
bool MinidumpWriter::WriteThreadListStream(MDRawDirectory* dirent) {
  google_breakpad::wasteful_vector<PPU_Thr_Info>* ppu_thrs =
      dumper_->GetPPUThrInfo();
  google_breakpad::wasteful_vector<SPU_Thr_Info>* spu_thrs =
      dumper_->GetSPUThrInfo();
  const unsigned num_ppu_threads = ppu_thrs->size();
  const unsigned num_spu_threads = spu_thrs->size();
  const unsigned num_threads = num_ppu_threads + num_spu_threads;

  google_breakpad::TypedMDRVA<uint32_t> list(&minidump_writer_);
  if (!list.AllocateObjectAndArray(num_threads, sizeof(MDRawThread)))
    return false;

  dirent->stream_type = MD_THREAD_LIST_STREAM;
  dirent->location = list.location();

  *list.get() = num_threads;

  // Write PPU thread list.
  for (unsigned i = 0; i < num_ppu_threads; ++i) {
    MDRawThread thread;
    PPU_Thr_Info thr_info = (*ppu_thrs)[i];
    my_memset(&thread, 0, sizeof(thread));
    u_int64_t thr_id = ElfOffTo64(thr_info.ppu_thr_id);
    // PPU Thread id is 64 bits, but minidump uses 32 bits for thread id.
    thread.thread_id = thr_info.ppu_thr_id.low;
    thread.suspend_count = (thr_info.state >= SLEEP);
    thread.priority_class = (thr_info.base_priority);
    thread.priority = (thr_info.priority);

    u_int64_t stack_addr = ElfOffTo64(thr_info.stack_addr);
    u_int64_t stack_size = ElfOffTo64(thr_info.stack_size);
    thread.stack.start_of_memory_range = stack_addr;

    if (thread.thread_id == dumper_->GetCrashThread()) {
      google_breakpad::UntypedMDRVA memory(&minidump_writer_);
      if (!memory.Allocate(stack_size))
        return false;
      uint8_t* stack_copy = reinterpret_cast<uint8_t*>(Alloc(stack_size));
      dumper_->CopyFromProcess(stack_copy, stack_addr, stack_size);
      memory.Copy(stack_copy, stack_size);
      thread.stack.memory = memory.location();
      memory_blocks_.push_back(thread.stack);
    } else {
      size_t stack_size = dumper_->GetMemorySize(thr_id);
      if (stack_size > 0) {
        google_breakpad::UntypedMDRVA memory(&minidump_writer_);
        if (!memory.Allocate(stack_size))
          return false;
        uint8_t* stack_copy = reinterpret_cast<uint8_t*>(Alloc(stack_size));
        dumper_->GetMemory(stack_copy, stack_addr, thr_id);
        memory.Copy(stack_copy, stack_size);
        thread.stack.memory = memory.location();
        memory_blocks_.push_back(thread.stack);
      } else {
        thread.stack.memory.data_size = 0;
        thread.stack.memory.rva = minidump_writer_.position();
      }
    }

    google_breakpad::TypedMDRVA<MDRawContextPPC64> ppu(&minidump_writer_);
    if (!ppu.Allocate())
      return false;
    my_memset(ppu.get(), 0, sizeof(MDRawContextPPC64));
    ppu.get()->context_flags = MD_CONTEXT_PPC64_FULL;
    PPU_Reg_Info* ppu_reg = dumper_->GetPPURegInfo(thr_info.ppu_thr_id);
    if (ppu_reg) {
      ppu.get()->srr0 = ElfOffTo64(ppu_reg->pc);
      ppu.get()->srr1 = 0;
      for (unsigned i = 0; i < PPU_REG_NGPR; ++i) {
        ppu.get()->gpr[i] = ElfOffTo64(ppu_reg->gpr[i]);
      }
      ppu.get()->cr = ppu_reg->cr;
      ppu.get()->xer = ElfOffTo64(ppu_reg->xer);
      ppu.get()->lr = ElfOffTo64(ppu_reg->lr);
      ppu.get()->ctr = ElfOffTo64(ppu_reg->ctr);
      for (unsigned i = 0; i < PPU_REG_NFPR; ++i) {
        ppu.get()->float_save.fpregs[i] = ElfOffTo64(ppu_reg->fpr[i]);
      }
      ppu.get()->float_save.fpscr = ppu_reg->fpscr;
      for (unsigned i = 0; i < PPU_REG_NVMX; ++i) {
        ppu.get()->vector_save.save_vr[i] = ElfLongOffTo128(ppu_reg->vmx[i]);
      }
      ppu.get()->vector_save.save_vscr = ElfLongOffTo128(ppu_reg->vscr);
      thread.thread_context = ppu.location();
      if (thread.thread_id == dumper_->GetCrashThread()) {
        crashing_thread_context_ = ppu.location();
      }
    } else {
      dumper_->GetSrr0(ElfOffTo64(thr_info.ppu_thr_id), &ppu.get()->srr0);
      ppu.get()->gpr[1] = stack_addr;
      thread.thread_context = ppu.location();
    }

    list.CopyIndexAfterObject(i, &thread, sizeof(thread));
  }

  // Write SPU thread list.
  for (unsigned i = 0; i < num_spu_threads; ++i) {
    MDRawThread thread;
    SPU_Thr_Info thr_info = (*spu_thrs)[i];
    SPU_Thrgrp_Info thrgrp_info;
    if (!dumper_->GetThrgrpInfo(thr_info.spu_thrgrp_id, &thrgrp_info))
      return false;
    my_memset(&thread, 0, sizeof(thread));
    thread.thread_id = thr_info.spu_thr_id;
    thread.suspend_count = (thrgrp_info.state == SUSPENDED ||
                            thrgrp_info.state == WAITING_SUSPENDED ||
                            thrgrp_info.state == STOPPED);
    thread.priority_class = (thrgrp_info.priority);
    thread.priority = (thrgrp_info.priority);
    list.CopyIndexAfterObject(i + num_ppu_threads, &thread, sizeof(thread));
  }

  return true;
}

bool MinidumpWriter::WriteModuleListStream(MDRawDirectory* dirent) {
  google_breakpad::TypedMDRVA<uint32_t> list(&minidump_writer_);
  int num_modules = 1;
  if (!list.AllocateObjectAndArray(num_modules, MD_MODULE_SIZE))
    return false;

  dirent->stream_type = MD_MODULE_LIST_STREAM;
  dirent->location = list.location();
  *list.get() = num_modules;

  MDRawModule module;
  my_memset(&module, 0, MD_MODULE_SIZE);
  module.base_of_image = 0x00000000;
  module.size_of_image = 0xffffffff;

  const char *symbol_name = symbol_info_->GetSymbolName();
  MDGUID symbol_id = symbol_info_->GetId();

  MDLocationDescriptor module_name_ld;
  if (!minidump_writer_.WriteString(symbol_name,
                                    strlen(symbol_name),
                                    &module_name_ld))
    return false;
  module.module_name_rva = module_name_ld.rva;

  google_breakpad::UntypedMDRVA cv_record(&minidump_writer_);
  if (!cv_record.Allocate(MDCVInfoPDB70_minsize + strlen(symbol_name) + 1))
    return false;
  uint8_t cv_buf[MDCVInfoPDB70_minsize + NAME_MAX];
  uint8_t* cv_ptr = cv_buf;
  const uint32_t cv_signature = MD_CVINFOPDB70_SIGNATURE;
  my_memcpy(cv_ptr, &cv_signature, sizeof(cv_signature));
  cv_ptr += sizeof(cv_signature);
  my_memcpy(cv_ptr, &symbol_id, sizeof(MDGUID));
  cv_ptr += sizeof(MDGUID);
  my_memset(cv_ptr, 0, sizeof(uint32_t)); // Set age to 0 on Linux.
  cv_ptr += sizeof(uint32_t);
  my_memcpy(cv_ptr, symbol_name, strlen(symbol_name) + 1);
  cv_record.Copy(cv_buf, MDCVInfoPDB70_minsize + strlen(symbol_name) + 1);

  module.cv_record = cv_record.location();
  list.CopyIndexAfterObject(0, &module, MD_MODULE_SIZE);
  return true;
}

bool MinidumpWriter::WriteMemoryListStream(MDRawDirectory* dirent) {
  google_breakpad::TypedMDRVA<uint32_t> list(&minidump_writer_);
  if (memory_blocks_.size()) {
    if (!list.AllocateObjectAndArray(memory_blocks_.size(),
                                     sizeof(MDMemoryDescriptor)))
      return false;
    } else {
      // Still create the memory list stream, although it will have zero
      // memory blocks.
      if (!list.Allocate())
        return false;
    }

  dirent->stream_type = MD_MEMORY_LIST_STREAM;
  dirent->location = list.location();

  *list.get() = memory_blocks_.size();
  for (unsigned i = 0; i < memory_blocks_.size(); ++i) {
    list.CopyIndexAfterObject(i, &memory_blocks_[i],
                              sizeof(MDMemoryDescriptor));
  }
  return true;
}

u_int64_t ConvertPPUExceptionToMdException(u_int64_t excep) {
  switch(excep) {
    case PPU_TRAP_EXCEP: return MD_EXCEPTION_CODE_PS3_TRAP_EXCEP;
    case PPU_PRIV_INSTR: return MD_EXCEPTION_CODE_PS3_PRIV_INSTR;
    case PPU_ILLEGAL_INSTR: return MD_EXCEPTION_CODE_PS3_ILLEGAL_INSTR;
    case PPU_INSTR_STORAGE: return MD_EXCEPTION_CODE_PS3_INSTR_STORAGE;
    case PPU_INSTR_SEGMENT: return MD_EXCEPTION_CODE_PS3_INSTR_SEGMENT;
    case PPU_DATA_STORAGE: return MD_EXCEPTION_CODE_PS3_DATA_STORAGE;
    case PPU_DATA_SEGMENT: return MD_EXCEPTION_CODE_PS3_DATA_SEGMENT;
    case PPU_FLOAT_POINT: return MD_EXCEPTION_CODE_PS3_FLOAT_POINT;
    case PPU_DABR_MATCH: return MD_EXCEPTION_CODE_PS3_DABR_MATCH;
    case PPU_ALIGN_EXCEP: return MD_EXCEPTION_CODE_PS3_ALIGN_EXCEP;
    case PPU_MEMORY_ACCESS: return MD_EXCEPTION_CODE_PS3_MEMORY_ACCESS;
    default: return MD_EXCEPTION_CODE_PS3_UNKNOWN;
  }
}

u_int64_t ConvertSPUExceptionToMDException(u_int32_t excep) {
  switch(excep) {
    case SPU_MDA_ALIGN: return MD_EXCEPTION_CODE_PS3_COPRO_ALIGN;
    case SPU_INVALID_DMACOM: return MD_EXCEPTION_CODE_PS3_COPRO_INVALID_COM;
    case SPU_SPU_ERR: return MD_EXCEPTION_CODE_PS3_COPRO_ERR;
    case SPU_MFC_FIR: return MD_EXCEPTION_CODE_PS3_COPRO_FIR;
    case SPU_DATA_SEGMENT: return MD_EXCEPTION_CODE_PS3_COPRO_DATA_SEGMENT;
    case SPU_DATA_STORAGE: return MD_EXCEPTION_CODE_PS3_COPRO_DATA_STORAGE;
    case SPU_STOP_INSTR: return MD_EXCEPTION_CODE_PS3_COPRO_STOP_INSTR;
    case SPU_HALT_INSTR: return MD_EXCEPTION_CODE_PS3_COPRO_HALT_INSTR;
    case SPU_HALTINST_UNKNOWN:
        return MD_EXCEPTION_CODE_PS3_COPRO_HALTINST_UNKNOWN;
    case SPU_MEMORY_ACCESS: return MD_EXCEPTION_CODE_PS3_COPRO_MEMORY_ACCESS;
    default: return MD_EXCEPTION_CODE_PS3_UNKNOWN;
  }
}

bool MinidumpWriter::WriteExceptionStream(MDRawDirectory* dirent) {
  google_breakpad::TypedMDRVA<MDRawExceptionStream> exc(&minidump_writer_);
  if (!exc.Allocate())
    return false;
  my_memset(exc.get(), 0, sizeof(MDRawExceptionStream));

  dirent->stream_type = MD_EXCEPTION_STREAM;
  dirent->location = exc.location();

  // TODO: Add SPU dump exceps.
  google_breakpad::wasteful_vector<PPU_Dump_Excep>* ppu_dump_excep =
      dumper_->GetPPUDumpExcep();
  google_breakpad::wasteful_vector<SPU_Dump_Excep>* spu_dump_excep =
      dumper_->GetSPUDumpExcep();
  google_breakpad::wasteful_vector<RSX_Dump_Excep>* rsx_dump_excep =
      dumper_->GetRSXDumpExcep();

  if ((ppu_dump_excep->size()) > 0) {
    PPU_Dump_Excep dump_excep = (*ppu_dump_excep)[0];
    exc.get()->thread_id = dump_excep.ppu_thr_id.low;
    exc.get()->exception_record.exception_code =
      ConvertPPUExceptionToMdException(ElfOffTo64(dump_excep.excep_type));
    exc.get()->exception_record.exception_address =
      ElfOffTo64(dump_excep.dar);
    exc.get()->thread_context = crashing_thread_context_;
  } else if ((spu_dump_excep->size()) > 0) {
    SPU_Dump_Excep dump_excep = (*spu_dump_excep)[0];
    exc.get()->thread_id = dump_excep.spu_thr_id;
    exc.get()->exception_record.exception_code =
      ConvertSPUExceptionToMDException(dump_excep.excep_type);
  } else if ((rsx_dump_excep->size()) > 0) {
    RSX_Dump_Excep dump_excep = (*rsx_dump_excep)[0];
    exc.get()->exception_record.exception_code =
      MD_EXCEPTION_CODE_PS3_GRAPHIC;
  }

  return true;
}

bool MinidumpWriter::WriteSystemInfoStream(MDRawDirectory* dirent) {
  google_breakpad::TypedMDRVA<MDRawSystemInfo> si(&minidump_writer_);
  if (!si.Allocate())
    return false;
  my_memset(si.get(), 0, sizeof(MDRawSystemInfo));

  dirent->stream_type = MD_SYSTEM_INFO_STREAM;
  dirent->location = si.location();

  System_Info* system_info = dumper_->GetSystemInfo();
  si.get()->processor_architecture = MD_CPU_ARCHITECTURE_PPC64;
  si.get()->processor_level = 1;
  si.get()->major_version = system_info->sys_version;
  si.get()->platform_id = MD_OS_PS3;
  return true;
}

bool WriteMinidump(const char* filename,
                   CoreDumper* dumper,
                   SymbolInfo* symbol_info) {
  MinidumpWriter writer(filename, dumper, symbol_info);
  if (!writer.Init())
    return false;
  return writer.Dump();
}

} // namespace crash_report
