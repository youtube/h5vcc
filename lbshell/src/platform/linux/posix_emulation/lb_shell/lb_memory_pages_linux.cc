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

#include "lb_memory_pages.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

enum {
  kOneMeg = 1024 * 1024U,
  kSixtyFourK = 16 * 1024U,
  kMaxVirtualRegions = 8U,
};

// We try to emulate the virtual memory API on game consoles.
// But allocating a "virtual address" here actually grabs a mmapped chunk.
// So lb_allocate_physical_memory() and lb_map_memory() are no-ops,
// as the memory is already mapped.

typedef struct VirtualMemInfo {
  lb_virtual_mem_t mem_base;
  size_t size;
} VirtualMemInfo;

VirtualMemInfo s_virtual_regions[kMaxVirtualRegions];
int s_virtual_region_count;

lb_virtual_mem_t lb_allocate_virtual_address(size_t size,
                                             size_t /* page_size */) {
  void* mem = mmap(0,
                   size,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANON,
                   -1,
                   0);
  if (mem != MAP_FAILED) {
    VirtualMemInfo info;
    info.mem_base = (lb_virtual_mem_t)mem;
    info.size = size;
    s_virtual_regions[s_virtual_region_count++] = info;
  }
  return (lb_virtual_mem_t)mem;
}

void lb_free_virtual_address(lb_virtual_mem_t virtual_address) {
  for (int i = 0; i < s_virtual_region_count; ++i) {
    if (s_virtual_regions[i].mem_base == virtual_address) {
      munmap(reinterpret_cast<void*>(s_virtual_regions[i].mem_base),
             s_virtual_regions[i].size);
      s_virtual_regions[i] = s_virtual_regions[s_virtual_region_count - 1];
      s_virtual_region_count--;
      return;
    }
  }
}

int lb_allocate_physical_memory(size_t memory_size,
                                size_t page_size,
                                lb_physical_mem_t* mem_id) {
  // No-op. For compatibility with other platforms.
  *mem_id = 0;
  return 0;
}

void lb_free_physical_memory(lb_physical_mem_t physical_address) {
  // No-op. For compatibility with other platforms.
}

int lb_map_memory(lb_virtual_mem_t virtual_address,
                  lb_physical_mem_t physical_address) {
  // No-op. For compatibility with other platforms.
  return 0;
}

void lb_unmap_memory(lb_virtual_mem_t virtual_address,
                     lb_physical_mem_t* physical_address) {
  // No-op. For compatibility with other platforms.
}

size_t lb_get_total_system_memory() {
  // Limit ourselves to remain similar to more constrained platforms.
  return 1024U * kOneMeg;
}

size_t lb_get_virtual_region_size() {
  return lb_get_total_system_memory();
}
