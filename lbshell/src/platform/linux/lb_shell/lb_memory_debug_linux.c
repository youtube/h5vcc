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

void __real_free(void* ptr);
char** __real_backtrace_symbols(void *const *buffer, int size);
char *__real___cxa_demangle(const char *mangled, char *output, size_t *length,
                            int *status);

#if LB_ENABLE_MEMORY_DEBUGGING
#include "lb_memory_internal.h"

#include <assert.h>
#include <execinfo.h>
#include <string.h>
#include <unistd.h>

#include "lb_memory_debug.h"
#include "lb_memory_pages.h"
#include "lb_mutex.h"

#include <sys/types.h>

#include <sys/fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

// This next line sets up lb_memory_init so that it is called before main().
// There is no analog for deinitialization, so lb_memory_deinit() does not
// get called on Linux.
void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook)(void) = lb_memory_init;

void lb_memory_init() {
#if LB_MEMORY_COUNT
  memset(&memory_stats, 0, sizeof(memory_stats));

  long page_size = sysconf(_SC_PAGE_SIZE);

  // Read /proc/self/statm for the code and data sizes.
  long vsize, resident, shared, code, lib, data, dt;
  char mem_stat_buf[256];
  memset(mem_stat_buf, 0, sizeof(mem_stat_buf));

  int stat_file = open("/proc/self/statm", O_RDONLY);
  assert(stat_file != -1);
  int bytes_read = read(stat_file, mem_stat_buf, sizeof(mem_stat_buf));
  assert(bytes_read != -1);
  close(stat_file);

  sscanf(mem_stat_buf,
         "%ld %ld %ld %ld %ld %ld %ld",
         &vsize,
         &resident,
         &shared,
         &code,
         &lib,
         &data,
         &dt);

  memory_stats.cached_info_.total_memory = lb_get_total_system_memory();
  memory_stats.cached_info_.os_size = 0;
  memory_stats.cached_info_.executable_size = (code + data) * page_size;
  memory_stats.cached_info_.hidden_memory = 0;
  memory_stats.cached_info_.user_memory = memory_stats.cached_info_.total_memory -
      memory_stats.cached_info_.executable_size;
#endif

  lb_memory_init_common();
}

void lb_memory_deinit() {
  lb_memory_deinit_common();
}

size_t lb_get_unallocated_memory() {
  size_t system_size;
  size_t in_use_size;
  if (ALLOCATOR(malloc_stats_np)(&system_size, &in_use_size) != 0) {
    CRASH();
    return 0;
  }

  return memory_stats.cached_info_.user_memory - system_size;
}

// These functions are part of the set that all allocators must implement, but
// they are non-standard and not provided by the OS's default allocator.

void __real_dump_heap() {
  // NOP: heap dump not supported
}

// mallinfo only reports on one arena, so we fake this using the data we
// already have.
int __real_malloc_stats_np(size_t *system_size, size_t *in_use_size) {
  *system_size = memory_stats.cached_info_.user_memory;
  *in_use_size = memory_stats.reserved_;
  return 0;
}

// This describes the memory ranges available from the default OS allocator:
void __real_malloc_ranges_np(uintptr_t *start1, uintptr_t *end1,
                             uintptr_t *start2, uintptr_t *end2,
                             uintptr_t *start3, uintptr_t *end3) {

  *start1 = 0;
  *end1 = 0;

  // The maps file in the procfs describes all the virtual mappings.
  // Format of each line is:
  // start-end permissions  offset device inode pathname
  // e.g.
  // 00400000-03fbf000 r-xp 00000000 fc:01 5396758                            /lib/x86_64-linux-gnu/libnss_dns-2.15.so

  // We look for the specially-named [heap] entry,
  // that's where most of our allocations are from.

  // Note: Do no memory allocations in here.
  int fd = open("/proc/self/maps", O_RDONLY);
  if (fd < 0) {
    return;
  }

  // Should be enough for 1 line of maps output.
  char line_buf[1024];

  int eof = 0;
  while (!eof) {
    int valid_chars = 0;
    char c;
    do {
      int bytes_read = read(fd, &c, sizeof(c));
      if (bytes_read <= 0) {
        eof = 1;
        break;
      }
      line_buf[valid_chars++] = c;
    } while (c != '\n');

    line_buf[valid_chars] = 0;
    if (strstr(line_buf, "[heap]") != 0) {
      // Parse this one for its virtual address range.
      long start = 0;
      long end = 0;
      int args_parsed = sscanf(line_buf, "%lx-%lx", &start, &end);
      assert(args_parsed == 2);
      *start1 = start;
      *end1 = (uintptr_t)start + (uintptr_t)lb_get_total_system_memory();

      // We're done.
      break;
    }
  }
  close(fd);

  *start2 = *end2 = *end1;
  *start3 = *end3 = *end2;
}

#if LB_MEMORY_GUARD
void lb_memory_guard_activate(uintptr_t guard_addr) {
}
void lb_memory_guard_deactivate(uintptr_t guard_addr) {
}
#endif

void lb_backtrace(uint32_t skip,
                  uint32_t count,
                  uintptr_t *backtraceDest,
                  uint64_t *option) {
  void* buffer[skip + count + 1]; // one for lb_backtrace itself.
  backtrace(buffer, skip + count + 1);
  memcpy(backtraceDest, buffer + 1 + skip, sizeof(buffer[0]) * count);
}

char** __wrap_backtrace_symbols(void *const *buffer, int size) {
  // backtrace_symbols in libc doesn't respect wrap_malloc,
  // but always calls into libc malloc.
  // Make a copy of the symbols and return those.

  char** symbols = __real_backtrace_symbols(buffer, size);

  /*
  The allocated block is as follows:
  | char* | char* | ... | char* |symbol 0\0symbol 1\0...symbol N\0|
  | ------<size> pointers-------|---------symbol data-------------|

  Each of the pointers points into the symbol data block.
  */

  int symbol_buffer_size = sizeof(char*) * size;

  for (int i = 0; i < size; ++i) {
    symbol_buffer_size += strlen(symbols[i]) + 1;
  }

  char** our_symbols = malloc(symbol_buffer_size);
  memcpy(our_symbols, symbols, symbol_buffer_size);

  // Fix up strings in our_symbols to point to the relocated buffer.
  for (int i = 0; i < size; ++i) {
    int offset = (uintptr_t)symbols[i] - (uintptr_t)symbols;
    our_symbols[i] = (char*)(uintptr_t)our_symbols + offset;
  }

  __real_free(symbols);
  return our_symbols;
}

char *__wrap___cxa_demangle(const char *mangled, char *output, size_t *length,
                            int *status) {
  // The caller could have allocated an output buffer in advance.
  // __cxa_demangle is supposed to use realloc to expand the buffer given.
  // To avoid having __real_realloc called on a buffer which was allocated
  // with __wrap_malloc, we pass NULL to __real___cxa_demangle.
  char *symbol_name = __real___cxa_demangle(mangled, NULL, NULL, status);
  if (!symbol_name) {
    // *status has been set by __real___cxa_demangle.
    return NULL;
  }

  size_t symbol_length = strlen(symbol_name) + 1;
  if (length && symbol_length < *length) {
    // the caller should NEVER provide length but no output buffer!
    assert(output);
    // reuse the caller's buffer
    memcpy(output, symbol_name, symbol_length);
  } else if (output) {
    // resize the caller's buffer, which may safely be NULL.
    output = realloc(output, symbol_length);
    if (output) {
      memcpy(output, symbol_name, symbol_length);
    } else {
      // override *status from __real___cxa_demangle.
      // -1 means "malloc failure" according to the docs.
      *status = -1;
    }
  } else {
    // allocate a new buffer
    output = strdup(symbol_name);
  }

  __real_free(symbol_name);
  return output;
}
#else  // #if LB_ENABLE_MEMORY_DEBUGGING
char** __wrap_backtrace_symbols(void *const *buffer, int size) {
  return __real_backtrace_symbols(buffer, size);
}
char *__wrap___cxa_demangle(const char *mangled, char *output, size_t *length,
                            int *status) {
  return __real___cxa_demangle(mangled, output, length, status);
}
#endif  // #if LB_ENABLE_MEMORY_DEBUGGING

