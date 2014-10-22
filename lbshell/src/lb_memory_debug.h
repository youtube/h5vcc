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

#ifndef SRC_LB_MEMORY_DEBUG_H_
#define SRC_LB_MEMORY_DEBUG_H_

#include "lb_memory_manager.h"
#include "lb_mutex.h"

#if LB_ENABLE_MEMORY_DEBUGGING == 0
#error should only be included when LB_ENABLE_MEMORY_DEBUGGING == 1
#endif

#define CRASH() ((void(*)())0)()

namespace LB {
namespace Memory {

struct Stats {
  uint32_t requested;
  uint32_t reserved;
  uint32_t allocations;
  // TODO(rjogrady): This used to be uint64_t but we don't have
  // 64-bit atomic increments yet in lb_platform.h.
  uint32_t allocations_total;
  Info cached_info;
};

struct DelayedFreeSlot {
  void* base_ptr;
  void* ptr;
  size_t size_reserved;
  size_t size_requested;
};

// The maximum number of slots available to hold delayed free pointers.
static const int kDelayedFreeSlotsNum = 8192;
// We will free any memory block greater than this immediately to avoid flushing
// many small blocks.
static const int kMaxDelayedFreeSize = 48 * 1024 * 1024;
// We will do real free if the total delayed free memory amount is greater
// than the following value or if all slots are occupied.
static const int kMaxTotalDelayedFreeSize = 64 * 1024 * 1024;

struct DelayedFreeSlots {
  lb_shell_mutex_t delayed_free_mutex;
  DelayedFreeSlot slots[kDelayedFreeSlotsNum];

  // The following two variables turn the delayed_free_slots into a ring
  // buffer. When they are the same we treat the buffer as empty, so the
  // maximum capacity of the ring buffer is (kDelayedFreeSlotsNum - 1).
  int next_slot_to_append;
  int next_slot_to_free;

  // Sum of delayed free memory blocks size in bytes.
  int delayed_free_amount;
};

struct AllocationMetadata;
// These nodes will be in contiguous memory.  Each node is either a pointer to
// the metadata for an active allocation, or it's a node in a linked list of
// unused nodes.  In this way, we can always find a free node in O(1) or return
// a node to the free list in O(1).  The first node is the head of the list.
struct TrackerNode {
  struct TrackerNode *next_free;  // NULL if metadata is not, or at list's end
  struct AllocationMetadata *metadata;  // NULL if next_free is not
};

// In practice, I've seen around 75,000 allocations at rest up to around 100,000
// allocations during video playback.
static const int kMaxTrackableAllocations = 512 * 1024;  // uses up 4 MB

struct Tracker {
  lb_shell_mutex_t mutex;

  // a contiguous array of kMaxTrackableAllocations nodes, where node 0 is the
  // head of the free list.  this shows up as part of executable_size.
  struct TrackerNode table[kMaxTrackableAllocations];
};

struct AllocationMetadata {
  void *base_ptr;
  ssize_t size_reserved;   // reserved by us
  ssize_t size_requested;  // requested by the user
  ssize_t size_usable;     // usable by the user
  ssize_t alignment;
  uintptr_t caller_address;  // pointer to the stack frame that asked for memory
  struct TrackerNode *tracker_node;  // can be NULL
};

static const int kMaxModuleNameLength = 256;
struct LoadedModuleInfo {
  char name[kMaxModuleNameLength];
  uintptr_t base;  // The base address of the module
};

LB_BASE_EXPORT Stats* GetStats();
LB_BASE_EXPORT Tracker* GetTracker();

// This functions returns the alignment
// that an allocation has to be aligned
// with to use the lock/unlock functions.
// This is usually the system page size.
size_t GetRegionLockAlignment();
// Lock memory so it will generate a SIGSEGV on access.
// The pointer passed in has to be aligned with lock boundary.
// Returns true on success.
bool LockRegion(void* p, size_t size);
// Unlock memory so it can be accessed.
bool UnlockRegion(void* p, size_t size);

int Backtrace(
    uint32_t skip,
    uint32_t count,
    uintptr_t *backtraceDest,
    uint64_t *option);

// Returns the number of modules that are loaded, and fills the passed in
// modules array with information about them.
int GetLoadedModulesInfo(uint32_t max_modules,
                         LoadedModuleInfo* modules);

// kMetadataSize ranges from 20 to 68 bytes.
static const size_t kMetadataSize = sizeof(AllocationMetadata);

}  // namespace Memory
}  // namespace LB

#endif  // SRC_LB_MEMORY_DEBUG_H_
