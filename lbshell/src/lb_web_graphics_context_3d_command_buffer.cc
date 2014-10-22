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
#include "lb_web_graphics_context_3d_command_buffer.h"

#if defined(__LB_XB360__)
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#include "external/chromium/base/bind.h"
#include "external/chromium/base/bind_helpers.h"
#include "external/chromium/base/callback.h"
#include "external/chromium/base/debug/trace_event.h"
#include "external/chromium/base/synchronization/waitable_event.h"
#include "external/chromium/gpu/command_buffer/client/gles2_implementation.h"
#include "external/chromium/gpu/command_buffer/client/gles2_lib.h"
#include "external/chromium/gpu/command_buffer/client/transfer_buffer.h"
#include "external/chromium/gpu/command_buffer/common/constants.h"
#include "external/chromium/gpu/command_buffer/service/context_group.h"
#include "external/chromium/gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "external/chromium/gpu/command_buffer/service/gpu_scheduler.h"
#include "external/chromium/gpu/command_buffer/service/transfer_buffer_manager.h"
#include "external/chromium/ui/gl/gl_context.h"
#include "external/chromium/ui/gl/gl_surface.h"
#include "external/chromium/webkit/gpu/gl_bindings_skia_cmd_buffer.h"

#include "lb_gl_command_buffer.h"

namespace {
const int32 kCommandBufferSize = 1024 * 1024;
const size_t kMinTransferBufferSize = 1 * 256 * 1024;
const size_t kDefaultMaxTransferBufferSize = 16 * 1024 * 1024;
}

scoped_refptr<gfx::GLShareGroup>
    LBWebGraphicsContext3DCommandBuffer::service_share_group_;
bool LBWebGraphicsContext3DCommandBuffer::system_initialized_ = false;

LBWebGraphicsContext3DCommandBuffer::InitOptions::InitOptions(
    MessageLoop* graphics_message_loop,
    int width, int height,
    gfx::AcceleratedWidget window)
    : graphics_message_loop(graphics_message_loop)
    , width(width)
    , height(height)
    , parent(NULL)
    , window(window)
    , share(NULL)
    , max_transfer_buffer_size(kDefaultMaxTransferBufferSize) {
  DCHECK_GE(width, 1);
  DCHECK_GE(height, 1);
}

void LBWebGraphicsContext3DCommandBuffer::InitializeCommandBufferSystem() {
  gles2::Initialize();

  system_initialized_ = true;
}

void LBWebGraphicsContext3DCommandBuffer::ShutdownCommandBufferSystem() {
  system_initialized_ = false;

  gles2::Terminate();
}

LBWebGraphicsContext3DCommandBuffer::LBWebGraphicsContext3DCommandBuffer(
    const InitOptions& options)
    : parent_(NULL)
    , parent_texture_id_(0)
    , bound_fbo_(0)
    , pumped_commands_cond_(&pumping_commands_lock_) {
  DCHECK(system_initialized_);

  width_ = options.width;
  height_ = options.height;

  DCHECK(options.graphics_message_loop);
  graphics_message_loop_ = options.graphics_message_loop;

  share_ = options.share;

  {
    gpu::TransferBufferManager* tbm = new gpu::TransferBufferManager();
    tbm->Initialize();
    transfer_buffer_manager_.reset(tbm);
  }

  command_buffer_.reset(
      new LBGLCommandBuffer(transfer_buffer_manager_.get()));
  if (!command_buffer_->Initialize()) {
    LOG(FATAL) << "Could not initialize command buffer.";
  }

  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBWebGraphicsContext3DCommandBuffer::ServiceSideInitialize,
                 base::Unretained(this),
                 options.width, options.height, options.window));

  command_buffer_->SetPutOffsetChangeCallback(
      base::Bind(
          &LBWebGraphicsContext3DCommandBuffer::PumpCommands,
          base::Unretained(this)));
  command_buffer_->SetGetBufferChangeCallback(
      base::Bind(
          &LBWebGraphicsContext3DCommandBuffer::GetBufferChanged,
          base::Unretained(this)));

  // Create the GLES2 helper, which writes the command buffer protocol.
  gles2_helper_.reset(new gpu::gles2::GLES2CmdHelper(command_buffer_.get()));
  if (!gles2_helper_->Initialize(kCommandBufferSize)) {
    DLOG(FATAL) << "Could not create the GLES2 command buffer helper.";
  }

  // Create a transfer buffer.
  transfer_buffer_.reset(new gpu::TransferBuffer(gles2_helper_.get()));

  gpu::gles2::ShareGroup* share_group = NULL;
  if (share_) {
    share_group = options.share->gl()->share_group();
  }

  // Create the object exposing the OpenGL API.
  gl_.reset(new gpu::gles2::GLES2Implementation(
      gles2_helper_.get(),
      share_group,
      transfer_buffer_.get(),
      true,
      false));

  // We initialize the transfer buffer starting size to its max size.  This
  // is because for __LB_SHELL__, if we were to require more than the max
  // transfer buffer size, we would prefer slow performance over surprise
  // increased memory usage, which could result in a crash since we have
  // restricted memory.  Ideally though, this value is set so that we really
  // never exceed it.
  if (!gl_->Initialize(
      options.max_transfer_buffer_size,
      kMinTransferBufferSize,
      options.max_transfer_buffer_size)) {
    DLOG(FATAL) << "Could not initialize GLES2Implementation.";
  }

  setParentContext(options.parent);
}

void LBWebGraphicsContext3DCommandBuffer::ServiceSideInitialize(
    int width, int height,
    gfx::AcceleratedWidget window) {
  DCHECK_EQ(MessageLoop::current(), graphics_message_loop_);

  gpu::gles2::ContextGroup* context_group = NULL;
  if (share_) {
    context_group = share_->service_side_.decoder_->GetContextGroup();
  } else {
    context_group = new gpu::gles2::ContextGroup(NULL, NULL, NULL, false);
  }
  service_side_.decoder_.reset(gpu::gles2::GLES2Decoder::Create(
      context_group));

  service_side_.gpu_scheduler_.reset(new gpu::GpuScheduler(
      command_buffer_.get(),
      service_side_.decoder_.get(),
      service_side_.decoder_.get()));

  service_side_.decoder_->set_engine(service_side_.gpu_scheduler_.get());

  if (window) {
    service_side_.surface_ = gfx::GLSurface::CreateViewGLSurface(false, window);
  } else {
    service_side_.surface_ = gfx::GLSurface::CreateOffscreenGLSurface(
        false, gfx::Size(width, height));
  }
  DCHECK(service_side_.surface_.get());

  if (service_share_group_.get() == NULL) {
    service_share_group_ = new gfx::GLShareGroup();
  }

  // Share the API resources only so that we can access/render to parent
  // textures
  service_side_.context_ = gfx::GLContext::CreateGLContext(
      service_share_group_.get(),
      service_side_.surface_.get(),
      gfx::PreferDiscreteGpu);
  DCHECK(service_side_.context_.get());

  if (!service_side_.context_->MakeCurrent(service_side_.surface_.get())) {
    LOG(FATAL) << "Could not make context current.";
  }

  // Graphics context attributes, empty vector means default settings.
  std::vector<int> config_attribs;

#if defined(__LB_ANDROID__)
  // From EGL/egl.h
  const int32 EGL_ALPHA_SIZE = 0x3021;
  const int32 EGL_BLUE_SIZE = 0x3022;
  const int32 EGL_GREEN_SIZE = 0x3023;
  const int32 EGL_RED_SIZE = 0x3024;

  // Make sure that a surfaces with an alpha channel get created to support
  // video punch out
  int attribs[] = {
    EGL_ALPHA_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
  };
  config_attribs.assign(attribs, attribs + arraysize(attribs));
#endif

  gpu::gles2::DisallowedFeatures disallowed_features;
  disallowed_features.swap_buffer_complete_callback = true;
  disallowed_features.gpu_memory_manager = true;
  if (!service_side_.decoder_->Initialize(
      service_side_.surface_,
      service_side_.context_,
      window ? false : true,
      gfx::Size(width, height),
      disallowed_features,
      // Enable those features we need for accelerated painting.
      "GL_EXT_texture_format_BGRA8888 GL_EXT_read_format_bgra",
      config_attribs)) {
    LOG(FATAL) << "Could not initialize decoder.";
  }
}

LBWebGraphicsContext3DCommandBuffer::~LBWebGraphicsContext3DCommandBuffer() {
  if (parent_ && parent_texture_id_ != 0) {
    parent_->gl_->FreeTextureId(parent_texture_id_);
    parent_texture_id_ = 0;
  }

  gl_.reset(NULL);
  transfer_buffer_.reset(NULL);

  SyncWithServiceSide();
  gles2_helper_.reset(NULL);

  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBWebGraphicsContext3DCommandBuffer::ServiceSideShutDown,
                 base::Unretained(this)));

  // Wait for the service side shutdown to finish executing before returning
  SyncWithServiceSide();

  command_buffer_.reset(NULL);
  transfer_buffer_manager_.reset(NULL);
}

void LBWebGraphicsContext3DCommandBuffer::ServiceSideShutDown() {
  service_side_.context_ = NULL;
  service_side_.surface_ = NULL;
  service_side_.gpu_scheduler_.reset(NULL);

  service_side_.decoder_->MakeCurrent();
  service_side_.decoder_->Destroy(true);
  service_side_.decoder_.reset(NULL);

  DCHECK(service_share_group_.get());
  if (service_share_group_->GetHandle() == NULL) {
    // If there are no more contexts in the share group (i.e. we are currently
    // destroying the last context), free the share group.
    service_share_group_ = NULL;
  }
}

void LBWebGraphicsContext3DCommandBuffer::ServiceSideSetParent(
    LBWebGraphicsContext3DCommandBuffer* parent,
    uint32 parent_texture_id) {
  TRACE_EVENT0("lb_graphics",
               "LBWebGraphicsContext3DCommandBuffer::ServiceSideSetParent");
  DCHECK_EQ(MessageLoop::current(), graphics_message_loop_);

  gpu::gles2::GLES2Decoder* decoder = NULL;
  if (parent) {
    decoder = parent->service_side_.decoder_.get();
  }

  if (!service_side_.decoder_->SetParent(decoder, parent_texture_id)) {
    DLOG(FATAL) << "Error setting parent decoder.";
  }
}

#if defined(__LB_XB360__)
void LBWebGraphicsContext3DCommandBuffer::ServiceSideBindTextureToVideo(
    WebGLId texture) {
  if (!service_side_.decoder_->MakeCurrent())
    return;

  WebGLId service_texture = 0;
  if (!service_side_.decoder_->GetServiceTextureId(texture, &service_texture))
    return;

  // Bind the service-size texture ID to the internal video resource.
  PFNGLBINDTEXTURETOVIDEOLBSHELLPROC glBindTextureToVideoLBSHELL =
      reinterpret_cast<PFNGLBINDTEXTURETOVIDEOLBSHELLPROC>(eglGetProcAddress(
          "glBindTextureToVideoLBSHELL"));
  PCHECK(glBindTextureToVideoLBSHELL) <<
      "glBindTextureToVideoLBSHELL extension not available";
  glBindTextureToVideoLBSHELL(service_texture);
}
#endif

void LBWebGraphicsContext3DCommandBuffer::SyncWithServiceSide() {
  TRACE_EVENT0("lb_graphics",
               "LBWebGraphicsContext3DCommandBuffer::SyncWithServiceSide");
  DCHECK_NE(MessageLoop::current(), graphics_message_loop_);

  base::WaitableEvent wait_event(true, false);
  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(&wait_event)));
  wait_event.Wait();
}

void LBWebGraphicsContext3DCommandBuffer::PumpCommands(
    bool sync, int32 put_offset) {
  base::AutoLock lock(pumping_commands_lock_);

  TRACE_EVENT1("lb_graphics",
               "LBWebGraphicsContext3DCommandBuffer::PumpCommands",
               "sync", sync);
  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBWebGraphicsContext3DCommandBuffer::ServiceSidePumpCommands,
                 base::Unretained(this), put_offset));

  if (sync) {
    pumped_commands_cond_.Wait();
  }
}

void LBWebGraphicsContext3DCommandBuffer::ServiceSidePumpCommands(
    int32 put_offset) {
  DCHECK_EQ(MessageLoop::current(), graphics_message_loop_);
  TRACE_EVENT0("lb_graphics",
               "LBWebGraphicsContext3DCommandBuffer::ServiceSidePumpCommands");
  service_side_.decoder_->MakeCurrent();
  service_side_.gpu_scheduler_->PutChanged(put_offset);
  gpu::CommandBuffer::State state = command_buffer_->GetState();
  DCHECK_EQ(state.error, gpu::error::kNoError);

  base::AutoLock lock(pumping_commands_lock_);
  pumped_commands_cond_.Broadcast();
}

bool LBWebGraphicsContext3DCommandBuffer::GetBufferChanged(
    int32 transfer_buffer_id) {
  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(
          &LBWebGraphicsContext3DCommandBuffer::ServiceSideGetBufferChanged,
          base::Unretained(this),
          transfer_buffer_id));
  return true;
}

void LBWebGraphicsContext3DCommandBuffer::ServiceSideGetBufferChanged(
    int32 transfer_buffer_id) {
  bool result = service_side_.gpu_scheduler_->SetGetBuffer(transfer_buffer_id);
  DCHECK(result);
}


bool LBWebGraphicsContext3DCommandBuffer::makeContextCurrent() {
  // If the graphics thread issues a sync call from the client side,
  // the program will deadlock.  The graphics thread should not be
  // issuing GL commands, only executing them.
  DCHECK_NE(MessageLoop::current(), graphics_message_loop_);

  gles2::SetGLContext(gl_.get());

  if (command_buffer_->GetState().error != gpu::error::kNoError) {
    return false;
  }

  return true;
}

int LBWebGraphicsContext3DCommandBuffer::width() {
  return width_;
}

int LBWebGraphicsContext3DCommandBuffer::height() {
  return height_;
}

bool LBWebGraphicsContext3DCommandBuffer::isGLES2Compliant() {
  return true;
}

bool LBWebGraphicsContext3DCommandBuffer::setParentContext(
    WebGraphicsContext3D* parent_context) {
  LBWebGraphicsContext3DCommandBuffer* new_parent =
      static_cast<LBWebGraphicsContext3DCommandBuffer*>(parent_context);

  if (parent_ == new_parent)
    return true;

  // Allocate a texture ID with respect to the parent and change the parent.
  uint32 new_parent_texture_id = 0;

  if (new_parent) {
    // Wait for the new parent to finish any remaining commands to make sure
    // that the texture id accounting stays consistent.
    new_parent->finish();
    new_parent_texture_id = new_parent->gl_->MakeTextureId();
  }

  graphics_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBWebGraphicsContext3DCommandBuffer::ServiceSideSetParent,
                 base::Unretained(this), new_parent, new_parent_texture_id));

  SyncWithServiceSide();

  // Free the previous parent's texture ID.
  if (parent_ && parent_texture_id_ != 0) {
    // Wait for the old parent to finish any remaining commands to make sure
    // that the texture id accounting stays consistent.
    parent_->finish();
    parent_->gl_->FreeTextureId(parent_texture_id_);
  }

  parent_ = new_parent;
  parent_texture_id_ = new_parent_texture_id;

  return true;
}

WebGLId LBWebGraphicsContext3DCommandBuffer::getPlatformTextureId() {
  return parent_texture_id_;
}

#if defined(__LB_XB360__)
void LBWebGraphicsContext3DCommandBuffer::bindTextureToVideoLBSHELL(
    WebGLId texture) {
  graphics_message_loop_->PostTask(FROM_HERE, base::Bind(
      &LBWebGraphicsContext3DCommandBuffer::ServiceSideBindTextureToVideo,
      base::Unretained(this), texture));
  SyncWithServiceSide();
}
#endif

void LBWebGraphicsContext3DCommandBuffer::prepareTexture() {
  // Don't request latest error status from service. Just use the locally cached
  // information from the last flush.
  DCHECK_EQ(command_buffer_->GetState().error, gpu::error::kNoError);

  gl_->SwapBuffers();
  gl_->Flush();
}

void LBWebGraphicsContext3DCommandBuffer::postSubBufferCHROMIUM(
    int x, int y, int width, int height) {
  gl_->PostSubBufferCHROMIUM(x, y, width, height);
}

void LBWebGraphicsContext3DCommandBuffer::reshape(
    int width, int height) {
  width_ = width;
  height_ = height;

  gl_->ResizeCHROMIUM(width, height);
}

bool LBWebGraphicsContext3DCommandBuffer::readBackFramebuffer(
    unsigned char* pixels,
    size_t buffer_size,
    WebGLId framebuffer,
    int width,
    int height) {

  if (buffer_size != static_cast<size_t>(4 * width * height)) {
    return false;
  }

  bool mustRestoreFBO = (bound_fbo_ != framebuffer);
  if (mustRestoreFBO) {
    gl_->BindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  }
  gl_->ReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Swizzle red and blue channels
  // TODO(kbr): expose GL_BGRA as extension
  for (size_t i = 0; i < buffer_size; i += 4) {
    std::swap(pixels[i], pixels[i + 2]);
  }

  if (mustRestoreFBO) {
    gl_->BindFramebuffer(GL_FRAMEBUFFER, bound_fbo_);
  }

  return true;
}

void LBWebGraphicsContext3DCommandBuffer::synthesizeGLError(
    WGC3Denum error) {
  NOTIMPLEMENTED();
}

void* LBWebGraphicsContext3DCommandBuffer::mapBufferSubDataCHROMIUM(
    WGC3Denum target,
    WGC3Dintptr offset,
    WGC3Dsizeiptr size,
    WGC3Denum access) {
  return gl_->MapBufferSubDataCHROMIUM(target, offset, size, access);
}

void LBWebGraphicsContext3DCommandBuffer::unmapBufferSubDataCHROMIUM(
    const void* mem) {
  return gl_->UnmapBufferSubDataCHROMIUM(mem);
}

void* LBWebGraphicsContext3DCommandBuffer::mapTexSubImage2DCHROMIUM(
    WGC3Denum target,
    WGC3Dint level,
    WGC3Dint xoffset,
    WGC3Dint yoffset,
    WGC3Dsizei width,
    WGC3Dsizei height,
    WGC3Denum format,
    WGC3Denum type,
    WGC3Denum access) {
  return gl_->MapTexSubImage2DCHROMIUM(
      target, level, xoffset, yoffset, width, height, format, type, access);
}

void LBWebGraphicsContext3DCommandBuffer::unmapTexSubImage2DCHROMIUM(
    const void* mem) {
  gl_->UnmapTexSubImage2DCHROMIUM(mem);
}

void LBWebGraphicsContext3DCommandBuffer::setVisibilityCHROMIUM(
    bool visible) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DCommandBuffer::
    setMemoryAllocationChangedCallbackCHROMIUM(
        WebGraphicsMemoryAllocationChangedCallbackCHROMIUM* callback) {
}

void LBWebGraphicsContext3DCommandBuffer::sendManagedMemoryStatsCHROMIUM(
    const WebKit::WebGraphicsManagedMemoryStats* stats) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DCommandBuffer::discardFramebufferEXT(
    WGC3Denum target, WGC3Dsizei numAttachments, const WGC3Denum* attachments) {
  gl_->DiscardFramebufferEXT(target, numAttachments, attachments);
}

void LBWebGraphicsContext3DCommandBuffer::discardBackbufferCHROMIUM() {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DCommandBuffer::ensureBackbufferCHROMIUM() {
  NOTIMPLEMENTED();
}

unsigned LBWebGraphicsContext3DCommandBuffer::insertSyncPoint() {
  NOTIMPLEMENTED();
  return 0;
}

void LBWebGraphicsContext3DCommandBuffer::waitSyncPoint(unsigned syncPoint) {
  NOTIMPLEMENTED();
}

WebGLId LBWebGraphicsContext3DCommandBuffer::createStreamTextureCHROMIUM(
    WebGLId texture) {
  NOTIMPLEMENTED();
  return 0;
}

void LBWebGraphicsContext3DCommandBuffer::destroyStreamTextureCHROMIUM(
    WebGLId texture) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DCommandBuffer::
    rateLimitOffscreenContextCHROMIUM() {
  gl_->RateLimitOffscreenContextCHROMIUM();
}

WebKit::WebString LBWebGraphicsContext3DCommandBuffer::
    getRequestableExtensionsCHROMIUM() {
  return WebKit::WebString::fromUTF8(
      gl_->GetRequestableExtensionsCHROMIUM());
}

void LBWebGraphicsContext3DCommandBuffer::requestExtensionCHROMIUM(
    const char* extension) {
  gl_->RequestExtensionCHROMIUM(extension);
}

void LBWebGraphicsContext3DCommandBuffer::blitFramebufferCHROMIUM(
    WGC3Dint srcX0, WGC3Dint srcY0, WGC3Dint srcX1, WGC3Dint srcY1,
    WGC3Dint dstX0, WGC3Dint dstY0, WGC3Dint dstX1, WGC3Dint dstY1,
    WGC3Dbitfield mask, WGC3Denum filter) {
  gl_->BlitFramebufferEXT(
      srcX0, srcY0, srcX1, srcY1,
      dstX0, dstY0, dstX1, dstY1,
      mask, filter);
}

void LBWebGraphicsContext3DCommandBuffer::
    renderbufferStorageMultisampleCHROMIUM(
        WGC3Denum target, WGC3Dsizei samples, WGC3Denum internalformat,
        WGC3Dsizei width, WGC3Dsizei height) {
  gl_->RenderbufferStorageMultisampleEXT(
      target, samples, internalformat, width, height);
}

void
LBWebGraphicsContext3DCommandBuffer::setSwapBuffersCompleteCallbackCHROMIUM(
    WebGraphicsSwapBuffersCompleteCallbackCHROMIUM* callback) {
  NOTIMPLEMENTED();
}

// Helper macros to reduce the amount of code.

#define DELEGATE_TO_GL(name, glname)                                    \
void LBWebGraphicsContext3DCommandBuffer::name() {                      \
  gl_->glname();                                                        \
}

#define DELEGATE_TO_GL_1(name, glname, t1)                              \
void LBWebGraphicsContext3DCommandBuffer::name(t1 a1) {                 \
  gl_->glname(a1);                                                      \
}

#define DELEGATE_TO_GL_1R(name, glname, t1, rt)                         \
rt LBWebGraphicsContext3DCommandBuffer::name(t1 a1) {                   \
  return gl_->glname(a1);                                               \
}

#define DELEGATE_TO_GL_1RB(name, glname, t1, rt)                        \
rt LBWebGraphicsContext3DCommandBuffer::name(t1 a1) {                   \
  return gl_->glname(a1) ? true : false;                                \
}

#define DELEGATE_TO_GL_2(name, glname, t1, t2)                          \
void LBWebGraphicsContext3DCommandBuffer::name(                         \
    t1 a1, t2 a2) {                                                     \
  gl_->glname(a1, a2);                                                  \
}

#define DELEGATE_TO_GL_2R(name, glname, t1, t2, rt)                     \
rt LBWebGraphicsContext3DCommandBuffer::name(t1 a1, t2 a2) { \
  return gl_->glname(a1, a2);                                           \
}

#define DELEGATE_TO_GL_3(name, glname, t1, t2, t3)                      \
void LBWebGraphicsContext3DCommandBuffer::name(                         \
    t1 a1, t2 a2, t3 a3) {                                              \
  gl_->glname(a1, a2, a3);                                              \
}

#define DELEGATE_TO_GL_4(name, glname, t1, t2, t3, t4)                  \
void LBWebGraphicsContext3DCommandBuffer::name(                         \
    t1 a1, t2 a2, t3 a3, t4 a4) {                                       \
  gl_->glname(a1, a2, a3, a4);                                          \
}

#define DELEGATE_TO_GL_5(name, glname, t1, t2, t3, t4, t5)              \
void LBWebGraphicsContext3DCommandBuffer::name(                         \
    t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) {                                \
  gl_->glname(a1, a2, a3, a4, a5);                                      \
}

#define DELEGATE_TO_GL_6(name, glname, t1, t2, t3, t4, t5, t6)          \
void LBWebGraphicsContext3DCommandBuffer::name(                         \
    t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) {                         \
  gl_->glname(a1, a2, a3, a4, a5, a6);                                  \
}

#define DELEGATE_TO_GL_7(name, glname, t1, t2, t3, t4, t5, t6, t7)      \
void LBWebGraphicsContext3DCommandBuffer::name(                         \
    t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7) {                  \
  gl_->glname(a1, a2, a3, a4, a5, a6, a7);                              \
}

#define DELEGATE_TO_GL_8(name, glname, t1, t2, t3, t4, t5, t6, t7, t8)  \
void LBWebGraphicsContext3DCommandBuffer::name(                         \
    t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8) {           \
  gl_->glname(a1, a2, a3, a4, a5, a6, a7, a8);                          \
}

#define DELEGATE_TO_GL_9(name, glname, t1, t2, t3, t4, t5, t6, t7, t8, t9) \
void LBWebGraphicsContext3DCommandBuffer::name(                         \
    t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8, t9 a9) {    \
  gl_->glname(a1, a2, a3, a4, a5, a6, a7, a8, a9);                      \
}

DELEGATE_TO_GL_1(activeTexture, ActiveTexture, WGC3Denum)

DELEGATE_TO_GL_2(attachShader, AttachShader, WebGLId, WebGLId)

DELEGATE_TO_GL_3(bindAttribLocation, BindAttribLocation, WebGLId,
                 WGC3Duint, const WGC3Dchar*)

DELEGATE_TO_GL_2(bindBuffer, BindBuffer, WGC3Denum, WebGLId)

void LBWebGraphicsContext3DCommandBuffer::bindFramebuffer(
    WGC3Denum target,
    WebGLId framebuffer) {
  gl_->BindFramebuffer(target, framebuffer);
  bound_fbo_ = framebuffer;
}

DELEGATE_TO_GL_2(bindRenderbuffer, BindRenderbuffer, WGC3Denum, WebGLId)

DELEGATE_TO_GL_2(bindTexture, BindTexture, WGC3Denum, WebGLId)

DELEGATE_TO_GL_4(blendColor, BlendColor,
                 WGC3Dclampf, WGC3Dclampf, WGC3Dclampf, WGC3Dclampf)

DELEGATE_TO_GL_1(blendEquation, BlendEquation, WGC3Denum)

DELEGATE_TO_GL_2(blendEquationSeparate, BlendEquationSeparate,
                 WGC3Denum, WGC3Denum)

DELEGATE_TO_GL_2(blendFunc, BlendFunc, WGC3Denum, WGC3Denum)

DELEGATE_TO_GL_4(blendFuncSeparate, BlendFuncSeparate,
                 WGC3Denum, WGC3Denum, WGC3Denum, WGC3Denum)

DELEGATE_TO_GL_4(bufferData, BufferData,
                 WGC3Denum, WGC3Dsizeiptr, const void*, WGC3Denum)

DELEGATE_TO_GL_4(bufferSubData, BufferSubData,
                 WGC3Denum, WGC3Dintptr, WGC3Dsizeiptr, const void*)

DELEGATE_TO_GL_1R(checkFramebufferStatus, CheckFramebufferStatus,
                  WGC3Denum, WGC3Denum)

DELEGATE_TO_GL_1(clear, Clear, WGC3Dbitfield)

DELEGATE_TO_GL_4(clearColor, ClearColor,
                 WGC3Dclampf, WGC3Dclampf, WGC3Dclampf, WGC3Dclampf)

DELEGATE_TO_GL_1(clearDepth, ClearDepthf, WGC3Dclampf)

DELEGATE_TO_GL_1(clearStencil, ClearStencil, WGC3Dint)

DELEGATE_TO_GL_4(colorMask, ColorMask,
                 WGC3Dboolean, WGC3Dboolean, WGC3Dboolean, WGC3Dboolean)

DELEGATE_TO_GL_1(compileShader, CompileShader, WebGLId)

DELEGATE_TO_GL_8(compressedTexImage2D, CompressedTexImage2D,
                 WGC3Denum, WGC3Dint, WGC3Denum, WGC3Dint, WGC3Dint,
                 WGC3Dsizei, WGC3Dsizei, const void*)

DELEGATE_TO_GL_9(compressedTexSubImage2D, CompressedTexSubImage2D,
                 WGC3Denum, WGC3Dint, WGC3Dint, WGC3Dint, WGC3Dint, WGC3Dint,
                 WGC3Denum, WGC3Dsizei, const void*)

DELEGATE_TO_GL_8(copyTexImage2D, CopyTexImage2D,
                 WGC3Denum, WGC3Dint, WGC3Denum, WGC3Dint, WGC3Dint,
                 WGC3Dsizei, WGC3Dsizei, WGC3Dint)

DELEGATE_TO_GL_8(copyTexSubImage2D, CopyTexSubImage2D,
                 WGC3Denum, WGC3Dint, WGC3Dint, WGC3Dint, WGC3Dint, WGC3Dint,
                 WGC3Dsizei, WGC3Dsizei)

DELEGATE_TO_GL_1(cullFace, CullFace, WGC3Denum)

DELEGATE_TO_GL_1(depthFunc, DepthFunc, WGC3Denum)

DELEGATE_TO_GL_1(depthMask, DepthMask, WGC3Dboolean)

DELEGATE_TO_GL_2(depthRange, DepthRangef, WGC3Dclampf, WGC3Dclampf)

DELEGATE_TO_GL_2(detachShader, DetachShader, WebGLId, WebGLId)

DELEGATE_TO_GL_1(disable, Disable, WGC3Denum)

DELEGATE_TO_GL_1(disableVertexAttribArray, DisableVertexAttribArray,
                 WGC3Duint)

DELEGATE_TO_GL_3(drawArrays, DrawArrays, WGC3Denum, WGC3Dint, WGC3Dsizei)

void LBWebGraphicsContext3DCommandBuffer::drawElements(
    WGC3Denum mode,
    WGC3Dsizei count,
    WGC3Denum type,
    WGC3Dintptr offset) {
  gl_->DrawElements(
      mode, count, type,
      reinterpret_cast<void*>(static_cast<intptr_t>(offset)));
}

DELEGATE_TO_GL_1(enable, Enable, WGC3Denum)

DELEGATE_TO_GL_1(enableVertexAttribArray, EnableVertexAttribArray,
                 WGC3Duint)

DELEGATE_TO_GL(finish, Finish)

DELEGATE_TO_GL(flush, Flush)

DELEGATE_TO_GL_4(framebufferRenderbuffer, FramebufferRenderbuffer,
                 WGC3Denum, WGC3Denum, WGC3Denum, WebGLId)

DELEGATE_TO_GL_5(framebufferTexture2D, FramebufferTexture2D,
                 WGC3Denum, WGC3Denum, WGC3Denum, WebGLId, WGC3Dint)

DELEGATE_TO_GL_1(frontFace, FrontFace, WGC3Denum)

DELEGATE_TO_GL_1(generateMipmap, GenerateMipmap, WGC3Denum)

bool LBWebGraphicsContext3DCommandBuffer::getActiveAttrib(
    WebGLId program, WGC3Duint index,
    ActiveInfo& info) {  // NOLINT(runtime/references)
  if (!program) {
    synthesizeGLError(GL_INVALID_VALUE);
    return false;
  }
  GLint max_name_length = -1;
  gl_->GetProgramiv(
      program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_name_length);
  if (max_name_length < 0)
    return false;
  scoped_array<GLchar> name(new GLchar[max_name_length]);
  if (!name.get()) {
    synthesizeGLError(GL_OUT_OF_MEMORY);
    return false;
  }
  GLsizei length = 0;
  GLint size = -1;
  GLenum type = 0;
  gl_->GetActiveAttrib(
      program, index, max_name_length, &length, &size, &type, name.get());
  if (size < 0) {
    return false;
  }
  info.name = WebKit::WebString::fromUTF8(name.get(), length);
  info.type = type;
  info.size = size;
  return true;
}

bool LBWebGraphicsContext3DCommandBuffer::getActiveUniform(
    WebGLId program, WGC3Duint index,
    ActiveInfo& info) {  // NOLINT(runtime/references)
  GLint max_name_length = -1;
  gl_->GetProgramiv(
      program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_length);
  if (max_name_length < 0)
    return false;
  scoped_array<GLchar> name(new GLchar[max_name_length]);
  if (!name.get()) {
    synthesizeGLError(GL_OUT_OF_MEMORY);
    return false;
  }
  GLsizei length = 0;
  GLint size = -1;
  GLenum type = 0;
  gl_->GetActiveUniform(
      program, index, max_name_length, &length, &size, &type, name.get());
  if (size < 0) {
    return false;
  }
  info.name = WebKit::WebString::fromUTF8(name.get(), length);
  info.type = type;
  info.size = size;
  return true;
}

DELEGATE_TO_GL_4(getAttachedShaders, GetAttachedShaders,
                 WebGLId, WGC3Dsizei, WGC3Dsizei*, WebGLId*)

DELEGATE_TO_GL_2R(getAttribLocation, GetAttribLocation,
                  WebGLId, const WGC3Dchar*, WGC3Dint)

DELEGATE_TO_GL_2(getBooleanv, GetBooleanv, WGC3Denum, WGC3Dboolean*)

DELEGATE_TO_GL_3(getBufferParameteriv, GetBufferParameteriv,
                 WGC3Denum, WGC3Denum, WGC3Dint*)

WebKit::WebGraphicsContext3D::Attributes
LBWebGraphicsContext3DCommandBuffer::getContextAttributes() {
  NOTIMPLEMENTED();
  return WebKit::WebGraphicsContext3D::Attributes();
}

WGC3Denum LBWebGraphicsContext3DCommandBuffer::getError() {
  return gl_->GetError();
}

bool LBWebGraphicsContext3DCommandBuffer::isContextLost() {
  return false;
}

DELEGATE_TO_GL_2(getFloatv, GetFloatv, WGC3Denum, WGC3Dfloat*)

DELEGATE_TO_GL_4(getFramebufferAttachmentParameteriv,
                 GetFramebufferAttachmentParameteriv,
                 WGC3Denum, WGC3Denum, WGC3Denum, WGC3Dint*)

DELEGATE_TO_GL_2(getIntegerv, GetIntegerv, WGC3Denum, WGC3Dint*)

DELEGATE_TO_GL_3(getProgramiv, GetProgramiv, WebGLId, WGC3Denum, WGC3Dint*)

WebKit::WebString LBWebGraphicsContext3DCommandBuffer::
    getProgramInfoLog(WebGLId program) {
  GLint logLength = 0;
  gl_->GetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
  if (!logLength)
    return WebKit::WebString();
  scoped_array<GLchar> log(new GLchar[logLength]);
  if (!log.get())
    return WebKit::WebString();
  GLsizei returnedLogLength = 0;
  gl_->GetProgramInfoLog(
      program, logLength, &returnedLogLength, log.get());
  DCHECK_EQ(logLength, returnedLogLength + 1);
  WebKit::WebString res =
      WebKit::WebString::fromUTF8(log.get(), returnedLogLength);
  return res;
}

DELEGATE_TO_GL_3(getRenderbufferParameteriv, GetRenderbufferParameteriv,
                 WGC3Denum, WGC3Denum, WGC3Dint*)

DELEGATE_TO_GL_3(getShaderiv, GetShaderiv, WebGLId, WGC3Denum, WGC3Dint*)

WebKit::WebString LBWebGraphicsContext3DCommandBuffer::
    getShaderInfoLog(WebGLId shader) {
  GLint logLength = 0;
  gl_->GetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
  if (!logLength)
    return WebKit::WebString();
  scoped_array<GLchar> log(new GLchar[logLength]);
  if (!log.get())
    return WebKit::WebString();
  GLsizei returnedLogLength = 0;
  gl_->GetShaderInfoLog(
      shader, logLength, &returnedLogLength, log.get());
  DCHECK_EQ(logLength, returnedLogLength + 1);
  WebKit::WebString res =
      WebKit::WebString::fromUTF8(log.get(), returnedLogLength);
  return res;
}

DELEGATE_TO_GL_4(getShaderPrecisionFormat, GetShaderPrecisionFormat,
                 WGC3Denum, WGC3Denum, WGC3Dint*, WGC3Dint*)

WebKit::WebString LBWebGraphicsContext3DCommandBuffer::
    getShaderSource(WebGLId shader) {
  GLint logLength = 0;
  gl_->GetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &logLength);
  if (!logLength)
    return WebKit::WebString();
  scoped_array<GLchar> log(new GLchar[logLength]);
  if (!log.get())
    return WebKit::WebString();
  GLsizei returnedLogLength = 0;
  gl_->GetShaderSource(
      shader, logLength, &returnedLogLength, log.get());
  if (!returnedLogLength)
    return WebKit::WebString();
  DCHECK_EQ(logLength, returnedLogLength + 1);
  WebKit::WebString res =
      WebKit::WebString::fromUTF8(log.get(), returnedLogLength);
  return res;
}

WebKit::WebString LBWebGraphicsContext3DCommandBuffer::
    getTranslatedShaderSourceANGLE(WebGLId shader) {
  NOTIMPLEMENTED();
  return WebKit::WebString();
}

WebKit::WebString LBWebGraphicsContext3DCommandBuffer::getString(
    WGC3Denum name) {
  return WebKit::WebString::fromUTF8(
      reinterpret_cast<const char*>(gl_->GetString(name)));
}

DELEGATE_TO_GL_3(getTexParameterfv, GetTexParameterfv,
                 WGC3Denum, WGC3Denum, WGC3Dfloat*)

DELEGATE_TO_GL_3(getTexParameteriv, GetTexParameteriv,
                 WGC3Denum, WGC3Denum, WGC3Dint*)

DELEGATE_TO_GL_3(getUniformfv, GetUniformfv, WebGLId, WGC3Dint, WGC3Dfloat*)

DELEGATE_TO_GL_3(getUniformiv, GetUniformiv, WebGLId, WGC3Dint, WGC3Dint*)

DELEGATE_TO_GL_2R(getUniformLocation, GetUniformLocation,
                  WebGLId, const WGC3Dchar*, WGC3Dint)

DELEGATE_TO_GL_3(getVertexAttribfv, GetVertexAttribfv,
                 WGC3Duint, WGC3Denum, WGC3Dfloat*)

DELEGATE_TO_GL_3(getVertexAttribiv, GetVertexAttribiv,
                 WGC3Duint, WGC3Denum, WGC3Dint*)

WGC3Dsizeiptr LBWebGraphicsContext3DCommandBuffer::
    getVertexAttribOffset(WGC3Duint index, WGC3Denum pname) {
  GLvoid* value = NULL;
  // NOTE: If pname is ever a value that returns more then 1 element
  // this will corrupt memory.
  gl_->GetVertexAttribPointerv(index, pname, &value);
  return static_cast<WGC3Dsizeiptr>(reinterpret_cast<intptr_t>(value));
}

DELEGATE_TO_GL_2(hint, Hint, WGC3Denum, WGC3Denum)

DELEGATE_TO_GL_1RB(isBuffer, IsBuffer, WebGLId, WGC3Dboolean)

DELEGATE_TO_GL_1RB(isEnabled, IsEnabled, WGC3Denum, WGC3Dboolean)

DELEGATE_TO_GL_1RB(isFramebuffer, IsFramebuffer, WebGLId, WGC3Dboolean)

DELEGATE_TO_GL_1RB(isProgram, IsProgram, WebGLId, WGC3Dboolean)

DELEGATE_TO_GL_1RB(isRenderbuffer, IsRenderbuffer, WebGLId, WGC3Dboolean)

DELEGATE_TO_GL_1RB(isShader, IsShader, WebGLId, WGC3Dboolean)

DELEGATE_TO_GL_1RB(isTexture, IsTexture, WebGLId, WGC3Dboolean)

DELEGATE_TO_GL_1(lineWidth, LineWidth, WGC3Dfloat)

DELEGATE_TO_GL_1(linkProgram, LinkProgram, WebGLId)

DELEGATE_TO_GL_2(pixelStorei, PixelStorei, WGC3Denum, WGC3Dint)

DELEGATE_TO_GL_2(polygonOffset, PolygonOffset, WGC3Dfloat, WGC3Dfloat)

DELEGATE_TO_GL_7(readPixels, ReadPixels,
                 WGC3Dint, WGC3Dint, WGC3Dsizei, WGC3Dsizei, WGC3Denum,
                 WGC3Denum, void*)

void LBWebGraphicsContext3DCommandBuffer::releaseShaderCompiler() {
}

DELEGATE_TO_GL_4(renderbufferStorage, RenderbufferStorage,
                 WGC3Denum, WGC3Denum, WGC3Dsizei, WGC3Dsizei)

DELEGATE_TO_GL_2(sampleCoverage, SampleCoverage, WGC3Dfloat, WGC3Dboolean)

DELEGATE_TO_GL_4(scissor, Scissor, WGC3Dint, WGC3Dint, WGC3Dsizei, WGC3Dsizei)

void LBWebGraphicsContext3DCommandBuffer::shaderSource(
    WebGLId shader, const WGC3Dchar* string) {
  GLint length = strlen(string);
  gl_->ShaderSource(shader, 1, &string, &length);
}

DELEGATE_TO_GL_3(stencilFunc, StencilFunc, WGC3Denum, WGC3Dint, WGC3Duint)

DELEGATE_TO_GL_4(stencilFuncSeparate, StencilFuncSeparate,
                 WGC3Denum, WGC3Denum, WGC3Dint, WGC3Duint)

DELEGATE_TO_GL_1(stencilMask, StencilMask, WGC3Duint)

DELEGATE_TO_GL_2(stencilMaskSeparate, StencilMaskSeparate,
                 WGC3Denum, WGC3Duint)

DELEGATE_TO_GL_3(stencilOp, StencilOp,
                 WGC3Denum, WGC3Denum, WGC3Denum)

DELEGATE_TO_GL_4(stencilOpSeparate, StencilOpSeparate,
                 WGC3Denum, WGC3Denum, WGC3Denum, WGC3Denum)

DELEGATE_TO_GL_9(texImage2D, TexImage2D,
                 WGC3Denum, WGC3Dint, WGC3Denum, WGC3Dsizei, WGC3Dsizei,
                 WGC3Dint, WGC3Denum, WGC3Denum, const void*)

DELEGATE_TO_GL_3(texParameterf, TexParameterf,
                 WGC3Denum, WGC3Denum, WGC3Dfloat);

static const unsigned int kTextureWrapR = 0x8072;

void LBWebGraphicsContext3DCommandBuffer::texParameteri(
    WGC3Denum target, WGC3Denum pname, WGC3Dint param) {
  if (pname == kTextureWrapR) {
    return;
  }
  gl_->TexParameteri(target, pname, param);
}

DELEGATE_TO_GL_9(texSubImage2D, TexSubImage2D,
                 WGC3Denum, WGC3Dint, WGC3Dint, WGC3Dint, WGC3Dsizei,
                 WGC3Dsizei, WGC3Denum, WGC3Denum, const void*)

DELEGATE_TO_GL_2(uniform1f, Uniform1f, WGC3Dint, WGC3Dfloat)

DELEGATE_TO_GL_3(uniform1fv, Uniform1fv, WGC3Dint, WGC3Dsizei,
                 const WGC3Dfloat*)

DELEGATE_TO_GL_2(uniform1i, Uniform1i, WGC3Dint, WGC3Dint)

DELEGATE_TO_GL_3(uniform1iv, Uniform1iv, WGC3Dint, WGC3Dsizei, const WGC3Dint*)

DELEGATE_TO_GL_3(uniform2f, Uniform2f, WGC3Dint, WGC3Dfloat, WGC3Dfloat)

DELEGATE_TO_GL_3(uniform2fv, Uniform2fv, WGC3Dint, WGC3Dsizei,
                 const WGC3Dfloat*)

DELEGATE_TO_GL_3(uniform2i, Uniform2i, WGC3Dint, WGC3Dint, WGC3Dint)

DELEGATE_TO_GL_3(uniform2iv, Uniform2iv, WGC3Dint, WGC3Dsizei, const WGC3Dint*)

DELEGATE_TO_GL_4(uniform3f, Uniform3f, WGC3Dint,
                 WGC3Dfloat, WGC3Dfloat, WGC3Dfloat)

DELEGATE_TO_GL_3(uniform3fv, Uniform3fv, WGC3Dint, WGC3Dsizei,
                 const WGC3Dfloat*)

DELEGATE_TO_GL_4(uniform3i, Uniform3i, WGC3Dint, WGC3Dint, WGC3Dint, WGC3Dint)

DELEGATE_TO_GL_3(uniform3iv, Uniform3iv, WGC3Dint, WGC3Dsizei, const WGC3Dint*)

DELEGATE_TO_GL_5(uniform4f, Uniform4f, WGC3Dint,
                 WGC3Dfloat, WGC3Dfloat, WGC3Dfloat, WGC3Dfloat)

DELEGATE_TO_GL_3(uniform4fv, Uniform4fv, WGC3Dint, WGC3Dsizei,
                 const WGC3Dfloat*)

DELEGATE_TO_GL_5(uniform4i, Uniform4i, WGC3Dint,
                 WGC3Dint, WGC3Dint, WGC3Dint, WGC3Dint)

DELEGATE_TO_GL_3(uniform4iv, Uniform4iv, WGC3Dint, WGC3Dsizei, const WGC3Dint*)

DELEGATE_TO_GL_4(uniformMatrix2fv, UniformMatrix2fv,
                 WGC3Dint, WGC3Dsizei, WGC3Dboolean, const WGC3Dfloat*)

DELEGATE_TO_GL_4(uniformMatrix3fv, UniformMatrix3fv,
                 WGC3Dint, WGC3Dsizei, WGC3Dboolean, const WGC3Dfloat*)

DELEGATE_TO_GL_4(uniformMatrix4fv, UniformMatrix4fv,
                 WGC3Dint, WGC3Dsizei, WGC3Dboolean, const WGC3Dfloat*)

DELEGATE_TO_GL_1(useProgram, UseProgram, WebGLId)

DELEGATE_TO_GL_1(validateProgram, ValidateProgram, WebGLId)

DELEGATE_TO_GL_2(vertexAttrib1f, VertexAttrib1f, WGC3Duint, WGC3Dfloat)

DELEGATE_TO_GL_2(vertexAttrib1fv, VertexAttrib1fv, WGC3Duint,
                 const WGC3Dfloat*)

DELEGATE_TO_GL_3(vertexAttrib2f, VertexAttrib2f, WGC3Duint,
                 WGC3Dfloat, WGC3Dfloat)

DELEGATE_TO_GL_2(vertexAttrib2fv, VertexAttrib2fv, WGC3Duint,
                 const WGC3Dfloat*)

DELEGATE_TO_GL_4(vertexAttrib3f, VertexAttrib3f, WGC3Duint,
                 WGC3Dfloat, WGC3Dfloat, WGC3Dfloat)

DELEGATE_TO_GL_2(vertexAttrib3fv, VertexAttrib3fv, WGC3Duint,
                 const WGC3Dfloat*)

DELEGATE_TO_GL_5(vertexAttrib4f, VertexAttrib4f, WGC3Duint,
                 WGC3Dfloat, WGC3Dfloat, WGC3Dfloat, WGC3Dfloat)

DELEGATE_TO_GL_2(vertexAttrib4fv, VertexAttrib4fv, WGC3Duint,
                 const WGC3Dfloat*)

void LBWebGraphicsContext3DCommandBuffer::vertexAttribPointer(
    WGC3Duint index, WGC3Dint size, WGC3Denum type, WGC3Dboolean normalized,
    WGC3Dsizei stride, WGC3Dintptr offset) {
  gl_->VertexAttribPointer(
      index, size, type, normalized, stride,
      reinterpret_cast<void*>(static_cast<intptr_t>(offset)));
}

DELEGATE_TO_GL_4(viewport, Viewport,
                 WGC3Dint, WGC3Dint, WGC3Dsizei, WGC3Dsizei)

WebGLId LBWebGraphicsContext3DCommandBuffer::createBuffer() {
  GLuint out;
  gl_->GenBuffers(1, &out);
  return out;
}

WebGLId LBWebGraphicsContext3DCommandBuffer::createFramebuffer() {
  GLuint out = 0;
  gl_->GenFramebuffers(1, &out);
  return out;
}

WebGLId LBWebGraphicsContext3DCommandBuffer::createProgram() {
  return gl_->CreateProgram();
}

WebGLId LBWebGraphicsContext3DCommandBuffer::createRenderbuffer() {
  GLuint out;
  gl_->GenRenderbuffers(1, &out);
  return out;
}

DELEGATE_TO_GL_1R(createShader, CreateShader, WGC3Denum, WebGLId);

WebGLId LBWebGraphicsContext3DCommandBuffer::createTexture() {
  GLuint out;
  gl_->GenTextures(1, &out);
  return out;
}

void LBWebGraphicsContext3DCommandBuffer::deleteBuffer(
    WebGLId buffer) {
  gl_->DeleteBuffers(1, &buffer);
}

void LBWebGraphicsContext3DCommandBuffer::deleteFramebuffer(
    WebGLId framebuffer) {
  gl_->DeleteFramebuffers(1, &framebuffer);
}

void LBWebGraphicsContext3DCommandBuffer::deleteProgram(
    WebGLId program) {
  gl_->DeleteProgram(program);
}

void LBWebGraphicsContext3DCommandBuffer::deleteRenderbuffer(
    WebGLId renderbuffer) {
  gl_->DeleteRenderbuffers(1, &renderbuffer);
}

void LBWebGraphicsContext3DCommandBuffer::deleteShader(
    WebGLId shader) {
  gl_->DeleteShader(shader);
}

void LBWebGraphicsContext3DCommandBuffer::deleteTexture(
    WebGLId texture) {
  gl_->DeleteTextures(1, &texture);
}

void LBWebGraphicsContext3DCommandBuffer::texSubImageSub(WGC3Denum format,
                                                         int dstOffX,
                                                         int dstOffY,
                                                         int dstWidth,
                                                         int dstHeight,
                                                         int srcX,
                                                         int srcY,
                                                         int srcWidth,
                                                         const void* image) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DCommandBuffer::setContextLostCallback(
    WebGraphicsContext3D::WebGraphicsContextLostCallback* cb) {
  NOTIMPLEMENTED();
}

WGC3Denum LBWebGraphicsContext3DCommandBuffer::getGraphicsResetStatusARB() {
  NOTIMPLEMENTED();
  return 0;
}

WebGLId LBWebGraphicsContext3DCommandBuffer::createVertexArrayOES() {
  NOTIMPLEMENTED();
  return 0;
}

void LBWebGraphicsContext3DCommandBuffer::deleteVertexArrayOES(WebGLId array) {
  NOTIMPLEMENTED();
}

WGC3Dboolean LBWebGraphicsContext3DCommandBuffer::isVertexArrayOES(
    WebGLId array) {
  NOTIMPLEMENTED();
  return 0;
}

void LBWebGraphicsContext3DCommandBuffer::bindVertexArrayOES(WebGLId array) {
  NOTIMPLEMENTED();
}

DELEGATE_TO_GL_5(texImageIOSurface2DCHROMIUM, TexImageIOSurface2DCHROMIUM,
                 WGC3Denum, WGC3Dint, WGC3Dint, WGC3Duint, WGC3Duint)

DELEGATE_TO_GL_5(texStorage2DEXT, TexStorage2DEXT,
                 WGC3Denum, WGC3Dint, WGC3Duint, WGC3Dint, WGC3Dint)

WebGLId LBWebGraphicsContext3DCommandBuffer::createQueryEXT() {
  GLuint out;
  gl_->GenQueriesEXT(1, &out);
  return out;
}

void LBWebGraphicsContext3DCommandBuffer::
    deleteQueryEXT(WebGLId query) {
  gl_->DeleteQueriesEXT(1, &query);
}

DELEGATE_TO_GL_1R(isQueryEXT, IsQueryEXT, WebGLId, WGC3Dboolean)
DELEGATE_TO_GL_2(beginQueryEXT, BeginQueryEXT, WGC3Denum, WebGLId)
DELEGATE_TO_GL_1(endQueryEXT, EndQueryEXT, WGC3Denum)
DELEGATE_TO_GL_3(getQueryivEXT, GetQueryivEXT, WGC3Denum, WGC3Denum, WGC3Dint*)
DELEGATE_TO_GL_3(getQueryObjectuivEXT, GetQueryObjectuivEXT,
                 WebGLId, WGC3Denum, WGC3Duint*)

DELEGATE_TO_GL_5(copyTextureCHROMIUM, CopyTextureCHROMIUM, WGC3Denum, WGC3Duint,
                 WGC3Duint, WGC3Dint, WGC3Denum)

void LBWebGraphicsContext3DCommandBuffer::insertEventMarkerEXT(
    const WGC3Dchar* marker) {
  gl_->InsertEventMarkerEXT(0, marker);
}

void LBWebGraphicsContext3DCommandBuffer::pushGroupMarkerEXT(
    const WGC3Dchar* marker) {
  gl_->PushGroupMarkerEXT(0, marker);
}

DELEGATE_TO_GL(popGroupMarkerEXT, PopGroupMarkerEXT);

void LBWebGraphicsContext3DCommandBuffer::asyncTexImage2DCHROMIUM(
    WGC3Denum target,
    WGC3Dint level,
    WGC3Denum internalformat,
    WGC3Dsizei width,
    WGC3Dsizei height,
    WGC3Dint border,
    WGC3Denum format,
    WGC3Denum type,
    const void* pixels) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DCommandBuffer::asyncTexSubImage2DCHROMIUM(
    WGC3Denum target,
    WGC3Dint level,
    WGC3Dint xoffset,
    WGC3Dint yoffset,
    WGC3Dsizei width,
    WGC3Dsizei height,
    WGC3Denum format,
    WGC3Denum type,
    const void* pixels) {
  NOTIMPLEMENTED();
}

DELEGATE_TO_GL_2(bindTexImage2DCHROMIUM, BindTexImage2DCHROMIUM,
                 WGC3Denum, WGC3Dint)
DELEGATE_TO_GL_2(releaseTexImage2DCHROMIUM, ReleaseTexImage2DCHROMIUM,
                 WGC3Denum, WGC3Dint)

void* LBWebGraphicsContext3DCommandBuffer::mapBufferCHROMIUM(
    WGC3Denum target, WGC3Denum access) {
  return gl_->MapBufferCHROMIUM(target, access);
}

WGC3Dboolean LBWebGraphicsContext3DCommandBuffer::
    unmapBufferCHROMIUM(WGC3Denum target) {
  return gl_->UnmapBufferCHROMIUM(target);
}

DELEGATE_TO_GL_3(bindUniformLocationCHROMIUM, BindUniformLocationCHROMIUM,
                 WebGLId, WGC3Dint, const WGC3Dchar*)

DELEGATE_TO_GL(shallowFlushCHROMIUM, ShallowFlushCHROMIUM)

DELEGATE_TO_GL_1(genMailboxCHROMIUM, GenMailboxCHROMIUM, WGC3Dbyte*)
DELEGATE_TO_GL_2(produceTextureCHROMIUM, ProduceTextureCHROMIUM,
                 WGC3Denum, const WGC3Dbyte*)
DELEGATE_TO_GL_2(consumeTextureCHROMIUM, ConsumeTextureCHROMIUM,
                 WGC3Denum, const WGC3Dbyte*)

#if !defined(__LB_DISABLE_SKIA_GPU__)
GrGLInterface* LBWebGraphicsContext3DCommandBuffer::onCreateGrGLInterface() {
  return webkit::gpu::CreateCommandBufferSkiaGLBinding();
}
#endif
