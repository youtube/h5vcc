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

#ifndef SRC_LB_WEB_GRAPHICS_CONTEXT_3D_COMMAND_BUFFER_H_
#define SRC_LB_WEB_GRAPHICS_CONTEXT_3D_COMMAND_BUFFER_H_

#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/base/synchronization/condition_variable.h"
#include "external/chromium/base/synchronization/lock.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebGraphicsContext3D.h"
#include "external/chromium/ui/gfx/native_widget_types.h"
#include "lb_web_graphics_context_3d.h"

using WebKit::WebGLId;
using WebKit::WGC3Dbyte;
using WebKit::WGC3Dchar;
using WebKit::WGC3Denum;
using WebKit::WGC3Dboolean;
using WebKit::WGC3Dbitfield;
using WebKit::WGC3Dint;
using WebKit::WGC3Dsizei;
using WebKit::WGC3Duint;
using WebKit::WGC3Dfloat;
using WebKit::WGC3Dclampf;
using WebKit::WGC3Dintptr;
using WebKit::WGC3Dsizeiptr;

class LBGLCommandBuffer;

namespace gpu {
class GpuScheduler;
class TransferBufferManagerInterface;
class TransferBuffer;
namespace gles2 {
class ContextGroup;
class GLES2CmdHelper;
class GLES2Decoder;
class GLES2Implementation;
class ShareGroup;
}
}  // namespace gpu

namespace gfx {
class GLContext;
class GLSurface;
class GLShareGroup;
}

void InitializeCommandBufferSystem();
void ShutdownCommandBufferSystem();

class LBWebGraphicsContext3DCommandBuffer : public LBWebGraphicsContext3D {
 public:
  struct InitOptions {
    InitOptions(
      MessageLoop* graphics_message_loop,
      int width, int height,
      gfx::AcceleratedWidget window);

    // The graphics message loop that this context will post commands to
    MessageLoop* graphics_message_loop;

    // The width and height of the output buffer for this context
    int width;
    int height;

    // The parent context specifies a context that can access the output
    // buffer of this context as a texture.
    LBWebGraphicsContext3DCommandBuffer* parent;

    // The widget to send this context's output to
    gfx::AcceleratedWidget window;

    // If specified, a context that we will share our graphics resources
    // with.
    LBWebGraphicsContext3DCommandBuffer* share;

    // The maximum size of the transfer buffer used to send large resources
    // like buffers and textures from the client thread issuing commands to the
    // graphics context and the graphics thread.  If this is too small, regular
    // sync flushes with the graphics thread can occur which will stall the
    // pipeline.
    size_t max_transfer_buffer_size;
  };

  explicit LBWebGraphicsContext3DCommandBuffer(const InitOptions& options);

  virtual ~LBWebGraphicsContext3DCommandBuffer();

  // These functions only need to be called once
  static void InitializeCommandBufferSystem();
  static void ShutdownCommandBufferSystem();

  gpu::gles2::GLES2Implementation* gl() { return gl_.get(); }

#if defined(__LB_XB360__)
  // Binds the given texture ID to the video texture, using the
  // glBindTextureToVideoLBSHELL extension.
  void bindTextureToVideoLBSHELL(WebGLId texture);
#endif

  GRAPHICS_CONTEXT_3D_METHOD_OVERRIDES

 private:
  // Callback for when the command buffer should be flushed.  If sync is
  // true, it should not return until all commands have been processed.
  void PumpCommands(bool sync, int32 put_offset);
  // Actually pumps the commands in the command buffer in to the graphics
  // hardware.
  void ServiceSidePumpCommands(int32 put_offset);

  bool GetBufferChanged(int32 transfer_buffer_id);
  void ServiceSideGetBufferChanged(int32 transfer_buffer_id);

  // Called on the graphics thread to initialize all service side data
  // structures.
  void ServiceSideInitialize(int width, int height,
                             gfx::AcceleratedWidget window);
  // Shut down all service-side data structures
  void ServiceSideShutDown();

  // Called on the graphics thread when setting a new parent context
  void ServiceSideSetParent(LBWebGraphicsContext3DCommandBuffer* parent,
                            uint32 parent_texture_id);

#if defined(__LB_XB360__)
  // Service-side implementation of bindTextureToVideoLBSHELL
  void ServiceSideBindTextureToVideo(WebGLId texture);
#endif

  // When called, will wait for the service side to finish executing all
  // messages in its message queue.
  void SyncWithServiceSide();

  // Keep one instance of all sharing objects.  This implies that all
  // contexts will share resources, but this is okay for us.
  static scoped_refptr<gfx::GLShareGroup> service_share_group_;
  static bool system_initialized_;

  // The graphics_message_loop_ is the message loop is where we post all
  // commands that actually issue graphics commands to the hardware.
  MessageLoop* graphics_message_loop_;

  LBWebGraphicsContext3DCommandBuffer* parent_;
  WebGLId parent_texture_id_;

  // Context to share all GL resources with.
  LBWebGraphicsContext3DCommandBuffer* share_;

  int width_;
  int height_;

  scoped_ptr<LBGLCommandBuffer> command_buffer_;
  scoped_ptr<gpu::TransferBufferManagerInterface> transfer_buffer_manager_;
  scoped_ptr<gpu::TransferBuffer> transfer_buffer_;

  // These client side data structures are manipulated by
  // whichever thread has last called makeContextCurrent on the
  // WebGraphicsContext3D object.  It contains elements required for
  // serializing OpenGL commands in to the command buffer.
  scoped_ptr<gpu::gles2::GLES2CmdHelper> gles2_helper_;
  scoped_ptr<gpu::gles2::GLES2Implementation> gl_;

  // The ServiceSide data structure is managed by the graphics thread and
  // deals with what happens to OpenGL commands as they come out of the
  // command buffer queue.  I.e. this is the side where the OpenGL commands
  // actually get executed.
  struct ServiceSide {
    scoped_ptr<gpu::GpuScheduler> gpu_scheduler_;
    scoped_ptr<gpu::gles2::GLES2Decoder> decoder_;
    scoped_refptr<gfx::GLContext> context_;
    scoped_refptr<gfx::GLSurface> surface_;
  };
  ServiceSide service_side_;

  WebGLId bound_fbo_;

  base::Lock pumping_commands_lock_;
  base::ConditionVariable pumped_commands_cond_;
};

#endif  // SRC_LB_WEB_GRAPHICS_CONTEXT_3D_COMMAND_BUFFER_H_
