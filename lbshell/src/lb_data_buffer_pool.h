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
// This object provides support of reusable data block or blocks.
// User calls AcquireDataBuffer to get a memory block. If there is no block
// available, a new block will be created. User needs to call ReturnBuffer
// after process is done, otherwise the memory block might be leaked.

#ifndef SRC_LB_DATA_BUFFER_POOL_H_
#define SRC_LB_DATA_BUFFER_POOL_H_

#include "external/chromium/base/synchronization/lock.h"
#include "external/chromium/base/logging.h"
#include "lb_memory_manager.h"

// Base class for data buffer pool classes
template<typename T>
class DataBufferPoolBase {
 public:
  // Acquire a buffer
  T AcquireDataBuffer() {
    base::AutoLock lock(lock_);
    if (!buffers_.empty()) {
      T buffer = buffers_.back();
      buffers_.pop_back();
      return buffer;
    }
    // Not found, create a new object
    T buffer = CreateObject();
    return buffer;
  }

  // Return buffer to pool.
  void ReturnBuffer(T data) {
    base::AutoLock lock(lock_);
    buffers_.push_back(data);
  }

 protected:
  // Sub class needs to provide a function to create a data object
  virtual T CreateObject() = 0;

  std::vector<T> buffers_;    // Data buffers
  base::Lock lock_;           // Protect data buffers
};

// Buffer pool for raw pointers
template<typename T>
class DataPointerBufferPool : public DataBufferPoolBase<T*> {
 public:
  typedef DataBufferPoolBase<T*> BaseClass;
  // Release all cached objects
  virtual ~DataPointerBufferPool() { Clear(); }

  void Clear() {
    base::AutoLock lock(BaseClass::lock_);
    for (int i = BaseClass::buffers_.size() - 1; i >= 0; --i) {
      delete BaseClass::buffers_[i];
    }
    BaseClass::buffers_.resize(0);
  }

 private:
  virtual T* CreateObject() { return new T; }
};

// Buffer pool for reference counted objects
template<typename T>
class DataRefPtrBufferPool : public DataBufferPoolBase<scoped_refptr<T> > {
 private:
  virtual scoped_refptr<T> CreateObject() { return new T; }
};

// A simple array object with size information
template<typename T>
struct ArrayDataItemObject {
  T*         raw_pointer;
  uint32_t   size;  // Number of items in raw_pointer. Use 1 for single object
};

// Buffer pool for array data objects
template<typename T, uint32_t capacity>
class DataArrayBufferPool {
 public:
  typedef ArrayDataItemObject<T> DataObject;
  virtual ~DataArrayBufferPool() { Clear(); }

  // Acquire a buffer with at least "size" objects
  DataObject AcquireDataArrayBuffer(const uint32_t size) {
    base::AutoLock lock(lock_);
    DataObject buffer;
    if (!buffers_.empty()) {
      buffer = buffers_.back();
      buffers_.pop_back();
      if (buffer.size < size) {
        delete [] buffer.raw_pointer;
        buffer = CreateObject(size);
      }
    } else {
      // Not found, create a new object
      buffer = CreateObject(size);
    }
    return buffer;
  }

  // Return buffer to pool. parameter size needs to be the same as when
  // the buffer is created with AcquireDataArrayBuffer
  void ReturnArrayBuffer(T* data, const uint32_t size) {
    base::AutoLock lock(lock_);
    DataObject buffer;
    DCHECK(data);
    DCHECK_GE(size, capacity);
    buffer.raw_pointer = data;
    buffer.size = size;
    buffers_.push_back(buffer);
  }

  // Release all cached objects
  void Clear() {
    base::AutoLock lock(lock_);
    for (int i = buffers_.size() - 1; i >= 0; --i) {
      delete [] buffers_[i].raw_pointer;
    }
    buffers_.resize(0);
  }

 private:
  virtual DataObject CreateObject(const uint32_t size) {
    DataObject buffer;
    const uint32_t actual_size = std::max(capacity, size);
    buffer.raw_pointer = new T[actual_size];
    buffer.size = actual_size;
    return buffer;
  }

  std::vector<DataObject> buffers_;    // Data buffers
  base::Lock lock_;                    // Protect data buffers
};

#endif  // SRC_LB_DATA_BUFFER_POOL_H_

