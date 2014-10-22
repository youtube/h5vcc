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

#ifndef SRC_LB_GL_COMMAND_BUFFER_H_
#define SRC_LB_GL_COMMAND_BUFFER_H_

#include "base/callback.h"
#include "base/shared_memory.h"
#include "gpu/command_buffer/common/command_buffer.h"
#include "gpu/command_buffer/common/command_buffer_shared.h"

namespace gpu {
class TransferBufferManagerInterface;
}

// An object that implements a shared memory command buffer and a synchronous
// API to manage the put and get pointers.
class GPU_EXPORT LBGLCommandBuffer : public gpu::CommandBuffer {
 public:
  typedef base::Callback<void (bool, int32)> PutBufferChangedCallback;
  typedef base::Callback<bool (int32)> GetBufferChangedCallback;
  explicit LBGLCommandBuffer(
      gpu::TransferBufferManagerInterface* transfer_buffer_manager);
  virtual ~LBGLCommandBuffer();

  // CommandBuffer implementation:
  virtual bool Initialize() OVERRIDE;
  virtual State GetState() OVERRIDE;
  virtual State GetLastState() OVERRIDE;
  virtual void Flush(int32 put_offset) OVERRIDE;
  virtual State FlushSync(int32 put_offset, int32 last_known_get) OVERRIDE;
  virtual void SetGetBuffer(int32 transfer_buffer_id) OVERRIDE;
  virtual void SetGetOffset(int32 get_offset) OVERRIDE;
  virtual int32 CreateTransferBuffer(size_t size, int32 id_request) OVERRIDE;
  virtual int32 RegisterTransferBuffer(base::SharedMemory* shared_memory,
                                       size_t size,
                                       int32 id_request) OVERRIDE;
  virtual void DestroyTransferBuffer(int32 id) OVERRIDE;
  virtual gpu::Buffer GetTransferBuffer(int32 handle) OVERRIDE;
  virtual void SetToken(int32 token) OVERRIDE;
  virtual void SetParseError(gpu::error::Error error) OVERRIDE;
  virtual void SetContextLostReason(gpu::error::ContextLostReason) OVERRIDE;

  // Sets a callback that is called whenever the put offset is changed. When
  // called with sync==true, the callback must not return until some progress
  // has been made (unless the command buffer is empty), i.e. the get offset
  // must have changed. It need not process the entire command buffer though.
  // This allows concurrency between the writer and the reader while giving the
  // writer a means of waiting for the reader to make some progress before
  // attempting to write more to the command buffer. Takes ownership of
  // callback.
  virtual void SetPutOffsetChangeCallback(
      const PutBufferChangedCallback& callback);
  // Sets a callback that is called whenever the get buffer is changed.
  virtual void SetGetBufferChangeCallback(
      const GetBufferChangedCallback& callback);
  virtual void SetParseErrorCallback(const base::Closure& callback);

  // Setup the transfer buffer that shared state should be copied into.
  void SetSharedStateBuffer(int32 transfer_buffer_id);

  // Copy the current state into the shared state transfer buffer.
  void UpdateState();

 private:
  int32 ring_buffer_id_;
  gpu::Buffer ring_buffer_;
  gpu::CommandBufferSharedState* shared_state_;
  int32 num_entries_;
  int32 get_offset_;
  int32 put_offset_;
  PutBufferChangedCallback put_offset_change_callback_;
  GetBufferChangedCallback get_buffer_change_callback_;
  base::Closure parse_error_callback_;
  gpu::TransferBufferManagerInterface* transfer_buffer_manager_;
  int32 token_;
  uint32 generation_;
  gpu::error::Error error_;
  gpu::error::ContextLostReason context_lost_reason_;

  DISALLOW_COPY_AND_ASSIGN(LBGLCommandBuffer);
};


#endif  // SRC_LB_GL_COMMAND_BUFFER_H_
