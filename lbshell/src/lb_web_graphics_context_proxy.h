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
// LBWebGraphicsContext3D is a singleton and should not be owned by WebKit.
// This provides an ownable proxy to LBWebGraphicsContext3D.
#ifndef SRC_LB_WEB_GRAPHICS_CONTEXT_PROXY_H_
#define SRC_LB_WEB_GRAPHICS_CONTEXT_PROXY_H_

#include "lb_graphics.h"

class LBWebGraphicsContextProxy : public WebKit::WebGraphicsContext3D {
 public:
  LBWebGraphicsContextProxy() {
    impl_ = LBGraphics::GetPtr()->GetSkiaContext();
  }

  virtual ~LBWebGraphicsContextProxy() {
    // NOTE: Do NOT delete the instance of the singleton!  We don't own it!
  }

 private:
  LBWebGraphicsContext3D *impl_;

 public:
  // The functions below were defined by a series of semi-automated scripts
  // that took WebGraphicsContext3D.h as input.

  virtual bool makeContextCurrent() OVERRIDE { return impl_->makeContextCurrent(); }
  virtual int width() OVERRIDE { return impl_->width(); }
  virtual int height() OVERRIDE { return impl_->height(); }
  virtual void reshape(int width, int height) OVERRIDE { impl_->reshape(width, height); }
  virtual void setVisibilityCHROMIUM(bool visible) OVERRIDE { impl_->setVisibilityCHROMIUM(visible); }
  virtual void setMemoryAllocationChangedCallbackCHROMIUM(WebGraphicsMemoryAllocationChangedCallbackCHROMIUM* callback) OVERRIDE { impl_->setMemoryAllocationChangedCallbackCHROMIUM(callback); }
  virtual void sendManagedMemoryStatsCHROMIUM(const WebKit::WebGraphicsManagedMemoryStats* stats) OVERRIDE { return impl_->sendManagedMemoryStatsCHROMIUM(stats); }
  virtual void discardFramebufferEXT(WGC3Denum target, WGC3Dsizei numAttachments, const WGC3Denum* attachments) OVERRIDE { impl_->discardFramebufferEXT(target, numAttachments, attachments); }
  virtual void discardBackbufferCHROMIUM() OVERRIDE { return impl_->discardBackbufferCHROMIUM(); }
  virtual void ensureBackbufferCHROMIUM() OVERRIDE { return impl_->ensureBackbufferCHROMIUM(); }
  virtual bool isGLES2Compliant() OVERRIDE { return impl_->isGLES2Compliant(); }
  virtual bool setParentContext(WebGraphicsContext3D* parentContext) OVERRIDE { return impl_->setParentContext(parentContext); }
  virtual unsigned insertSyncPoint() OVERRIDE { return impl_->insertSyncPoint(); }
  virtual void waitSyncPoint(unsigned syncPoint) OVERRIDE { return impl_->waitSyncPoint(syncPoint); }
  virtual bool readBackFramebuffer(unsigned char* pixels, size_t bufferSize, WebGLId framebuffer, int width, int height) OVERRIDE { return impl_->readBackFramebuffer(pixels, bufferSize, framebuffer, width, height); }
  virtual WebGLId getPlatformTextureId() OVERRIDE { return impl_->getPlatformTextureId(); }
  virtual void prepareTexture() OVERRIDE { impl_->prepareTexture(); }
  virtual void postSubBufferCHROMIUM(int x, int y, int width, int height) OVERRIDE { impl_->postSubBufferCHROMIUM(x, y, width, height); }
  virtual void synthesizeGLError(WGC3Denum error) OVERRIDE { impl_->synthesizeGLError(error); }
  virtual bool isContextLost() OVERRIDE { return impl_->isContextLost(); }
  virtual void* mapBufferSubDataCHROMIUM(WGC3Denum target, WGC3Dintptr offset, WGC3Dsizeiptr size, WGC3Denum access) { return impl_->mapBufferSubDataCHROMIUM(target, offset, size, access); }
  virtual void unmapBufferSubDataCHROMIUM(const void* subdata) OVERRIDE { impl_->unmapBufferSubDataCHROMIUM(subdata); }
  virtual void* mapTexSubImage2DCHROMIUM(WGC3Denum target, WGC3Dint level, WGC3Dint xoffset, WGC3Dint yoffset, WGC3Dsizei width, WGC3Dsizei height, WGC3Denum format, WGC3Denum type, WGC3Denum access) { return impl_->mapTexSubImage2DCHROMIUM(target, level, xoffset, yoffset, width, height, format, type, access); }
  virtual void unmapTexSubImage2DCHROMIUM(const void* subimage) OVERRIDE { return impl_->unmapTexSubImage2DCHROMIUM(subimage); }
  virtual WebString getRequestableExtensionsCHROMIUM() OVERRIDE { return impl_->getRequestableExtensionsCHROMIUM(); }
  virtual void requestExtensionCHROMIUM(const char* extension) OVERRIDE { impl_->requestExtensionCHROMIUM(extension); }
  virtual void blitFramebufferCHROMIUM(WGC3Dint srcX0, WGC3Dint srcY0, WGC3Dint srcX1, WGC3Dint srcY1, WGC3Dint dstX0, WGC3Dint dstY0, WGC3Dint dstX1, WGC3Dint dstY1, WGC3Dbitfield mask, WGC3Denum filter) OVERRIDE { impl_->blitFramebufferCHROMIUM(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter); }
  virtual void renderbufferStorageMultisampleCHROMIUM(WGC3Denum target, WGC3Dsizei samples, WGC3Denum internalformat, WGC3Dsizei width, WGC3Dsizei height) OVERRIDE { impl_->renderbufferStorageMultisampleCHROMIUM(target, samples, internalformat, width, height); }
  virtual void setSwapBuffersCompleteCallbackCHROMIUM(WebGraphicsSwapBuffersCompleteCallbackCHROMIUM* callback) OVERRIDE { impl_->setSwapBuffersCompleteCallbackCHROMIUM(callback); }
  virtual void rateLimitOffscreenContextCHROMIUM() OVERRIDE { impl_->rateLimitOffscreenContextCHROMIUM(); }
  virtual WebGLId createStreamTextureCHROMIUM(WebGLId texture) OVERRIDE { return impl_->createStreamTextureCHROMIUM(texture); }
  virtual void destroyStreamTextureCHROMIUM(WebGLId texture) OVERRIDE { impl_->destroyStreamTextureCHROMIUM(texture); }
  virtual void activeTexture(WGC3Denum texture) OVERRIDE { impl_->activeTexture(texture); }
  virtual void attachShader(WebGLId program, WebGLId shader) OVERRIDE { impl_->attachShader(program, shader); }
  virtual void bindAttribLocation(WebGLId program, WGC3Duint index, const WGC3Dchar* name) OVERRIDE { impl_->bindAttribLocation(program, index, name); }
  virtual void bindBuffer(WGC3Denum target, WebGLId buffer) OVERRIDE { impl_->bindBuffer(target, buffer); }
  virtual void bindFramebuffer(WGC3Denum target, WebGLId framebuffer) OVERRIDE { impl_->bindFramebuffer(target, framebuffer); }
  virtual void bindRenderbuffer(WGC3Denum target, WebGLId renderbuffer) OVERRIDE { impl_->bindRenderbuffer(target, renderbuffer); }
  virtual void bindTexture(WGC3Denum target, WebGLId texture) OVERRIDE { impl_->bindTexture(target, texture); }
  virtual void blendColor(WGC3Dclampf red, WGC3Dclampf green, WGC3Dclampf blue, WGC3Dclampf alpha) OVERRIDE { impl_->blendColor(red, green, blue, alpha); }
  virtual void blendEquation(WGC3Denum mode) OVERRIDE { impl_->blendEquation(mode); }
  virtual void blendEquationSeparate(WGC3Denum modeRGB, WGC3Denum modeAlpha) OVERRIDE { impl_->blendEquationSeparate(modeRGB, modeAlpha); }
  virtual void blendFunc(WGC3Denum sfactor, WGC3Denum dfactor) OVERRIDE { impl_->blendFunc(sfactor, dfactor); }
  virtual void blendFuncSeparate(WGC3Denum srcRGB, WGC3Denum dstRGB, WGC3Denum srcAlpha, WGC3Denum dstAlpha) OVERRIDE { impl_->blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha); }
  virtual void bufferData(WGC3Denum target, WGC3Dsizeiptr size, const void* data, WGC3Denum usage) OVERRIDE { impl_->bufferData(target, size, data, usage); }
  virtual void bufferSubData(WGC3Denum target, WGC3Dintptr offset, WGC3Dsizeiptr size, const void* data) OVERRIDE { impl_->bufferSubData(target, offset, size, data); }
  virtual WGC3Denum checkFramebufferStatus(WGC3Denum target) OVERRIDE { return impl_->checkFramebufferStatus(target); }
  virtual void clear(WGC3Dbitfield mask) OVERRIDE { impl_->clear(mask); }
  virtual void clearColor(WGC3Dclampf red, WGC3Dclampf green, WGC3Dclampf blue, WGC3Dclampf alpha) OVERRIDE { impl_->clearColor(red, green, blue, alpha); }
  virtual void clearDepth(WGC3Dclampf depth) OVERRIDE { impl_->clearDepth(depth); }
  virtual void clearStencil(WGC3Dint s) OVERRIDE { impl_->clearStencil(s); }
  virtual void colorMask(WGC3Dboolean red, WGC3Dboolean green, WGC3Dboolean blue, WGC3Dboolean alpha) OVERRIDE { impl_->colorMask(red, green, blue, alpha); }
  virtual void compileShader(WebGLId shader) OVERRIDE { impl_->compileShader(shader); }
  virtual void compressedTexImage2D(WGC3Denum target, WGC3Dint level, WGC3Denum internalformat, WGC3Dsizei width, WGC3Dsizei height, WGC3Dint border, WGC3Dsizei imageSize, const void* data) OVERRIDE { return impl_->compressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data); }
  virtual void compressedTexSubImage2D(WGC3Denum target, WGC3Dint level, WGC3Dint xoffset, WGC3Dint yoffset, WGC3Dsizei width, WGC3Dsizei height, WGC3Denum format, WGC3Dsizei imageSize, const void* data) OVERRIDE { return impl_->compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data); }
  virtual void copyTexImage2D(WGC3Denum target, WGC3Dint level, WGC3Denum internalformat, WGC3Dint x, WGC3Dint y, WGC3Dsizei width, WGC3Dsizei height, WGC3Dint border) OVERRIDE { return impl_->copyTexImage2D(target, level, internalformat, x, y, width, height, border); }
  virtual void copyTexSubImage2D(WGC3Denum target, WGC3Dint level, WGC3Dint xoffset, WGC3Dint yoffset, WGC3Dint x, WGC3Dint y, WGC3Dsizei width, WGC3Dsizei height) OVERRIDE { return impl_->copyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height); }
  virtual void cullFace(WGC3Denum mode) OVERRIDE { impl_->cullFace(mode); }
  virtual void depthFunc(WGC3Denum func) OVERRIDE { impl_->depthFunc(func); }
  virtual void depthMask(WGC3Dboolean flag) OVERRIDE { impl_->depthMask(flag); }
  virtual void depthRange(WGC3Dclampf zNear, WGC3Dclampf zFar) OVERRIDE { impl_->depthRange(zNear, zFar); }
  virtual void detachShader(WebGLId program, WebGLId shader) OVERRIDE { impl_->detachShader(program, shader); }
  virtual void disable(WGC3Denum cap) OVERRIDE { impl_->disable(cap); }
  virtual void disableVertexAttribArray(WGC3Duint index) OVERRIDE { impl_->disableVertexAttribArray(index); }
  virtual void drawArrays(WGC3Denum mode, WGC3Dint first, WGC3Dsizei count) OVERRIDE { impl_->drawArrays(mode, first, count); }
  virtual void drawElements(WGC3Denum mode, WGC3Dsizei count, WGC3Denum type, WGC3Dintptr offset) OVERRIDE { impl_->drawElements(mode, count, type, offset); }
  virtual void enable(WGC3Denum cap) OVERRIDE { impl_->enable(cap); }
  virtual void enableVertexAttribArray(WGC3Duint index) OVERRIDE { impl_->enableVertexAttribArray(index); }
  virtual void finish() OVERRIDE { impl_->finish(); }
  virtual void flush() OVERRIDE { impl_->flush(); }
  virtual void framebufferRenderbuffer(WGC3Denum target, WGC3Denum attachment, WGC3Denum renderbuffertarget, WebGLId renderbuffer) OVERRIDE { impl_->framebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer); }
  virtual void framebufferTexture2D(WGC3Denum target, WGC3Denum attachment, WGC3Denum textarget, WebGLId texture, WGC3Dint level) OVERRIDE { return impl_->framebufferTexture2D(target, attachment, textarget, texture, level); }
  virtual void frontFace(WGC3Denum mode) OVERRIDE { impl_->frontFace(mode); }
  virtual void generateMipmap(WGC3Denum target) OVERRIDE { impl_->generateMipmap(target); }
  virtual bool getActiveAttrib(WebGLId program, WGC3Duint index, ActiveInfo& info) OVERRIDE { return impl_->getActiveAttrib(program, index, info); }
  virtual bool getActiveUniform(WebGLId program, WGC3Duint index, ActiveInfo& info) OVERRIDE { return impl_->getActiveUniform(program, index, info); }
  virtual void getAttachedShaders(WebGLId program, WGC3Dsizei maxCount, WGC3Dsizei* count, WebGLId* shaders) OVERRIDE { impl_->getAttachedShaders(program, maxCount, count, shaders); }
  virtual WGC3Dint getAttribLocation(WebGLId program, const WGC3Dchar* name) OVERRIDE { return impl_->getAttribLocation(program, name); }
  virtual void getBooleanv(WGC3Denum pname, WGC3Dboolean* value) OVERRIDE { impl_->getBooleanv(pname, value); }
  virtual void getBufferParameteriv(WGC3Denum target, WGC3Denum pname, WGC3Dint* value) OVERRIDE { impl_->getBufferParameteriv(target, pname, value); }
  virtual Attributes getContextAttributes() OVERRIDE { return impl_->getContextAttributes(); }
  virtual WGC3Denum getError() OVERRIDE { return impl_->getError(); }
  virtual void getFloatv(WGC3Denum pname, WGC3Dfloat* value) OVERRIDE { impl_->getFloatv(pname, value); }
  virtual void getFramebufferAttachmentParameteriv(WGC3Denum target, WGC3Denum attachment, WGC3Denum pname, WGC3Dint* value) OVERRIDE { impl_->getFramebufferAttachmentParameteriv(target, attachment, pname, value); }
  virtual void getIntegerv(WGC3Denum pname, WGC3Dint* value) OVERRIDE { impl_->getIntegerv(pname, value); }
  virtual void getProgramiv(WebGLId program, WGC3Denum pname, WGC3Dint* value) OVERRIDE { impl_->getProgramiv(program, pname, value); }
  virtual WebString getProgramInfoLog(WebGLId program) OVERRIDE { return impl_->getProgramInfoLog(program); }
  virtual void getRenderbufferParameteriv(WGC3Denum target, WGC3Denum pname, WGC3Dint* value) OVERRIDE { impl_->getRenderbufferParameteriv(target, pname, value); }
  virtual void getShaderiv(WebGLId shader, WGC3Denum pname, WGC3Dint* value) OVERRIDE { impl_->getShaderiv(shader, pname, value); }
  virtual WebString getShaderInfoLog(WebGLId shader) OVERRIDE { return impl_->getShaderInfoLog(shader); }
  virtual void getShaderPrecisionFormat(WGC3Denum shadertype, WGC3Denum precisiontype, WGC3Dint* range, WGC3Dint* precision) OVERRIDE { impl_->getShaderPrecisionFormat(shadertype, precisiontype, range, precision); }
  virtual WebString getShaderSource(WebGLId shader) OVERRIDE { return impl_->getShaderSource(shader); }
  virtual WebString getString(WGC3Denum name) OVERRIDE { return impl_->getString(name); }
  virtual void getTexParameterfv(WGC3Denum target, WGC3Denum pname, WGC3Dfloat* value) OVERRIDE { impl_->getTexParameterfv(target, pname, value); }
  virtual void getTexParameteriv(WGC3Denum target, WGC3Denum pname, WGC3Dint* value) OVERRIDE { impl_->getTexParameteriv(target, pname, value); }
  virtual void getUniformfv(WebGLId program, WGC3Dint location, WGC3Dfloat* value) OVERRIDE { impl_->getUniformfv(program, location, value); }
  virtual void getUniformiv(WebGLId program, WGC3Dint location, WGC3Dint* value) OVERRIDE { impl_->getUniformiv(program, location, value); }
  virtual WGC3Dint getUniformLocation(WebGLId program, const WGC3Dchar* name) OVERRIDE { return impl_->getUniformLocation(program, name); }
  virtual void getVertexAttribfv(WGC3Duint index, WGC3Denum pname, WGC3Dfloat* value) OVERRIDE { impl_->getVertexAttribfv(index, pname, value); }
  virtual void getVertexAttribiv(WGC3Duint index, WGC3Denum pname, WGC3Dint* value) OVERRIDE { impl_->getVertexAttribiv(index, pname, value); }
  virtual WGC3Dsizeiptr getVertexAttribOffset(WGC3Duint index, WGC3Denum pname) OVERRIDE { return impl_->getVertexAttribOffset(index, pname); }
  virtual void hint(WGC3Denum target, WGC3Denum mode) OVERRIDE { impl_->hint(target, mode); }
  virtual WGC3Dboolean isBuffer(WebGLId buffer) OVERRIDE { return impl_->isBuffer(buffer); }
  virtual WGC3Dboolean isEnabled(WGC3Denum cap) OVERRIDE { return impl_->isEnabled(cap); }
  virtual WGC3Dboolean isFramebuffer(WebGLId framebuffer) OVERRIDE { return impl_->isFramebuffer(framebuffer); }
  virtual WGC3Dboolean isProgram(WebGLId program) OVERRIDE { return impl_->isProgram(program); }
  virtual WGC3Dboolean isRenderbuffer(WebGLId renderbuffer) OVERRIDE { return impl_->isRenderbuffer(renderbuffer); }
  virtual WGC3Dboolean isShader(WebGLId shader) OVERRIDE { return impl_->isShader(shader); }
  virtual WGC3Dboolean isTexture(WebGLId texture) OVERRIDE { return impl_->isTexture(texture); }
  virtual void lineWidth(WGC3Dfloat width) OVERRIDE { impl_->lineWidth(width); }
  virtual void linkProgram(WebGLId program) OVERRIDE { impl_->linkProgram(program); }
  virtual void pixelStorei(WGC3Denum pname, WGC3Dint param) OVERRIDE { impl_->pixelStorei(pname, param); }
  virtual void polygonOffset(WGC3Dfloat factor, WGC3Dfloat units) OVERRIDE { impl_->polygonOffset(factor, units); }
  virtual void readPixels(WGC3Dint x, WGC3Dint y, WGC3Dsizei width, WGC3Dsizei height, WGC3Denum format, WGC3Denum type, void* pixels) OVERRIDE { impl_->readPixels(x, y, width, height, format, type, pixels); }
  virtual void releaseShaderCompiler() OVERRIDE { impl_->releaseShaderCompiler(); }
  virtual void renderbufferStorage(WGC3Denum target, WGC3Denum internalformat, WGC3Dsizei width, WGC3Dsizei height) OVERRIDE { impl_->renderbufferStorage(target, internalformat, width, height); }
  virtual void sampleCoverage(WGC3Dclampf value, WGC3Dboolean invert) OVERRIDE { impl_->sampleCoverage(value, invert); }
  virtual void scissor(WGC3Dint x, WGC3Dint y, WGC3Dsizei width, WGC3Dsizei height) OVERRIDE { impl_->scissor(x, y, width, height); }
  virtual void shaderSource(WebGLId shader, const WGC3Dchar* string) OVERRIDE { impl_->shaderSource(shader, string); }
  virtual void stencilFunc(WGC3Denum func, WGC3Dint ref, WGC3Duint mask) OVERRIDE { impl_->stencilFunc(func, ref, mask); }
  virtual void stencilFuncSeparate(WGC3Denum face, WGC3Denum func, WGC3Dint ref, WGC3Duint mask) OVERRIDE { impl_->stencilFuncSeparate(face, func, ref, mask); }
  virtual void stencilMask(WGC3Duint mask) OVERRIDE { impl_->stencilMask(mask); }
  virtual void stencilMaskSeparate(WGC3Denum face, WGC3Duint mask) OVERRIDE { impl_->stencilMaskSeparate(face, mask); }
  virtual void stencilOp(WGC3Denum fail, WGC3Denum zfail, WGC3Denum zpass) OVERRIDE { impl_->stencilOp(fail, zfail, zpass); }
  virtual void stencilOpSeparate(WGC3Denum face, WGC3Denum fail, WGC3Denum zfail, WGC3Denum zpass) OVERRIDE { impl_->stencilOpSeparate(face, fail, zfail, zpass); }
  virtual void texImage2D(WGC3Denum target, WGC3Dint level, WGC3Denum internalformat, WGC3Dsizei width, WGC3Dsizei height, WGC3Dint border, WGC3Denum format, WGC3Denum type, const void* pixels) OVERRIDE { return impl_->texImage2D(target, level, internalformat, width, height, border, format, type, pixels); }
  virtual void texParameterf(WGC3Denum target, WGC3Denum pname, WGC3Dfloat param) OVERRIDE { impl_->texParameterf(target, pname, param); }
  virtual void texParameteri(WGC3Denum target, WGC3Denum pname, WGC3Dint param) OVERRIDE { impl_->texParameteri(target, pname, param); }
  virtual void texSubImage2D(WGC3Denum target, WGC3Dint level, WGC3Dint xoffset, WGC3Dint yoffset, WGC3Dsizei width, WGC3Dsizei height, WGC3Denum format, WGC3Denum type, const void* pixels) OVERRIDE { return impl_->texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels); }
  virtual void uniform1f(WGC3Dint location, WGC3Dfloat x) OVERRIDE { return impl_->uniform1f(location, x); }
  virtual void uniform1fv(WGC3Dint location, WGC3Dsizei count, const WGC3Dfloat* v) OVERRIDE { return impl_->uniform1fv(location, count, v); }
  virtual void uniform1i(WGC3Dint location, WGC3Dint x) OVERRIDE { return impl_->uniform1i(location, x); }
  virtual void uniform1iv(WGC3Dint location, WGC3Dsizei count, const WGC3Dint* v) OVERRIDE { return impl_->uniform1iv(location, count, v); }
  virtual void uniform2f(WGC3Dint location, WGC3Dfloat x, WGC3Dfloat y) OVERRIDE { return impl_->uniform2f(location, x, y); }
  virtual void uniform2fv(WGC3Dint location, WGC3Dsizei count, const WGC3Dfloat* v) OVERRIDE { return impl_->uniform2fv(location, count, v); }
  virtual void uniform2i(WGC3Dint location, WGC3Dint x, WGC3Dint y) OVERRIDE { return impl_->uniform2i(location, x, y); }
  virtual void uniform2iv(WGC3Dint location, WGC3Dsizei count, const WGC3Dint* v) OVERRIDE { return impl_->uniform2iv(location, count, v); }
  virtual void uniform3f(WGC3Dint location, WGC3Dfloat x, WGC3Dfloat y, WGC3Dfloat z) OVERRIDE { return impl_->uniform3f(location, x, y, z); }
  virtual void uniform3fv(WGC3Dint location, WGC3Dsizei count, const WGC3Dfloat* v) OVERRIDE { return impl_->uniform3fv(location, count, v); }
  virtual void uniform3i(WGC3Dint location, WGC3Dint x, WGC3Dint y, WGC3Dint z) OVERRIDE { return impl_->uniform3i(location, x, y, z); }
  virtual void uniform3iv(WGC3Dint location, WGC3Dsizei count, const WGC3Dint* v) OVERRIDE { return impl_->uniform3iv(location, count, v); }
  virtual void uniform4f(WGC3Dint location, WGC3Dfloat x, WGC3Dfloat y, WGC3Dfloat z, WGC3Dfloat w) OVERRIDE { return impl_->uniform4f(location, x, y, z, w); }
  virtual void uniform4fv(WGC3Dint location, WGC3Dsizei count, const WGC3Dfloat* v) OVERRIDE { return impl_->uniform4fv(location, count, v); }
  virtual void uniform4i(WGC3Dint location, WGC3Dint x, WGC3Dint y, WGC3Dint z, WGC3Dint w) OVERRIDE { return impl_->uniform4i(location, x, y, z, w); }
  virtual void uniform4iv(WGC3Dint location, WGC3Dsizei count, const WGC3Dint* v) OVERRIDE { return impl_->uniform4iv(location, count, v); }
  virtual void uniformMatrix2fv(WGC3Dint location, WGC3Dsizei count, WGC3Dboolean transpose, const WGC3Dfloat* value) OVERRIDE { return impl_->uniformMatrix2fv(location, count, transpose, value); }
  virtual void uniformMatrix3fv(WGC3Dint location, WGC3Dsizei count, WGC3Dboolean transpose, const WGC3Dfloat* value) OVERRIDE { return impl_->uniformMatrix3fv(location, count, transpose, value); }
  virtual void uniformMatrix4fv(WGC3Dint location, WGC3Dsizei count, WGC3Dboolean transpose, const WGC3Dfloat* value) OVERRIDE { return impl_->uniformMatrix4fv(location, count, transpose, value); }
  virtual void useProgram(WebGLId program) OVERRIDE { impl_->useProgram(program); }
  virtual void validateProgram(WebGLId program) OVERRIDE { impl_->validateProgram(program); }
  virtual void vertexAttrib1f(WGC3Duint index, WGC3Dfloat x) OVERRIDE { return impl_->vertexAttrib1f(index, x); }
  virtual void vertexAttrib1fv(WGC3Duint index, const WGC3Dfloat* values) OVERRIDE { return impl_->vertexAttrib1fv(index, values); }
  virtual void vertexAttrib2f(WGC3Duint index, WGC3Dfloat x, WGC3Dfloat y) OVERRIDE { return impl_->vertexAttrib2f(index, x, y); }
  virtual void vertexAttrib2fv(WGC3Duint index, const WGC3Dfloat* values) OVERRIDE { return impl_->vertexAttrib2fv(index, values); }
  virtual void vertexAttrib3f(WGC3Duint index, WGC3Dfloat x, WGC3Dfloat y, WGC3Dfloat z) OVERRIDE { return impl_->vertexAttrib3f(index, x, y, z); }
  virtual void vertexAttrib3fv(WGC3Duint index, const WGC3Dfloat* values) OVERRIDE { return impl_->vertexAttrib3fv(index, values); }
  virtual void vertexAttrib4f(WGC3Duint index, WGC3Dfloat x, WGC3Dfloat y, WGC3Dfloat z, WGC3Dfloat w) OVERRIDE { return impl_->vertexAttrib4f(index, x, y, z, w); }
  virtual void vertexAttrib4fv(WGC3Duint index, const WGC3Dfloat* values) OVERRIDE { return impl_->vertexAttrib4fv(index, values); }
  virtual void vertexAttribPointer(WGC3Duint index, WGC3Dint size, WGC3Denum type, WGC3Dboolean normalized, WGC3Dsizei stride, WGC3Dintptr offset) OVERRIDE { impl_->vertexAttribPointer(index, size, type, normalized, stride, offset); }
  virtual void viewport(WGC3Dint x, WGC3Dint y, WGC3Dsizei width, WGC3Dsizei height) OVERRIDE { impl_->viewport(x, y, width, height); }
  virtual WebGLId createBuffer() OVERRIDE { return impl_->createBuffer(); }
  virtual WebGLId createFramebuffer() OVERRIDE { return impl_->createFramebuffer(); }
  virtual WebGLId createProgram() OVERRIDE { return impl_->createProgram(); }
  virtual WebGLId createRenderbuffer() OVERRIDE { return impl_->createRenderbuffer(); }
  virtual WebGLId createShader(WGC3Denum shader) OVERRIDE { return impl_->createShader(shader); }
  virtual WebGLId createTexture() OVERRIDE { return impl_->createTexture(); }
  virtual void deleteBuffer(WebGLId buffer) OVERRIDE { impl_->deleteBuffer(buffer); }
  virtual void deleteFramebuffer(WebGLId framebuffer) OVERRIDE { impl_->deleteFramebuffer(framebuffer); }
  virtual void deleteProgram(WebGLId program) OVERRIDE { impl_->deleteProgram(program); }
  virtual void deleteRenderbuffer(WebGLId renderbuffer) OVERRIDE { impl_->deleteRenderbuffer(renderbuffer); }
  virtual void deleteShader(WebGLId shader) OVERRIDE { impl_->deleteShader(shader); }
  virtual void deleteTexture(WebGLId texture) OVERRIDE { impl_->deleteTexture(texture); }
  virtual void texSubImageSub(WGC3Denum format, int dstOffX, int dstOffY, int dstWidth, int dstHeight, int srcX, int srcY, int srcWidth, const void* image) OVERRIDE { impl_->texSubImageSub(format, dstOffX, dstOffY, dstWidth, dstHeight, srcX, srcY, srcWidth, image); }
#if defined(__LB_WIIU__)
  virtual void setOutputSurfaceLBSHELL(int surface_id) { impl_->setOutputSurfaceLBSHELL(surface_id); }
#endif
  virtual void setContextLostCallback(WebGraphicsContextLostCallback* callback) OVERRIDE { impl_->setContextLostCallback(callback); }
  virtual void setErrorMessageCallback(WebGraphicsErrorMessageCallback* callback) OVERRIDE { impl_->setErrorMessageCallback(callback); }
  virtual WGC3Denum getGraphicsResetStatusARB() OVERRIDE { return impl_->getGraphicsResetStatusARB(); }
  virtual WebString getTranslatedShaderSourceANGLE(WebGLId shader) OVERRIDE { return impl_->getTranslatedShaderSourceANGLE(shader); }
  virtual void texImageIOSurface2DCHROMIUM(WGC3Denum target, WGC3Dint width, WGC3Dint height, WGC3Duint ioSurfaceId, WGC3Duint plane) OVERRIDE { return impl_->texImageIOSurface2DCHROMIUM(target, width, height, ioSurfaceId, plane); }
  virtual void texStorage2DEXT(WGC3Denum target, WGC3Dint levels, WGC3Duint internalformat, WGC3Dint width, WGC3Dint height) OVERRIDE { return impl_->texStorage2DEXT(target, levels, internalformat, width, height); }
  virtual WebGLId createQueryEXT() OVERRIDE { return impl_->createQueryEXT(); }
  virtual void deleteQueryEXT(WebGLId query) OVERRIDE { impl_->deleteQueryEXT(query); }
  virtual WGC3Dboolean isQueryEXT(WebGLId query) OVERRIDE { return impl_->isQueryEXT(query); }
  virtual void beginQueryEXT(WGC3Denum target, WebGLId query) OVERRIDE { impl_->beginQueryEXT(target, query); }
  virtual void endQueryEXT(WGC3Denum target) OVERRIDE { impl_->endQueryEXT(target); }
  virtual void getQueryivEXT(WGC3Denum target, WGC3Denum pname, WGC3Dint* params) OVERRIDE { impl_->getQueryivEXT(target, pname, params); }
  virtual void getQueryObjectuivEXT(WebGLId query, WGC3Denum pname, WGC3Duint* params) OVERRIDE { impl_->getQueryObjectuivEXT(query, pname, params); }
  virtual void bindUniformLocationCHROMIUM(WebGLId program, WGC3Dint location, const WGC3Dchar* uniform) OVERRIDE { impl_->bindUniformLocationCHROMIUM(program, location, uniform); }
  virtual void copyTextureCHROMIUM(WGC3Denum target, WGC3Duint sourceId, WGC3Duint destId, WGC3Dint level, WGC3Denum internalFormat) OVERRIDE { impl_->copyTextureCHROMIUM(target, sourceId, destId, level, internalFormat); }
  virtual void shallowFlushCHROMIUM() OVERRIDE { impl_->shallowFlushCHROMIUM(); }
  virtual void genMailboxCHROMIUM(WebKit::WGC3Dbyte* mailbox) { impl_->genMailboxCHROMIUM(mailbox); }
  virtual void produceTextureCHROMIUM(WebKit::WGC3Denum target, const WebKit::WGC3Dbyte* mailbox) OVERRIDE { impl_->produceTextureCHROMIUM(target, mailbox); }
  virtual void consumeTextureCHROMIUM(WGC3Denum target, const WebKit::WGC3Dbyte* mailbox) OVERRIDE { impl_->consumeTextureCHROMIUM(target, mailbox); }
  virtual void insertEventMarkerEXT(const WGC3Dchar* marker) OVERRIDE { impl_->insertEventMarkerEXT(marker); }
  virtual void pushGroupMarkerEXT(const WGC3Dchar* marker) OVERRIDE { impl_->pushGroupMarkerEXT(marker); }
  virtual void popGroupMarkerEXT(void) OVERRIDE { impl_->popGroupMarkerEXT(); }
  virtual WebGLId createVertexArrayOES() OVERRIDE { return impl_->createVertexArrayOES(); }
  virtual void deleteVertexArrayOES(WebGLId array) OVERRIDE { impl_->deleteVertexArrayOES(array); }
  virtual WGC3Dboolean isVertexArrayOES(WebGLId array) OVERRIDE { return impl_->isVertexArrayOES(array); }
  virtual void bindVertexArrayOES(WebGLId array) OVERRIDE { impl_->bindVertexArrayOES(array); }
  virtual void bindTexImage2DCHROMIUM(WGC3Denum target, WGC3Dint imageId) OVERRIDE { impl_->bindTexImage2DCHROMIUM(target, imageId); }
  virtual void releaseTexImage2DCHROMIUM(WGC3Denum target, WGC3Dint imageId) OVERRIDE { impl_->releaseTexImage2DCHROMIUM(target, imageId); }
  virtual void* mapBufferCHROMIUM(WGC3Denum target, WGC3Denum access) OVERRIDE { return impl_->mapBufferCHROMIUM(target, access); }
  virtual WGC3Dboolean unmapBufferCHROMIUM(WGC3Denum target) OVERRIDE { return impl_->unmapBufferCHROMIUM(target); }
  virtual void asyncTexImage2DCHROMIUM(WGC3Denum target, WGC3Dint level, WGC3Denum internalformat, WGC3Dsizei width, WGC3Dsizei height, WGC3Dint border, WGC3Denum format, WGC3Denum type, const void* pixels) OVERRIDE { impl_->asyncTexImage2DCHROMIUM(target, level, internalformat, width, height, border, format, type, pixels); }
  virtual void asyncTexSubImage2DCHROMIUM(WGC3Denum target, WGC3Dint level, WGC3Dint xoffset, WGC3Dint yoffset, WGC3Dsizei width, WGC3Dsizei height, WGC3Denum format, WGC3Denum type, const void* pixels) OVERRIDE { impl_->asyncTexSubImage2DCHROMIUM(target, level, xoffset, yoffset, width, height, format, type, pixels); }
#if !defined(__LB_DISABLE_SKIA_GPU__)
  virtual GrGLInterface* onCreateGrGLInterface() OVERRIDE { return impl_->onCreateGrGLInterface(); }
#endif
};

#endif  // SRC_LB_WEB_GRAPHICS_CONTEXT_PROXY_H_
