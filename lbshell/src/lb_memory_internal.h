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
// An interface that must be implemented internally per-platform.
#ifndef SRC_LB_MEMORY_INTERNAL_H_
#define SRC_LB_MEMORY_INTERNAL_H_

#include "lb_memory_manager.h"

// Valid allocator prefixes:
//   __real_ : OS default allocator.
//   dl : dlmalloc
#define ALLOCATOR_PREFIX dl
#define ALLOCATOR_HAS_REALLOC 1
// NOTE: __real_ on WiiU has no realloc.

#define _ALLOCATOR_MANGLER_2(prefix, fn) prefix##fn
#define _ALLOCATOR_MANGLER(prefix, fn) _ALLOCATOR_MANGLER_2(prefix, fn)
#define ALLOCATOR(fn) _ALLOCATOR_MANGLER(ALLOCATOR_PREFIX, fn)

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

// defined by us, our internal memory functions:
void lb_memory_init();
void lb_memory_init_common();
void lb_memory_deinit();
void lb_memory_deinit_common();
void *lb_memory_allocate(size_t boundary, size_t size, int do_guard,
                         int indirections);
void *lb_memory_reallocate(void *ptr, size_t size,
                           int do_guard, int indirections);
void lb_memory_deallocate(void *ptr);
size_t lb_memory_usable_size(void *ptr);

// defined by the underlying allocator:
void ALLOCATOR(_malloc_init)();
void ALLOCATOR(_malloc_finalize)();
void* ALLOCATOR(memalign)(size_t boundary, size_t size);
void* ALLOCATOR(realloc)(void* ptr, size_t size);
void ALLOCATOR(free)(void* ptr);
size_t ALLOCATOR(malloc_usable_size)(void* ptr);
int ALLOCATOR(malloc_stats_np)(size_t *system_size, size_t *in_use_size);
void ALLOCATOR(malloc_ranges_np)(uintptr_t *start1, uintptr_t *end1,
                                 uintptr_t *start2, uintptr_t *end2,
                                 uintptr_t *start3, uintptr_t *end3);
void ALLOCATOR(dump_heap)();

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus

#endif  // SRC_LB_MEMORY_INTERNAL_H_
