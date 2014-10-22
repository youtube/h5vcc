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

#include "lb_log_writer.h"

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lb_mutex.h"
#include "oom_png.h"

#if LB_ENABLE_MEMORY_DEBUGGING

#include "lb_memory_debug.h"
#include "lb_memory_debug_platform.h"

#define LOG_FILE MEMORY_LOG_PATH"/memory_log.txt"

namespace {

static const int kBufferSize = 1 << 20;
static const int kNumBuffers = 2;
struct LogBuffer {
  char buffer[kBufferSize];
  size_t num_bytes;
};

LogBuffer log_buffers[kNumBuffers] = { {"", 0}, {"", 0} };
int current_log_buffer = 0;
LogBuffer* log_buffer_to_flush = NULL;
int log_file = -1;

lb_shell_cond_t log_buffer_cond;
lb_shell_mutex_t log_buffer_mutex;
pthread_t flush_thread;

bool quit_thread = false;
bool thread_started = false;

bool SwapBuffers() {
  if (!thread_started) {
    // LogWriter not started yet. Cannot swap.
    return false;
  }

  // If the non-logging threads block on this for too long, try increasing
  // the size of the buffers.
  int result = lb_shell_mutex_lock(&log_buffer_mutex);
  assert(result == 0);

  bool can_swap = log_buffer_to_flush == NULL;
  if (!can_swap) {
    oom_fprintf(1, "Cannot swap buffer while a flush is pending.\n");
  } else {
    log_buffer_to_flush = &(log_buffers[current_log_buffer]);
    current_log_buffer = (current_log_buffer + 1) % kNumBuffers;
    assert(log_buffers[current_log_buffer].num_bytes == 0);
  }

  result = lb_shell_cond_signal(&log_buffer_cond);
  assert(result == 0);

  result = lb_shell_mutex_unlock(&log_buffer_mutex);
  assert(result == 0);

#if defined(__LB_WIIU__)
  // logging thread may not wake to kick off the write right away unless
  // we explicitly yield here.
  OSYieldThread();
#endif

  return can_swap;
}

void* ThreadEntryFunc(void* data) {
  int result = lb_shell_mutex_lock(&log_buffer_mutex);
  assert(result == 0);
  while (!quit_thread) {
    if (log_buffer_to_flush != NULL) {
      int bytes_written = write(
          log_file,
          log_buffer_to_flush->buffer,
          log_buffer_to_flush->num_bytes);
      if (bytes_written != log_buffer_to_flush->num_bytes) {
        oom_fprintf(1, "Error writing to memory log. Aborting logging\n");
        break;
      }
      log_buffer_to_flush->num_bytes = 0;
      log_buffer_to_flush = NULL;
    }
    result = lb_shell_cond_wait(&log_buffer_cond, &log_buffer_mutex);
    assert(result == 0);
  }
  result = lb_shell_mutex_unlock(&log_buffer_mutex);
  assert(result == 0);
  return 0;
}

void MemoryLogWriteModuleInfo() {
  const uint32_t kMaxModules = 256;
  LB::Memory::LoadedModuleInfo modules_info[kMaxModules];
  int num_modules = LB::Memory::GetLoadedModulesInfo(kMaxModules, modules_info);
  assert(num_modules >= 0);  // -1 means an error occurred.

  char print_buffer[1024];
  // Write out module information for each module
  for (int i = 0; i< num_modules; ++i) {
    snprintf(print_buffer, sizeof(print_buffer), "L %s %" PRIxPTR "\n",
             modules_info[i].name, modules_info[i].base);
    print_buffer[sizeof(print_buffer) - 1] = 0;
    write(log_file, print_buffer, strlen(print_buffer));
  }
}

}  // end anonymous namespace

namespace LB {
namespace Memory {

void LogWriterStart() {
  if (!thread_started) {
    // Do not reset the LogBuffers here, as they may have been written to
    // before the log thread was started.
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    log_file = open(LOG_FILE, O_WRONLY|O_CREAT|O_TRUNC, mode);
    if (log_file < 0) {
      oom_fprintf(1, "Error creating memory log file\n");
      return;
    }

    int result;
    result = lb_shell_mutex_init(&log_buffer_mutex);
    assert(result == 0);

    result = lb_shell_cond_init(&log_buffer_cond, &log_buffer_mutex);
    assert(result == 0);

    quit_thread = false;
    result = pthread_create(&flush_thread, NULL, ThreadEntryFunc, NULL);
    assert(result == 0);

    MemoryLogWriteModuleInfo();

    thread_started = true;
  }
}

void LogWriterStop() {
  if (thread_started) {
    quit_thread = true;
    SwapBuffers();
    int result = lb_shell_cond_signal(&log_buffer_cond);
    assert(result == 0);

    result = pthread_join(flush_thread, NULL);
    assert(result == 0);

    result = lb_shell_mutex_destroy(&log_buffer_mutex);
    assert(result == 0);

    result = lb_shell_cond_destroy(&log_buffer_cond);
    assert(result == 0);

    close(log_file);
    log_file = -1;

    thread_started = false;
    quit_thread = false;
  }
}

void LogWriterAppend(const char* data, size_t num_bytes) {
  if (num_bytes > kBufferSize) {
    // We can never log this, and it's probably an error, but let's not write
    // over the end of the buffer.
    oom_fprintf(
        1,
        "Log data is larger than the full buffer size. Dropping log data\n");
    return;
  }
  // This function may be called before memory_log_writer_start.
  if (log_buffers[current_log_buffer].num_bytes + num_bytes > kBufferSize) {
    if (!SwapBuffers()) {
      // Failed to swap the buffer, so we will have to drop this log
      oom_fprintf(1, "Dropping log data. Try increasing buffer size\n");
      return;
    }
  }
  LogBuffer& current_buffer = log_buffers[current_log_buffer];
  memcpy(current_buffer.buffer + current_buffer.num_bytes, data, num_bytes);
  current_buffer.num_bytes += num_bytes;
}

}  // namespace Memory
}  // namespace LB

#endif  // LB_ENABLE_MEMORY_DEBUGGING
