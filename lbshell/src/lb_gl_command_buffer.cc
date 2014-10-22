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


// This file is forked from:
// external/chromium/gpu/command_buffer/service/command_buffer_service.cc
// The main motivation for forking it was to add code to avoid busy-waiting
// on FlushSync calls.

#include "lb_gl_command_buffer.h"

#include <limits>

#include "base/process_util.h"
#include "base/debug/trace_event.h"
#include "gpu/command_buffer/common/cmd_buffer_common.h"
#include "gpu/command_buffer/common/command_buffer_shared.h"
#include "gpu/command_buffer/service/transfer_buffer_manager.h"

using ::base::SharedMemory;

LBGLCommandBuffer::LBGLCommandBuffer(
    gpu::TransferBufferManagerInterface* transfer_buffer_manager)
    : ring_buffer_id_(-1),
      shared_state_(NULL),
      num_entries_(0),
      get_offset_(0),
      put_offset_(0),
      transfer_buffer_manager_(transfer_buffer_manager),
      token_(0),
      generation_(0),
      error_(gpu::error::kNoError),
      context_lost_reason_(gpu::error::kUnknown) {
}

LBGLCommandBuffer::~LBGLCommandBuffer() {
}

bool LBGLCommandBuffer::Initialize() {
  return true;
}

LBGLCommandBuffer::State LBGLCommandBuffer::GetState() {
  State state;
  state.num_entries = num_entries_;
  state.get_offset = get_offset_;
  state.put_offset = put_offset_;
  state.token = token_;
  state.error = error_;
  state.context_lost_reason = context_lost_reason_;
  state.generation = ++generation_;

  return state;
}

LBGLCommandBuffer::State LBGLCommandBuffer::GetLastState() {
  return GetState();
}

void LBGLCommandBuffer::UpdateState() {
  if (shared_state_) {
    LBGLCommandBuffer::State state = GetState();
    shared_state_->Write(state);
  }
}

LBGLCommandBuffer::State LBGLCommandBuffer::FlushSync(
    int32 put_offset, int32 last_known_get) {
  TRACE_EVENT0("lb_graphics", "LBGLCommandBuffer::FlushSync");

  if (put_offset < 0 || put_offset > num_entries_) {
    error_ = gpu::error::kOutOfBounds;
    return GetState();
  }

  put_offset_ = put_offset;

  // Signal that the put offset has changed and that there are commands
  // in need of processing.  Also indicate that we should not return from
  // the callback until the get offset has been modified.
  if (!put_offset_change_callback_.is_null())
    put_offset_change_callback_.Run(true, put_offset_);

  return GetState();
}

void LBGLCommandBuffer::Flush(int32 put_offset) {
  TRACE_EVENT0("lb_graphics", "LBGLCommandBuffer::Flush");

  if (put_offset < 0 || put_offset > num_entries_) {
    error_ = gpu::error::kOutOfBounds;
    return;
  }

  put_offset_ = put_offset;

  // Signal that the put offset has changed and that there are commands
  // in need of processing.
  if (!put_offset_change_callback_.is_null())
    put_offset_change_callback_.Run(false, put_offset_);
}

void LBGLCommandBuffer::SetGetBuffer(int32 transfer_buffer_id) {
  DCHECK_EQ(-1, ring_buffer_id_);
  DCHECK_EQ(put_offset_, get_offset_);  // Only if it's empty.
  ring_buffer_ = GetTransferBuffer(transfer_buffer_id);
  DCHECK(ring_buffer_.ptr);
  ring_buffer_id_ = transfer_buffer_id;
  num_entries_ = ring_buffer_.size / sizeof(gpu::CommandBufferEntry);
  put_offset_ = 0;
  SetGetOffset(0);

  // Indicate that commands have been processed (i.e. the get buffer has
  // changed)
  if (!get_buffer_change_callback_.is_null()) {
    get_buffer_change_callback_.Run(ring_buffer_id_);
  }

  UpdateState();
}

void LBGLCommandBuffer::SetSharedStateBuffer(int32 transfer_buffer_id) {
  gpu::Buffer buffer = GetTransferBuffer(transfer_buffer_id);
  shared_state_ = reinterpret_cast<gpu::CommandBufferSharedState*>(buffer.ptr);

  UpdateState();
}

void LBGLCommandBuffer::SetGetOffset(int32 get_offset) {
  DCHECK(get_offset >= 0 && get_offset < num_entries_);
  get_offset_ = get_offset;
}

int32 LBGLCommandBuffer::CreateTransferBuffer(size_t size,
                                                 int32 id_request) {
  return transfer_buffer_manager_->CreateTransferBuffer(size, id_request);
}

int32 LBGLCommandBuffer::RegisterTransferBuffer(
    base::SharedMemory* shared_memory, size_t size, int32 id_request) {
  return transfer_buffer_manager_->RegisterTransferBuffer(
      shared_memory, size, id_request);
}

void LBGLCommandBuffer::DestroyTransferBuffer(int32 handle) {
  transfer_buffer_manager_->DestroyTransferBuffer(handle);
  if (handle == ring_buffer_id_) {
    ring_buffer_id_ = -1;
    ring_buffer_ = gpu::Buffer();
    num_entries_ = 0;
    get_offset_ = 0;
    put_offset_ = 0;
  }
}

gpu::Buffer LBGLCommandBuffer::GetTransferBuffer(int32 handle) {
  return transfer_buffer_manager_->GetTransferBuffer(handle);
}

void LBGLCommandBuffer::SetToken(int32 token) {
  token_ = token;
}

void LBGLCommandBuffer::SetParseError(gpu::error::Error error) {
  if (error_ == gpu::error::kNoError) {
    error_ = error;
    if (!parse_error_callback_.is_null())
      parse_error_callback_.Run();
  }
}

void LBGLCommandBuffer::SetContextLostReason(
    gpu::error::ContextLostReason reason) {
  context_lost_reason_ = reason;
}

void LBGLCommandBuffer::SetPutOffsetChangeCallback(
    const PutBufferChangedCallback& callback) {
  put_offset_change_callback_ = callback;
}

void LBGLCommandBuffer::SetGetBufferChangeCallback(
    const GetBufferChangedCallback& callback) {
  get_buffer_change_callback_ = callback;
}

void LBGLCommandBuffer::SetParseErrorCallback(
    const base::Closure& callback) {
  parse_error_callback_ = callback;
}

