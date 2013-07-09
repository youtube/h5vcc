/*
 * Copyright 2013 Google Inc. All Rights Reserved.
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
#ifndef _DLMALLOC_CONFIG_H_
#define _DLMALLOC_CONFIG_H_

#include "lb_shell/lb_mutex.h"

// Want this for all our targets so these symbols
// dont conflict with the normal malloc functions.
#define USE_DL_PREFIX 1

#if !defined (__LB_SHELL__FOR_RELEASE__)
#define MALLOC_INSPECT_ALL 1
#endif

// Only turn this on if needed- it's quite slow.
//#define DEBUG 1
#define ABORT_ON_ASSERT_FAILURE 0

#define USE_LOCKS 2 // Use our custom lock implementation
#define USE_SPIN_LOCKS 0
/* Locks */
#define MLOCK_T lb_shell_mutex_t
#define INITIAL_LOCK(lk) lb_shell_mutex_init(lk)
#define DESTROY_LOCK(lk)  lb_shell_mutex_destroy(lk)
#define ACQUIRE_LOCK(lk)  lb_shell_mutex_lock(lk)
#define RELEASE_LOCK(lk)  lb_shell_mutex_unlock(lk)
#define TRY_LOCK(lk) lb_shell_mutex_trylock(lk)
static MLOCK_T malloc_global_mutex;

#define MORECORE lbshell_morecore
static void* lbshell_morecore(int size);
static void* lbshell_mmap(size_t size);
static int lbshell_munmap(void* addr, size_t size);

#if !defined (__LB_SHELL__FOR_RELEASE__)
void lbshell_inspect_mmap_heap(void(*handler)(void*, void *, size_t, void*),
                           void* arg);
#endif
void dl_malloc_init();

#if defined(__LB_LINUX__)
#define HAVE_MREMAP 0
#include <stdint.h>
// For emulation purposes, we'll say 64K, regardless
// of what the system is using.
#define PAGESIZE ((size_t)(64 * 1024U))
#define malloc_getpagesize PAGESIZE
#define HAVE_MORECORE 1
#define HAVE_MMAP 2 // 2 == use our custom implementation
#define USE_BUILTIN_FFS 1
#define DEFAULT_MMAP_THRESHOLD ((size_t)(256 * 1024U))

#elif defined(__LB_PS3__)
#define HAVE_MORECORE 1
#define HAVE_MMAP 2 // 2 == use our custom implementation
#define LACKS_SYS_MMAN_H
#define MALLOC_ALIGNMENT ((size_t)16U)
#define USE_BUILTIN_FFS 1
// 64K size pages pay some price in performance
// but we get more granularity.
#define PAGESIZE ((size_t)(64 * 1024U))
#define DEFAULT_MMAP_THRESHOLD ((size_t)(256 * 1024U))

#elif defined(__LB_WIIU__)
#define PAGESIZE ((size_t)(128 * 1024U))
#define HAVE_MORECORE 1
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT ((size_t)8U)
#define USE_BUILTIN_FFS 1
#endif /* defined (__LB_LINUX__) */

#endif /* ifndef _DLMALLOC_CONFIG_H_ */