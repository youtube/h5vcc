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

#include <string>

#include "lb_platform.h"

// Enable memory debugging features. Note that some features such as
// kLbContinuousMemoryLog, kLbMemoryCount and kLbMemoryDumpCallers
// require malloc override supported by compiler (to initialize data).
#if defined(__LB_SHELL__FOR_RELEASE__)
#define LB_ENABLE_MEMORY_DEBUGGING        0
#else  // Non retail build
#define LB_ENABLE_MEMORY_DEBUGGING        1
#endif

// Value to indicate the default platform alignment.
static const int kNoAlignment = 1;

/*************************************************************************
        Begin of memory debugging switches and setting
*************************************************************************/
namespace LB {
namespace Memory {

enum DebugFlags {
  // Turn on crash on out of memory event.
  kOomCrash = (1 << 0),
  // Turn on counting of memory allocated.
  kCount = (1 << 1),
  // Write a special pattern over allocated and freed memory.
  kScribble = (1 << 2),
  // Add scribbled memory before and after any allocation
  // and check for buffer overrun on delete.
  kAllocationGuard = (1 << 3),
  // Delay the free of memory.
  // This is helpful to identify possible write after delete.
  kDelayedFree = (1 << 4),
  // Keep a dumpable list of all allocations and pointers to the callers.
  kDumpCallers = (1 << 5),
  // Dump a fragmentation graph on oom (requires kLbMemoryDumpCallers).
  kDumpGraph = (1 << 6),
  // Start continuous memory fragmentation graphs on startup, 1 per second.
  kContinuousGraph = (1 << 7),
  // Write a stream of allocations to disk as they are happening.
  // 1GB of data can be generated in 10 minutes or less.
  // Consider using in combination with SHUTDOWN_APPLICATION_AFTER.
  kContinuousLog = (1 << 8),
};

#if LB_ENABLE_MEMORY_DEBUGGING
#define LB_ENABLE(x) (x)
#else
#define LB_ENABLE(x) 0
#endif
#define LB_DISABLE(x) 0

static const uint32_t kDebugSettings = 0
    | LB_ENABLE(kOomCrash)
    | LB_ENABLE(kCount)
#if !defined(__LB_SHELL__FOR_QA__)
    // Scribbling can have a large effect on performance in QA builds, so
    // do not enable it there.
    | LB_ENABLE(kScribble)
#endif
    | LB_DISABLE(kAllocationGuard)
    | LB_DISABLE(kDelayedFree)
    | LB_ENABLE(kDumpCallers)
    | LB_ENABLE(kDumpGraph)
    | LB_DISABLE(kContinuousGraph)
    | LB_DISABLE(kContinuousMemoryLog);

#undef LB_ENABLE
#undef LB_DISABLE

// This size has to be a multiple of the largest possible allocation alignment.
static const uint32_t kAllocationGuardBytes =
    kDebugSettings & kAllocationGuard ? 32 : 0;
static const int kShutdownApplicationAfter = 0;

COMPILE_ASSERT(
    (kDebugSettings & kDumpCallers) || !(kDebugSettings & kContinuousGraph),
    Continuous_graph_requires_dump_callers);

LB_ALWAYS_INLINE static bool IsOomCrashEnabled() {
  return kDebugSettings & kOomCrash ? true : false;
}

LB_ALWAYS_INLINE static bool IsCountEnabled() {
  return kDebugSettings & kCount ? true : false;
}

LB_ALWAYS_INLINE static bool IsScribbleEnabled() {
  return kDebugSettings & kScribble ? true : false;
}

LB_ALWAYS_INLINE static bool IsGuardEnabled() {
  return kDebugSettings & kAllocationGuard ? true : false;
}

LB_ALWAYS_INLINE static bool IsDelayedFreeEnabled() {
  return kDebugSettings & kDelayedFree ? true : false;
}

LB_ALWAYS_INLINE static bool IsDumpCallersEnabled() {
  return kDebugSettings & kDumpCallers ? true : false;
}

LB_ALWAYS_INLINE static bool IsDumpGraphEnabled() {
  return kDebugSettings & kDumpGraph ? true : false;
}

LB_ALWAYS_INLINE static bool IsContinuousGraphEnabled() {
  return kDebugSettings & kContinuousGraph ? true : false;
}

LB_ALWAYS_INLINE static bool IsContinuousLogEnabled() {
  return kDebugSettings & kContinuousLog ? true : false;
}

LB_ALWAYS_INLINE static int ShutdownApplicationMinutes() {
  // Shut down the application after so many minutes.  Set to 0 to disable.
  return kShutdownApplicationAfter ? true : false;
}

struct Info {
  ssize_t total_memory;  // should be == os + exe + hidden + user
  ssize_t os_size;
  ssize_t executable_size;
  ssize_t hidden_memory;  // taken away to emulate a consumer console
  ssize_t user_memory;  // should be app + sys + free

  ssize_t application_memory;
  ssize_t system_memory;
  ssize_t free_memory;
};

LB_BASE_EXPORT void GetInfo(Info *info);
LB_BASE_EXPORT void DumpCallers(const char *filename);
LB_BASE_EXPORT void DumpFragmentationGraph(const char *filename);

// As this function may spawn a thread and access the filesystem, it should
// be called once these systems are completely setup.
LB_BASE_EXPORT void InitLogWriter();

// Writes out an explicit counting event to the memory log.
LB_BASE_EXPORT void LogNamedCounter(const std::string& name, uint64_t counter);

// Platform-specific initialization functions.
void Init();

// Init routines common to all LBShell platforms.
void InitCommon();

// Platform-specific uninitialization functions.
void Deinit();

// Deinit routines common to all LBShell platforms.
void DeinitCommon();

// Indicate that the system is ready for backtrace.
// On some systems backtraces can't be captured during
// very early initialization phases.
void SetBacktraceEnabled(bool b);
bool GetBacktraceEnabled();

}  // namespace Memory
}  // namespace LB

// Valid allocator prefixes:
//   __real_ : OS default allocator.
//   dl : dlmalloc
#if defined(__LB_ANDROID__)
#define ALLOCATOR_PREFIX __real_
#else
#define ALLOCATOR_PREFIX dl
#endif

#define _ALLOCATOR_MANGLER_2(prefix, fn) prefix##fn
#define _ALLOCATOR_MANGLER(prefix, fn) _ALLOCATOR_MANGLER_2(prefix, fn)
#define ALLOCATOR(fn) _ALLOCATOR_MANGLER(ALLOCATOR_PREFIX, fn)

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cplusplus

LB_BASE_EXPORT void *lb_memory_allocate(
    size_t boundary, size_t size, int indirections);
LB_BASE_EXPORT void *lb_memory_reallocate(
    void *ptr, size_t size, int indirections);
LB_BASE_EXPORT void lb_memory_deallocate(void *ptr);
LB_BASE_EXPORT size_t lb_memory_requested_size(void *ptr);
LB_BASE_EXPORT size_t lb_memory_usable_size(void *ptr);

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
#endif  // SRC_LB_MEMORY_MANAGER_H_
