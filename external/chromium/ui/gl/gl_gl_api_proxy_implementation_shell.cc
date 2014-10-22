// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "gl_gl_api_proxy_implementation_shell.h"

#include "chromium/ui/gl/gl_gl_api_implementation_shell.h"

namespace gfx {

GLApiProxyShell::GLApiProxyShell() {
  gl_api_shell_.reset(new GLApiShell());
}

GLApiProxyShell::~GLApiProxyShell() {

}

void GLApiProxyShell::glActiveTextureFn(GLenum texture) {
  gl_api_shell_->glActiveTextureFn(texture);
}

void GLApiProxyShell::glAttachShaderFn(GLuint program, GLuint shader) {
  gl_api_shell_->glAttachShaderFn(program, shader);
}

void GLApiProxyShell::glBeginQueryFn(GLenum target, GLuint id) {
  gl_api_shell_->glBeginQueryFn(target, id);
}

void GLApiProxyShell::glBeginQueryARBFn(GLenum target, GLuint id) {
  gl_api_shell_->glBeginQueryARBFn(target, id);
}

void GLApiProxyShell::glBindAttribLocationFn(GLuint program, GLuint index, const char* name) {
  gl_api_shell_->glBindAttribLocationFn(program, index, name);
}

void GLApiProxyShell::glBindBufferFn(GLenum target, GLuint buffer) {
  gl_api_shell_->glBindBufferFn(target, buffer);
}

void GLApiProxyShell::glBindFragDataLocationFn(GLuint program, GLuint colorNumber, const char* name) {
  gl_api_shell_->glBindFragDataLocationFn(program, colorNumber, name);
}

void GLApiProxyShell::glBindFragDataLocationIndexedFn(GLuint program, GLuint colorNumber, GLuint index, const char* name) {
  gl_api_shell_->glBindFragDataLocationIndexedFn(program, colorNumber, index, name);
}

void GLApiProxyShell::glBindFramebufferEXTFn(GLenum target, GLuint framebuffer) {
  gl_api_shell_->glBindFramebufferEXTFn(target, framebuffer);
}

void GLApiProxyShell::glBindRenderbufferEXTFn(GLenum target, GLuint renderbuffer) {
  gl_api_shell_->glBindRenderbufferEXTFn(target, renderbuffer);
}

void GLApiProxyShell::glBindTextureFn(GLenum target, GLuint texture) {
  gl_api_shell_->glBindTextureFn(target, texture);
}

void GLApiProxyShell::glBlendColorFn(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  gl_api_shell_->glBlendColorFn(red, green, blue, alpha);
}

void GLApiProxyShell::glBlendEquationFn( GLenum mode ) {
  gl_api_shell_->glBlendEquationFn( mode );
}

void GLApiProxyShell::glBlendEquationSeparateFn(GLenum modeRGB, GLenum modeAlpha) {
  gl_api_shell_->glBlendEquationSeparateFn(modeRGB, modeAlpha);
}

void GLApiProxyShell::glBlendFuncFn(GLenum sfactor, GLenum dfactor) {
  gl_api_shell_->glBlendFuncFn(sfactor, dfactor);
}

void GLApiProxyShell::glBlendFuncSeparateFn(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
  gl_api_shell_->glBlendFuncSeparateFn(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void GLApiProxyShell::glBlitFramebufferEXTFn(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
  gl_api_shell_->glBlitFramebufferEXTFn(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void GLApiProxyShell::glBlitFramebufferANGLEFn(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
  gl_api_shell_->glBlitFramebufferANGLEFn(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void GLApiProxyShell::glBufferDataFn(GLenum target, GLsizei size, const void* data, GLenum usage) {
  gl_api_shell_->glBufferDataFn(target, size, data, usage);
}

void GLApiProxyShell::glBufferSubDataFn(GLenum target, GLint offset, GLsizei size, const void* data) {
  gl_api_shell_->glBufferSubDataFn(target, offset, size, data);
}

GLenum GLApiProxyShell::glCheckFramebufferStatusEXTFn(GLenum target) {
  return gl_api_shell_->glCheckFramebufferStatusEXTFn(target);
}

void GLApiProxyShell::glClearFn(GLbitfield mask) {
  gl_api_shell_->glClearFn(mask);
}

void GLApiProxyShell::glClearColorFn(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  gl_api_shell_->glClearColorFn(red, green, blue, alpha);
}

void GLApiProxyShell::glClearDepthFn(GLclampd depth) {
  gl_api_shell_->glClearDepthFn(depth);
}

void GLApiProxyShell::glClearDepthfFn(GLclampf depth) {
  gl_api_shell_->glClearDepthfFn(depth);
}

void GLApiProxyShell::glClearStencilFn(GLint s) {
  gl_api_shell_->glClearStencilFn(s);
}

void GLApiProxyShell::glColorMaskFn(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
  gl_api_shell_->glColorMaskFn(red, green, blue, alpha);
}

void GLApiProxyShell::glCompileShaderFn(GLuint shader) {
  gl_api_shell_->glCompileShaderFn(shader);
}

void GLApiProxyShell::glCompressedTexImage2DFn(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) {
  gl_api_shell_->glCompressedTexImage2DFn(target, level, internalformat, width, height, border, imageSize, data);
}

void GLApiProxyShell::glCompressedTexSubImage2DFn(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data) {
  gl_api_shell_->glCompressedTexSubImage2DFn(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void GLApiProxyShell::glCopyTexImage2DFn(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
  gl_api_shell_->glCopyTexImage2DFn(target, level, internalformat, x, y, width, height, border);
}

void GLApiProxyShell::glCopyTexSubImage2DFn(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
  gl_api_shell_->glCopyTexSubImage2DFn(target, level, xoffset, yoffset, x, y, width, height);
}

GLuint GLApiProxyShell::glCreateProgramFn(void) {
  return gl_api_shell_->glCreateProgramFn();
}

GLuint GLApiProxyShell::glCreateShaderFn(GLenum type) {
  return gl_api_shell_->glCreateShaderFn(type);
}

void GLApiProxyShell::glCullFaceFn(GLenum mode) {
  gl_api_shell_->glCullFaceFn(mode);
}

void GLApiProxyShell::glDeleteBuffersARBFn(GLsizei n, const GLuint* buffers) {
  gl_api_shell_->glDeleteBuffersARBFn(n, buffers);
}

void GLApiProxyShell::glDeleteFramebuffersEXTFn(GLsizei n, const GLuint* framebuffers) {
  gl_api_shell_->glDeleteFramebuffersEXTFn(n, framebuffers);
}

void GLApiProxyShell::glDeleteProgramFn(GLuint program) {
  gl_api_shell_->glDeleteProgramFn(program);
}

void GLApiProxyShell::glDeleteQueriesFn(GLsizei n, const GLuint* ids) {
  gl_api_shell_->glDeleteQueriesFn(n, ids);
}

void GLApiProxyShell::glDeleteQueriesARBFn(GLsizei n, const GLuint* ids) {
  gl_api_shell_->glDeleteQueriesARBFn(n, ids);
}

void GLApiProxyShell::glDeleteRenderbuffersEXTFn(GLsizei n, const GLuint* renderbuffers) {
  gl_api_shell_->glDeleteRenderbuffersEXTFn(n, renderbuffers);
}

void GLApiProxyShell::glDeleteShaderFn(GLuint shader) {
  gl_api_shell_->glDeleteShaderFn(shader);
}

void GLApiProxyShell::glDeleteTexturesFn(GLsizei n, const GLuint* textures) {
  gl_api_shell_->glDeleteTexturesFn(n, textures);
}

void GLApiProxyShell::glDepthFuncFn(GLenum func) {
  gl_api_shell_->glDepthFuncFn(func);
}

void GLApiProxyShell::glDepthMaskFn(GLboolean flag) {
  gl_api_shell_->glDepthMaskFn(flag);
}

void GLApiProxyShell::glDepthRangeFn(GLclampd zNear, GLclampd zFar) {
  gl_api_shell_->glDepthRangeFn(zNear, zFar);
}

void GLApiProxyShell::glDepthRangefFn(GLclampf zNear, GLclampf zFar) {
  gl_api_shell_->glDepthRangefFn(zNear, zFar);
}

void GLApiProxyShell::glDetachShaderFn(GLuint program, GLuint shader) {
  gl_api_shell_->glDetachShaderFn(program, shader);
}

void GLApiProxyShell::glDisableFn(GLenum cap) {
  gl_api_shell_->glDisableFn(cap);
}

void GLApiProxyShell::glDisableVertexAttribArrayFn(GLuint index) {
  gl_api_shell_->glDisableVertexAttribArrayFn(index);
}

void GLApiProxyShell::glDrawArraysFn(GLenum mode, GLint first, GLsizei count) {
  gl_api_shell_->glDrawArraysFn(mode, first, count);
}

void GLApiProxyShell::glDrawBufferFn(GLenum mode) {
  gl_api_shell_->glDrawBufferFn(mode);
}

void GLApiProxyShell::glDrawBuffersARBFn(GLsizei n, const GLenum* bufs) {
  gl_api_shell_->glDrawBuffersARBFn(n, bufs);
}

void GLApiProxyShell::glDrawElementsFn(GLenum mode, GLsizei count, GLenum type, const void* indices) {
  gl_api_shell_->glDrawElementsFn(mode, count, type, indices);
}

void GLApiProxyShell::glEGLImageTargetTexture2DOESFn(GLenum target, GLeglImageOES image) {
  gl_api_shell_->glEGLImageTargetTexture2DOESFn(target, image);
}

void GLApiProxyShell::glEGLImageTargetRenderbufferStorageOESFn(GLenum target, GLeglImageOES image) {
  gl_api_shell_->glEGLImageTargetRenderbufferStorageOESFn(target, image);
}

void GLApiProxyShell::glEnableFn(GLenum cap) {
  gl_api_shell_->glEnableFn(cap);
}

void GLApiProxyShell::glEnableVertexAttribArrayFn(GLuint index) {
  gl_api_shell_->glEnableVertexAttribArrayFn(index);
}

void GLApiProxyShell::glEndQueryFn(GLenum target) {
  gl_api_shell_->glEndQueryFn(target);
}

void GLApiProxyShell::glEndQueryARBFn(GLenum target) {
  gl_api_shell_->glEndQueryARBFn(target);
}

void GLApiProxyShell::glFinishFn(void) {
  gl_api_shell_->glFinishFn();
}

void GLApiProxyShell::glFlushFn(void) {
  gl_api_shell_->glFlushFn();
}

void GLApiProxyShell::glFramebufferRenderbufferEXTFn(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
  gl_api_shell_->glFramebufferRenderbufferEXTFn(target, attachment, renderbuffertarget, renderbuffer);
}

void GLApiProxyShell::glFramebufferTexture2DEXTFn(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
  gl_api_shell_->glFramebufferTexture2DEXTFn(target, attachment, textarget, texture, level);
}

void GLApiProxyShell::glFrontFaceFn(GLenum mode) {
  gl_api_shell_->glFrontFaceFn(mode);
}

void GLApiProxyShell::glGenBuffersARBFn(GLsizei n, GLuint* buffers) {
  gl_api_shell_->glGenBuffersARBFn(n, buffers);
}

void GLApiProxyShell::glGenQueriesFn(GLsizei n, GLuint* ids) {
  gl_api_shell_->glGenQueriesFn(n, ids);
}

void GLApiProxyShell::glGenQueriesARBFn(GLsizei n, GLuint* ids) {
  gl_api_shell_->glGenQueriesARBFn(n, ids);
}

void GLApiProxyShell::glGenerateMipmapEXTFn(GLenum target) {
  gl_api_shell_->glGenerateMipmapEXTFn(target);
}

void GLApiProxyShell::glGenFramebuffersEXTFn(GLsizei n, GLuint* framebuffers) {
  gl_api_shell_->glGenFramebuffersEXTFn(n, framebuffers);
}

void GLApiProxyShell::glGenRenderbuffersEXTFn(GLsizei n, GLuint* renderbuffers) {
  gl_api_shell_->glGenRenderbuffersEXTFn(n, renderbuffers);
}

void GLApiProxyShell::glGenTexturesFn(GLsizei n, GLuint* textures) {
  gl_api_shell_->glGenTexturesFn(n, textures);
}

void GLApiProxyShell::glGetActiveAttribFn(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name) {
  gl_api_shell_->glGetActiveAttribFn(program, index, bufsize, length, size, type, name);
}

void GLApiProxyShell::glGetActiveUniformFn(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name) {
  gl_api_shell_->glGetActiveUniformFn(program, index, bufsize, length, size, type, name);
}

void GLApiProxyShell::glGetAttachedShadersFn(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders) {
  gl_api_shell_->glGetAttachedShadersFn(program, maxcount, count, shaders);
}

GLint GLApiProxyShell::glGetAttribLocationFn(GLuint program, const char* name) {
  return gl_api_shell_->glGetAttribLocationFn(program, name);
}

void GLApiProxyShell::glGetBooleanvFn(GLenum pname, GLboolean* params) {
  gl_api_shell_->glGetBooleanvFn(pname, params);
}

void GLApiProxyShell::glGetBufferParameterivFn(GLenum target, GLenum pname, GLint* params) {
  gl_api_shell_->glGetBufferParameterivFn(target, pname, params);
}

GLenum GLApiProxyShell::glGetErrorFn(void) {
  return gl_api_shell_->glGetErrorFn();
}

void GLApiProxyShell::glGetFloatvFn(GLenum pname, GLfloat* params) {
  gl_api_shell_->glGetFloatvFn(pname, params);
}

void GLApiProxyShell::glGetFramebufferAttachmentParameterivEXTFn(GLenum target, GLenum attachment, GLenum pname, GLint* params) {
  gl_api_shell_->glGetFramebufferAttachmentParameterivEXTFn(target, attachment, pname, params);
}

GLenum GLApiProxyShell::glGetGraphicsResetStatusARBFn(void) {
  return gl_api_shell_->glGetGraphicsResetStatusARBFn();
}

void GLApiProxyShell::glGetIntegervFn(GLenum pname, GLint* params) {
  gl_api_shell_->glGetIntegervFn(pname, params);
}

void GLApiProxyShell::glGetProgramBinaryFn(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, GLvoid* binary) {
  gl_api_shell_->glGetProgramBinaryFn(program, bufSize, length, binaryFormat, binary);
}

void GLApiProxyShell::glGetProgramivFn(GLuint program, GLenum pname, GLint* params) {
  gl_api_shell_->glGetProgramivFn(program, pname, params);
}

void GLApiProxyShell::glGetProgramInfoLogFn(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog) {
  gl_api_shell_->glGetProgramInfoLogFn(program, bufsize, length, infolog);
}

void GLApiProxyShell::glGetQueryivFn(GLenum target, GLenum pname, GLint* params) {
  gl_api_shell_->glGetQueryivFn(target, pname, params);
}

void GLApiProxyShell::glGetQueryivARBFn(GLenum target, GLenum pname, GLint* params) {
  gl_api_shell_->glGetQueryivARBFn(target, pname, params);
}

void GLApiProxyShell::glGetQueryObjecti64vFn(GLuint id, GLenum pname, GLint64* params) {
  gl_api_shell_->glGetQueryObjecti64vFn(id, pname, params);
}

void GLApiProxyShell::glGetQueryObjectivFn(GLuint id, GLenum pname, GLint* params) {
  gl_api_shell_->glGetQueryObjectivFn(id, pname, params);
}

void GLApiProxyShell::glGetQueryObjectui64vFn(GLuint id, GLenum pname, GLuint64* params) {
  gl_api_shell_->glGetQueryObjectui64vFn(id, pname, params);
}

void GLApiProxyShell::glGetQueryObjectuivFn(GLuint id, GLenum pname, GLuint* params) {
  gl_api_shell_->glGetQueryObjectuivFn(id, pname, params);
}

void GLApiProxyShell::glGetQueryObjectuivARBFn(GLuint id, GLenum pname, GLuint* params) {
  gl_api_shell_->glGetQueryObjectuivARBFn(id, pname, params);
}

void GLApiProxyShell::glGetRenderbufferParameterivEXTFn(GLenum target, GLenum pname, GLint* params) {
  gl_api_shell_->glGetRenderbufferParameterivEXTFn(target, pname, params);
}

void GLApiProxyShell::glGetShaderivFn(GLuint shader, GLenum pname, GLint* params) {
  gl_api_shell_->glGetShaderivFn(shader, pname, params);
}

void GLApiProxyShell::glGetShaderInfoLogFn(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog) {
  gl_api_shell_->glGetShaderInfoLogFn(shader, bufsize, length, infolog);
}

void GLApiProxyShell::glGetShaderPrecisionFormatFn(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) {
  gl_api_shell_->glGetShaderPrecisionFormatFn(shadertype, precisiontype, range, precision);
}

void GLApiProxyShell::glGetShaderSourceFn(GLuint shader, GLsizei bufsize, GLsizei* length, char* source) {
  gl_api_shell_->glGetShaderSourceFn(shader, bufsize, length, source);
}

const GLubyte* GLApiProxyShell::glGetStringFn(GLenum name) {
  return gl_api_shell_->glGetStringFn(name);
}

void GLApiProxyShell::glGetTexLevelParameterfvFn(GLenum target, GLint level, GLenum pname, GLfloat* params) {
  gl_api_shell_->glGetTexLevelParameterfvFn(target, level, pname, params);
}

void GLApiProxyShell::glGetTexLevelParameterivFn(GLenum target, GLint level, GLenum pname, GLint* params) {
  gl_api_shell_->glGetTexLevelParameterivFn(target, level, pname, params);
}

void GLApiProxyShell::glGetTexParameterfvFn(GLenum target, GLenum pname, GLfloat* params) {
  gl_api_shell_->glGetTexParameterfvFn(target, pname, params);
}

void GLApiProxyShell::glGetTexParameterivFn(GLenum target, GLenum pname, GLint* params) {
  gl_api_shell_->glGetTexParameterivFn(target, pname, params);
}

void GLApiProxyShell::glGetTranslatedShaderSourceANGLEFn(GLuint shader, GLsizei bufsize, GLsizei* length, char* source) {
  gl_api_shell_->glGetTranslatedShaderSourceANGLEFn(shader, bufsize, length, source);
}

void GLApiProxyShell::glGetUniformfvFn(GLuint program, GLint location, GLfloat* params) {
  gl_api_shell_->glGetUniformfvFn(program, location, params);
}

void GLApiProxyShell::glGetUniformivFn(GLuint program, GLint location, GLint* params) {
  gl_api_shell_->glGetUniformivFn(program, location, params);
}

GLint GLApiProxyShell::glGetUniformLocationFn(GLuint program, const char* name) {
  return gl_api_shell_->glGetUniformLocationFn(program, name);
}

void GLApiProxyShell::glGetVertexAttribfvFn(GLuint index, GLenum pname, GLfloat* params) {
  gl_api_shell_->glGetVertexAttribfvFn(index, pname, params);
}

void GLApiProxyShell::glGetVertexAttribivFn(GLuint index, GLenum pname, GLint* params) {
  gl_api_shell_->glGetVertexAttribivFn(index, pname, params);
}

void GLApiProxyShell::glGetVertexAttribPointervFn(GLuint index, GLenum pname, void** pointer) {
  gl_api_shell_->glGetVertexAttribPointervFn(index, pname, pointer);
}

void GLApiProxyShell::glHintFn(GLenum target, GLenum mode) {
  gl_api_shell_->glHintFn(target, mode);
}

GLboolean GLApiProxyShell::glIsBufferFn(GLuint buffer) {
  return gl_api_shell_->glIsBufferFn(buffer);
}

GLboolean GLApiProxyShell::glIsEnabledFn(GLenum cap) {
  return gl_api_shell_->glIsEnabledFn(cap);
}

GLboolean GLApiProxyShell::glIsFramebufferEXTFn(GLuint framebuffer) {
  return gl_api_shell_->glIsFramebufferEXTFn(framebuffer);
}

GLboolean GLApiProxyShell::glIsProgramFn(GLuint program) {
  return gl_api_shell_->glIsProgramFn(program);
}

GLboolean GLApiProxyShell::glIsQueryARBFn(GLuint query) {
  return gl_api_shell_->glIsQueryARBFn(query);
}

GLboolean GLApiProxyShell::glIsRenderbufferEXTFn(GLuint renderbuffer) {
  return gl_api_shell_->glIsRenderbufferEXTFn(renderbuffer);
}

GLboolean GLApiProxyShell::glIsShaderFn(GLuint shader) {
  return gl_api_shell_->glIsShaderFn(shader);
}

GLboolean GLApiProxyShell::glIsTextureFn(GLuint texture) {
  return gl_api_shell_->glIsTextureFn(texture);
}

void GLApiProxyShell::glLineWidthFn(GLfloat width) {
  gl_api_shell_->glLineWidthFn(width);
}

void GLApiProxyShell::glLinkProgramFn(GLuint program) {
  gl_api_shell_->glLinkProgramFn(program);
}

void* GLApiProxyShell::glMapBufferFn(GLenum target, GLenum access) {
  return gl_api_shell_->glMapBufferFn(target, access);
}

void GLApiProxyShell::glPixelStoreiFn(GLenum pname, GLint param) {
  gl_api_shell_->glPixelStoreiFn(pname, param);
}

void GLApiProxyShell::glPointParameteriFn(GLenum pname, GLint param) {
  gl_api_shell_->glPointParameteriFn(pname, param);
}

void GLApiProxyShell::glPolygonOffsetFn(GLfloat factor, GLfloat units) {
  gl_api_shell_->glPolygonOffsetFn(factor, units);
}

void GLApiProxyShell::glProgramBinaryFn(GLuint program, GLenum binaryFormat, const GLvoid* binary, GLsizei length) {
  gl_api_shell_->glProgramBinaryFn(program, binaryFormat, binary, length);
}

void GLApiProxyShell::glProgramParameteriFn(GLuint program, GLenum pname, GLint value) {
  gl_api_shell_->glProgramParameteriFn(program, pname, value);
}

void GLApiProxyShell::glQueryCounterFn(GLuint id, GLenum target) {
  gl_api_shell_->glQueryCounterFn(id, target);
}

void GLApiProxyShell::glReadBufferFn(GLenum src) {
  gl_api_shell_->glReadBufferFn(src);
}

void GLApiProxyShell::glReadPixelsFn(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) {
  gl_api_shell_->glReadPixelsFn(x, y, width, height, format, type, pixels);
}

void GLApiProxyShell::glReleaseShaderCompilerFn(void) {
  gl_api_shell_->glReleaseShaderCompilerFn();
}

void GLApiProxyShell::glRenderbufferStorageMultisampleEXTFn(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
  gl_api_shell_->glRenderbufferStorageMultisampleEXTFn(target, samples, internalformat, width, height);
}

void GLApiProxyShell::glRenderbufferStorageMultisampleANGLEFn(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
  gl_api_shell_->glRenderbufferStorageMultisampleANGLEFn(target, samples, internalformat, width, height);
}

void GLApiProxyShell::glRenderbufferStorageEXTFn(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
  gl_api_shell_->glRenderbufferStorageEXTFn(target, internalformat, width, height);
}

void GLApiProxyShell::glSampleCoverageFn(GLclampf value, GLboolean invert) {
  gl_api_shell_->glSampleCoverageFn(value, invert);
}

void GLApiProxyShell::glScissorFn(GLint x, GLint y, GLsizei width, GLsizei height) {
  gl_api_shell_->glScissorFn(x, y, width, height);
}

void GLApiProxyShell::glShaderBinaryFn(GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length) {
  gl_api_shell_->glShaderBinaryFn(n, shaders, binaryformat, binary, length);
}

void GLApiProxyShell::glShaderSourceFn(GLuint shader, GLsizei count, const char** str, const GLint* length) {
  gl_api_shell_->glShaderSourceFn(shader, count, str, length);
}

void GLApiProxyShell::glStencilFuncFn(GLenum func, GLint ref, GLuint mask) {
  gl_api_shell_->glStencilFuncFn(func, ref, mask);
}

void GLApiProxyShell::glStencilFuncSeparateFn(GLenum face, GLenum func, GLint ref, GLuint mask) {
  gl_api_shell_->glStencilFuncSeparateFn(face, func, ref, mask);
}

void GLApiProxyShell::glStencilMaskFn(GLuint mask) {
  gl_api_shell_->glStencilMaskFn(mask);
}

void GLApiProxyShell::glStencilMaskSeparateFn(GLenum face, GLuint mask) {
  gl_api_shell_->glStencilMaskSeparateFn(face, mask);
}

void GLApiProxyShell::glStencilOpFn(GLenum fail, GLenum zfail, GLenum zpass) {
  gl_api_shell_->glStencilOpFn(fail, zfail, zpass);
}

void GLApiProxyShell::glStencilOpSeparateFn(GLenum face, GLenum fail, GLenum zfail, GLenum zpass) {
  gl_api_shell_->glStencilOpSeparateFn(face, fail, zfail, zpass);
}

void GLApiProxyShell::glTexImage2DFn(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) {
  gl_api_shell_->glTexImage2DFn(target, level, internalformat, width, height, border, format, type, pixels);
}

void GLApiProxyShell::glTexParameterfFn(GLenum target, GLenum pname, GLfloat param) {
  gl_api_shell_->glTexParameterfFn(target, pname, param);
}

void GLApiProxyShell::glTexParameterfvFn(GLenum target, GLenum pname, const GLfloat* params) {
  gl_api_shell_->glTexParameterfvFn(target, pname, params);
}

void GLApiProxyShell::glTexParameteriFn(GLenum target, GLenum pname, GLint param) {
  gl_api_shell_->glTexParameteriFn(target, pname, param);
}

void GLApiProxyShell::glTexParameterivFn(GLenum target, GLenum pname, const GLint* params) {
  gl_api_shell_->glTexParameterivFn(target, pname, params);
}

void GLApiProxyShell::glTexStorage2DEXTFn(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
  gl_api_shell_->glTexStorage2DEXTFn(target, levels, internalformat, width, height);
}

void GLApiProxyShell::glTexSubImage2DFn(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) {
  gl_api_shell_->glTexSubImage2DFn(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void GLApiProxyShell::glUniform1fFn(GLint location, GLfloat x) {
  gl_api_shell_->glUniform1fFn(location, x);
}

void GLApiProxyShell::glUniform1fvFn(GLint location, GLsizei count, const GLfloat* v) {
  gl_api_shell_->glUniform1fvFn(location, count, v);
}

void GLApiProxyShell::glUniform1iFn(GLint location, GLint x) {
  gl_api_shell_->glUniform1iFn(location, x);
}

void GLApiProxyShell::glUniform1ivFn(GLint location, GLsizei count, const GLint* v) {
  gl_api_shell_->glUniform1ivFn(location, count, v);
}

void GLApiProxyShell::glUniform2fFn(GLint location, GLfloat x, GLfloat y) {
  gl_api_shell_->glUniform2fFn(location, x, y);
}

void GLApiProxyShell::glUniform2fvFn(GLint location, GLsizei count, const GLfloat* v) {
  gl_api_shell_->glUniform2fvFn(location, count, v);
}

void GLApiProxyShell::glUniform2iFn(GLint location, GLint x, GLint y) {
  gl_api_shell_->glUniform2iFn(location, x, y);
}

void GLApiProxyShell::glUniform2ivFn(GLint location, GLsizei count, const GLint* v) {
  gl_api_shell_->glUniform2ivFn(location, count, v);
}

void GLApiProxyShell::glUniform3fFn(GLint location, GLfloat x, GLfloat y, GLfloat z) {
  gl_api_shell_->glUniform3fFn(location, x, y, z);
}

void GLApiProxyShell::glUniform3fvFn(GLint location, GLsizei count, const GLfloat* v) {
  gl_api_shell_->glUniform3fvFn(location, count, v);
}

void GLApiProxyShell::glUniform3iFn(GLint location, GLint x, GLint y, GLint z) {
  gl_api_shell_->glUniform3iFn(location, x, y, z);
}

void GLApiProxyShell::glUniform3ivFn(GLint location, GLsizei count, const GLint* v) {
  gl_api_shell_->glUniform3ivFn(location, count, v);
}

void GLApiProxyShell::glUniform4fFn(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  gl_api_shell_->glUniform4fFn(location, x, y, z, w);
}

void GLApiProxyShell::glUniform4fvFn(GLint location, GLsizei count, const GLfloat* v) {
  gl_api_shell_->glUniform4fvFn(location, count, v);
}

void GLApiProxyShell::glUniform4iFn(GLint location, GLint x, GLint y, GLint z, GLint w) {
  gl_api_shell_->glUniform4iFn(location, x, y, z, w);
}

void GLApiProxyShell::glUniform4ivFn(GLint location, GLsizei count, const GLint* v) {
  gl_api_shell_->glUniform4ivFn(location, count, v);
}

void GLApiProxyShell::glUniformMatrix2fvFn(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
  gl_api_shell_->glUniformMatrix2fvFn(location, count, transpose, value);
}

void GLApiProxyShell::glUniformMatrix3fvFn(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
  gl_api_shell_->glUniformMatrix3fvFn(location, count, transpose, value);
}

void GLApiProxyShell::glUniformMatrix4fvFn(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
  gl_api_shell_->glUniformMatrix4fvFn(location, count, transpose, value);
}

GLboolean GLApiProxyShell::glUnmapBufferFn(GLenum target) {
  return gl_api_shell_->glUnmapBufferFn(target);
}

void GLApiProxyShell::glUseProgramFn(GLuint program) {
  gl_api_shell_->glUseProgramFn(program);
}

void GLApiProxyShell::glValidateProgramFn(GLuint program) {
  gl_api_shell_->glValidateProgramFn(program);
}

void GLApiProxyShell::glVertexAttrib1fFn(GLuint indx, GLfloat x) {
  gl_api_shell_->glVertexAttrib1fFn(indx, x);
}

void GLApiProxyShell::glVertexAttrib1fvFn(GLuint indx, const GLfloat* values) {
  gl_api_shell_->glVertexAttrib1fvFn(indx, values);
}

void GLApiProxyShell::glVertexAttrib2fFn(GLuint indx, GLfloat x, GLfloat y) {
  gl_api_shell_->glVertexAttrib2fFn(indx, x, y);
}

void GLApiProxyShell::glVertexAttrib2fvFn(GLuint indx, const GLfloat* values) {
  gl_api_shell_->glVertexAttrib2fvFn(indx, values);
}

void GLApiProxyShell::glVertexAttrib3fFn(GLuint indx, GLfloat x, GLfloat y, GLfloat z) {
  gl_api_shell_->glVertexAttrib3fFn(indx, x, y, z);
}

void GLApiProxyShell::glVertexAttrib3fvFn(GLuint indx, const GLfloat* values) {
  gl_api_shell_->glVertexAttrib3fvFn(indx, values);
}

void GLApiProxyShell::glVertexAttrib4fFn(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  gl_api_shell_->glVertexAttrib4fFn(indx, x, y, z, w);
}

void GLApiProxyShell::glVertexAttrib4fvFn(GLuint indx, const GLfloat* values) {
  gl_api_shell_->glVertexAttrib4fvFn(indx, values);
}

void GLApiProxyShell::glVertexAttribPointerFn(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr) {
  gl_api_shell_->glVertexAttribPointerFn(indx, size, type, normalized, stride, ptr);
}

void GLApiProxyShell::glViewportFn(GLint x, GLint y, GLsizei width, GLsizei height) {
  gl_api_shell_->glViewportFn(x, y, width, height);
}

void GLApiProxyShell::glGenFencesNVFn(GLsizei n, GLuint* fences) {
  gl_api_shell_->glGenFencesNVFn(n, fences);
}

void GLApiProxyShell::glDeleteFencesNVFn(GLsizei n, const GLuint* fences) {
  gl_api_shell_->glDeleteFencesNVFn(n, fences);
}

void GLApiProxyShell::glSetFenceNVFn(GLuint fence, GLenum condition) {
  gl_api_shell_->glSetFenceNVFn(fence, condition);
}

GLboolean GLApiProxyShell::glTestFenceNVFn(GLuint fence) {
  return gl_api_shell_->glTestFenceNVFn(fence);
}

void GLApiProxyShell::glFinishFenceNVFn(GLuint fence) {
  gl_api_shell_->glFinishFenceNVFn(fence);
}

GLboolean GLApiProxyShell::glIsFenceNVFn(GLuint fence) {
  return gl_api_shell_->glIsFenceNVFn(fence);
}

void GLApiProxyShell::glGetFenceivNVFn(GLuint fence, GLenum pname, GLint* params) {
  gl_api_shell_->glGetFenceivNVFn(fence, pname, params);
}

GLsync GLApiProxyShell::glFenceSyncFn(GLenum condition, GLbitfield flags) {
  return gl_api_shell_->glFenceSyncFn(condition, flags);
}

void GLApiProxyShell::glDeleteSyncFn(GLsync sync) {
  gl_api_shell_->glDeleteSyncFn(sync);
}

void GLApiProxyShell::glGetSyncivFn(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length,GLint* values) {
  gl_api_shell_->glGetSyncivFn(sync, pname, bufSize, length,values);
}

void GLApiProxyShell::glDrawArraysInstancedANGLEFn(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
  gl_api_shell_->glDrawArraysInstancedANGLEFn(mode, first, count, primcount);
}

void GLApiProxyShell::glDrawElementsInstancedANGLEFn(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount) {
  gl_api_shell_->glDrawElementsInstancedANGLEFn(mode, count, type, indices, primcount);
}

void GLApiProxyShell::glVertexAttribDivisorANGLEFn(GLuint index, GLuint divisor) {
  gl_api_shell_->glVertexAttribDivisorANGLEFn(index, divisor);
}

void GLApiProxyShell::glGenVertexArraysOESFn(GLsizei n, GLuint* arrays) {
  gl_api_shell_->glGenVertexArraysOESFn(n, arrays);
}

void GLApiProxyShell::glDeleteVertexArraysOESFn(GLsizei n, const GLuint* arrays) {
  gl_api_shell_->glDeleteVertexArraysOESFn(n, arrays);
}

void GLApiProxyShell::glBindVertexArrayOESFn(GLuint array) {
  gl_api_shell_->glBindVertexArrayOESFn(array);
}

GLboolean GLApiProxyShell::glIsVertexArrayOESFn(GLuint array) {
  return gl_api_shell_->glIsVertexArrayOESFn(array);
}

void GLApiProxyShell::glDiscardFramebufferEXTFn(GLenum target, GLsizei numAttachments, const GLenum* attachments) {
  gl_api_shell_->glDiscardFramebufferEXTFn(target, numAttachments, attachments);
}


}  // namespace gfx