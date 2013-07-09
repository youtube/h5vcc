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

#include "lb_memory_manager.h"

#include <fcntl.h>
#include <sys/stat.h>

#include "lb_memory_internal.h"
#include "lb_memory_pages.h"
#include "lb_shell/lb_mutex.h"
#include "oom_png.h"

#if LB_ENABLE_MEMORY_DEBUGGING

#include "lb_memory_debug.h"
#include "lb_memory_debug_platform.h"

// Global variables.

#if defined(__LB_PS3__)
static const int kMinBoundary = 16;
// Guard pages must be 4k-aligned and 4k-wide.
static const int kGuardBoundary = 4096;
static const int kGuardSize = 4096;
#elif defined(__LB_WIIU__)
static const int kMinBoundary = 8;
#elif defined(__LB_LINUX__)
static const int kMinBoundary = sizeof(void *) * 2;
#endif
// When using dlmalloc, kMinBoundary must match dlmalloc's MALLOC_ALIGNMENT.


#if LB_CONTINUOUS_MEMORY_LOG || LB_MEMORY_DUMP_CALLERS
static const int kStartStackLevel = 2;
#endif
#if LB_CONTINUOUS_MEMORY_LOG
static const int kEndStackLevel = 6;
#endif

#define SCRIBBLE_DATA_FOR_ALLOCATE   "\xba\xad\xf0\x0d"
#define SCRIBBLE_DATA_FOR_DEALLOCATE "\xde\xad\xbe\xef"

#if LB_MEMORY_COUNT
lb_memory_stats_t memory_stats;
#endif
#if LB_MEMORY_DUMP_CALLERS
lb_memory_tracker_t memory_tracker;
#endif

extern const char *global_screenshot_output_path;
extern uint64_t global_webkit_frame_counter;
extern int global_lifetime;  // minutes

#if LB_CONTINUOUS_MEMORY_LOG
static lb_shell_mutex_t s_memory_log_mutex;
static int memory_log = -1;  // memory log file
static char memory_log_buf[8192];
static size_t memory_log_fullness = 0;

void lb_init_memory_log() {
  if (memory_log < 0) {
    memory_log = open(MEMORY_LOG_PATH"/memory_log.txt",
                      O_WRONLY|O_CREAT|O_TRUNC, 0644);
  }
}

static void write_memory_log(const char *fmt, ...) {
  if (memory_log >= 0) {
    char buf[128];
    size_t n;
    va_list ap;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (memory_log_fullness + n > sizeof(memory_log_buf)) {
      write(memory_log, memory_log_buf, memory_log_fullness);
      memory_log_fullness = 0;
    }

    memcpy(memory_log_buf + memory_log_fullness, buf, n);
    memory_log_fullness += n;
  }
}

void lb_close_memory_log() {
  if (memory_log >= 0) {
    // flush data
    if (memory_log_fullness)
      write(memory_log, memory_log_buf, memory_log_fullness);
    close(memory_log);
    memory_log_fullness = 0;
    memory_log == -1;
  }
}
#endif

// Guard is 4k wide and 4k-aligned.
// Data is some arbitrary size and has it's own alignment (at least 16).
// Padding is what is needed to keep guard and data properly aligned.
// An unguarded allocation looks like:
//   | padding | metadata | data |
// A rear-guarded allocation looks like:
//   | padding | metadata | data | guard |
// A front-guarded allocation looks like:
//   | padding | metadata | guard | data |

static inline struct allocation_metadata *get_metadata(void *data_ptr) {
  uintptr_t data_addr = (uintptr_t)data_ptr;

  // To start, assume that the metadata lives just behind the data.
  uintptr_t meta_addr = data_addr - kMetadataSize;

#if LB_MEMORY_GUARD && LB_GUARD_AT_FRONT
  // There may or may not be a front guard there.  This will affect the address
  // of the metadata.
  uintptr_t guard_addr = data_addr - kGuardSize;
  if (ALIGNED(guard_addr, kGuardSize)) {
    // Potentially a valid guard address based on alignment.
    // Let's query the system to see if we're watching it.
    meta_addr = lb_memory_get_metadata_from_guard(meta_addr, guard_addr);
  }
#else
  // The presence of a rear guard won't affect the metadata addr.
#endif

  return (struct allocation_metadata *)meta_addr;
}

void lb_memory_init_common() {

#if LB_MEMORY_DUMP_CALLERS
  int i;
  // initialize the table
  for (i = 0; i < kMaxTrackableAllocations; i++) {
    memory_tracker.table_[i].metadata = NULL;
    memory_tracker.table_[i].next_free = &memory_tracker.table_[i + 1];
  }
  memory_tracker.table_[kMaxTrackableAllocations - 1].next_free = NULL;

  lb_shell_mutex_init(&memory_tracker.mutex_);
#endif

#if LB_MEMORY_COUNT
  lb_shell_mutex_init(&memory_stats.mutex_);
#endif

#if LB_CONTINUOUS_MEMORY_LOG
  // NOTE: MEMORY_LOG_PATH must be known at compile-time,
  // since this executes before any memory allocations can occur.
  lb_shell_mutex_init(&s_memory_log_mutex);
#if !defined(__LB_WIIU__)
  // on wiiu this function needs to be called when file system is initialized.
  lb_init_memory_log();
#endif
#endif
}

void lb_memory_deinit_common() {
#if LB_MEMORY_COUNT
  lb_shell_mutex_destroy(&memory_stats.mutex_);
#endif

#if LB_MEMORY_DUMP_CALLERS
  lb_shell_mutex_destroy(&memory_tracker.mutex_);
#endif

#if LB_CONTINUOUS_MEMORY_LOG
  lb_shell_mutex_destroy(&s_memory_log_mutex);
#endif
}

static inline void scribble(char *dest, const char *message, size_t size) {
  size_t len = strlen(message);

  while (size > len) {
    memcpy(dest, message, len);
    dest += len;
    size -= len;
  }

  if (size) {
    memcpy(dest, message, size);
  }
}

struct allocation_specs {
  size_t size_aligned;
  size_t alignment;
  size_t guard_size;
  size_t guard_offset;
  size_t reservation;
  size_t data_offset;
};

struct allocation_addresses {
  uintptr_t return_addr;
  void *return_ptr;
#if LB_MEMORY_GUARD
  uintptr_t guard_addr;
#endif
};

static inline void compute_allocation_specs(struct allocation_specs *specs,
                                            size_t boundary, size_t size,
                                            int do_guard) {
  specs->size_aligned = ROUND_UP(size, boundary);

#if LB_MEMORY_GUARD
  if (do_guard) {
    // front guard page or rear guard page
    specs->alignment = MAX(boundary, kGuardBoundary);
    specs->guard_size = kGuardSize;
  } else
#endif
  {
    // no guard page
    specs->alignment = boundary;
    specs->guard_size = 0;
  }

#if LB_MEMORY_GUARD && !LB_GUARD_AT_FRONT
  if (do_guard) {
    // rear guard page
    specs->guard_offset = ROUND_UP(kMetadataSize + specs->size_aligned,
                                   specs->alignment);
    specs->reservation = specs->guard_offset + specs->guard_size;
    specs->data_offset = specs->guard_offset - specs->size_aligned;
  } else
#endif
  {
    // front guard page or no guard page
    specs->data_offset = ROUND_UP(kMetadataSize + specs->guard_size,
                                  specs->alignment);
    specs->reservation = specs->data_offset + specs->size_aligned;
    specs->guard_offset = specs->data_offset - specs->guard_size;
  }
}

static inline void compute_addresses(struct allocation_addresses *addresses,
                                     struct allocation_specs *specs,
                                     void *base_ptr,
                                     size_t boundary, int do_guard) {
  addresses->return_addr = ((uintptr_t)base_ptr) + specs->data_offset;
  addresses->return_ptr = (void *)addresses->return_addr;

#if defined(_DEBUG)
  if (!ALIGNED(addresses->return_addr, boundary)) CRASH();  // mis-aligned data
#endif

#if LB_MEMORY_GUARD
  // compute the guard address.
  addresses->guard_addr =
      do_guard ? ((uintptr_t)base_ptr) + specs->guard_offset : 0;

  // activate the guard area.
  // IMPORTANT!  Must come before get_metadata!
  if (do_guard) {
    lb_memory_guard_activate(addresses->guard_addr);
  }
#endif
}

static inline void check_addresses(void *base_ptr,
                                   struct allocation_metadata *metadata,
                                   struct allocation_specs *specs,
                                   struct allocation_addresses *addresses) {
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
  CHECK_ADDR(addresses->return_ptr, specs->size_aligned);
# if LB_MEMORY_GUARD
  if (addresses->guard_addr) CHECK_ADDR(addresses->guard_addr, kGuardSize);
# endif
#endif
}

static inline void fill_metadata(struct allocation_metadata *metadata,
                                 void *base_ptr, size_t size, int indirections,
                                 struct allocation_specs *specs,
                                 struct allocation_addresses *addresses) {
  // fill in metadata:
  metadata->base_ptr = base_ptr;
  metadata->size_reserved = specs->reservation;
  metadata->size_requested = size;

  // only extra space available to the user is the extra to round up to the
  // requested boundary size.
  metadata->size_usable = specs->size_aligned;

  metadata->alignment = specs->alignment;

#if LB_MEMORY_GUARD
  metadata->guard_addr = addresses->guard_addr;
#endif

#if LB_MEMORY_DUMP_CALLERS
  uintptr_t caller_address = 0;
  // get the address of the function that wanted to allocate memory.
  // we skip 2 frames to cover the wrapped malloc function and this function.
  // we can be told to skip additional frames.
  lb_backtrace(kStartStackLevel + indirections, 1, &caller_address, NULL);

  metadata->caller_address = caller_address;
  metadata->tracker_node = NULL;
#endif
}

static inline void track(struct allocation_metadata *metadata,
                         int indirections) {
#if LB_MEMORY_COUNT
  lb_shell_mutex_lock(&memory_stats.mutex_);
  memory_stats.requested_ += metadata->size_requested;
  memory_stats.reserved_ += metadata->size_reserved;
  memory_stats.allocations_++;
  memory_stats.allocations_total_++;
  lb_shell_mutex_unlock(&memory_stats.mutex_);
#endif

#if LB_CONTINUOUS_MEMORY_LOG
  lb_shell_mutex_lock(&s_memory_log_mutex);
  write_memory_log(
      "+ %"PRIx32" %"PRIx32" %"PRId64,
      (uint32_t)metadata->base_ptr,
      (uint32_t)metadata->size_requested,
      (uint64_t)global_webkit_frame_counter);

  const int frame_count = kEndStackLevel - kStartStackLevel + 1;
  uintptr_t caller_addresses[frame_count];

  lb_backtrace(kStartStackLevel + indirections,
               frame_count, caller_addresses, NULL);

  for (int frame = 0; frame < frame_count; ++frame) {
    write_memory_log(" %"PRIx32, caller_addresses[frame]);
  }
  write_memory_log("\n");
  lb_shell_mutex_unlock(&s_memory_log_mutex);
#endif

#if LB_MEMORY_DUMP_CALLERS
  lb_shell_mutex_lock(&memory_tracker.mutex_);
  // find a free node.
  struct TrackerNode *node = memory_tracker.table_[0].next_free;
  if (node) {  // we could run out
    // link around this one to remove it from the free list.
    memory_tracker.table_[0].next_free = node->next_free;
    // mark it as not free.
    node->next_free = NULL;
    // mark the metadata.
    node->metadata = metadata;
    // point back from the metadata to this node.
    metadata->tracker_node = node;
  }
  lb_shell_mutex_unlock(&memory_tracker.mutex_);
#endif
}

static inline void untrack(struct allocation_metadata *metadata) {
#if LB_MEMORY_COUNT
  lb_shell_mutex_lock(&memory_stats.mutex_);
  memory_stats.requested_ -= metadata->size_requested;
  memory_stats.reserved_ -= metadata->size_reserved;
  memory_stats.allocations_--;
  lb_shell_mutex_unlock(&memory_stats.mutex_);
#endif

#if LB_CONTINUOUS_MEMORY_LOG
  lb_shell_mutex_lock(&s_memory_log_mutex);
  write_memory_log(
      "- %"PRIx32"\n",
      (uint32_t)metadata->base_ptr);
  lb_shell_mutex_unlock(&s_memory_log_mutex);
#endif

#if LB_MEMORY_DUMP_CALLERS
  lb_shell_mutex_lock(&memory_tracker.mutex_);
  struct TrackerNode *node = metadata->tracker_node;
  if (node) {  // we might have been out at the time
    // mark it as unused
    node->metadata = NULL;
    // add it back to the free list.
    node->next_free = memory_tracker.table_[0].next_free;
    memory_tracker.table_[0].next_free = node;
  }
  lb_shell_mutex_unlock(&memory_tracker.mutex_);
#endif
}

// NOTE: There is no guard() to mirror unguard(), because the guard
// activation MUST come before any calls to get_metadata.  So guards
// are activated in compute_addresses().
static inline void unguard(struct allocation_metadata *metadata) {
#if LB_MEMORY_GUARD
  if (metadata->guard_addr) {
    lb_memory_guard_deactivate(metadata->guard_addr);
    metadata->guard_addr = 0;
  }
#endif
}

void *lb_memory_allocate(size_t boundary, size_t size, int do_guard,
                         int indirections) {
  boundary = MAX(boundary, kMinBoundary);

  struct allocation_specs specs;
  compute_allocation_specs(&specs, boundary, size, do_guard);
  // ROUND_UP() can result in integer overflow; fail if that occurs
  if (specs.size_aligned < size) return NULL;

  // allocate the space.
  void *base_ptr = ALLOCATOR(memalign)(specs.alignment, specs.reservation);
  CRASH_ON_NULL(base_ptr, specs.reservation, specs.alignment);

  struct allocation_addresses addresses;
  compute_addresses(&addresses, &specs, base_ptr, boundary, do_guard);

  struct allocation_metadata *metadata = get_metadata(addresses.return_ptr);
  check_addresses(base_ptr, metadata, &specs, &addresses);

  fill_metadata(metadata, base_ptr, size, indirections, &specs, &addresses);

#if LB_MEMORY_SCRIBBLE
  scribble((char *)addresses.return_ptr, SCRIBBLE_DATA_FOR_ALLOCATE,
           metadata->size_usable);
#endif

  track(metadata, indirections);

  return addresses.return_ptr;
}

void *lb_memory_reallocate(void *ptr, size_t size,
                           int do_guard, int indirections) {
  if (!ptr) {
    // realloc(NULL, size) is the same as malloc(size).
    return lb_memory_allocate(kMinBoundary, size, do_guard, indirections + 1);
  }

#if ALLOCATOR_HAS_REALLOC
  // get the old metadata
  struct allocation_metadata *metadata = get_metadata(ptr);

  // tear down the guard and remove the allocation from the tracking table
  unguard(metadata);
  untrack(metadata);
  void *base_ptr = metadata->base_ptr;
  metadata->base_ptr = NULL;  // to detect double-free.

  // set up the new allocation, which may occupy the same address as the old
  struct allocation_specs specs;
  compute_allocation_specs(&specs, kMinBoundary, size, do_guard);
  // ROUND_UP() can result in integer overflow; fail if that occurs
  if (specs.size_aligned < size) return NULL;
#if defined(_DEBUG)
  // Make sure the old and new alignments are the same.
  if (specs.alignment != metadata->alignment) CRASH();
#endif

  // allocate the space and get the new base pointer, which may be the same
  // as the old base pointer
  void *base_ptr2 = ALLOCATOR(realloc)(base_ptr, specs.reservation);
  CRASH_ON_NULL(base_ptr2, specs.reservation, specs.alignment);

  struct allocation_addresses addresses;
  compute_addresses(&addresses, &specs, base_ptr2, kMinBoundary, do_guard);

  // get the new metadata address, which could be the same as the old one
  struct allocation_metadata *metadata2 = get_metadata(addresses.return_ptr);
  check_addresses(base_ptr2, metadata2, &specs, &addresses);

  // fix up the new metadata
  fill_metadata(metadata2, base_ptr2, size, indirections, &specs, &addresses);

  // set up tracking again
  track(metadata2, indirections);

  // return the new address.
  return addresses.return_ptr;
#else
  // NOTE: This fallback is prone to fragmentation.
  void *ptr2 = lb_memory_allocate(kMinBoundary, size, do_guard,
                                  indirections + 1);
  size_t copy_size = get_metadata(ptr)->size_requested;
  if (copy_size > size) {
    copy_size = size;
  }

  if (copy_size) {
    memcpy(ptr2, ptr, copy_size);
  }
  lb_memory_deallocate(ptr);
  return ptr2;
#endif
}

void lb_memory_deallocate(void *ptr) {
  if (!ptr) return;  // it seems that freeing NULL is considered okay.

  // get the metadata
  struct allocation_metadata *metadata = get_metadata(ptr);

#if defined(_DEBUG)
  if (!metadata->base_ptr) CRASH();  // double-free
#endif

  unguard(metadata);

#if LB_MEMORY_SCRIBBLE
  scribble((char *)ptr, SCRIBBLE_DATA_FOR_DEALLOCATE, metadata->size_usable);
#endif

  untrack(metadata);

  void *base_ptr = metadata->base_ptr;
  metadata->base_ptr = NULL;  // to detect double-free.
  ALLOCATOR(free)(base_ptr);
}

size_t lb_memory_requested_size(void *ptr) {
  if (!ptr) return 0;
  return get_metadata(ptr)->size_requested;
}

size_t lb_memory_usable_size(void *ptr) {
  if (!ptr) return 0;
  return get_metadata(ptr)->size_usable;
}

#if LB_MEMORY_COUNT
void lb_memory_stats(lb_memory_info_t *info) {
  *info = memory_stats.cached_info_;

  size_t system_size;
  size_t in_use_size;
  if (ALLOCATOR(malloc_stats_np)(&system_size, &in_use_size) != 0) {
    CRASH();
    return;
  }

  size_t unallocated = lb_get_unallocated_memory();
  size_t allocator_unused = system_size - in_use_size;

  // free_memory is what is not claimed by anyone + what the allocator has
  // claimed but not yet given out.
  info->free_memory = unallocated + allocator_unused;
  // application_memory includes the overhead of our debugging metadata,
  // but not the allocator's metadata.
  info->application_memory = memory_stats.reserved_;
  // system_memory does not include the overhead of the allocator's metadata.
  info->system_memory = info->user_memory - in_use_size - info->free_memory;
}
#endif

#if LB_MEMORY_DUMP_CALLERS
void lb_memory_dump_callers(const char *filename) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", global_screenshot_output_path, filename);
  FILE *f = fopen(path, "w");

  char buf[64];
  int i;

  lb_shell_mutex_lock(&memory_tracker.mutex_);  // DON'T ALLOCATE INSIDE THIS BLOCK!
  for (i = 0; i < kMaxTrackableAllocations; i++) {
    struct allocation_metadata *metadata = memory_tracker.table_[i].metadata;
    if (metadata == NULL) continue;

    // fprintf would allocate a buffer internally, so print into our own buffer
    // which lives on the stack.
    size_t n = snprintf(buf, sizeof(buf), "%016"PRIx64" %016"PRIx64"\n",
        (uint64_t)metadata->caller_address,
        (uint64_t)metadata->size_requested);
    fwrite(buf, n, 1, f);
  }
  lb_shell_mutex_unlock(&memory_tracker.mutex_);

  fclose(f);
}
#endif

#if LB_MEMORY_DUMP_GRAPH
// NOTE: This is a macro because it is used in FRAG_GRAPH_MAX_BLOCKS, which must
// itself be all-macro.
#define ROUND_UP_DIVIDE(a, b) (((a) + (b) - 1) / (b))
// NOTE: This is a macro because this is plain C and std::min isn't available.
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// NOTE: These must be pure macros and not constant ints because they feed into
// the length of a static array.
#if defined(__LB_PS3__)
# define FRAG_GRAPH_BLOCK_SIZE 4096  // bytes represented by each pixel
# define FRAG_GRAPH_ADDRESS_SPACE (512UL << 20)  // a 512MB space
#elif defined(__LB_WIIU__) || defined(__LB_LINUX__)
# define FRAG_GRAPH_BLOCK_SIZE 8192  // bytes represented by each pixel
# define FRAG_GRAPH_ADDRESS_SPACE (2UL << 30)  // a 2GB space
#endif

#define FRAG_GRAPH_MAX_BLOCKS ROUND_UP_DIVIDE(FRAG_GRAPH_ADDRESS_SPACE, \
                                              FRAG_GRAPH_BLOCK_SIZE)

// compute a block number by remapping addr into the address ranges
static int calculate_graph_block_number(const uintptr_t addr,
                                        const uintptr_t range_start[3],
                                        const uintptr_t range_end[3]) {
  if (addr < range_start[0])
    return FRAG_GRAPH_MAX_BLOCKS;  // invalid
  if (addr < range_end[0])
    return (addr - range_start[0]) / FRAG_GRAPH_BLOCK_SIZE;

  uintptr_t len0 = range_end[0] - range_start[0];
  if (addr < range_start[1])
    return FRAG_GRAPH_MAX_BLOCKS;  // invalid
  if (addr < range_end[1])
    return (len0 + addr - range_start[1]) / FRAG_GRAPH_BLOCK_SIZE;

  uintptr_t len1 = len0 + range_end[1] - range_start[1];
  if (addr < range_start[2])
    return FRAG_GRAPH_MAX_BLOCKS;  // invalid
  if (addr < range_end[2])
    return (len1 + addr - range_start[2]) / FRAG_GRAPH_BLOCK_SIZE;

  return FRAG_GRAPH_MAX_BLOCKS;  // invalid
}

void lb_memory_dump_fragmentation_graph(const char *filename) {
  lb_shell_mutex_lock(&memory_tracker.mutex_);

  // note that path is static and affects neither the heap nor the stack.
  static char path[256];
  snprintf(path, sizeof(path), "%s/%s", global_screenshot_output_path, filename);
  // temporarily truncate this path into its parent directory name.
  char *last_slash = strrchr(path, '/');
  *last_slash = '\0';
  // create the parent directory.
  mkdir(path, 0755);
  // put the path back to its original state.
  *last_slash = '/';

#if LB_MEMORY_COUNT
  lb_memory_info_t info;
  lb_memory_stats(&info);
#endif

  const int width = 256;  //  each row of 4k blocks is 1MB, 8k blocks is 2MB
  // We must choose quant so that overflow does not occur on unsigned char data
  // later on.  We must satisfy FRAG_GRAPH_BLOCK_SIZE / size_quantum <= 128.
  // We only use the top-half of the spectrum for counting "fullness".
  const int size_quantum = (FRAG_GRAPH_BLOCK_SIZE / 128) + 1;

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
  const int num_blocks = ROUND_UP_DIVIDE(range_size, FRAG_GRAPH_BLOCK_SIZE);
  const int height = ROUND_UP_DIVIDE(num_blocks, width);

  if (num_blocks > FRAG_GRAPH_MAX_BLOCKS) {
    // This would indicate that either the description of the address space is
    // wrong, or that FRAG_GRAPH_MAX_BLOCKS needs to be raised.
    CRASH();
  }

  // create an OOM-safe PNG.
  png_structp png = oom_png_create(path, width, height, 8, PNG_COLOR_TYPE_GRAY);
  if (!png) {
    lb_shell_mutex_unlock(&memory_tracker.mutex_);
    return;
  }

  // compute the image data.
  // note that this data is static and affects neither the heap nor the stack.
  static unsigned char data[FRAG_GRAPH_MAX_BLOCKS];
  memset(data, 0, FRAG_GRAPH_MAX_BLOCKS);
  // walk through all tracked allocations.
  int i;
  for (i = 0; i < kMaxTrackableAllocations; i++) {
    struct allocation_metadata *metadata = memory_tracker.table_[i].metadata;
    if (metadata == NULL) continue;

    // this allocation starts somewhere within the block numbered block_num
    // and may extend beyond.
    size_t size = metadata->size_reserved;
    int block_num = calculate_graph_block_number((uintptr_t)metadata->base_ptr,
                                                 range_start, range_end);

    if (block_num >= num_blocks) {
      // This would indicate that the description of the address space is wrong.
      continue;
    }

    // walk through every block that this allocation touches.
    while (size) {
      size_t size_in_block = MIN(size, FRAG_GRAPH_BLOCK_SIZE);
      // we start blocks off at 50% gray the first time they are seen in use.
      if (data[block_num] == 0) data[block_num] += 0x80;
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
  size_t blocks_unusable_at_end = ROUND_UP_DIVIDE(unusable_at_end,
                                                  FRAG_GRAPH_BLOCK_SIZE);
  i = num_blocks - blocks_unusable_at_end;
  memset(data + i, 0x40, blocks_unusable_at_end);

  // locate the largest free space.
  uintptr_t largest_free_space_block = 0;
  size_t largest_free_space = 0;
  int current_free_space_block = 0;
  int num_free_space_blocks = 0;
  int used = 1;  // plain-C bool
  for (i = 0; i <= num_blocks; i++) {
    if (used) {
      if (i < num_blocks && data[i] == 0) {
        used = 0;
        current_free_space_block = i;
        ++num_free_space_blocks;
      }
    } else {
      if (i == num_blocks || data[i] != 0) {
        size_t current_free_space =
            (i - current_free_space_block) * FRAG_GRAPH_BLOCK_SIZE;
        used = 1;

        if (current_free_space > largest_free_space) {
          largest_free_space = current_free_space;
          largest_free_space_block = current_free_space_block;
        }
      }
    }
  }

  // print info about the largest free space.
  oom_fprintf(1, "The largest free space is %ld bytes at block 0x%lx.\n",
      (long)largest_free_space, (long)largest_free_space_block);
  oom_fprintf(1, "Number of free regions is %ld.\n",
      (long)num_free_space_blocks);
#if LB_MEMORY_COUNT
  oom_fprintf(1, "Total free memory is %ld bytes.\n",
      (long)info.free_memory);
  oom_fprintf(1, "Average free region size is %ld bytes.\n",
      (long)info.free_memory / (long)(num_free_space_blocks ? num_free_space_blocks : 1));
#endif
  oom_fprintf(1, "App lifetime %ld minutes.\n",
      (long)global_lifetime);

  // write the image data.
  unsigned char *row = data;
  for (i = 0; i < height; i++) {
    png_write_row(png, row);
    row += width;
  }

  // finish the PNG.
  oom_png_destroy(png);

  lb_shell_mutex_unlock(&memory_tracker.mutex_);
}
#endif // #if LB_MEMORY_DUMP_GRAPH



void lb_memory_dump_stats() {
#if LB_MEMORY_COUNT
  oom_fprintf(1, "MB requested: %.3f\n",
      ((double)memory_stats.requested_) / 1024.0 / 1024.0);
  oom_fprintf(1, "MB reserved: %.3f\n",
      ((double)memory_stats.reserved_) / 1024.0 / 1024.0);
  oom_fprintf(1, "unfreed allocations: %d\n",
      memory_stats.allocations_);
  oom_fprintf(1, "lifetime total allocations: %"PRId64"\n",
      memory_stats.allocations_total_);
  ALLOCATOR(dump_heap)();
#endif
}

void CRASH_ON_NULL(void* ptr, size_t size, uint32_t alignment) {
#if LB_MEMORY_OOM_CRASH
  if (!ptr) {
    oom_fprintf(1, "Unable to allocate %ld bytes with alignment of %ld.\n",
        (long)size, (long)alignment);
    lb_memory_dump_stats();
#if LB_MEMORY_DUMP_GRAPH
    lb_memory_dump_fragmentation_graph("oom.png");
#endif
#if LB_CONTINUOUS_MEMORY_LOG
    lb_close_memory_log();
#endif
    CRASH();
  }
#endif
}

#endif  // #if LB_ENABLE_MEMORY_DEBUGGING
