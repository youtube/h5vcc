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
// Platform-specific mutexes.
#ifndef SRC_LB_MUTEX_H_
#define SRC_LB_MUTEX_H_

#include <pthread.h>

typedef pthread_mutex_t lb_shell_mutex_t;

#define lb_shell_mutex_lock(mutexp) \
    pthread_mutex_lock(mutexp)

// 0 == success
#define lb_shell_mutex_trylock(mutexp) \
    pthread_mutex_trylock(mutexp)

#define lb_shell_mutex_unlock(mutexp) \
    pthread_mutex_unlock(mutexp)

#define lb_shell_mutex_init(mutexp) ({ \
      pthread_mutexattr_t attr; \
      pthread_mutexattr_init(&attr); \
      pthread_mutex_init(mutexp, &attr); \
    })

#define lb_shell_mutex_destroy(mutexp) \
    pthread_mutex_destroy(mutexp)


typedef pthread_cond_t lb_shell_cond_t;

#define lb_shell_cond_wait(condp, mutexp) \
    pthread_cond_wait(condp, mutexp)

#define lb_shell_cond_timedwait(condp, mutexp, usecs) ({ \
      struct timespec abstime; \
      clock_gettime(CLOCK_REALTIME, &abstime); \
      abstime.tv_sec += usecs / (1000 * 1000); \
      abstime.tv_nsec += (usecs % (1000 * 1000)) * 1000; \
      if (abstime.tv_nsec > 1000 * 1000 * 1000) { \
        abstime.tv_nsec -= 1000 * 1000 * 1000; \
        abstime.tv_sec += 1; \
      } \
      pthread_cond_timedwait(condp, mutexp, &abstime); \
    })

#define lb_shell_cond_broadcast(condp) \
    pthread_cond_broadcast(condp)

#define lb_shell_cond_signal(condp) \
    pthread_cond_signal(condp)

#define lb_shell_cond_init(condp, mutexp) ({\
      pthread_condattr_t attr; \
      pthread_condattr_init(&attr); \
      pthread_cond_init(condp, &attr); \
    })

#define lb_shell_cond_destroy(condp) \
    pthread_cond_destroy(condp)

#endif
