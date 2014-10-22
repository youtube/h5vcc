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
// Debugging implementation of memory functions.

#ifdef LB_MACRO_MALLOC_OVERRIDE
#undef free
#undef memalign
#undef realloc
#endif

#include "lb_memory_manager.h"

#include <algorithm>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "lb_globals.h"
#include "lb_log_writer.h"
#include "lb_memory_pages.h"
#include "lb_mutex.h"
#include "oom_png.h"

#if LB_ENABLE_MEMORY_DEBUGGING

#include "lb_memory_debug.h"

// Global variables.

namespace LB {
namespace Memory {

namespace {
#if defined(__LB_ANDROID__)
const size_t kMinBoundary = sizeof(void*) * 2;
#elif defined(__LB_LINUX__)
const size_t kMinBoundary = sizeof(void*) * 2;
#elif defined(__LB_PS3__)
const size_t kMinBoundary = 16;
#elif defined(__LB_PS4__)
const size_t kMinBoundary = 16;
#elif defined(__LB_WIIU__)
const size_t kMinBoundary = 8;
#elif defined(__LB_XB1__)
const size_t kMinBoundary = 16;
#elif defined(__LB_XB360__)
const size_t kMinBoundary = 16;
#else
#error "Unimplemented platform"
#endif
// When using dlmalloc, kMinBoundary must match dlmalloc's MALLOC_ALIGNMENT.

const int kStartStackLevel = 2;
const int kEndStackLevel = 6;

const uint32_t kScribbleDataForAllocate = 0xbaadf00d;
const uint32_t kScribbleDataForDeallocate = 0xdeadbeef;
const uint32_t kScribbleDataForGuard = 0xbaadbeef;


// Allocated blocks are some arbitrary size and alignment
// (at least kMinBoundary).
// Padding is what is needed to keep data properly aligned.
// An allocation looks like:
//   | padding | metadata | guard | data | guard
// If kScribble is enabled, newly initialized memory is
// cleared to kScribbleDataForAllocate,
// freed memory is set to kScribbleDataForDeallocate, and guard bytes
// are cleared to kScribbleDataForGuard.
// Guard bytes are inserted if kAllocationGuard is enabled
// in lb_memory_manager.h

Stats s_memory_stats;
Tracker s_memory_tracker;
DelayedFreeSlots s_delayed_free_slots;
lb_shell_mutex_t s_memory_log_mutex;
// Has the memory system been initialized.
bool s_initialized;

#if defined(__LB_PS4__)
// Disable this until app is started up.
// Some very early allocations from the SDK
// cause a crash dereferencing the frame pointer.
bool s_backtrace_enabled = false;
#else
bool s_backtrace_enabled = true;
#endif

class AutoLock {
 public:
  explicit AutoLock(lb_shell_mutex_t* lock) : lock_(lock) {
    int ret = lb_shell_mutex_lock(lock_);
    assert(ret == 0);
  }

  ~AutoLock() {
    lb_shell_mutex_unlock(lock_);
  }

 private:
  lb_shell_mutex_t* lock_;
  AutoLock(const AutoLock&);
  void operator=(const AutoLock&);
};

struct AllocationSpecs {
  size_t size_aligned;
  size_t alignment;
  size_t reservation;
  size_t data_offset;
};

void WriteMemoryLog(const char *fmt, ...);

inline uintptr_t RoundUp(uintptr_t size, size_t boundary) {
  return (size + boundary - 1) & ~(boundary - 1);
}

inline uint32_t RoundUpDivide(uint32_t val, uint32_t divisor) {
  return (val + divisor - 1) / divisor;
}

inline bool IsAligned(uintptr_t addr, size_t boundary) {
  return (addr & (boundary - 1)) == 0;
}

inline bool IsAligned(void* addr, size_t boundary) {
  return IsAligned(reinterpret_cast<uintptr_t>(addr), boundary);
}

void* OffsetPointer(void *base, size_t offset) {
  uintptr_t base_as_int = reinterpret_cast<uintptr_t>(base);
  return reinterpret_cast<void*>(base_as_int + offset);
}

AllocationMetadata *GetMetadata(void *data_ptr) {
  uintptr_t data_addr = (uintptr_t)data_ptr;

  // The metadata lives just in front of the data and guard.
  uintptr_t meta_addr = data_addr - kMetadataSize - kAllocationGuardBytes;

  return reinterpret_cast<AllocationMetadata*>(meta_addr);
}

void CloseLog() {
  LogWriterStop();
}

void DumpStats() {
  if (IsCountEnabled()) {
    oom_fprintf(1, "MB requested: %.3f\n",
        static_cast<double>(s_memory_stats.requested) / 1024.0 / 1024.0);
    oom_fprintf(1, "MB reserved: %.3f\n",
        static_cast<double>(s_memory_stats.reserved) / 1024.0 / 1024.0);
    oom_fprintf(1, "unfreed allocations: %d\n",
        s_memory_stats.allocations);
    oom_fprintf(1, "lifetime total allocations: %" PRId64 "\n",
        s_memory_stats.allocations_total);
    ALLOCATOR(dump_heap)();
  }
}

void CrashOnNull(void *ptr, size_t size, uint32_t alignment) {
  if (IsOomCrashEnabled()) {
    if (!ptr) {
      oom_fprintf(1,
          "Unable to allocate %" PRIu64 " bytes with alignment of %d.\n",
          static_cast<uint64_t>(size), static_cast<int>(alignment));
      DumpStats();
      if (IsDumpGraphEnabled()) {
        DumpFragmentationGraph("oom.png");
      }
      if (IsContinuousLogEnabled()) {
        CloseLog();
      }
      CRASH();
    }
  }
}

void Scribble(char *dest, const uint32_t val, size_t size_bytes) {
  const size_t word_count = size_bytes / sizeof(val);
  const size_t bytes_remaining = size_bytes % sizeof(val);
  uint32_t* dest_as_32 = reinterpret_cast<uint32_t*>(dest);
  std::fill_n(dest_as_32, word_count, val);
  memcpy(&dest[word_count * sizeof(val)], &val, bytes_remaining);
}

void VerifyScribble(const char *src, const uint32_t val, size_t size_bytes) {
  const size_t word_count = size_bytes / sizeof(val);
  const size_t bytes_remaining = size_bytes % sizeof(val);
  const uint32_t* src_as_32 = reinterpret_cast<const uint32_t*>(src);
  for (int i = 0; i < word_count; ++i) {
    if (src_as_32[i] != val) {
      CRASH();
    }
  }
  if (memcmp(&src[word_count * sizeof(val)], &val, bytes_remaining) != 0) {
    CRASH();
  }
}

void FillGuardBytes(void *dest, size_t size) {
  char *write_ptr = static_cast<char*>(dest);
  Scribble(write_ptr - kAllocationGuardBytes, kScribbleDataForGuard,
      kAllocationGuardBytes);
  Scribble(write_ptr + size, kScribbleDataForGuard,
      kAllocationGuardBytes);
}

void VerifyGuardBytes(const char *dest, size_t size) {
  VerifyScribble(dest - kAllocationGuardBytes, kScribbleDataForGuard,
      kAllocationGuardBytes);
  VerifyScribble(dest + size, kScribbleDataForGuard,
      kAllocationGuardBytes);
}

void FreeNextSlot() {
  DelayedFreeSlot& slot =
      s_delayed_free_slots.slots[s_delayed_free_slots.next_slot_to_free];
  if (!UnlockRegion(slot.base_ptr, slot.size_reserved)) {
    CRASH();
  }
  VerifyScribble(
      static_cast<const char*>(slot.ptr) - kAllocationGuardBytes,
      kScribbleDataForDeallocate,
      slot.size_requested + kAllocationGuardBytes * 2);

  s_delayed_free_slots.delayed_free_amount -= slot.size_reserved;
  ALLOCATOR(free)(slot.base_ptr);
  s_delayed_free_slots.next_slot_to_free =
      (s_delayed_free_slots.next_slot_to_free + 1) % kDelayedFreeSlotsNum;
}

void DelayedFree(void *base_ptr, void *ptr, size_t size_reserved,
                 size_t size_requested) {
  if (s_initialized && IsDelayedFreeEnabled()) {
    if (!IsAligned(ptr, kMinBoundary)) {
      CRASH();
    }
    AutoLock lock(&s_delayed_free_slots.delayed_free_mutex);
    // If the ring buffer is full, free one slot.
    if ((s_delayed_free_slots.next_slot_to_append + 1) % kDelayedFreeSlotsNum ==
        s_delayed_free_slots.next_slot_to_free) {
      FreeNextSlot();
    }

    VerifyGuardBytes(static_cast<const char*>(ptr), size_requested);
    Scribble(static_cast<char *>(ptr) - kAllocationGuardBytes,
             kScribbleDataForDeallocate,
             size_requested + kAllocationGuardBytes * 2);
    if (size_reserved > kMaxDelayedFreeSize) {
      // Free it immediately to avoid flushing many small blocks.
      ALLOCATOR(free)(base_ptr);
    } else {
      DelayedFreeSlot& slot =
          s_delayed_free_slots.slots[s_delayed_free_slots.next_slot_to_append];
      slot.base_ptr = base_ptr;
      slot.ptr = ptr;
      slot.size_reserved = size_reserved;
      slot.size_requested = size_requested;
      s_delayed_free_slots.delayed_free_amount += size_reserved;
      s_delayed_free_slots.next_slot_to_append =
          (s_delayed_free_slots.next_slot_to_append + 1) % kDelayedFreeSlotsNum;
      if (!LockRegion(base_ptr, size_reserved))
        CRASH();
      // Check if the accumulated delayed free amount is too large.
      while (
          s_delayed_free_slots.delayed_free_amount > kMaxTotalDelayedFreeSize)
        FreeNextSlot();
    }
  } else {
    VerifyGuardBytes(static_cast<const char*>(ptr), size_requested);

    if (IsScribbleEnabled()) {
      Scribble(static_cast<char *>(ptr), kScribbleDataForDeallocate,
               size_requested);
    }
    ALLOCATOR(free)(base_ptr);
  }
}

void ComputeAllocationSpecs(AllocationSpecs *specs,
                            size_t boundary, size_t size) {
  if (!IsAligned(static_cast<uintptr_t>(kAllocationGuardBytes), boundary)) {
    CRASH();
  }
  specs->size_aligned = RoundUp(size, boundary);

  specs->alignment = boundary;

  specs->data_offset = RoundUp(kMetadataSize, specs->alignment) +
      kAllocationGuardBytes;
  specs->reservation = specs->data_offset + specs->size_aligned +
      kAllocationGuardBytes * 2;

  if (IsDelayedFreeEnabled()) {
    boundary = std::max(boundary, GetRegionLockAlignment());
    specs->reservation = RoundUp(specs->reservation, boundary);
    specs->alignment = boundary;
  }
}


void *ComputeAddresses(AllocationSpecs *specs,
                       void *base_ptr,
                       size_t boundary) {
  void *return_addr = OffsetPointer(base_ptr, specs->data_offset);
#if defined(_DEBUG)
  if (!IsAligned(return_addr, boundary)) {
    CRASH();
  }
#endif
  return return_addr;
}

void CheckAddresses(void *base_ptr,
                    AllocationMetadata *metadata,
                    AllocationSpecs *specs,
                    void *return_addr) {
#if defined(_DEBUG)
  uintptr_t first_addr = (uintptr_t)base_ptr;
  uintptr_t last_addr = first_addr + specs->reservation;
# define CHECK_ADDR(x, s) \
  { \
    if ((s) < 0) CRASH(); \
    if ((uintptr_t)(x) < first_addr) CRASH(); \
    if ((s) && ((uintptr_t)(x) >= last_addr)) CRASH(); \
    if ((uintptr_t)(x) + (s) > last_addr) CRASH(); \
  }
  CHECK_ADDR(base_ptr, specs->reservation);
  CHECK_ADDR(metadata, kMetadataSize);
  CHECK_ADDR(return_addr, specs->size_aligned);
#endif
}

void FillMetadata(AllocationMetadata *metadata,
                  void *base_ptr, size_t size, int indirections,
                  AllocationSpecs *specs) {
  // fill in metadata:
  metadata->base_ptr = base_ptr;
  metadata->size_reserved = specs->reservation;
  metadata->size_requested = size;

  // only extra space available to the user is the extra to round up to the
  // requested boundary size.
  metadata->size_usable = specs->size_aligned;

  metadata->alignment = specs->alignment;

  if (IsDumpCallersEnabled()) {
    uintptr_t caller_address = 0;
    // get the address of the function that wanted to allocate memory.
    // we skip 2 frames to cover the wrapped malloc function and this function.
    // we can be told to skip additional frames.
    if (GetBacktraceEnabled()) {
      Backtrace(kStartStackLevel + indirections, 1, &caller_address, NULL);
    }

    metadata->caller_address = caller_address;
    metadata->tracker_node = NULL;
  }
}

void Track(AllocationMetadata *metadata, int indirections) {
  if (!s_initialized) {
    return;
  }

  if (IsCountEnabled()) {
    LB::Platform::atomic_add_32(&s_memory_stats.requested,
        metadata->size_requested);
    LB::Platform::atomic_add_32(&s_memory_stats.reserved,
          metadata->size_reserved);
    LB::Platform::atomic_inc_32(&s_memory_stats.allocations);
    LB::Platform::atomic_inc_32(&s_memory_stats.allocations_total);
  }

  if (IsContinuousLogEnabled()) {
    AutoLock lock(&s_memory_log_mutex);
    WriteMemoryLog(
        "+ %" PRIxPTR " %" PRIxPTR " %" PRId32,
        metadata->base_ptr,
        metadata->size_requested,
        GetGlobalsPtr()->lifetime);

    const int kFrameCount = kEndStackLevel - kStartStackLevel + 1;
    uintptr_t caller_addresses[kFrameCount];
    if (GetBacktraceEnabled()) {
      Backtrace(kStartStackLevel + indirections,
                   kFrameCount, caller_addresses, NULL);
      for (int frame = 0; frame < kFrameCount; ++frame) {
        WriteMemoryLog(" %" PRIxPTR, caller_addresses[frame]);
      }
    }
    WriteMemoryLog("\n");
  }

  if (IsDumpCallersEnabled()) {
    AutoLock lock(&s_memory_tracker.mutex);
    // find a free node.
    TrackerNode *node = s_memory_tracker.table[0].next_free;
    if (node) {  // we could run out
      // link around this one to remove it from the free list.
      s_memory_tracker.table[0].next_free = node->next_free;
      // mark it as not free.
      node->next_free = NULL;
      // mark the metadata.
      node->metadata = metadata;
      // point back from the metadata to this node.
      metadata->tracker_node = node;
    }
  }
}

void Untrack(AllocationMetadata *metadata) {
  if (!s_initialized) {
    return;
  }

  if (IsCountEnabled()) {
    LB::Platform::atomic_sub_32(&s_memory_stats.requested,
          metadata->size_requested);
    LB::Platform::atomic_sub_32(&s_memory_stats.reserved,
          metadata->size_reserved);
    LB::Platform::atomic_dec_32(&s_memory_stats.allocations);
  }

  if (IsContinuousLogEnabled()) {
    AutoLock lock(&s_memory_log_mutex);
    WriteMemoryLog("- %" PRIxPTR "\n", metadata->base_ptr);
  }

  if (IsDumpCallersEnabled()) {
    AutoLock lock(&s_memory_tracker.mutex);
    TrackerNode *node = metadata->tracker_node;
    if (node) {  // we might have been out at the time
      // mark it as unused
      node->metadata = NULL;
      // add it back to the free list.
      node->next_free = s_memory_tracker.table[0].next_free;
      s_memory_tracker.table[0].next_free = node;
    }
  }
}


void WriteMemoryLog(const char *fmt, ...) {
  char buf[128];
  size_t n;
  va_list ap;

  va_start(ap, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  LogWriterAppend(buf, n);
}

}  // end namespace

Stats* GetStats() {
  return &s_memory_stats;
}

Tracker* GetTracker() {
  return &s_memory_tracker;
}

void LogNamedCounter(const std::string& name, uint64_t counter) {
  if (!s_initialized) {
    return;
  }

  AutoLock lock(&s_memory_log_mutex);
  WriteMemoryLog(
      "C \"%s\" %" PRIu64 " %" PRId32 "\n",
      name.c_str(),
      counter,
      GetGlobalsPtr()->lifetime);
}

void InitLogWriter() {
  LogWriterStart();
}

void InitCommon() {
  lb_shell_mutex_init(&s_memory_tracker.mutex);
  lb_shell_mutex_init(&s_memory_log_mutex);
  lb_shell_mutex_init(&s_delayed_free_slots.delayed_free_mutex);

  // initialize the table
  for (int i = 0; i < kMaxTrackableAllocations; i++) {
    s_memory_tracker.table[i].metadata = NULL;
    s_memory_tracker.table[i].next_free = &s_memory_tracker.table[i + 1];
  }
  s_memory_tracker.table[kMaxTrackableAllocations - 1].next_free = NULL;

  s_delayed_free_slots.next_slot_to_append = 0;
  s_delayed_free_slots.next_slot_to_free = 0;
  s_delayed_free_slots.delayed_free_amount = 0;

  s_initialized = true;
}

void DeinitCommon() {
  if (IsContinuousLogEnabled()) {
    CloseLog();
  }

  lb_shell_mutex_destroy(&s_delayed_free_slots.delayed_free_mutex);
  lb_shell_mutex_destroy(&s_memory_log_mutex);
  lb_shell_mutex_destroy(&s_memory_tracker.mutex);
}


void GetInfo(Info *info) {
  if (IsCountEnabled()) {
    *info = s_memory_stats.cached_info;

    size_t system_size;
    size_t in_use_size;
    if (ALLOCATOR(malloc_stats_np)(&system_size, &in_use_size) != 0) {
      CRASH();
      return;
    }

    ssize_t unallocated = lb_get_unallocated_memory();
    ssize_t allocator_unused = (ssize_t)system_size - in_use_size;

    // free_memory is what is not claimed by anyone + what the allocator has
    // claimed but not yet given out.
    info->free_memory = unallocated + allocator_unused;
    // application_memory includes the overhead of our debugging metadata,
    // but not the allocator's metadata.
    info->application_memory = s_memory_stats.reserved;
    // system_memory does not include the overhead of the allocator's metadata.
    info->system_memory = info->user_memory - in_use_size - info->free_memory;
  }
}

void DumpCallers(const char *filename) {
  if (!s_initialized) {
    return;
  }

  if (IsDumpCallersEnabled()) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s",
             GetGlobalsPtr()->screenshot_output_path, filename);
    FILE *f = fopen(path, "w");

    char buf[64];
    int i;

    AutoLock lock(&s_memory_tracker.mutex);
    // DON'T ALLOCATE INSIDE THIS BLOCK!
    for (i = 0; i < kMaxTrackableAllocations; i++) {
      AllocationMetadata *metadata = s_memory_tracker.table[i].metadata;
      if (metadata == NULL) {
        continue;
      }

      // fprintf would allocate a buffer internally,
      // so print into our own buffer which lives on the stack.
#if !defined(__LB_ANDROID__)
      size_t n = snprintf(buf, sizeof(buf), "%016" PRIxPTR" %016" PRIxPTR"\n",
          metadata->caller_address,
          metadata->size_requested);
#else
      size_t n = snprintf(buf, sizeof(buf), "%016" PRIxPTR" %016" PRIxPTR"\n",
          (uint32_t)metadata->caller_address,
          (uint32_t)metadata->size_requested);
#endif
      fwrite(buf, n, 1, f);
    }
    fclose(f);
  }
}

#if defined(__LB_ANDROID__)
// TODO: Correct size for Android
const size_t kFragGraphBlockSize = 8192;  // bytes represented by each pixel
const size_t kFragGraphAddressSpace = 2UL << 30;  // a 2GB space
#elif defined(__LB_PS3__)
const size_t kFragGraphBlockSize = 4096;  // bytes represented by each pixel
const size_t kFragGraphAddressSpace = 512 << 20;  // a 512MB space
#elif defined(__LB_PS4__)
// TODO: Correct size for PS4.
const size_t kFragGraphBlockSize = 4096;  // bytes represented by each pixel
const size_t kFragGraphAddressSpace = 768 << 20;  // a 768MB space
#elif defined(__LB_WIIU__) || defined(__LB_LINUX__)
const size_t kFragGraphBlockSize = 8192;  // bytes represented by each pixel
const size_t kFragGraphAddressSpace = 2UL << 30;  // a 2GB space
#elif defined(__LB_XB1__)
// TODO: Correct size for XB1.
const size_t kFragGraphBlockSize = 4096;  // bytes represented by each pixel
const size_t kFragGraphAddressSpace = 512 << 20;  // a 512MB space
#elif defined(__LB_XB360__)
// TODO: Correct size for XB360.
const size_t kFragGraphBlockSize = 4096;  // bytes represented by each pixel
const size_t kFragGraphAddressSpace = 512UL << 20;  // a 512MB space
#else
#error "Unimplemented platform"
#endif

const size_t kFragGraphMaxBlocks =
    (kFragGraphAddressSpace + kFragGraphBlockSize - 1) / kFragGraphBlockSize;

// compute a block number by remapping addr into the address ranges
static int CalculateGraphBlockNumber(const uintptr_t addr,
                                        const uintptr_t range_start[3],
                                        const uintptr_t range_end[3]) {
  if (addr < range_start[0])
    return kFragGraphMaxBlocks;  // invalid
  if (addr < range_end[0])
    return (addr - range_start[0]) / kFragGraphBlockSize;

  uintptr_t len0 = range_end[0] - range_start[0];
  if (addr < range_start[1])
    return kFragGraphMaxBlocks;  // invalid
  if (addr < range_end[1])
    return (len0 + addr - range_start[1]) / kFragGraphBlockSize;

  uintptr_t len1 = len0 + range_end[1] - range_start[1];
  if (addr < range_start[2])
    return kFragGraphMaxBlocks;  // invalid
  if (addr < range_end[2])
    return (len1 + addr - range_start[2]) / kFragGraphBlockSize;

  return kFragGraphMaxBlocks;  // invalid
}

void DumpFragmentationGraph(const char *filename) {
  if (!s_initialized) {
    return;
  }

  AutoLock(&s_memory_tracker.mutex);

  // note that path is static and affects neither the heap nor the stack.
  static char path[256];
  int len = snprintf(path, sizeof(path), "%s/%s",
           GetGlobalsPtr()->screenshot_output_path, filename);
  if (len == sizeof(path)) {
    assert(false && "Fragmentation graph path too long.");
    return;
  }
  // temporarily truncate this path into its parent directory name.
  char *last_slash = strrchr(path, '/');
  *last_slash = '\0';

  // create the parent directory.
  if (mkdir(path, 0755)) {
    if (errno != EEXIST) {
      return;
    }
  }

  // put the path back to its original state.
  *last_slash = '/';

  Info info;
  GetInfo(&info);

  const int width = 256;  //  each row of 4k blocks is 1MB, 8k blocks is 2MB
  // We must choose quant so that overflow does not occur on unsigned char data
  // later on.  We must satisfy kFragGraphBlockSize / size_quantum <= 128.
  // We only use the top-half of the spectrum for counting "fullness".
  const int size_quantum = (kFragGraphBlockSize / 128) + 1;

  // Get a description of the allocator's address ranges.
  uintptr_t range_start[3];
  uintptr_t range_end[3];
  ALLOCATOR(malloc_ranges_np)(&range_start[0], &range_end[0],
                              &range_start[1], &range_end[1],
                              &range_start[2], &range_end[2]);

  // Count the size of the address space.
  size_t range_size = range_end[0] - range_start[0] +
                      range_end[1] - range_start[1] +
                      range_end[2] - range_start[2];
  const int num_blocks = RoundUpDivide(range_size, kFragGraphBlockSize);
  const int height = RoundUpDivide(num_blocks, width);

  if (num_blocks > kFragGraphMaxBlocks) {
    // This would indicate that either the description of the address space is
    // wrong, or that kFragGraphMaxBlocks needs to be raised.
    CRASH();
  }

  // create an OOM-safe PNG.
  png_structp png = oom_png_create(path, width, height, 8, PNG_COLOR_TYPE_GRAY);
  if (!png) {
    return;
  }

  // compute the image data.
  // note that this data is static and affects neither the heap nor the stack.
  static unsigned char data[kFragGraphMaxBlocks];
  memset(data, 0, kFragGraphMaxBlocks);
  // walk through all tracked allocations.
  for (int i = 0; i < kMaxTrackableAllocations; i++) {
    AllocationMetadata *metadata = s_memory_tracker.table[i].metadata;
    if (metadata == NULL) {
      continue;
    }

    // this allocation starts somewhere within the block numbered block_num
    // and may extend beyond.
    size_t size = metadata->size_reserved;
    int block_num = CalculateGraphBlockNumber((uintptr_t)metadata->base_ptr,
                                                 range_start, range_end);

    if (block_num >= num_blocks) {
      // This would indicate that the description of the address space is wrong.
      continue;
    }

    // walk through every block that this allocation touches.
    while (size) {
      size_t size_in_block = std::min(size, kFragGraphBlockSize);
      // we start blocks off at 50% gray the first time they are seen in use.
      if (data[block_num] == 0) {
        data[block_num] += 0x80;
      }
      // add to this block's brightness.
      data[block_num] += (size_in_block / size_quantum);

      size -= size_in_block;
      block_num++;
    }
  }

  // Mark off data used by the OS, executable, and system libraries.
  // This data is not in the same virtual address space as the allocator,
  // but it is part of the same physical space and should count against
  // what can be allocated.
  size_t unusable_at_end = info.os_size + info.executable_size +
                           info.system_memory + info.hidden_memory;
  size_t blocks_unusable_at_end = RoundUpDivide(unusable_at_end,
                                                  kFragGraphBlockSize);
  int unusable_offset = num_blocks - blocks_unusable_at_end;
  memset(data + unusable_offset, 0x40, blocks_unusable_at_end);

  // locate the largest free space.
  uintptr_t largest_free_space_block = 0;
  size_t largest_free_space = 0;
  int current_free_space_block = 0;
  int num_free_space_blocks = 0;
  bool used = true;
  for (int i = 0; i <= num_blocks; i++) {
    if (used) {
      if (i < num_blocks && data[i] == 0) {
        used = false;
        current_free_space_block = i;
        ++num_free_space_blocks;
      }
    } else {
      if (i == num_blocks || data[i] != 0) {
        size_t current_free_space =
            (i - current_free_space_block) * kFragGraphBlockSize;
        used = true;

        if (current_free_space > largest_free_space) {
          largest_free_space = current_free_space;
          largest_free_space_block = current_free_space_block;
        }
      }
    }
  }

  // print info about the largest free space.
  oom_fprintf(1, "The largest free space is %" PRId64 " bytes at block "
      "0x%" PRIx64 ".\n",
      static_cast<int64_t>(largest_free_space),
      static_cast<int64_t>(largest_free_space_block));
  oom_fprintf(1, "Number of free regions is %" PRId64 ".\n",
      static_cast<int64_t>(num_free_space_blocks));
  if (IsCountEnabled()) {
    oom_fprintf(1, "Total free memory is %" PRId64 " bytes.\n",
        static_cast<int64_t>(info.free_memory));
    oom_fprintf(1, "Average free region size is %" PRId64 " bytes.\n",
        static_cast<int64_t>(info.free_memory) /
        static_cast<int64_t>(
            num_free_space_blocks ? num_free_space_blocks : 1));
  }

  oom_fprintf(1, "App lifetime %d minutes.\n",
      GetGlobalsPtr()->lifetime / 60000);

  // write the image data.
  unsigned char *row = data;
  for (int i = 0; i < height; i++) {
    png_write_row(png, row);
    row += width;
  }

  // finish the PNG.
  oom_png_destroy(png);
}

#if !defined(__LB_ANDROID__)
// Android provides its own implementation. These are just no-ops.
// This functions returns the alignment that an allocation has to be aligned
// with to use the lock/unlock functions. This is usually the system page size.
size_t GetRegionLockAlignment() {
  return 1;
}
// Lock memory so it will generate a SIGSEGV on access. The pointer passed in
// has to be aligned with lock boundary. Returns true on success.
bool LockRegion(void* p, size_t size) {
  return true;
}
// Unlock memory so it can be accessed.
bool UnlockRegion(void* p, size_t size) {
  return true;
}

#endif  // !defined(__LB_ANDROID__)

void* Allocate(size_t boundary, size_t size, int indirections) {
  boundary = std::max(boundary, kMinBoundary);

  AllocationSpecs specs;
  ComputeAllocationSpecs(&specs, boundary, size);
  // RoundUp() can result in integer overflow; fail if that occurs
  if (specs.size_aligned < size) {
    return NULL;
  }

  // allocate the space.
  void *base_ptr = ALLOCATOR(memalign)(specs.alignment, specs.reservation);
  CrashOnNull(base_ptr, specs.reservation, specs.alignment);

  void *return_addr = ComputeAddresses(&specs, base_ptr, boundary);

  AllocationMetadata *metadata = GetMetadata(return_addr);
  CheckAddresses(base_ptr, metadata, &specs, return_addr);

  FillMetadata(metadata, base_ptr, size, indirections, &specs);

  if (IsScribbleEnabled()) {
  Scribble(static_cast<char *>(return_addr),
           kScribbleDataForAllocate, metadata->size_requested);
  }
  FillGuardBytes(static_cast<char *>(return_addr),
                 metadata->size_requested);

  Track(metadata, indirections);

  return return_addr;
}

void* Reallocate(void *ptr, size_t size, int indirections) {
  if (!ptr) {
    // realloc(NULL, size) is the same as malloc(size).
    return lb_memory_allocate(kMinBoundary, size, indirections + 1);
  }

  // Delayed free relies on memory allocated aligned to lock boundary. However,
  // realloc doesn't have this guarantee. So explicitly use allocate and free
  // instead.
  const bool use_realloc = !IsDelayedFreeEnabled();
  if (use_realloc) {
    // get the old metadata
    AllocationMetadata *metadata = GetMetadata(ptr);

    VerifyGuardBytes(static_cast<char*>(ptr), metadata->size_requested);

    // remove the allocation from the tracking table
    Untrack(metadata);
    void *base_ptr = metadata->base_ptr;
    metadata->base_ptr = NULL;  // to detect double-free.

    // set up the new allocation, which may occupy the same address as the old
    AllocationSpecs specs;
    ComputeAllocationSpecs(&specs, kMinBoundary, size);
    // RoundUp() can result in integer overflow; fail if that occurs
    if (specs.size_aligned < size) {
      return NULL;
    }
#if defined(_DEBUG)
    // Make sure the old and new alignments are the same.
    if (specs.alignment != metadata->alignment) {
      CRASH();
    }
#endif

    // allocate the space and get the new base pointer, which may be the same
    // as the old base pointer
    void *base_ptr2 = ALLOCATOR(realloc)(base_ptr, specs.reservation);
    CrashOnNull(base_ptr2, specs.reservation, specs.alignment);

    void *return_addr = ComputeAddresses(&specs, base_ptr2, kMinBoundary);

    // get the new metadata address, which could be the same as the old one
    AllocationMetadata *metadata2 = GetMetadata(return_addr);
    CheckAddresses(base_ptr2, metadata2, &specs, return_addr);

    // fix up the new metadata
    FillMetadata(metadata2, base_ptr2, size, indirections, &specs);

    FillGuardBytes(return_addr, metadata2->size_requested);

    // set up tracking again
    Track(metadata2, indirections);

    // return the new address.
    return return_addr;
  } else {
    // NOTE: This fallback is prone to fragmentation.
    void *ptr2 = lb_memory_allocate(kMinBoundary, size, indirections + 1);
    size_t copy_size = GetMetadata(ptr)->size_requested;
    if (copy_size > size) {
      copy_size = size;
    }

    if (copy_size) {
      memcpy(ptr2, ptr, copy_size);
    }
    lb_memory_deallocate(ptr);
    return ptr2;
  }
}

void Deallocate(void* ptr) {
  if (!ptr) {
    return;  // it seems that freeing NULL is considered okay.
  }
  // get the metadata
  AllocationMetadata *metadata = GetMetadata(ptr);

#if defined(_DEBUG)
  if (!metadata->base_ptr) {
    CRASH();  // double-free
  }
#endif
  Untrack(metadata);

  void *base_ptr = metadata->base_ptr;
  metadata->base_ptr = NULL;  // to detect double-free.
  DelayedFree(base_ptr, ptr, metadata->size_reserved,
               metadata->size_requested);
}

void SetBacktraceEnabled(bool b) {
  s_backtrace_enabled = b;
}

bool GetBacktraceEnabled() {
  return s_backtrace_enabled;
}

}  // namespace Memory
}  // namespace LB

void *lb_memory_allocate(size_t boundary, size_t size, int indirections) {
  return LB::Memory::Allocate(boundary, size, indirections + 1);
}

void *lb_memory_reallocate(void *ptr, size_t size, int indirections) {
  return LB::Memory::Reallocate(ptr, size, indirections + 1);
}

void lb_memory_deallocate(void *ptr) {
  LB::Memory::Deallocate(ptr);
}

size_t lb_memory_requested_size(void *ptr) {
  if (!ptr) {
    return 0;
  }
  return LB::Memory::GetMetadata(ptr)->size_requested;
}

size_t lb_memory_usable_size(void *ptr) {
  if (!ptr) {
    return 0;
  }
  return LB::Memory::GetMetadata(ptr)->size_usable;
}

#endif  // #if LB_ENABLE_MEMORY_DEBUGGING
