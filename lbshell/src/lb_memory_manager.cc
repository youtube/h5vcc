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
// Replace the system allocation functions with our own versions. This
// helps us to track and debug memory problems.

#include "lb_memory_manager.h"
#include "lb_memory_internal.h"

#include <string.h>

static const size_t kMaxSize = ~static_cast<size_t>(0);

#if LB_ENABLE_MEMORY_DEBUGGING

static const int kNoAlignment = 1;

extern "C" {

void* __wrap_malloc(size_t size) {
  return lb_memory_allocate(kNoAlignment, size, LB_MEMORY_GUARD_EXTERN, 0);
}

void* lb_malloc(size_t size) {
  return lb_memory_allocate(kNoAlignment, size, LB_MEMORY_GUARD_LB, 0);
}

void* __wrap_calloc(size_t nelem, size_t size) {
  const size_t bytes = nelem * size;
  // Check for overflow
  if (nelem && kMaxSize / nelem < size) {
    return NULL;
  }

  void *ptr = lb_memory_allocate(kNoAlignment, bytes, LB_MEMORY_GUARD_EXTERN,
                                 0);
  if (ptr) {
    memset(ptr, 0, bytes);
  }
  return ptr;
}

void* lb_calloc(size_t nelem, size_t size) {
  const size_t bytes = nelem * size;
  // Check for overflow
  if (nelem && kMaxSize / nelem < size) {
    return NULL;
  }

  void *ptr = lb_memory_allocate(kNoAlignment, bytes, LB_MEMORY_GUARD_LB, 0);
  if (ptr) {
    memset(ptr, 0, bytes);
  }
  return ptr;
}

void* __wrap_realloc(void* ptr, size_t size) {
  return lb_memory_reallocate(ptr, size, LB_MEMORY_GUARD_EXTERN, 0);
}

void* lb_realloc(void* ptr, size_t size) {
  return lb_memory_reallocate(ptr, size, LB_MEMORY_GUARD_LB, 0);
}

void* __wrap_memalign(size_t boundary, size_t size) {
  return lb_memory_allocate(boundary, size, LB_MEMORY_GUARD_EXTERN, 0);
}

void* lb_memalign(size_t boundary, size_t size) {
  return lb_memory_allocate(boundary, size, LB_MEMORY_GUARD_LB, 0);
}

void __wrap_free(void* ptr) {
  return lb_memory_deallocate(ptr);
}

size_t __wrap_malloc_usable_size(void* ptr) {
  return lb_memory_usable_size(ptr);
}

char* __wrap_strdup(const char* ptr) {
  const int size = strlen(ptr) + 1;
  char* s = (char*)malloc(size);
  if (s) {
    memcpy(s, ptr, size);
  }
  return s;
}

}  // extern "C"

// scalar new and its matching delete
void* operator new(size_t size) {
  return lb_memory_allocate(kNoAlignment, size, LB_MEMORY_GUARD_EXTERN, 0);
}

void operator delete(void* ptr) {
  return lb_memory_deallocate(ptr);
}

// array new and its matching delete[]
void* operator new[](size_t size) {
  return lb_memory_allocate(kNoAlignment, size, LB_MEMORY_GUARD_EXTERN, 0);
}

void operator delete[](void* ptr) {
  return lb_memory_deallocate(ptr);
}

void* operator new(size_t size, int dummy) {
  return lb_memory_allocate(kNoAlignment, size, LB_MEMORY_GUARD_LB, 0);
}

// array new and its matching delete[]
void* operator new[](size_t size, int dummy) {
  return lb_memory_allocate(kNoAlignment, size, LB_MEMORY_GUARD_LB, 0);
}

#else  // #if LB_ENABLE_MEMORY_DEBUGGING

// If we don't need to debug memory (as in Gold build),
// connect these symbols directly to the allocator.

extern "C" {

#if defined(__LB_LINUX__)
static const int kMinimumAlignment = 4;
#elif defined(__LB_PS3__)
static const int kMinimumAlignment = 16;
#elif defined(__LB_WIIU__)
static const int kMinimumAlignment = 8;
#endif

void* __wrap_malloc(size_t size) {
  return ALLOCATOR(memalign)(kMinimumAlignment, size);
}

void* __wrap_calloc(size_t nelem, size_t size) {
  const size_t bytes = nelem * size;
  // Check for overflow
  if (nelem && kMaxSize / nelem < size) {
    return NULL;
  }
  void *ptr = ALLOCATOR(memalign)(kMinimumAlignment, bytes);
  memset(ptr, 0, bytes);
  return ptr;
}

void* __wrap_realloc(void* ptr, size_t size) {
#if ALLOCATOR_HAS_REALLOC
  return ALLOCATOR(realloc)(ptr, size);
#else
  // NOTE: This fallback is prone to fragmentation.
  void *ptr2 = ALLOCATOR(memalign)(kMinimumAlignment, size);
  size_t copy_size = ALLOCATOR(malloc_usable_size)(ptr);
  if (copy_size > size) {
    copy_size = size;
  }
  if (copy_size) {
    memcpy(ptr2, ptr, copy_size);
  }
  ALLOCATOR(free)(ptr);
  return ptr2;
#endif
}

void* __wrap_memalign(size_t boundary, size_t size) {
  if (boundary < kMinimumAlignment) boundary = kMinimumAlignment;
  return ALLOCATOR(memalign)(boundary, size);
}

void __wrap_free(void* ptr) {
  return ALLOCATOR(free)(ptr);
}

size_t __wrap_malloc_usable_size(void* ptr) {
  return ALLOCATOR(malloc_usable_size)(ptr);
}

char* __wrap_strdup(const char* ptr) {
  const int size = strlen(ptr) + 1;
  char* s = (char*)ALLOCATOR(memalign)(kMinimumAlignment, size);
  if (s) {
    memcpy(s, ptr, size);
  }
  return s;
}

}  // extern "C"

#endif  // #if LB_ENABLE_MEMORY_DEBUGGING
