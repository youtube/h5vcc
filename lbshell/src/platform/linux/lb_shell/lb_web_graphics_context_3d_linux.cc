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
// Provides a abstract OpenGL interface to WebKit/Chromium.  In Chromium proper
// this object consumes commands from the layout engine for delegation to the
// renderer which is running in another process.  It is also assumed that one
// of these will be created for every page, tab, WebGL context, and accelerated
// canvas.  Since Leanback HTML5 doesn't use an accelerated canvas and this app
// is designed to render a single page in a single process this object
// delegates its commands directly to OpenGL and actually is configured to
// render directly into the device framebuffer, to eliminate expensive copy
// operations.  When prepareTexture() is called this object actually is telling
// the hardware to swap this texture from the back buffer to the front.

#include "lb_web_graphics_context_3d_linux.h"

#include <GL/glx.h>
#include <time.h>

#include <string>

#include "external/chromium/base/bind.h"
#include "external/chromium/base/files/dir_reader_linux.h"
#include "external/chromium/base/file_path.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/base/platform_file.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/base/time.h"
#include "external/chromium/third_party/libpng/png.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "lb_graphics_linux.h"
#include "lb_memory_manager.h"
#include "lb_opengl_helpers.h"
#include "lb_web_view_host.h"

#if LB_WEBKIT_GL_TRACE
#define GL_TRACE(...) DLOG(INFO) << base::StringPrintf(__VA_ARGS__)
#else
#define GL_TRACE(...)
#endif


LBWebGraphicsContext3DLinux::LBWebGraphicsContext3DLinux(
    LBGraphicsLinux* graphics, int width, int height)
    : graphics_(static_cast<LBGraphicsLinux*>(graphics))
    , framebuffer_(0)
    , texture_(0)
    , cur_pbo_(0)
    , gl_sync_(0)
    , have_set_context_(false) {
  DCHECK_EQ(graphics_->GetDeviceWidth(), width);
  DCHECK_EQ(graphics_->GetDeviceHeight(), height);

  webkit_pbos_[0] = 0;
  webkit_pbos_[1] = 0;

  // Create the context the Webkit thread will use.
  gl_context_ = glXCreateContext(graphics_->GetXDisplay(),
                                 graphics_->GetXVisualInfo(),
                                 /* share GL resources with
                                    our main context */
                                 graphics_->GetMasterContext(),
                                 true);
  if (!gl_context_) {
    DLOG(FATAL) << "Failed to create GL context.";
  }
}

LBWebGraphicsContext3DLinux::~LBWebGraphicsContext3DLinux() {
  if (texture_ != 0) {
    glDeleteTextures(1, &texture_);
  }
  if (framebuffer_ != 0) {
    glDeleteFramebuffers(1, &framebuffer_);
  }
}

bool LBWebGraphicsContext3DLinux::PixelBufferReady() const {
  if (!glIsSync(gl_sync_)) {
    return false;
  }

  GLenum result = glClientWaitSync(gl_sync_, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
  return (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED);
}

GLuint LBWebGraphicsContext3DLinux::TakeNextPixelBuffer() {
  DCHECK(PixelBufferReady());

  glDeleteSync(gl_sync_);
  gl_sync_ = 0;

  cur_pbo_ = 1 - cur_pbo_;
  return webkit_pbos_[1 - cur_pbo_];
}

// ============================================================================
// WebKit Methods

// Makes the OpenGL context current on the current thread. Returns true on
// success.
bool LBWebGraphicsContext3DLinux::makeContextCurrent() {
  glXMakeCurrent(graphics_->GetXDisplay(),
                 graphics_->GetXWindow(),
                 gl_context_);

  if (!have_set_context_) {
    // Now that we've been made current (for the first time), do some extra
    // setup like preparing our render target

    // Setup the render texture
    DCHECK(glXGetCurrentContext() == gl_context_);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 graphics_->GetDeviceWidth(),
                 graphics_->GetDeviceHeight(),
                 0,
                 GL_BGRA,
                 GL_UNSIGNED_BYTE,
                 0);

    // Setup the framebuffer object
    glGenFramebuffers(1, &framebuffer_);
    GLVERIFY();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    GLVERIFY();
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           texture_,
                           0);
    GLVERIFY();
    DCHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Setup the pixel buffer objects
    glGenBuffers(arraysize(webkit_pbos_), webkit_pbos_);
    for (int i = 0; i < arraysize(webkit_pbos_); ++i) {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, webkit_pbos_[i]);
      glBufferData(GL_PIXEL_UNPACK_BUFFER,
                   graphics_->GetDeviceWidth() *
                      graphics_->GetDeviceHeight() * 4,
                   NULL,
                   GL_STREAM_DRAW);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    have_set_context_ = true;
  }

  return true;
}

// The size of the region into which this WebGraphicsContext3D is rendering.
// Returns the last values passed to reshape().
int LBWebGraphicsContext3DLinux::width() {
  return graphics_->GetDeviceWidth();
}

int LBWebGraphicsContext3DLinux::height() {
  return graphics_->GetDeviceHeight();
}

// Resizes the region into which this WebGraphicsContext3D is drawing.
void LBWebGraphicsContext3DLinux::reshape(int width, int height) {
  // it is currently assumed that Chromium makes one LBWebGraphicsContext3DLinux
  // for all hardware-accelerated rendering.
  DCHECK_EQ(width, graphics_->GetDeviceWidth());
  DCHECK_EQ(height, graphics_->GetDeviceHeight());
}

// Query whether it is built on top of compliant GLES2 implementation.
bool LBWebGraphicsContext3DLinux::isGLES2Compliant() {
  // hopefully they'll never be the wiser :)
  return true;
}

bool LBWebGraphicsContext3DLinux::setParentContext(
    WebGraphicsContext3D* parentContext) {
  // this does what WebGraphicsContext3DInProcessImpl does
  NOTIMPLEMENTED();
  return false;
}

// Returns the id of the texture which is used for storing the contents of
// the framebuffer associated with this context. This texture is accessible
// by the gpu-based page compositor.
WebGLId LBWebGraphicsContext3DLinux::getPlatformTextureId() {
  // We're using the built-in renderbuffer.
  NOTIMPLEMENTED();
  return 0;
}

void LBWebGraphicsContext3DLinux::prepareTexture() {
  // Transfer our render-target texture to a PBO.
  glBindTexture(GL_TEXTURE_2D, texture_);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, webkit_pbos_[cur_pbo_]);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  // Insert a fence into the GL stream. The main context
  // will wait for this before trying to use the PBO.
  gl_sync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();
}

// Synthesizes an OpenGL error which will be returned from a
// later call to getError. This is used to emulate OpenGL ES
// 2.0 behavior on the desktop and to enforce additional error
// checking mandated by WebGL.
//
// Per the behavior of glGetError, this stores at most one
// instance of any given error, and returns them from calls to
// getError in the order they were added.
void LBWebGraphicsContext3DLinux::synthesizeGLError(WGC3Denum error) {
  NOTIMPLEMENTED();
}

bool LBWebGraphicsContext3DLinux::isContextLost() {
  // can't lose the context because we never create another one, ever
  return false;
}

// GL_CHROMIUM_map_sub
void* LBWebGraphicsContext3DLinux::mapBufferSubDataCHROMIUM(WGC3Denum target,
                                                            WGC3Dintptr offset,
                                                            WGC3Dsizeiptr size,
                                                            WGC3Denum access) {
  // This does what WebGraphicsContext3DInProcessImpl does, because this call
  // and the corresponding unmap..() call below are asking the GraphicsContext
  // top map a texture to a shared memory buffer so that it can be marshalled
  // across processes.  As our GL context lives only in one process it's not
  // needed, we don't report support for this extension and glGetString() and
  // therefore this function should never be called.
  NOTREACHED();
  return NULL;
}

void LBWebGraphicsContext3DLinux::unmapBufferSubDataCHROMIUM(const void*) {
  // this does what WebGraphicsContext3DInProcessImpl does
  NOTIMPLEMENTED();
}

void* LBWebGraphicsContext3DLinux::mapTexSubImage2DCHROMIUM(WGC3Denum target,
                                                            WGC3Dint level,
                                                            WGC3Dint xoffset,
                                                            WGC3Dint yoffset,
                                                            WGC3Dsizei width,
                                                            WGC3Dsizei height,
                                                            WGC3Denum format,
                                                            WGC3Denum type,
                                                            WGC3Denum access) {
  // In a multiprocess render environment these functions are used to allocate
  // shared memory blocks which the CPU paints in one process and are then
  // able to be subsequently uploaded to the card without an additional memcpy
  // to the GPU process.  The render process that doesn't use these functions
  // assumes that this graphics context object will have to copy the memory
  // to a command buffer for subsequent use by the GPU process.  Since this
  // graphics context just explicitly calls glTexSubImage2D there is no need
  // for a shared memory implementation and therefore correct behavior for this
  // function is to return NULL.  The NOTIMPLEMENTED() warning is removed
  // because this function gets called very frequently.
  return NULL;
}

void LBWebGraphicsContext3DLinux::unmapTexSubImage2DCHROMIUM(const void*) {
  // this does what WebGraphicsContext3DInProcessImpl does
  NOTIMPLEMENTED();
}

WebString LBWebGraphicsContext3DLinux::getRequestableExtensionsCHROMIUM() {
  // repeats the same extensions string as returned by
  // glGetString(GL_EXTENSIONS);
  return getString(GraphicsContext3D::EXTENSIONS);
}

void LBWebGraphicsContext3DLinux::requestExtensionCHROMIUM(const char*) {
  // this does what WebGraphicsContext3DInProcessImpl does
  NOTIMPLEMENTED();
}

// GL_CHROMIUM_framebuffer_multisample
void LBWebGraphicsContext3DLinux::blitFramebufferCHROMIUM(WGC3Dint srcX0,
                                                          WGC3Dint srcY0,
                                                          WGC3Dint srcX1,
                                                          WGC3Dint srcY1,
                                                          WGC3Dint dstX0,
                                                          WGC3Dint dstY0,
                                                          WGC3Dint dstX1,
                                                          WGC3Dint dstY1,
                                                          WGC3Dbitfield mask,
                                                          WGC3Denum filter) {
  // this does what WebGraphicsContext3DInProcessImpl does
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::renderbufferStorageMultisampleCHROMIUM(
    WGC3Denum target, WGC3Dsizei samples, WGC3Denum internalformat,
    WGC3Dsizei width, WGC3Dsizei height) {
  // this does what WebGraphicsContext3DInProcessImpl does
  NOTIMPLEMENTED();
}

// The entry points below map directly to the OpenGL ES 2.0 API.
// See: http://www.khronos.org/registry/gles/
// and: http://www.khronos.org/opengles/sdk/docs/man/
void LBWebGraphicsContext3DLinux::activeTexture(WGC3Denum texture) {
  GL_TRACE("glActiveTexture(GL_TEXTURE%d)", texture - GL_TEXTURE0);
  glActiveTexture(texture);
}

void LBWebGraphicsContext3DLinux::attachShader(WebGLId program, WebGLId shader) {
  GL_TRACE("glAttachShader(program: %d, shader: %d)",
     program, shader);

  glAttachShader(program, shader);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::bindAttribLocation(WebGLId program,
                                                     WGC3Duint index,
                                                     const WGC3Dchar* name) {
  GL_TRACE("glBindAttribLocation(program: %d, index: %d, name: %s)",
    program, index, name);
  glBindAttribLocation(program, index, name);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::bindBuffer(WGC3Denum target, WebGLId buffer) {
  GL_TRACE("glBindBuffer target: %d buffer: %d", target, buffer);
  glBindBuffer(target, buffer);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::bindFramebuffer(WGC3Denum target,
                                                  WebGLId framebuffer) {
  DCHECK_EQ(target, GraphicsContext3D::FRAMEBUFFER);
  GL_TRACE("glBindFramebuffer(GL_FRAMEBUFFER, framebuffer: %d)", framebuffer);

  // either a zero framebuffer to unbind the framebuffer, or the framebuffer
  // has to have the same handle as the one we've created.
  DCHECK(!framebuffer);

  if (!framebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    GLVERIFY();
  }
}

void LBWebGraphicsContext3DLinux::bindRenderbuffer(WGC3Denum target,
                                                   WebGLId renderbuffer) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::bindTexture(WGC3Denum target, WebGLId texture) {
  // only 2d textures supported
  DCHECK_EQ(target, GraphicsContext3D::TEXTURE_2D);
  GL_TRACE("glBindTexture(GL_TEXTURE_2D, texture: %d)", texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::blendColor(WGC3Dclampf red,
                                             WGC3Dclampf green,
                                             WGC3Dclampf blue,
                                             WGC3Dclampf alpha) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::blendEquation(WGC3Denum mode) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::blendEquationSeparate(WGC3Denum modeRGB,
                                                        WGC3Denum modeAlpha) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::blendFunc(WGC3Denum sfactor, WGC3Denum dfactor) {
  DCHECK_EQ(sfactor, GraphicsContext3D::ONE);
  DCHECK_EQ(dfactor, GraphicsContext3D::ONE_MINUS_SRC_ALPHA);
  GL_TRACE("glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)");
  glBlendFunc(sfactor, dfactor);
}

void LBWebGraphicsContext3DLinux::blendFuncSeparate(WGC3Denum srcRGB,
                                                    WGC3Denum dstRGB,
                                                    WGC3Denum srcAlpha,
                                                    WGC3Denum dstAlpha) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::bufferData(WGC3Denum target,
                                             WGC3Dsizeiptr size,
                                             const void* data,
                                             WGC3Denum usage) {
  GL_TRACE("glBufferData()");
  glBufferData(target, size, data, usage);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::bufferSubData(WGC3Denum target,
                                                WGC3Dintptr offset,
                                                WGC3Dsizeiptr size,
                                                const void* data) {
  NOTIMPLEMENTED();
}

WGC3Denum LBWebGraphicsContext3DLinux::checkFramebufferStatus(WGC3Denum target) {
  NOTIMPLEMENTED();
  // default framebuffer is always complete
  return GraphicsContext3D::FRAMEBUFFER_COMPLETE;
}

void LBWebGraphicsContext3DLinux::clear(WGC3Dbitfield mask) {
  if (mask & GraphicsContext3D::COLOR_BUFFER_BIT) {
    GL_TRACE("glClear(GL_COLOR_BUFFER_BIT | ...)");
    glClear(GL_COLOR_BUFFER_BIT);
  }
}

void LBWebGraphicsContext3DLinux::clearColor(WGC3Dclampf red,
                                             WGC3Dclampf green,
                                             WGC3Dclampf blue,
                                             WGC3Dclampf alpha) {
  GL_TRACE("glClearColor(red: %f, green: %f, blue: %f, alpha: %f)",
      red, green, blue, alpha);
  glClearColor(red, green, blue, alpha);
}

void LBWebGraphicsContext3DLinux::clearDepth(WGC3Dclampf depth) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::clearStencil(WGC3Dint s) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::colorMask(WGC3Dboolean red,
                                            WGC3Dboolean green,
                                            WGC3Dboolean blue,
                                            WGC3Dboolean alpha) {
  GL_TRACE("glColorMask(red: %d, green: %d, blue: %d, alpha: %d)",
    red, green, blue, alpha);
  glColorMask(red, green, blue, alpha);
}

namespace {
  // Remove 'precision' keyword from any shaders.
  // This is meaningless in GLSL, but is only supported
  // by GLSL 130.
  std::string SanitizeShaderSource(const std::string& src) {
    std::string new_src = src;

    size_t precision_pos = new_src.find("precision");
    while (precision_pos != std::string::npos) {
      size_t end_of_statement = new_src.find(';', precision_pos);
      DCHECK_NE(end_of_statement, std::string::npos);
      new_src.erase(precision_pos, end_of_statement - precision_pos + 1);
      precision_pos = new_src.find("precision");
    }
    return new_src;
  }

  // Given a shader Id, extract its source, pass it through
  // SanitizeShaderSource, and then update it in
  // the shader object.
  void SanitizeShaderSource(WebGLId shader) {
    GLint sourceLen;
    glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &sourceLen);

    std::string source(sourceLen, '\0');
    GLsizei storedSourceLen;
    glGetShaderSource(shader, sourceLen, &storedSourceLen, &source[0]);

    std::string modifiedSource = SanitizeShaderSource(source);

    GLint modifiedSourceLen = modifiedSource.size();
    const GLchar* sourcePtr = modifiedSource.c_str();
    glShaderSource(shader, 1, &sourcePtr, &modifiedSourceLen);
  }
}

void LBWebGraphicsContext3DLinux::compileShader(WebGLId shader) {
  GL_TRACE("glCompileShader(shader : %d)", shader);

  // The Mesa GL driver defaults its shaders to version 1.1 if
  // no version is specified, so add a specification of version 1.3
  // if none exists already, before compiling.
  SanitizeShaderSource(shader);
  glCompileShader(shader);
  GLVERIFY();
  GLHelpers::CheckShaderCompileStatus(shader, "built in");
}

void LBWebGraphicsContext3DLinux::compressedTexImage2D(WGC3Denum target,
                                                       WGC3Dint level,
                                                       WGC3Denum internalformat,
                                                       WGC3Dsizei width,
                                                       WGC3Dsizei height,
                                                       WGC3Dint border,
                                                       WGC3Dsizei imageSize,
                                                       const void* data) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::compressedTexSubImage2D(WGC3Denum target,
                                                          WGC3Dint level,
                                                          WGC3Dint xoffset,
                                                          WGC3Dint yoffset,
                                                          WGC3Dsizei width,
                                                          WGC3Dsizei height,
                                                          WGC3Denum format,
                                                          WGC3Dsizei imageSize,
                                                          const void* data) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::copyTexImage2D(WGC3Denum target,
                                                 WGC3Dint level,
                                                 WGC3Denum internalformat,
                                                 WGC3Dint x,
                                                 WGC3Dint y,
                                                 WGC3Dsizei width,
                                                 WGC3Dsizei height,
                                                 WGC3Dint border) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::copyTexSubImage2D(WGC3Denum target,
                                                    WGC3Dint level,
                                                    WGC3Dint xoffset,
                                                    WGC3Dint yoffset,
                                                    WGC3Dint x,
                                                    WGC3Dint y,
                                                    WGC3Dsizei width,
                                                    WGC3Dsizei height) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::cullFace(WGC3Denum mode) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::depthFunc(WGC3Denum func) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::depthMask(WGC3Dboolean flag) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::depthRange(WGC3Dclampf zNear,
                                             WGC3Dclampf zFar) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::detachShader(WebGLId program,
                                               WebGLId shader) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::disable(WGC3Denum cap) {
  switch (cap) {
    case GraphicsContext3D::BLEND:
      GL_TRACE("glDisable(GL_BLEND)");
      glDisable(GL_BLEND);
      break;

    case GraphicsContext3D::SCISSOR_TEST:
      GL_TRACE("glDisable(GL_SCISSOR_TEST)");
      glDisable(GL_SCISSOR_TEST);
      break;

    case GraphicsContext3D::CULL_FACE:
      GL_TRACE("glDisable(GL_CULL_FACE)");
      glDisable(GL_CULL_FACE);
      break;

    case GraphicsContext3D::DEPTH_TEST:
      GL_TRACE("glDisable(GL_DEPTH_TEST)");
      glDisable(GL_DEPTH_TEST);
      // always assumed to be disabled
      break;

    default:
      NOTIMPLEMENTED() << base::StringPrintf(" unsupported %x disabled", cap);
      break;
  }
}

void LBWebGraphicsContext3DLinux::disableVertexAttribArray(WGC3Duint index) {
  GL_TRACE("glDisableVertexAttribArray");
  glDisableVertexAttribArray(index);
}

void LBWebGraphicsContext3DLinux::drawArrays(WGC3Denum mode,
                                        WGC3Dint first,
                                        WGC3Dsizei count) {
  GL_TRACE("glDrawArrays");
  glDrawArrays(mode, first, count);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::drawElements(WGC3Denum mode,
                                               WGC3Dsizei count,
                                               WGC3Denum type,
                                               WGC3Dintptr offset) {
  GL_TRACE("glDrawElements");
  glDrawElements(mode, count, type, (const void*)offset);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::enable(WGC3Denum cap) {
  switch (cap) {
    case GraphicsContext3D::BLEND:
      GL_TRACE("glEnable(GL_BLEND)");
      glEnable(GL_BLEND);
      break;

    case GraphicsContext3D::DEPTH_TEST:
      NOTREACHED() << "it is assumed that DEPTH_TEST is always disabled.";
      break;

    case GraphicsContext3D::SCISSOR_TEST:
      GL_TRACE("glEnable(GL_SCISSOR_TEST)");
      glEnable(GL_SCISSOR_TEST);
      break;

    default:
      NOTIMPLEMENTED() << base::StringPrintf(" unsupported %x enabled", cap);
      break;
  }
}

void LBWebGraphicsContext3DLinux::enableVertexAttribArray(WGC3Duint index) {
  glEnableVertexAttribArray(index);
}

void LBWebGraphicsContext3DLinux::finish() {
  glFinish();
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::flush() {
  glFlush();
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::framebufferRenderbuffer(
    WGC3Denum target,
    WGC3Denum attachment,
    WGC3Denum renderbuffertarget,
    WebGLId renderbuffer) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::framebufferTexture2D(WGC3Denum target,
                                                       WGC3Denum attachment,
                                                       WGC3Denum textarget,
                                                       WebGLId texture,
                                                       WGC3Dint level) {
  // attach to framebuffer
  DCHECK_EQ(target, GraphicsContext3D::FRAMEBUFFER);
  // one color plane only
  DCHECK_EQ(attachment, GraphicsContext3D::COLOR_ATTACHMENT0);
  // attaching to a 2D texture
  DCHECK_EQ(textarget, GraphicsContext3D::TEXTURE_2D);
  // with no mipmapping
  DCHECK_EQ(level, 0);

  GL_TRACE("glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, "
      "GL_TEXTURE_2D, texture: %d, 0)", texture);
  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D,
                         texture,
                         level);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::frontFace(WGC3Denum mode) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::generateMipmap(WGC3Denum target) {
  NOTIMPLEMENTED();
}

bool LBWebGraphicsContext3DLinux::getActiveAttrib(WebGLId program,
                                                  WGC3Duint index,
                                                  ActiveInfo&) {
  NOTIMPLEMENTED();
  return false;
}

bool LBWebGraphicsContext3DLinux::getActiveUniform(WebGLId program,
                                                   WGC3Duint index,
                                                   ActiveInfo&) {
  NOTIMPLEMENTED();
  return false;
}

void LBWebGraphicsContext3DLinux::getAttachedShaders(WebGLId program,
                                                     WGC3Dsizei maxCount,
                                                     WGC3Dsizei* count,
                                                     WebGLId* shaders) {
  NOTIMPLEMENTED();
}

WGC3Dint LBWebGraphicsContext3DLinux::getAttribLocation(WebGLId program,
                                                        const WGC3Dchar* name) {
  NOTIMPLEMENTED();
  return 0;
}

void LBWebGraphicsContext3DLinux::getBooleanv(WGC3Denum pname,
                                              WGC3Dboolean* value) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::getBufferParameteriv(WGC3Denum target,
                                                       WGC3Denum pname,
                                                       WGC3Dint* value) {
  NOTIMPLEMENTED();
}

WebGraphicsContext3D::Attributes
LBWebGraphicsContext3DLinux::getContextAttributes() {
  return attributes_;
}

WGC3Denum LBWebGraphicsContext3DLinux::getError() {
  if (has_synthetic_error_) {
    return last_synthetic_error_;
    has_synthetic_error_ = false;
  }
  NOTIMPLEMENTED();
  return 0;
}

void LBWebGraphicsContext3DLinux::getFloatv(WGC3Denum pname,
                                            WGC3Dfloat* value) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::getFramebufferAttachmentParameteriv(
    WGC3Denum target,
    WGC3Denum attachment,
    WGC3Denum pname,
    WGC3Dint* value) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::getIntegerv(WGC3Denum pname,
                                              WGC3Dint* value) {
  switch (pname) {
    case GraphicsContext3D::MAX_VERTEX_ATTRIBS:
      glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, value);
      break;

    case GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS:
      glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, value);
      break;

    case GraphicsContext3D::MAX_TEXTURE_SIZE:
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, value);
      break;

    default:
      NOTIMPLEMENTED() << base::StringPrintf(" unsupported pname %x", pname);
      break;
  }
}

void LBWebGraphicsContext3DLinux::getProgramiv(WebGLId program,
                                               WGC3Denum pname,
                                               WGC3Dint* value) {
  glGetProgramiv(program, pname, value);
}

WebString LBWebGraphicsContext3DLinux::getProgramInfoLog(WebGLId program) {
  NOTIMPLEMENTED();
  return WebString();
}

void LBWebGraphicsContext3DLinux::getRenderbufferParameteriv(WGC3Denum target,
                                                             WGC3Denum pname,
                                                             WGC3Dint* value) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::getShaderiv(WebGLId shader,
                                              WGC3Denum pname,
                                              WGC3Dint* value) {
  glGetShaderiv(shader, pname, value);
}

WebString LBWebGraphicsContext3DLinux::getShaderInfoLog(WebGLId shader) {
  NOTIMPLEMENTED();
  return WebString();
}

void LBWebGraphicsContext3DLinux::getShaderPrecisionFormat(WGC3Denum shadertype,
                                                           WGC3Denum precisiontype,
                                                           WGC3Dint* range,
                                                           WGC3Dint* precision) {
  NOTIMPLEMENTED();
}

WebString LBWebGraphicsContext3DLinux::getShaderSource(WebGLId shader) {
  NOTIMPLEMENTED();
  return WebString();
}

WebString LBWebGraphicsContext3DLinux::getString(WGC3Denum name) {
  switch (name) {
    case GraphicsContext3D::EXTENSIONS:
      // Note: we report OES extensions even though we don't support them in
      // OpenGL.
      // This is so Chromium behaves consistently with our other platforms.
      return WebString::fromUTF8("GL_CHROMIUM_copy_texture_to_parent_texture "
                                 "GL_EXT_texture_format_BGRA8888 "
                                 "GL_OES_texture_npot "
                                 "GL_EXT_read_format_bgra "
                                 "GL_OES_packed_depth_stencil "
                                 "GL_OES_stencil_wrap "
                                 "GL_ARB_framebuffer_object ");
    case GraphicsContext3D::VERSION:
      return WebString::fromUTF8((const char*)glGetString(GL_VERSION));

    default:
      NOTIMPLEMENTED() << base::StringPrintf(" string %x not supported", name);
      break;
  }

  return WebString();
}

void LBWebGraphicsContext3DLinux::getTexParameterfv(WGC3Denum target,
                                                    WGC3Denum pname,
                                                    WGC3Dfloat* value) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::getTexParameteriv(WGC3Denum target,
                                                    WGC3Denum pname,
                                                    WGC3Dint* value) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::getUniformfv(WebGLId program,
                                               WGC3Dint location,
                                               WGC3Dfloat* value) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::getUniformiv(WebGLId program,
                                               WGC3Dint location,
                                               WGC3Dint* value) {
  NOTIMPLEMENTED();
}

// kMaxUniformsPerShader

WGC3Dint LBWebGraphicsContext3DLinux::getUniformLocation(
    WebGLId program,
    const WGC3Dchar* name) {
  GL_TRACE("getGetUniformLocation()");
  return glGetUniformLocation(program, name);
}

void LBWebGraphicsContext3DLinux::getVertexAttribfv(WGC3Duint index,
                                                    WGC3Denum pname,
                                                    WGC3Dfloat* value) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::getVertexAttribiv(WGC3Duint index,
                                                    WGC3Denum pname,
                                                    WGC3Dint* value) {
  NOTIMPLEMENTED();
}

WGC3Dsizeiptr LBWebGraphicsContext3DLinux::getVertexAttribOffset(
    WGC3Duint index,
    WGC3Denum pname) {
  NOTIMPLEMENTED();
  return NULL;
}

void LBWebGraphicsContext3DLinux::hint(WGC3Denum target, WGC3Denum mode) {
  NOTIMPLEMENTED();
}

WGC3Dboolean LBWebGraphicsContext3DLinux::isBuffer(WebGLId buffer) {
  NOTIMPLEMENTED();
  return false;
}

WGC3Dboolean LBWebGraphicsContext3DLinux::isEnabled(WGC3Denum cap) {
  NOTIMPLEMENTED();
  return false;
}

WGC3Dboolean LBWebGraphicsContext3DLinux::isFramebuffer(WebGLId framebuffer) {
  NOTIMPLEMENTED();
  return false;
}

WGC3Dboolean LBWebGraphicsContext3DLinux::isProgram(WebGLId program) {
  NOTIMPLEMENTED();
  return false;
}

WGC3Dboolean LBWebGraphicsContext3DLinux::isRenderbuffer(WebGLId renderbuffer) {
  NOTIMPLEMENTED();
  return false;
}

WGC3Dboolean LBWebGraphicsContext3DLinux::isShader(WebGLId shader) {
  NOTIMPLEMENTED();
  return false;
}

WGC3Dboolean LBWebGraphicsContext3DLinux::isTexture(WebGLId texture) {
  NOTIMPLEMENTED();
  return false;
}

void LBWebGraphicsContext3DLinux::lineWidth(WGC3Dfloat width) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::linkProgram(WebGLId program) {
  GL_TRACE("glLinkProgram(program: %d)", program);

  glLinkProgram(program);
}

void LBWebGraphicsContext3DLinux::pixelStorei(WGC3Denum pname, WGC3Dint param) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::polygonOffset(WGC3Dfloat factor,
                                                WGC3Dfloat units) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::readPixels(WGC3Dint x,
                                             WGC3Dint y,
                                             WGC3Dsizei width,
                                             WGC3Dsizei height,
                                             WGC3Denum format,
                                             WGC3Denum type,
                                             void* pixels) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::releaseShaderCompiler() {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::renderbufferStorage(WGC3Denum target,
                                                      WGC3Denum internalformat,
                                                      WGC3Dsizei width,
                                                      WGC3Dsizei height) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::sampleCoverage(WGC3Dclampf value,
                                                 WGC3Dboolean invert) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::scissor(WGC3Dint x,
                                          WGC3Dint y,
                                          WGC3Dsizei width,
                                          WGC3Dsizei height) {
  GL_TRACE("glScissor(x: %d, y: %d, width: %d, height: %d)",
      x, y, width, height);
  glScissor(x, y, width, height);
}

void LBWebGraphicsContext3DLinux::shaderSource(WebGLId shader,
                                               const WGC3Dchar* string) {
  GL_TRACE("glShaderSource(shader: %d, string: %s)", shader, string);
  glShaderSource(shader, 1, &string, NULL);
}

void LBWebGraphicsContext3DLinux::shaderBinary(WGC3Dsizei n,
                                               const WGC3Duint* shaders,
                                               WGC3Denum binaryFormat,
                                               const void* binary,
                                               WGC3Dsizei length) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::stencilFunc(WGC3Denum func,
                                              WGC3Dint ref,
                                              WGC3Duint mask) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::stencilFuncSeparate(WGC3Denum face,
                                                      WGC3Denum func,
                                                      WGC3Dint ref,
                                                      WGC3Duint mask) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::stencilMask(WGC3Duint mask) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::stencilMaskSeparate(WGC3Denum face,
                                                      WGC3Duint mask) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::stencilOp(WGC3Denum fail,
                                            WGC3Denum zfail,
                                            WGC3Denum zpass) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::stencilOpSeparate(WGC3Denum face,
                                                    WGC3Denum fail,
                                                    WGC3Denum zfail,
                                                    WGC3Denum zpass) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::texImage2D(WGC3Denum target,
                                             WGC3Dint level,
                                             WGC3Denum internalformat,
                                             WGC3Dsizei width,
                                             WGC3Dsizei height,
                                             WGC3Dint border,
                                             WGC3Denum format,
                                             WGC3Denum type,
                                             const void* pixels) {
  GL_TRACE("glTexImage2D(GL_TEXTURE_2D, width: %d, height: %d,"
           " format: %x, pixels: %p)",
    width, height, format, pixels);
  DCHECK(format == GL_RGBA || format == GL_BGRA);
  glTexImage2D(GL_TEXTURE_2D,
               level,
               GL_RGBA,  // internalformat,
               width,
               height,
               border,
               format,
               type,
               pixels);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::texParameterf(WGC3Denum target,
                                                WGC3Denum pname,
                                                WGC3Dfloat param) {
  GL_TRACE("glTexParameterf(target: %d pname: %d param: %f)",
      target, pname, param);
  glTexParameterf(target, pname, param);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::texParameteri(WGC3Denum target,
                                                WGC3Denum pname,
                                                WGC3Dint param) {
// This enum has special meaning to the Chromium renderer.
  enum {GL_TEXTURE_POOL_CHROMIUM = 0x6000};

  if (pname == GL_TEXTURE_POOL_CHROMIUM) {
    return;
  }

  GL_TRACE("glTexParameteri(target: %d pname: %d param: %d)",
     target, pname, param);
  glTexParameteri(target, pname, param);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::texSubImage2D(WGC3Denum target,
                                                WGC3Dint level,
                                                WGC3Dint xoffset,
                                                WGC3Dint yoffset,
                                                WGC3Dsizei width,
                                                WGC3Dsizei height,
                                                WGC3Denum format,
                                                WGC3Denum type,
                                                const void* pixels) {
  GL_TRACE("glTexSubImage2D(GL_TEXTURE_2D, xoffset: %d, yoffset: %d, "
    "width: %d, height: %d, pixels: %p)",
    xoffset, yoffset, width, height, pixels);
  DCHECK(target == GL_TEXTURE_2D);
  GLVERIFY();
  glTexSubImage2D(target,
                  level,
                  xoffset,
                  yoffset,
                  width,
                  height,
                  format,
                  type,
                  pixels);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::texSubImageSub(int dstOffX,
                                                 int dstOffY,
                                                 int dstWidth,
                                                 int dstHeight,
                                                 int srcX,
                                                 int srcY,
                                                 int srcWidth,
                                                 const void* image) {
  DCHECK(image);
  GL_TRACE("glTexSubImageSub(dstOffX: %d, dstOffY: %d, dstWidth: %d, "
         "dstHeight: %d, srcX: %d, srcY: %d, srcWidth: %d, image: %x",
         dstOffX, dstOffY, dstWidth, dstHeight, srcX, srcY, srcWidth, image);

  // Assumes 32-bit RGBA
  glPixelStorei(GL_UNPACK_ROW_LENGTH, srcWidth);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, srcY);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, srcX);

  glTexSubImage2D(GL_TEXTURE_2D,
                  0,
                  dstOffX,
                  dstOffY,
                  dstWidth,
                  dstHeight,
                  GL_BGRA,
                  GL_UNSIGNED_BYTE,
                  image);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
}

void LBWebGraphicsContext3DLinux::uniform1f(WGC3Dint location, WGC3Dfloat x) {
  glUniform1f(location, x);
}

void LBWebGraphicsContext3DLinux::uniform1fv(WGC3Dint location,
                                             WGC3Dsizei count,
                                             const WGC3Dfloat* v) {
  glUniform1fv(location, count, v);
}

void LBWebGraphicsContext3DLinux::uniform1i(WGC3Dint location, WGC3Dint x) {
  glUniform1i(location, x);
}

void LBWebGraphicsContext3DLinux::uniform1iv(WGC3Dint location,
                                             WGC3Dsizei count,
                                             const WGC3Dint* v) {
  glUniform1iv(location, count, v);
}

void LBWebGraphicsContext3DLinux::uniform2f(WGC3Dint location,
                                            WGC3Dfloat x,
                                            WGC3Dfloat y) {
  glUniform2f(location, x, y);
}

void LBWebGraphicsContext3DLinux::uniform2fv(WGC3Dint location,
                                             WGC3Dsizei count,
                                             const WGC3Dfloat* v) {
    glUniform2fv(location, count, v);
}

void LBWebGraphicsContext3DLinux::uniform2i(WGC3Dint location,
                                            WGC3Dint x, WGC3Dint y) {
  glUniform2i(location, x, y);
}

void LBWebGraphicsContext3DLinux::uniform2iv(WGC3Dint location,
                                             WGC3Dsizei count,
                                             const WGC3Dint* v) {
  glUniform2iv(location, count, v);
}

void LBWebGraphicsContext3DLinux::uniform3f(WGC3Dint location,
                                            WGC3Dfloat x,
                                            WGC3Dfloat y,
                                            WGC3Dfloat z) {
  glUniform3f(location, x, y, z);
}

void LBWebGraphicsContext3DLinux::uniform3fv(WGC3Dint location,
                                             WGC3Dsizei count,
                                             const WGC3Dfloat* v) {
  glUniform3fv(location, count, v);
}

void LBWebGraphicsContext3DLinux::uniform3i(WGC3Dint location,
                                            WGC3Dint x,
                                            WGC3Dint y,
                                            WGC3Dint z) {
  glUniform3i(location, x, y, z);
}

void LBWebGraphicsContext3DLinux::uniform3iv(WGC3Dint location,
                                             WGC3Dsizei count,
                                             const WGC3Dint* v) {
  glUniform3iv(location, count, v);
}

void LBWebGraphicsContext3DLinux::uniform4f(WGC3Dint location,
                                            WGC3Dfloat x,
                                            WGC3Dfloat y,
                                            WGC3Dfloat z,
                                            WGC3Dfloat w) {
  GL_TRACE("glUniform4f location %d: %f %f %f %f", location, x, y, z, w);
  glUniform4f(location, x, y, z, w);
}

void LBWebGraphicsContext3DLinux::uniform4fv(WGC3Dint location,
                                             WGC3Dsizei count,
                                             const WGC3Dfloat* v) {
  GL_TRACE("glUniform4fv location %d: v: %f %f %f %f",
    location, v[0], v[1], v[2], v[3]);
  glUniform4fv(location, count, v);
}

void LBWebGraphicsContext3DLinux::uniform4i(WGC3Dint location,
                                            WGC3Dint x,
                                            WGC3Dint y,
                                            WGC3Dint z,
                                            WGC3Dint w) {
  glUniform4i(location, x, y, z, w);
}

void LBWebGraphicsContext3DLinux::uniform4iv(WGC3Dint location,
                                             WGC3Dsizei count,
                                             const WGC3Dint* v) {
  glUniform4iv(location, count, v);
}

void LBWebGraphicsContext3DLinux::uniformMatrix2fv(WGC3Dint location,
                                                   WGC3Dsizei count,
                                                   WGC3Dboolean transpose,
                                                   const WGC3Dfloat* value) {
  glUniformMatrix2fv(location, count, transpose, value);
}

void LBWebGraphicsContext3DLinux::uniformMatrix3fv(WGC3Dint location,
                                                   WGC3Dsizei count,
                                                   WGC3Dboolean transpose,
                                                   const WGC3Dfloat* value) {
  glUniformMatrix3fv(location, count, transpose, value);
}

void LBWebGraphicsContext3DLinux::uniformMatrix4fv(WGC3Dint location,
                                                   WGC3Dsizei count,
                                                   WGC3Dboolean transpose,
                                                   const WGC3Dfloat* value) {
  GL_TRACE("glUniformMatrix4fv(%d: %s\n%f %f %f %f\n"
                                      "%f %f %f %f\n"
                                      "%f %f %f %f\n"
                                      "%f %f %f %f",
    location, transpose ? "transpose" : "no transpose",
    value[0], value[1], value[2], value[3],
    value[4], value[5], value[6], value[7],
    value[8], value[9], value[10], value[11],
    value[12], value[13], value[14], value[15]);
  glUniformMatrix4fv(location, count, transpose, value);
}

void LBWebGraphicsContext3DLinux::useProgram(WebGLId program) {
  GL_TRACE("glUseProgram(program: %d)", program);
  glUseProgram(program);
}

void LBWebGraphicsContext3DLinux::validateProgram(WebGLId program) {
  glValidateProgram(program);
}

void LBWebGraphicsContext3DLinux::vertexAttrib1f(WGC3Duint index,
                                                 WGC3Dfloat x) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::vertexAttrib1fv(WGC3Duint index,
                                                  const WGC3Dfloat* values) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::vertexAttrib2f(WGC3Duint index,
                                                 WGC3Dfloat x,
                                                 WGC3Dfloat y) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::vertexAttrib2fv(WGC3Duint index,
                                                  const WGC3Dfloat* values) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::vertexAttrib3f(WGC3Duint index,
                                                 WGC3Dfloat x,
                                                 WGC3Dfloat y,
                                                 WGC3Dfloat z) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::vertexAttrib3fv(WGC3Duint index,
                                                  const WGC3Dfloat* values) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::vertexAttrib4f(WGC3Duint index,
                                                 WGC3Dfloat x,
                                                 WGC3Dfloat y,
                                                 WGC3Dfloat z,
                                                 WGC3Dfloat w) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::vertexAttrib4fv(WGC3Duint index,
                                                  const WGC3Dfloat* values) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::vertexAttribPointer(WGC3Duint index,
                                                      WGC3Dint size,
                                                      WGC3Denum type,
                                                      WGC3Dboolean normalized,
                                                      WGC3Dsizei stride,
                                                      WGC3Dintptr offset) {
  glVertexAttribPointer(index,
                        size,
                        type,
                        normalized,
                        stride,
                        (const void*)offset);
}

void LBWebGraphicsContext3DLinux::viewport(WGC3Dint x,
                                           WGC3Dint y,
                                           WGC3Dsizei width,
                                           WGC3Dsizei height) {
  GL_TRACE("glViewport(x: %d, y: %d, width: %d, height: %d)",
      x, y, width, height);
  glViewport(x, y, width, height);
}

// Support for buffer creation and deletion.
WebGLId LBWebGraphicsContext3DLinux::createBuffer() {
  GL_TRACE("glGenBuffers()");
  GLuint buffer_id;
  glGenBuffers(1, &buffer_id);
  GLVERIFY();
  return buffer_id;
}

WebGLId LBWebGraphicsContext3DLinux::createFramebuffer() {
  GLuint framebuffer_id;
  glGenFramebuffers(1, &framebuffer_id);
  GLVERIFY();
  return framebuffer_id;
}

WebGLId LBWebGraphicsContext3DLinux::createProgram() {
  GLuint program_id =  glCreateProgram();
  GL_TRACE("glCreateProgram(%d)", program_id);
  return program_id;
}

WebGLId LBWebGraphicsContext3DLinux::createRenderbuffer() {
  NOTIMPLEMENTED();
  return 0;
}

WebGLId LBWebGraphicsContext3DLinux::createShader(WGC3Denum type) {
  GLuint shader_id = glCreateShader(type);
  GL_TRACE("glCreateShader(type: %d shader: %d)", type, shader_id);
  return shader_id;
}

WebGLId LBWebGraphicsContext3DLinux::createTexture() {
  GLuint texture_id;
  glGenTextures(1, &texture_id);
  GL_TRACE("glGenTextures(1, %d)", texture_id);
  return texture_id;
}

// we never delete anything!
void LBWebGraphicsContext3DLinux::deleteBuffer(WebGLId buffer) {
  glDeleteBuffers(1, &buffer);
}

void LBWebGraphicsContext3DLinux::deleteFramebuffer(WebGLId framebuffer) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::deleteProgram(WebGLId program) {
  GL_TRACE("glDeleteProgram(program: %d)", program);
  glDeleteProgram(program);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::deleteRenderbuffer(WebGLId renderbuffer) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::deleteShader(WebGLId shader) {
  GL_TRACE("glDeleteShader(shader: %d)", shader);
  glDeleteShader(shader);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::deleteTexture(WebGLId texture) {
  GL_TRACE("glDeleteTextures(texture: %d)", texture);
  glDeleteTextures(1, &texture);
  GLVERIFY();
}

void LBWebGraphicsContext3DLinux::setVisibilityCHROMIUM(bool visible) {
  NOTIMPLEMENTED();
}

bool LBWebGraphicsContext3DLinux::readBackFramebuffer(unsigned char* pixels,
                                                      size_t bufferSize,
                                                      WebGLId framebuffer,
                                                      int width,
                                                      int height) {
  NOTIMPLEMENTED();
  return false;
}

void LBWebGraphicsContext3DLinux::postSubBufferCHROMIUM(int x,
                                                        int y,
                                                        int width,
                                                        int height) {
  NOTIMPLEMENTED();
}

void LBWebGraphicsContext3DLinux::setMemoryAllocationChangedCallbackCHROMIUM(
  WebGraphicsMemoryAllocationChangedCallbackCHROMIUM* callback) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::sendManagedMemoryStatsCHROMIUM(
  const WebKit::WebGraphicsManagedMemoryStats* stats) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::discardFramebufferEXT(
    WGC3Denum target,
    WGC3Dsizei numAttachments,
    const WGC3Denum* attachments) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::discardBackbufferCHROMIUM() {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::ensureBackbufferCHROMIUM() {
  NOTIMPLEMENTED();
}
unsigned LBWebGraphicsContext3DLinux::insertSyncPoint() {
  NOTIMPLEMENTED();
  return 0;
}
void LBWebGraphicsContext3DLinux::waitSyncPoint(unsigned syncPoint) {
  NOTIMPLEMENTED();
}
WebGLId LBWebGraphicsContext3DLinux::createStreamTextureCHROMIUM(
  WebGLId texture) {
  NOTIMPLEMENTED();
  return 0;
}
void LBWebGraphicsContext3DLinux::destroyStreamTextureCHROMIUM(
  WebGLId texture) {
  NOTIMPLEMENTED();
}
WebString LBWebGraphicsContext3DLinux::getTranslatedShaderSourceANGLE(
  WebGLId shader) {
  NOTIMPLEMENTED();
  return WebString();
}
void LBWebGraphicsContext3DLinux::texImageIOSurface2DCHROMIUM(
    WGC3Denum target,
    WGC3Dint width,
    WGC3Dint height,
    WGC3Duint ioSurfaceId,
    WGC3Duint plane) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::texStorage2DEXT(WGC3Denum target,
                                                  WGC3Dint levels,
                                                  WGC3Duint internalformat,
                                                  WGC3Dint width,
                                                  WGC3Dint height) {
  NOTIMPLEMENTED();
}
WebGLId LBWebGraphicsContext3DLinux::createQueryEXT() {
  NOTIMPLEMENTED();
  return 0;
}
void LBWebGraphicsContext3DLinux::deleteQueryEXT(WebGLId query) {
  NOTIMPLEMENTED();
}
WGC3Dboolean LBWebGraphicsContext3DLinux::isQueryEXT(WebGLId query) {
  NOTIMPLEMENTED();
  return false;
}
void LBWebGraphicsContext3DLinux::beginQueryEXT(WGC3Denum target,
                                                WebGLId query) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::endQueryEXT(WGC3Denum target) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::getQueryivEXT(WGC3Denum target,
                                                WGC3Denum pname,
                                                WGC3Dint* params) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::getQueryObjectuivEXT(WebGLId query,
                                                       WGC3Denum pname,
                                                       WGC3Duint* params) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::bindUniformLocationCHROMIUM(
    WebGLId program,
    WGC3Dint location,
    const WGC3Dchar* uniform) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::copyTextureCHROMIUM(
    WGC3Denum target,
    WGC3Duint sourceId,
    WGC3Duint destId,
    WGC3Dint level,
    WGC3Denum internalFormat) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::shallowFlushCHROMIUM() {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::genMailboxCHROMIUM(WebKit::WGC3Dbyte* mailbox) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::produceTextureCHROMIUM(
    WGC3Denum target,
    const WebKit::WGC3Dbyte* mailbox) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::consumeTextureCHROMIUM(
    WGC3Denum target,
    const WebKit::WGC3Dbyte* mailbox) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::insertEventMarkerEXT(
  const WGC3Dchar* marker) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::pushGroupMarkerEXT(
  const WGC3Dchar* marker) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::popGroupMarkerEXT(void) {
  NOTIMPLEMENTED();
}
WebGLId LBWebGraphicsContext3DLinux::createVertexArrayOES() {
  NOTIMPLEMENTED();
  return 0;
}
void LBWebGraphicsContext3DLinux::deleteVertexArrayOES(WebGLId array) {
  NOTIMPLEMENTED();
}
WGC3Dboolean LBWebGraphicsContext3DLinux::isVertexArrayOES(WebGLId array) {
  NOTIMPLEMENTED();
  return false;
}
void LBWebGraphicsContext3DLinux::bindVertexArrayOES(WebGLId array) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::bindTexImage2DCHROMIUM(
    WGC3Denum target,
    WGC3Dint imageId) {
  NOTIMPLEMENTED();
}
void LBWebGraphicsContext3DLinux::releaseTexImage2DCHROMIUM(
    WGC3Denum target,
    WGC3Dint imageId) {
  NOTIMPLEMENTED();
}
void* LBWebGraphicsContext3DLinux::mapBufferCHROMIUM(
    WGC3Denum target,
    WGC3Denum access) {
  NOTIMPLEMENTED();
  return 0;
}
WGC3Dboolean LBWebGraphicsContext3DLinux::unmapBufferCHROMIUM(
  WGC3Denum target) {
  NOTIMPLEMENTED();
  return false;
}
void LBWebGraphicsContext3DLinux::asyncTexImage2DCHROMIUM(
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
void LBWebGraphicsContext3DLinux::asyncTexSubImage2DCHROMIUM(
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

void LBWebGraphicsContext3DLinux::setSwapBuffersCompleteCallbackCHROMIUM(
    WebGraphicsSwapBuffersCompleteCallbackCHROMIUM* callback) {
  NOTIMPLEMENTED();
}
