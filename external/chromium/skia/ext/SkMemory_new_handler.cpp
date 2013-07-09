// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "SkMemory_new_handler.h"

#include <stdio.h>
#include <new>

#include "base/process_util.h"

#include "third_party/skia/include/core/SkTypes.h"
#include "third_party/skia/include/core/SkThread.h"

// This implementation of sk_malloc_flags() and friends is identical
// to SkMemory_malloc.c, except that it disables the CRT's new_handler
// during malloc(), when SK_MALLOC_THROW is not set (ie., when
// sk_malloc_flags() would not abort on NULL).

class SkMemoryStats {
 public:
    SkMemoryStats() {
        bytes_allocated_ = 0;
        max_bytes_allocated_ = 0;
    }
#if defined (__LB_SHELL__FOR_RELEASE__)
    void updateBytes(size_t bytes) {}
    size_t getBlockSize(void* ptr) {return 0;}
#else
    void updateBytes(size_t bytes) {
        bytes_allocated_ += bytes;
        max_bytes_allocated_ = std::max(max_bytes_allocated_, bytes_allocated_);
    }
    size_t getBlockSize(void* ptr) {
        if (block_size_func_) {
            return block_size_func_(ptr);
        } else {
            return 0;
        }
    }
#endif
    size_t getBytesAllocated() const { return bytes_allocated_; }
    size_t getMaxBytesAllocated() const { return max_bytes_allocated_; }
    void setBlockSizeFunc(BlockSizeFunc f) { block_size_func_ = f; }
 private:
    size_t bytes_allocated_;
    size_t max_bytes_allocated_;
    static BlockSizeFunc block_size_func_;
};

static SkMemoryStats gSkMemStats;
BlockSizeFunc SkMemoryStats::block_size_func_;

size_t sk_get_bytes_allocated() {
    return gSkMemStats.getBytesAllocated();
}
size_t sk_get_max_bytes_allocated() {
    return gSkMemStats.getMaxBytesAllocated();
}

void sk_set_block_size_func(BlockSizeFunc f) {
    gSkMemStats.setBlockSizeFunc(f);
}

SK_DECLARE_STATIC_MUTEX(gSkNewHandlerMutex);

void sk_throw() {
    SkASSERT(!"sk_throw");
    abort();
}

void sk_out_of_memory(void) {
    SkASSERT(!"sk_out_of_memory");
    abort();
}

void* sk_malloc_throw(size_t size) {
    return sk_malloc_flags(size, SK_MALLOC_THROW);
}

void* sk_realloc_throw(void* addr, size_t size) {
    gSkMemStats.updateBytes(size - gSkMemStats.getBlockSize(addr));
    void* p = realloc(addr, size);
    if (size == 0) {
        return p;
    }
    if (p == NULL) {
        sk_throw();
    }
    return p;
}

void sk_free(void* p) {
    gSkMemStats.updateBytes(-gSkMemStats.getBlockSize(p));
    if (p) {
        free(p);
    }
}

void* sk_malloc_flags(size_t size, unsigned flags) {
    gSkMemStats.updateBytes(size);

    void* p;
#if defined(ANDROID)
    // Android doesn't have std::set_new_handler.
    p = malloc(size);
#else
    if (!(flags & SK_MALLOC_THROW)) {
#if defined(OS_MACOSX) && !defined(OS_IOS)
      p = base::UncheckedMalloc(size);
#else
      SkAutoMutexAcquire lock(gSkNewHandlerMutex);
      std::new_handler old_handler = std::set_new_handler(NULL);
      p = malloc(size);
      std::set_new_handler(old_handler);
#endif
    } else {
      p = malloc(size);
    }
#endif
    if (p == NULL) {
        if (flags & SK_MALLOC_THROW) {
            sk_throw();
        }
    }
    return p;
}
