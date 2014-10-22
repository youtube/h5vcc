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

#if defined(__LB_XB360__)
// Ensure that we include the pre-initialization module here. This is a good
// place to do it, since it is force-included into all executables and shared
// libraries.
#pragma comment(linker, "/include:forcePreinit")
#endif

#if !defined(__LB_PS4__)
#if LB_ENABLE_MEMORY_DEBUGGING

// scalar new and its matching delete
void* operator new(size_t size) {
  return lb_memory_allocate(kNoAlignment, size, 0);
}

void operator delete(void* ptr) {
  lb_memory_deallocate(ptr);
}

// array new and its matching delete[]
void* operator new[](size_t size) {
  return lb_memory_allocate(kNoAlignment, size, 0);
}

void operator delete[](void* ptr) {
  lb_memory_deallocate(ptr);
}

#else  // LB_ENABLE_MEMORY_DEBUGGING

// scalar new and its matching delete
void* operator new(size_t size) {
  return malloc(size);
}

void operator delete(void* ptr) {
  free(ptr);
}

// array new and its matching delete[]
void* operator new[](size_t size) {
  return malloc(size);
}

void operator delete[](void* ptr) {
  free(ptr);
}
#endif  // LB_ENABLE_MEMORY_DEBUGGING
#else

#if LB_ENABLE_MEMORY_DEBUGGING
void *user_new(std::size_t size, const std::nothrow_t& x) throw() {
  return lb_memory_allocate(kNoAlignment, size, 0);
}

void user_delete(void *ptr, const std::nothrow_t& x) throw() {
  lb_memory_deallocate(ptr);
}

// array new and its matching delete[]
void *user_new_array(std::size_t size, const std::nothrow_t& x) throw() {
  return lb_memory_allocate(kNoAlignment, size, 0);
}
void user_delete_array(void *ptr, const std::nothrow_t& x) throw() {
  lb_memory_deallocate(ptr);
}
#else
// Non-debugging mode- don't really need to
// override new/delete if we are just going to
// call the system ones.
#endif  // LB_ENABLE_MEMORY_DEBUGGING
#endif  // !defined(__LB_PS4__)
