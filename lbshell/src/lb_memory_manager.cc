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

#ifdef LB_MACRO_MALLOC_OVERRIDE
#undef free
#undef memalign
#undef realloc
#endif

#include "lb_memory_manager.h"

#include <string.h>

static const size_t kMaxSize = ~static_cast<size_t>(0);

#if LB_ENABLE_MEMORY_DEBUGGING

extern "C" {
void* __wrap_malloc(size_t size) {
  return lb_memory_allocate(kNoAlignment, size, 0);
}

void* __wrap_calloc(size_t nelem, size_t size) {
  const size_t bytes = nelem * size;
  // Check for overflow
  if (nelem && kMaxSize / nelem < size) {
    return NULL;
  }

  void *ptr = lb_memory_allocate(kNoAlignment, bytes, 0);
  if (ptr) {
    memset(ptr, 0, bytes);
  }
  return ptr;
}

void* __wrap_realloc(void* ptr, size_t size) {
  return lb_memory_reallocate(ptr, size, 0);
}

void* __wrap_memalign(size_t boundary, size_t size) {
  return lb_memory_allocate(boundary, size, 0);
}

void __wrap_free(void* ptr) {
  return lb_memory_deallocate(ptr);
}

size_t __wrap_malloc_usable_size(void* ptr) {
  return lb_memory_usable_size(ptr);
}

char* __wrap_strdup(const char* ptr) {
  const size_t size = strlen(ptr) + 1;
  char* s = reinterpret_cast<char*>(lb_memory_allocate(kNoAlignment, size, 0));
  if (s) {
    memcpy(s, ptr, size);
  }
  return s;
}
}  // extern "C"

#else  // #if LB_ENABLE_MEMORY_DEBUGGING

// If we don't need to debug memory (as in Gold build),
// connect these symbols directly to the allocator.

extern "C" {
#if defined(__LB_ANDROID__)
static const int kMinimumAlignment = 4;
#elif defined(__LB_LINUX__)
static const int kMinimumAlignment = 4;
#elif defined(__LB_PS3__)
static const int kMinimumAlignment = 16;
#elif defined(__LB_PS4__)
static const int kMinimumAlignment = 16;
#elif defined(__LB_WIIU__)
static const int kMinimumAlignment = 8;
#elif defined(__LB_XB1__)
static const int kMinimumAlignment = 4;
#elif defined(__LB_XB360__)
static const int kMinimumAlignment = 16;
#else
#error kMinimumAlignment undefined for this platform
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
  return ALLOCATOR(realloc)(ptr, size);
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
  char* s = reinterpret_cast<char*>(
      ALLOCATOR(memalign)(kMinimumAlignment, size));
  if (s) {
    memcpy(s, ptr, size);
  }
  return s;
}
}  // extern "C"

#endif  // #if LB_ENABLE_MEMORY_DEBUGGING
