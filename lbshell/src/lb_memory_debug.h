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

#ifndef SRC_LB_MEMORY_DEBUG_H
#define SRC_LB_MEMORY_DEBUG_H

#include "lb_memory_manager.h"
#include "lb_shell/lb_mutex.h"

#if LB_ENABLE_MEMORY_DEBUGGING == 0
#error should only be included when LB_ENABLE_MEMORY_DEBUGGING == 1
#endif

#define CRASH() ((void(*)())0)()

void CRASH_ON_NULL(void* ptr, size_t size, uint32_t alignment);

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ROUND_UP(size, boundary) (((size) + (boundary) - 1) & ~((boundary) - 1))
#define ALIGNED(address, boundary) (((address) & ((boundary) - 1)) == 0)

#if LB_MEMORY_COUNT
typedef struct {
  lb_shell_mutex_t mutex_;
  uint32_t requested_;
  uint32_t reserved_;
  uint32_t allocations_;
  uint64_t allocations_total_;
  lb_memory_info_t cached_info_;
} lb_memory_stats_t;
extern lb_memory_stats_t memory_stats;

void lb_memory_stats(lb_memory_info_t *info);
#endif

#if LB_MEMORY_DUMP_GRAPH
void lb_memory_dump_fragmentation_graph(const char *filename);
#endif

#if LB_MEMORY_GUARD && LB_GUARD_AT_FRONT
uintptr_t lb_memory_get_metadata_from_guard(uintptr_t meta_addr, uintptr_t guard_addr);
#endif

#if LB_MEMORY_GUARD
void lb_memory_guard_activate(uintptr_t guard_addr);
void lb_memory_guard_deactivate(uintptr_t guard_addr);
#endif

#if LB_MEMORY_DUMP_CALLERS
void lb_memory_dump_callers(const char *filename);

// In practice, I've seen around 75,000 allocations at rest up to around 100,000
// allocations during video playback.
#define kMaxTrackableAllocations (512 * 1024)  // uses up 4 MB
struct allocation_metadata;

// These nodes will be in contiguous memory.  Each node is either a pointer to
// the metadata for an active allocation, or it's a node in a linked list of
// unused nodes.  In this way, we can always find a free node in O(1) or return
// a node to the free list in O(1).  The first node is the head of the list.
struct TrackerNode {
  struct TrackerNode *next_free;  // NULL if metadata is not, or at list's end
  struct allocation_metadata *metadata;  // NULL if next_free is not
};

typedef struct {
  lb_shell_mutex_t mutex_;

  // a contiguous array of kMaxTrackableAllocations nodes, where node 0 is the
  // head of the free list.  this shows up as part of executable_size.
  struct TrackerNode table_[kMaxTrackableAllocations];
}  lb_memory_tracker_t;
extern lb_memory_tracker_t memory_tracker;

#endif

struct allocation_metadata {
  void *base_ptr;
  size_t size_reserved;   // reserved by us
  size_t size_requested;  // requested by the user
  size_t size_usable;     // usable by the user
  size_t alignment;
#if LB_MEMORY_GUARD
  uintptr_t guard_addr;  // can be 0 to indicate "no guard".
#endif
#if LB_MEMORY_DUMP_CALLERS
  uintptr_t caller_address;  // pointer to the stack frame that asked for memory
  struct TrackerNode *tracker_node;  // can be NULL
#endif
} __attribute__((packed));

static const size_t kMetadataSize = sizeof(struct allocation_metadata);
// kMetadataSize ranges from 20 to 68 bytes.

#endif
