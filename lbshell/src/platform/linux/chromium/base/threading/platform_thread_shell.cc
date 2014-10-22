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
#include "base/threading/platform_thread.h"

#include <errno.h>
#include <sched.h>
#include <sys/resource.h>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/safe_strerror_posix.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_restrictions.h"
#include "lb_memory_manager.h"
#include "lb_platform.h"

namespace base {

namespace {

LazyInstance< ThreadLocalPointer<char> >::Leaky
    current_thread_name = LAZY_INSTANCE_INITIALIZER;

struct ThreadParams {
  PlatformThread::Delegate* delegate;
  bool joinable;
};

void* ThreadFunc(void* params) {
  ThreadParams* thread_params = static_cast<ThreadParams*>(params);
  PlatformThread::Delegate* delegate = thread_params->delegate;
  if (!thread_params->joinable)
    base::ThreadRestrictions::SetSingletonAllowed(false);
  delete thread_params;

  delegate->ThreadMain();
  return NULL;
}

bool CreateThread(size_t stack_size, bool joinable, int priority,
                  PlatformThread::Delegate* delegate,
                  PlatformThreadHandle* thread_handle) {
  bool success = false;
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);

  // Pthreads are joinable by default, so only specify the detached attribute if
  // the thread should be non-joinable.
  if (!joinable) {
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
  }

  static const int kMinStackSize = 1024 * 1024;
  if (stack_size < kMinStackSize) {
    stack_size = kMinStackSize;
  }

  pthread_attr_setstacksize(&attributes, stack_size);

  ThreadParams* params = new ThreadParams;
  params->delegate = delegate;
  params->joinable = joinable;

  success = !pthread_create(thread_handle, &attributes, ThreadFunc, params);
  pthread_attr_destroy(&attributes);

  if (priority >= 0) {
    sched_param sp;
    sp.sched_priority = priority;
    pthread_setschedparam(*thread_handle, SCHED_OTHER, &sp);
  }

  if (!success)
    delete params;

  return success;
}

}  // namespace

// static
PlatformThreadId PlatformThread::CurrentId() {
  return static_cast<int64>(pthread_self());
}

// static
void PlatformThread::YieldCurrentThread() {
  sched_yield();
}

// static
void PlatformThread::Sleep(base::TimeDelta duration) {
  struct timespec sleep_time, remaining;

  // Break the duration into seconds and nanoseconds.
  // NOTE: TimeDelta's microseconds are int64s while timespec's
  // nanoseconds are longs, so this unpacking must prevent overflow.
  sleep_time.tv_sec = duration.InSeconds();
  duration -= TimeDelta::FromSeconds(sleep_time.tv_sec);
  sleep_time.tv_nsec = duration.InMicroseconds() * 1000;  // nanoseconds

  while (nanosleep(&sleep_time, &remaining) == -1 && errno == EINTR)
    sleep_time = remaining;
}

// static
void PlatformThread::SetName(const char* name) {
  current_thread_name.Pointer()->Set(const_cast<char*>(name));
  // setname accepts max length of 15 characters, plus terminator.
  char shortname[16];
  strncpy(shortname, name, sizeof(shortname));
  shortname[15] = 0;
  int value = pthread_setname_np(pthread_self(), shortname);
  if (value != 0 && value != EPERM) {
    DLOG(ERROR) << "Unable to rename thread " << shortname << " : " << value;
    DCHECK(0);
  }
}

// static
const char* PlatformThread::GetName() {
  return current_thread_name.Pointer()->Get();
}

// static
bool PlatformThread::Create(size_t stack_size, Delegate* delegate,
                            PlatformThreadHandle* thread_handle) {
  return CreateThread(stack_size, true /* joinable thread */,
                      -1, /* default priority */
                      delegate, thread_handle);
}

// static
bool PlatformThread::CreateNonJoinable(size_t stack_size, Delegate* delegate) {
  PlatformThreadHandle unused;

  bool result = CreateThread(stack_size, false /* non-joinable thread */,
                             -1, /* default priority */
                             delegate, &unused);
  return result;
}

// static
bool PlatformThread::CreateWithOptions(const PlatformThreadOptions& options,
                                       Delegate* delegate,
                                       PlatformThreadHandle* thread_handle) {
  bool result = CreateThread(options.stack_size, true /* joinable thread */,
                             options.priority,
                             delegate, thread_handle);
  return result;
}

// static
void PlatformThread::Join(PlatformThreadHandle thread_handle) {
  // Joining another thread may block the current thread for a long time, since
  // the thread referred to by |thread_handle| may still be running long-lived /
  // blocking tasks.
  base::ThreadRestrictions::AssertIOAllowed();
  pthread_join(thread_handle, NULL);
}

// static
void PlatformThread::SetThreadPriority(PlatformThreadHandle thread_handle,
                                       ThreadPriority priority) {
  if (priority == kThreadPriority_RealtimeAudio) {
    const struct sched_param kRealTimePrio = { 8 };
    if (pthread_setschedparam(thread_handle, SCHED_RR, &kRealTimePrio) == 0) {
      // Got real time priority, no need to set nice level.
      return;
    }
  }

  DCHECK_EQ(priority, kThreadPriority_Normal);

  // setpriority(2) will set a thread's priority if it is passed a tid as
  // the 'process identifier', not affecting the rest of the threads in the
  // process. Setting this priority will only succeed if the user has been
  // granted permission to adjust nice values on the system.
  DCHECK_NE(thread_handle, kInvalidThreadId);
  const int nice_setting = 0;  // normal
  if (setpriority(PRIO_PROCESS, thread_handle, nice_setting)) {
    DVPLOG(1) << "Failed to set nice value of thread ("
              << thread_handle << ") to " << nice_setting;
  }
}

}  // namespace base
