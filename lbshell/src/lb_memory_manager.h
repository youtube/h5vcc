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
// Replace the system new and malloc functions. And add a few macros for
// lb_shell project to use so that we can track function name, file name
// and line number for memory operations.
#ifndef SRC_LB_MEMORY_MANAGER_H_
#define SRC_LB_MEMORY_MANAGER_H_

#include <inttypes.h>
#include <malloc.h>
#include <stdlib.h>

// Enable memory debugging features. Note that some features such as
// LB_CONTINUOUS_MEMORY_LOG, LB_MEMORY_COUNT and LB_MEMORY_DUMP_CALLERS
// require malloc override supported by compiler (to initialize data).
 #if defined(__LB_SHELL__FOR_RELEASE__)
#define LB_ENABLE_MEMORY_DEBUGGING        0
#else  // Non retail build
#define LB_ENABLE_MEMORY_DEBUGGING        1
#endif

/*************************************************************************
        Begin of memory debugging switches and setting
*************************************************************************/
// Turn on crash on out of memory event.
#define LB_MEMORY_OOM_CRASH               1
// Turn on counting of memory allocated.
#define LB_MEMORY_COUNT                   (LB_ENABLE_MEMORY_DEBUGGING && 1)
// Turn on guard pages. affect malloc from lb_shell code only.
// (debug build only)
#define LB_MEMORY_GUARD_LB                (LB_ENABLE_MEMORY_DEBUGGING && 0)
// Turn on guard pages for all malloc calls from code outside of lb_shell.
// A LOT of memory will be used when this flag is on. (debug build only)
#define LB_MEMORY_GUARD_EXTERN            (LB_ENABLE_MEMORY_DEBUGGING && 0)
// Whether the guard page should be at the front or the back.
#define LB_GUARD_AT_FRONT                 (LB_ENABLE_MEMORY_DEBUGGING && 0)
// Write into the user's space on allocate/deallocate.
#define LB_MEMORY_SCRIBBLE                (LB_ENABLE_MEMORY_DEBUGGING && 0)
// Keep a dumpable list of all allocations and pointers to the callers.
#define LB_MEMORY_DUMP_CALLERS            (LB_ENABLE_MEMORY_DEBUGGING && 1)
// Dump a fragmentation graph on oom (requires LB_MEMORY_DUMP_CALLERS).
#define LB_MEMORY_DUMP_GRAPH              (LB_ENABLE_MEMORY_DEBUGGING && 1)
// Start continuous memory fragmentation graphs on startup, 1 per second.
#define LB_MEMORY_CONTINUOUS_GRAPH        (LB_ENABLE_MEMORY_DEBUGGING && 0)
// Write a stream of allocations to disk as they are happening.
// 1GB of data can be generated in 10 minutes or less.
// Consider using in combination with SHUTDOWN_APPLICATION_AFTER.
#define LB_CONTINUOUS_MEMORY_LOG          (LB_ENABLE_MEMORY_DEBUGGING && 0)
// Shut down the application after so many minutes.  Set to 0 to disable.
#define SHUTDOWN_APPLICATION_AFTER        0
/*************************************************************************
        End of memory debugging switches and setting
*************************************************************************/


// Enable memory guard when any of the specific guards is true
#define LB_MEMORY_GUARD (LB_MEMORY_GUARD_LB || LB_MEMORY_GUARD_EXTERN)

#if defined(_DEBUG)
// Debug builds should always enable the scribble flags, since these can help
// catch problems with uninitialized memory or use after freeing.
#undef  LB_MEMORY_SCRIBBLE
#define LB_MEMORY_SCRIBBLE    1
#endif

#if LB_ENABLE_MEMORY_DEBUGGING

// Redefine new operator. 0 is a dummy parameter to redirect new operator.
#define LB_NEW                new(0)
// Redefine malloc functions.
#define LB_MALLOC(...)        lb_malloc(__VA_ARGS__)
#define LB_CALLOC(...)        lb_calloc(__VA_ARGS__)
#define LB_REALLOC(...)       lb_realloc(__VA_ARGS__)
#define LB_MEMALIGN(...)      lb_memalign(__VA_ARGS__)

// Logging version of the malloc functions
#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

void* lb_malloc(size_t size);
void* lb_calloc(size_t nelem, size_t size);
void* lb_realloc(void* ptr, size_t size);
void* lb_memalign(size_t boundary, size_t size);

#if LB_MEMORY_COUNT
typedef struct {
  uint32_t total_memory;  // should be == os + exe + hidden + user
  uint32_t os_size;
  uint32_t executable_size;
  uint32_t hidden_memory;  // taken away to emulate a consumer console
  uint32_t user_memory;  // should be app + sys + free

  uint32_t application_memory;
  uint32_t system_memory;
  uint32_t free_memory;
} lb_memory_info_t;

void lb_memory_stats(lb_memory_info_t *info);
#endif

#if LB_MEMORY_DUMP_CALLERS
void lb_memory_dump_callers(const char *filename);
#endif
#if LB_MEMORY_DUMP_GRAPH
void lb_memory_dump_fragmentation_graph(const char *filename);
#endif

#if LB_MEMORY_GUARD
void lb_memory_guard_activate(uintptr_t guard_addr);
void lb_memory_guard_deactivate(uintptr_t guard_addr);
#endif

void lb_backtrace(uint32_t skip,
                  uint32_t count,
                  uintptr_t *backtraceDest,
                  uint64_t *option);

size_t lb_memory_requested_size(void *ptr);

#ifdef __cplusplus
}
#endif  // #ifdef __cplusplus

// Logging version of new operators
#ifdef __cplusplus
void* operator new(size_t size, int dummy);
void* operator new[](size_t size, int dummy);
#endif  // #ifdef __cplusplus

#else   // #if LB_ENABLE_MEMORY_DEBUGGING
#define LB_NEW                new
#define LB_MALLOC(...)        malloc(__VA_ARGS__)
#define LB_CALLOC(...)        calloc(__VA_ARGS__)
#define LB_REALLOC(...)       realloc(__VA_ARGS__)
#define LB_MEMALIGN(...)      memalign(__VA_ARGS__)
#endif  // #if LB_ENABLE_MEMORY_DEBUGGING

#endif  // SRC_LB_MEMORY_MANAGER_H_
