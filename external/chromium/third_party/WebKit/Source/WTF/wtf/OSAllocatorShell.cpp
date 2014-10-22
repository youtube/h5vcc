#include "config.h"
#include "OSAllocator.h"

#include <malloc.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <wtf/ThreadingPrimitives.h>

namespace WTF {

// ===== on allocating =====

// Allocations must be aligned to >= getpagesize().
// getpagesize() must be <= 64k for PageAllocationAligned.
// getpagesize() must be <= JSC::RegisterFile::commitSize (default 16k).
// To minimize waste in PageAllocationAligned, getpagesize() must be 64k
// and commitSize must be increased to 64k.


// Define this when performing leak analysis.  It will avoid pooling memory.
//#define LEAK_ANALYSIS


// Special case allocator for managing a fixed pool of 64K-size, 64K-aligned
// blocks.  JavaScript heap is a big consumer of these.
class ShellPageAllocator {
 public:
  ~ShellPageAllocator() {
    free(buffer_orig_);
    free(free_bitmap_);
  }

  void* allocatePage() {
    void* p = NULL;
    WTF::MutexLocker lock(mutex_);
    for (int i = 0; i < bitmap_size_; ++i) {
      if (free_bitmap_[i] != 0) {
        // index of the first bit set in this integer.
        int first_free = ffs(free_bitmap_[i]) - 1;
        int page_index = i * 32 + first_free;
        p = (void*) (uintptr_t(buffer_) + kPageSize * page_index);
        clear_bit(i, first_free);
#if !defined(__LB_SHELL__FOR_RELEASE__)
        excess_page_count_ = 0;
#endif
        break;
      }
    }
#if !defined(__LB_SHELL__FOR_RELEASE__)
    if (!p) {
      excess_page_count_++;
      excess_high_water_mark_ =
          std::max(excess_high_water_mark_, excess_page_count_);
    }
#endif
    return p;
  }

  void freePage(void* p) {
    uintptr_t address = (uintptr_t)p;
    ASSERT(address % kPageSize == 0);
    uintptr_t start = (uintptr_t)buffer_;
    uint32_t offset = (address - start) / kPageSize;
    uint32_t int_index = offset / 32;
    uint32_t bit_index = offset % 32;
    WTF::MutexLocker lock(mutex_);
    set_bit(int_index, bit_index);
  }

  // A valid page is one that was allocated from buffer_.
  bool isValidPage(void* p) const {
    uintptr_t address = (uintptr_t)p;
    uintptr_t start = (uintptr_t)buffer_;
    if (address >= start && address < start + buffer_size_) {
      return true;
    } else {
      return false;
    }
  }

  static ShellPageAllocator* getInstance() {
    if (!instance_) {
#if defined(LEAK_ANALYSIS)
      static const int kBufferSize = 64 * 1024;
#else
      static const int kBufferSize = 12 * 1024 * 1024;
#endif
      instance_ = new ShellPageAllocator(kBufferSize);
    }
    return instance_;
  }

  // Use the system allocator to get an aligned block.
  void* allocateBlock(size_t alignment, size_t size) {
    // Overallocate so we can guarantee an aligned block.
    // We write the address of the original pointer just behind
    // the block to be returned.
    void* ptr = malloc(size + alignment + sizeof(void*));
    void* aligned_ptr = alignPtr((void*)((uintptr_t)ptr + sizeof(void*)), alignment);

    // Check that we have room to write the header
    ASSERT((uintptr_t)aligned_ptr - (uintptr_t)ptr >= sizeof(void*));
    void** orig_ptr = (void**)((uintptr_t)aligned_ptr - sizeof(void*));
    *orig_ptr = ptr;
    return aligned_ptr;
  }

  // Free a block that was allocated with allocateBlock()
  void freeBlock(void* ptr) {
    void* orig_ptr = *(void**)((uintptr_t)ptr - sizeof(void*));
    free(orig_ptr);
  }

  static bool instanceExists() {
    return instance_ != NULL;
  }

  // This is not necessarily related to the OS page size.
  // JSC really wants 64K alignment so if our 'pages' are always 64K,
  // there is minimal waste.
  static const size_t kPageSize = 64 * 1024;

#if !defined(__LB_SHELL__FOR_RELEASE__)
  void updateAllocatedBytes(int bytes) {
    current_bytes_allocated_ += bytes;
  }
  int getCurrentBytesAllocated() const {
    return current_bytes_allocated_;
  }
#endif

 private:
  static ShellPageAllocator* instance_;

  void* buffer_;
  void* buffer_orig_;
  size_t buffer_size_;
  uint32_t* free_bitmap_;
  int bitmap_size_;

  WTF::Mutex mutex_;

  // Records how many pages couldn't fit in the buffer.
  // We can use this to size our buffer appropriately.
  // The high water mark is the maximum number of page requests
  // we couldn't fulfill for the current run.
#if !defined(__LB_SHELL__FOR_RELEASE__)
  int current_bytes_allocated_;
  int excess_page_count_;
  int excess_high_water_mark_;
#endif

  ShellPageAllocator(size_t buffer_size);

  static inline uint32_t align(uint32_t value, uint32_t align) {
    return (value + align - 1) & ~(align - 1);
  }

  static inline void* alignPtr(void* ptr, size_t alignment) {
    return (void*)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
  }

  void set_bit(uint32_t index, uint32_t bit) {
    ASSERT(index < bitmap_size_);
    free_bitmap_[index] |= (1 << bit);
  }

  void clear_bit(uint32_t index, uint32_t bit) {
    ASSERT(index < bitmap_size_);
    free_bitmap_[index] &= ~(1 << bit);
  }
};

// static
ShellPageAllocator* ShellPageAllocator::instance_ = NULL;

ShellPageAllocator::ShellPageAllocator(size_t buffer_size) {
  buffer_size_ = buffer_size;
  const int page_count = align(buffer_size_, kPageSize) / kPageSize;
  bitmap_size_ = align(page_count, 32) / 32;

  buffer_orig_ = malloc(buffer_size_ + kPageSize);
  buffer_ = alignPtr(buffer_orig_, kPageSize);

  ASSERT(buffer_ != NULL);
  free_bitmap_ = (uint32_t*)malloc(bitmap_size_ * sizeof(uint32_t));
  ASSERT(free_bitmap_);

  for (int i = 0; i < bitmap_size_; ++i) {
    free_bitmap_[i] = 0xffffffff;
  }

  // If number of pages doesn't fit neatly into bitmap, mark any trailing
  // extra bits as allocated so we never return them.
  const int trailing_bits = (32 - (page_count % 32)) % 32;
  for (int i = 32 - trailing_bits; i < 32; ++i) {
    clear_bit(bitmap_size_ - 1, i);
  }

#if !defined(__LB_SHELL__FOR_RELEASE__)
  current_bytes_allocated_ = 0;
  excess_page_count_ = 0;
  excess_high_water_mark_ = 0;
#endif
}

// static
void* OSAllocator::reserveUncommitted(size_t vm_size, Usage usage, bool writable, bool executable, bool includesGuardPages)
{
  ShellPageAllocator* allocator = ShellPageAllocator::getInstance();

#if !defined(__LB_SHELL__FOR_RELEASE__)
  allocator->updateAllocatedBytes(vm_size);
#endif

  void* p = 0;
  if (vm_size == ShellPageAllocator::kPageSize) {
    p = allocator->allocatePage();
  }
  if (!p) {
    const size_t alignment = usage == JSUnalignedPages ? 16 : ShellPageAllocator::kPageSize;
    return allocator->allocateBlock(alignment, vm_size);
  }
  return p;
}

// static
void OSAllocator::releaseDecommitted(void* addr, size_t size)
{
  ShellPageAllocator* allocator = ShellPageAllocator::getInstance();
  if (allocator->isValidPage(addr)) {
    allocator->freePage(addr);
  } else {
    allocator->freeBlock(addr);
  }

#if !defined(__LB_SHELL__FOR_RELEASE__)
  allocator->updateAllocatedBytes(-(int)size);
#endif
}

// static
void OSAllocator::commit(void* addr, size_t size, bool writable, bool executable)
{
  // no commit/decommit scheme on these platforms
}

// static
void OSAllocator::decommit(void* addr, size_t size)
{
  // no commit/decommit scheme on these platforms
}

// static
void* OSAllocator::reserveAndCommit(size_t size, Usage usage, bool writable, bool executable, bool includesGuardPages)
{
  // no commit/decommit scheme on these platforms
  return reserveUncommitted(size, usage, writable, executable, includesGuardPages);
}

// static
size_t OSAllocator::getCurrentBytesAllocated() {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (ShellPageAllocator::instanceExists()) {
    return ShellPageAllocator::getInstance()->getCurrentBytesAllocated();
  } else {
    return 0;
  }
#else
  return 0;
#endif
}

} // namespace WTF
