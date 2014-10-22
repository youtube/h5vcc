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

#include "gl_gl_api_implementation_shell.h"

#include "base/logging.h"
#include "base/stringprintf.h"

namespace gfx {

GLApiShell::GLApiShell() {
}

GLApiShell::~GLApiShell() {
}

void GLApiShell::glActiveTextureFn(GLenum texture) {
  glActiveTexture(texture);
}

void GLApiShell::glAttachShaderFn(GLuint program, GLuint shader) {
  glAttachShader(program, shader);
}

void GLApiShell::glBeginQueryFn(GLenum target, GLuint id) {
  glBeginQuery(target, id);
}

void GLApiShell::glBeginQueryARBFn(GLenum target, GLuint id) {
  glBeginQueryARB(target, id);
}

void GLApiShell::glBindAttribLocationFn(
    GLuint program, GLuint index, const char* name) {
  glBindAttribLocation(program, index, name);
}

void GLApiShell::glBindBufferFn(GLenum target, GLuint buffer) {
  glBindBuffer(target, buffer);
}

void GLApiShell::glBindFragDataLocationFn(
    GLuint program, GLuint colorNumber, const char* name) {
  glBindFragDataLocation(program, colorNumber, name);
}

void GLApiShell::glBindFragDataLocationIndexedFn(
    GLuint program, GLuint colorNumber, GLuint index, const char* name) {
  NOTIMPLEMENTED();
}

void GLApiShell::glBindFramebufferEXTFn(GLenum target, GLuint framebuffer) {
  glBindFramebufferEXT(target, framebuffer);
}

void GLApiShell::glBindRenderbufferEXTFn(GLenum target, GLuint renderbuffer) {
  glBindRenderbufferEXT(target, renderbuffer);
}

void GLApiShell::glBindTextureFn(GLenum target, GLuint texture) {
  glBindTexture(target, texture);
}

void GLApiShell::glBlendColorFn(
    GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  glBlendColor(red, green, blue, alpha);
}

void GLApiShell::glBlendEquationFn(GLenum mode) {
  glBlendEquation(mode);
}

void GLApiShell::glBlendEquationSeparateFn(GLenum modeRGB, GLenum modeAlpha) {
  glBlendEquationSeparate(modeRGB, modeAlpha);
}

void GLApiShell::glBlendFuncFn(GLenum sfactor, GLenum dfactor) {
  glBlendFunc(sfactor, dfactor);
}

void GLApiShell::glBlendFuncSeparateFn(
    GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
  glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void GLApiShell::glBlitFramebufferEXTFn(
    GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
    GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
    GLbitfield mask, GLenum filter) {
  NOTIMPLEMENTED();
}

void GLApiShell::glBlitFramebufferANGLEFn(
    GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
    GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
    GLbitfield mask, GLenum filter) {
  NOTIMPLEMENTED();
}

void GLApiShell::glBufferDataFn(
    GLenum target, GLsizei size, const void* data, GLenum usage) {
  glBufferData(target, size, data, usage);
}

void GLApiShell::glBufferSubDataFn(
    GLenum target, GLint offset, GLsizei size, const void* data) {
  glBufferSubData(target, offset, size, data);
}

GLenum GLApiShell::glCheckFramebufferStatusEXTFn(GLenum target) {
  return glCheckFramebufferStatusEXT(target);
}

void GLApiShell::glClearFn(GLbitfield mask) {
  glClear(mask);
}

void GLApiShell::glClearColorFn(
    GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  glClearColor(red, green, blue, alpha);
}

void GLApiShell::glClearDepthFn(GLclampd depth) {
  glClearDepth(depth);
}

void GLApiShell::glClearDepthfFn(GLclampf depth) {
  glClearDepthf(depth);
}

void GLApiShell::glClearStencilFn(GLint s) {
  glClearStencil(s);
}

void GLApiShell::glColorMaskFn(
    GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
  glColorMask(red, green, blue, alpha);
}

void GLApiShell::glCompileShaderFn(GLuint shader) {
  glCompileShader(shader);
}

void GLApiShell::glCompressedTexImage2DFn(
    GLenum target, GLint level, GLenum internalformat,
    GLsizei width, GLsizei height, GLint border, GLsizei imageSize,
    const void* data) {
  glCompressedTexImage2D(
      target, level, internalformat, width, height, border, imageSize, data);
}

void GLApiShell::glCompressedTexSubImage2DFn(
    GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLsizei width, GLsizei height, GLenum format, GLsizei imageSize,
    const void* data) {
  glCompressedTexSubImage2D(
      target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void GLApiShell::glCopyTexImage2DFn(
    GLenum target, GLint level, GLenum internalformat, GLint x, GLint y,
    GLsizei width, GLsizei height, GLint border) {
  glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

void GLApiShell::glCopyTexSubImage2DFn(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y,
    GLsizei width, GLsizei height) {
  glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

GLuint GLApiShell::glCreateProgramFn(void) {
  return glCreateProgram();
}

GLuint GLApiShell::glCreateShaderFn(GLenum type) {
  return glCreateShader(type);
}

void GLApiShell::glCullFaceFn(GLenum mode) {
  glCullFace(mode);
}

void GLApiShell::glDeleteBuffersARBFn(GLsizei n, const GLuint* buffers) {
  glDeleteBuffersARB(n, buffers);
}

void GLApiShell::glDeleteFramebuffersEXTFn(
    GLsizei n, const GLuint* framebuffers) {
  glDeleteFramebuffersEXT(n, framebuffers);
}

void GLApiShell::glDeleteProgramFn(GLuint program) {
  glDeleteProgram(program);
}

void GLApiShell::glDeleteQueriesFn(GLsizei n, const GLuint* ids) {
  glDeleteQueries(n, ids);
}

void GLApiShell::glDeleteQueriesARBFn(GLsizei n, const GLuint* ids) {
  glDeleteQueriesARB(n, ids);
}

void GLApiShell::glDeleteRenderbuffersEXTFn(
    GLsizei n, const GLuint* renderbuffers) {
  glDeleteRenderbuffersEXT(n, renderbuffers);
}

void GLApiShell::glDeleteShaderFn(GLuint shader) {
  glDeleteShader(shader);
}

void GLApiShell::glDeleteTexturesFn(
    GLsizei n, const GLuint* textures) {
  glDeleteTextures(n, textures);
}

void GLApiShell::glDepthFuncFn(GLenum func) {
  glDepthFunc(func);
}

void GLApiShell::glDepthMaskFn(GLboolean flag) {
  glDepthMask(flag);
}

void GLApiShell::glDepthRangeFn(GLclampd zNear, GLclampd zFar) {
  glDepthRange(zNear, zFar);
}

void GLApiShell::glDepthRangefFn(GLclampf zNear, GLclampf zFar) {
  glDepthRangef(zNear, zFar);
}

void GLApiShell::glDetachShaderFn(GLuint program, GLuint shader) {
  glDetachShader(program, shader);
}

void GLApiShell::glDisableFn(GLenum cap) {
  glDisable(cap);
}

void GLApiShell::glDisableVertexAttribArrayFn(GLuint index) {
  glDisableVertexAttribArray(index);
}

void GLApiShell::glDrawArraysFn(GLenum mode, GLint first, GLsizei count) {
  glDrawArrays(mode, first, count);
}

void GLApiShell::glDrawBufferFn(GLenum mode) {
  glDrawBuffer(mode);
}

void GLApiShell::glDrawBuffersARBFn(GLsizei n, const GLenum* bufs) {
  glDrawBuffersARB(n, bufs);
}

void GLApiShell::glDrawElementsFn(
    GLenum mode, GLsizei count, GLenum type, const void* indices) {
  glDrawElements(mode, count, type, indices);
}

void GLApiShell::glEGLImageTargetTexture2DOESFn(
    GLenum target, GLeglImageOES image) {
  NOTIMPLEMENTED();
}

void GLApiShell::glEGLImageTargetRenderbufferStorageOESFn(
    GLenum target, GLeglImageOES image) {
  NOTIMPLEMENTED();
}

void GLApiShell::glEnableFn(GLenum cap) {
  switch (cap) {
    case GL_BLEND:
      glEnable(GL_BLEND);
      break;
    case GL_DEPTH_TEST:
      NOTREACHED() << "it is assumed that DEPTH_TEST is always disabled.";
      break;
    case GL_SCISSOR_TEST:
      glEnable(GL_SCISSOR_TEST);
      break;
    default:
      NOTIMPLEMENTED() << base::StringPrintf(" unsupported %x enabled", cap);
      break;
  }
}

void GLApiShell::glEnableVertexAttribArrayFn(GLuint index) {
  glEnableVertexAttribArray(index);
}

void GLApiShell::glEndQueryFn(GLenum target) {
  glEndQuery(target);
}

void GLApiShell::glEndQueryARBFn(GLenum target) {
  glEndQueryARB(target);
}

void GLApiShell::glFinishFn(void) {
  glFinish();
}

void GLApiShell::glFlushFn(void) {
  glFlush();
}

void GLApiShell::glFramebufferRenderbufferEXTFn(
    GLenum target, GLenum attachment, GLenum renderbuffertarget,
    GLuint renderbuffer) {
  glFramebufferRenderbufferEXT(
      target, attachment, renderbuffertarget, renderbuffer);
}

void GLApiShell::glFramebufferTexture2DEXTFn(
    GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
    GLint level) {
  glFramebufferTexture2DEXT(target, attachment, textarget, texture, level);
}

void GLApiShell::glFrontFaceFn(GLenum mode) {
  glFrontFace(mode);
}

void GLApiShell::glGenBuffersARBFn(GLsizei n, GLuint* buffers) {
  glGenBuffersARB(n, buffers);
}

void GLApiShell::glGenQueriesFn(GLsizei n, GLuint* ids) {
  glGenQueries(n, ids);
}

void GLApiShell::glGenQueriesARBFn(GLsizei n, GLuint* ids) {
  glGenQueriesARB(n, ids);
}

void GLApiShell::glGenerateMipmapEXTFn(GLenum target) {
  glGenerateMipmapEXT(target);
}

void GLApiShell::glGenFramebuffersEXTFn(GLsizei n, GLuint* framebuffers) {
  glGenFramebuffersEXT(n, framebuffers);
}

void GLApiShell::glGenRenderbuffersEXTFn(GLsizei n, GLuint* renderbuffers) {
  glGenRenderbuffersEXT(n, renderbuffers);
}

void GLApiShell::glGenTexturesFn(GLsizei n, GLuint* textures) {
  glGenTextures(n, textures);
}

void GLApiShell::glGetActiveAttribFn(
    GLuint program, GLuint index, GLsizei bufsize, GLsizei* length,
    GLint* size, GLenum* type, char* name) {
  glGetActiveAttrib(program, index, bufsize, length, size, type, name);
}

void GLApiShell::glGetActiveUniformFn(
    GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size,
    GLenum* type, char* name) {
  glGetActiveUniform(program, index, bufsize, length, size, type, name);
}

void GLApiShell::glGetAttachedShadersFn(
    GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders) {
  glGetAttachedShaders(program, maxcount, count, shaders);
}

GLint GLApiShell::glGetAttribLocationFn(GLuint program, const char* name) {
  return glGetAttribLocation(program, name);
}

void GLApiShell::glGetBooleanvFn(GLenum pname, GLboolean* params) {
  glGetBooleanv(pname, params);
}

void GLApiShell::glGetBufferParameterivFn(
    GLenum target, GLenum pname, GLint* params) {
  glGetBufferParameteriv(target, pname, params);
}

GLenum GLApiShell::glGetErrorFn(void) {
  return glGetError();
}

void GLApiShell::glGetFloatvFn(GLenum pname, GLfloat* params) {
  glGetFloatv(pname, params);
}

void GLApiShell::glGetFramebufferAttachmentParameterivEXTFn(
    GLenum target, GLenum attachment, GLenum pname, GLint* params) {
  glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, params);
}

GLenum GLApiShell::glGetGraphicsResetStatusARBFn(void) {
  return glGetGraphicsResetStatusARB();
}

void GLApiShell::glGetIntegervFn(GLenum pname, GLint* params) {
  glGetIntegerv(pname, params);
}

void GLApiShell::glGetProgramBinaryFn(
    GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat,
    GLvoid* binary) {
  NOTIMPLEMENTED();
}

void GLApiShell::glGetProgramivFn(
    GLuint program, GLenum pname, GLint* params) {
  glGetProgramiv(program, pname, params);
}

void GLApiShell::glGetProgramInfoLogFn(
    GLuint program, GLsizei bufsize, GLsizei* length, char* infolog) {
  glGetProgramInfoLog(program, bufsize, length, infolog);
}

void GLApiShell::glGetQueryivFn(GLenum target, GLenum pname, GLint* params) {
  glGetQueryiv(target, pname, params);
}

void GLApiShell::glGetQueryivARBFn(GLenum target, GLenum pname, GLint* params) {
  glGetQueryivARB(target, pname, params);
}

void GLApiShell::glGetQueryObjecti64vFn(
    GLuint id, GLenum pname, GLint64* params) {
  NOTIMPLEMENTED();
}

void GLApiShell::glGetQueryObjectivFn(GLuint id, GLenum pname, GLint* params) {
  glGetQueryObjectiv(id, pname, params);
}

void GLApiShell::glGetQueryObjectui64vFn(
    GLuint id, GLenum pname, GLuint64* params) {
  NOTIMPLEMENTED();
}

void GLApiShell::glGetQueryObjectuivFn(
    GLuint id, GLenum pname, GLuint* params) {
  glGetQueryObjectuiv(id, pname, params);
}

void GLApiShell::glGetQueryObjectuivARBFn(
    GLuint id, GLenum pname, GLuint* params) {
  glGetQueryObjectuivARB(id, pname, params);
}

void GLApiShell::glGetRenderbufferParameterivEXTFn(
    GLenum target, GLenum pname, GLint* params) {
  glGetRenderbufferParameterivEXT(target, pname, params);
}

void GLApiShell::glGetShaderivFn(GLuint shader, GLenum pname, GLint* params) {
  glGetShaderiv(shader, pname, params);
}

void GLApiShell::glGetShaderInfoLogFn(
    GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog) {
  glGetShaderInfoLog(shader, bufsize, length, infolog);
}

void GLApiShell::glGetShaderPrecisionFormatFn(
    GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) {
  glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}

void GLApiShell::glGetShaderSourceFn(
    GLuint shader, GLsizei bufsize, GLsizei* length, char* source) {
  glGetShaderSource(shader, bufsize, length, source);
}

const GLubyte* GLApiShell::glGetStringFn(GLenum name) {
  switch (name) {
    case GL_EXTENSIONS:
      // Note: we report OES extensions even though we don't support them in
      // OpenGL.
      // This is so Chromium behaves consistently with our other platforms.
      return reinterpret_cast<const GLubyte*>(
          "GL_CHROMIUM_copy_texture_to_parent_texture "
          "GL_EXT_texture_format_BGRA8888 "
          "GL_OES_texture_npot "
          "GL_EXT_read_format_bgra "
          "GL_OES_packed_depth_stencil "
          "GL_OES_stencil_wrap "
          "GL_ARB_framebuffer_object ");
    case GL_VERSION:
      return glGetString(GL_VERSION);
    case GL_VENDOR:
      return NULL;
    case GL_RENDERER:
      return NULL;
    default:
      NOTIMPLEMENTED() << base::StringPrintf(" string %x not supported", name);
      break;
  }

  return NULL;
}

void GLApiShell::glGetTexLevelParameterfvFn(
    GLenum target, GLint level, GLenum pname, GLfloat* params) {
  glGetTexLevelParameterfv(target, level, pname, params);
}

void GLApiShell::glGetTexLevelParameterivFn(
    GLenum target, GLint level, GLenum pname, GLint* params) {
  glGetTexLevelParameteriv(target, level, pname, params);
}

void GLApiShell::glGetTexParameterfvFn(
    GLenum target, GLenum pname, GLfloat* params) {
  glGetTexParameterfv(target, pname, params);
}

void GLApiShell::glGetTexParameterivFn(
    GLenum target, GLenum pname, GLint* params) {
  glGetTexParameteriv(target, pname, params);
}

void GLApiShell::glGetTranslatedShaderSourceANGLEFn(
    GLuint shader, GLsizei bufsize, GLsizei* length, char* source) {
  NOTIMPLEMENTED();
}

void GLApiShell::glGetUniformfvFn(
    GLuint program, GLint location, GLfloat* params) {
  glGetUniformfv(program, location, params);
}

void GLApiShell::glGetUniformivFn(
    GLuint program, GLint location, GLint* params) {
  glGetUniformiv(program, location, params);
}

GLint GLApiShell::glGetUniformLocationFn(
    GLuint program, const char* name) {
  return glGetUniformLocation(program, name);
}

void GLApiShell::glGetVertexAttribfvFn(
    GLuint index, GLenum pname, GLfloat* params) {
  glGetVertexAttribfv(index, pname, params);
}

void GLApiShell::glGetVertexAttribivFn(
    GLuint index, GLenum pname, GLint* params) {
  glGetVertexAttribiv(index, pname, params);
}

void GLApiShell::glGetVertexAttribPointervFn(
    GLuint index, GLenum pname, void** pointer) {
  glGetVertexAttribPointerv(index, pname, pointer);
}

void GLApiShell::glHintFn(GLenum target, GLenum mode) {
  glHint(target, mode);
}

GLboolean GLApiShell::glIsBufferFn(GLuint buffer) {
  return glIsBuffer(buffer);
}

GLboolean GLApiShell::glIsEnabledFn(GLenum cap) {
  return glIsEnabled(cap);
}

GLboolean GLApiShell::glIsFramebufferEXTFn(GLuint framebuffer) {
  return glIsFramebufferEXT(framebuffer);
}

GLboolean GLApiShell::glIsProgramFn(GLuint program) {
  return glIsProgram(program);
}

GLboolean GLApiShell::glIsQueryARBFn(GLuint query) {
  return glIsQueryARB(query);
}

GLboolean GLApiShell::glIsRenderbufferEXTFn(GLuint renderbuffer) {
  return glIsRenderbufferEXT(renderbuffer);
}

GLboolean GLApiShell::glIsShaderFn(GLuint shader) {
  return glIsShader(shader);
}

GLboolean GLApiShell::glIsTextureFn(GLuint texture) {
  return glIsTexture(texture);
}

void GLApiShell::glLineWidthFn(GLfloat width) {
  glLineWidth(width);
}

void GLApiShell::glLinkProgramFn(GLuint program) {
  glLinkProgram(program);
}

void* GLApiShell::glMapBufferFn(GLenum target, GLenum access) {
  return glMapBuffer(target, access);
}

void GLApiShell::glPixelStoreiFn(GLenum pname, GLint param) {
  glPixelStorei(pname, param);
}

void GLApiShell::glPointParameteriFn(GLenum pname, GLint param) {
  glPointParameteri(pname, param);
}

void GLApiShell::glPolygonOffsetFn(GLfloat factor, GLfloat units) {
  glPolygonOffset(factor, units);
}

void GLApiShell::glProgramBinaryFn(
    GLuint program, GLenum binaryFormat, const GLvoid* binary, GLsizei length) {
  NOTIMPLEMENTED();
}

void GLApiShell::glProgramParameteriFn(
    GLuint program, GLenum pname, GLint value) {
  NOTIMPLEMENTED();
}

void GLApiShell::glQueryCounterFn(GLuint id, GLenum target) {
  NOTIMPLEMENTED();
}

void GLApiShell::glReadBufferFn(GLenum src) {
  glReadBuffer(src);
}

void GLApiShell::glReadPixelsFn(
    GLint x, GLint y, GLsizei width, GLsizei height,
    GLenum format, GLenum type, void* pixels) {
  glReadPixels(x, y, width, height, format, type, pixels);
}

void GLApiShell::glReleaseShaderCompilerFn(void) {
  glReleaseShaderCompiler();
}

void GLApiShell::glRenderbufferStorageMultisampleEXTFn(
    GLenum target, GLsizei samples, GLenum internalformat,
    GLsizei width, GLsizei height) {
  glRenderbufferStorageMultisampleEXT(
      target, samples, internalformat, width, height);
}

void GLApiShell::glRenderbufferStorageMultisampleANGLEFn(
    GLenum target, GLsizei samples, GLenum internalformat,
    GLsizei width, GLsizei height) {
  NOTIMPLEMENTED();
}

void GLApiShell::glRenderbufferStorageEXTFn(
    GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
  glRenderbufferStorageEXT(target, internalformat, width, height);
}

void GLApiShell::glSampleCoverageFn(GLclampf value, GLboolean invert) {
  glSampleCoverage(value, invert);
}

void GLApiShell::glScissorFn(GLint x, GLint y, GLsizei width, GLsizei height) {
  glScissor(x, y, width, height);
}

void GLApiShell::glShaderBinaryFn(
    GLsizei n, const GLuint* shaders, GLenum binaryformat,
    const void* binary, GLsizei length) {
  glShaderBinary(n, shaders, binaryformat, binary, length);
}

void GLApiShell::glShaderSourceFn(
    GLuint shader, GLsizei count, const char** str, const GLint* length) {
  glShaderSource(shader, count, str, length);
}

void GLApiShell::glStencilFuncFn(GLenum func, GLint ref, GLuint mask) {
  glStencilFunc(func, ref, mask);
}

void GLApiShell::glStencilFuncSeparateFn(
    GLenum face, GLenum func, GLint ref, GLuint mask) {
  glStencilFuncSeparate(face, func, ref, mask);
}

void GLApiShell::glStencilMaskFn(GLuint mask) {
  glStencilMask(mask);
}

void GLApiShell::glStencilMaskSeparateFn(GLenum face, GLuint mask) {
  glStencilMaskSeparate(face, mask);
}

void GLApiShell::glStencilOpFn(GLenum fail, GLenum zfail, GLenum zpass) {
  glStencilOp(fail, zfail, zpass);
}

void GLApiShell::glStencilOpSeparateFn(
    GLenum face, GLenum fail, GLenum zfail, GLenum zpass) {
  glStencilOpSeparate(face, fail, zfail, zpass);
}

void GLApiShell::glTexImage2DFn(
    GLenum target, GLint level, GLint internalformat,
    GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type,
    const void* pixels) {
  glTexImage2D(
      target, level, internalformat, width, height, border, format, type,
      pixels);
}

void GLApiShell::glTexParameterfFn(GLenum target, GLenum pname, GLfloat param) {
  glTexParameterf(target, pname, param);
}

void GLApiShell::glTexParameterfvFn(
    GLenum target, GLenum pname, const GLfloat* params) {
  glTexParameterfv(target, pname, params);
}

void GLApiShell::glTexParameteriFn(GLenum target, GLenum pname, GLint param) {
  glTexParameteri(target, pname, param);
}

void GLApiShell::glTexParameterivFn(
    GLenum target, GLenum pname, const GLint* params) {
  glTexParameteriv(target, pname, params);
}

void GLApiShell::glTexStorage2DEXTFn(
    GLenum target, GLsizei levels, GLenum internalformat,
    GLsizei width, GLsizei height) {
  NOTIMPLEMENTED();
}

void GLApiShell::glTexSubImage2DFn(
    GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLsizei width, GLsizei height, GLenum format, GLenum type,
    const void* pixels) {
  glTexSubImage2D(
      target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void GLApiShell::glUniform1fFn(GLint location, GLfloat x) {
  glUniform1f(location, x);
}

void GLApiShell::glUniform1fvFn(
    GLint location, GLsizei count, const GLfloat* v) {
  glUniform1fv(location, count, v);
}

void GLApiShell::glUniform1iFn(GLint location, GLint x) {
  glUniform1i(location, x);
}

void GLApiShell::glUniform1ivFn(GLint location, GLsizei count, const GLint* v) {
  glUniform1iv(location, count, v);
}

void GLApiShell::glUniform2fFn(GLint location, GLfloat x, GLfloat y) {
  glUniform2f(location, x, y);
}

void GLApiShell::glUniform2fvFn(
    GLint location, GLsizei count, const GLfloat* v) {
  glUniform2fv(location, count, v);
}

void GLApiShell::glUniform2iFn(GLint location, GLint x, GLint y) {
  glUniform2i(location, x, y);
}

void GLApiShell::glUniform2ivFn(GLint location, GLsizei count, const GLint* v) {
  glUniform2iv(location, count, v);
}

void GLApiShell::glUniform3fFn(
    GLint location, GLfloat x, GLfloat y, GLfloat z) {
  glUniform3f(location, x, y, z);
}

void GLApiShell::glUniform3fvFn(
    GLint location, GLsizei count, const GLfloat* v) {
  glUniform3fv(location, count, v);
}

void GLApiShell::glUniform3iFn(GLint location, GLint x, GLint y, GLint z) {
  glUniform3i(location, x, y, z);
}

void GLApiShell::glUniform3ivFn(GLint location, GLsizei count, const GLint* v) {
  glUniform3iv(location, count, v);
}

void GLApiShell::glUniform4fFn(
    GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  glUniform4f(location, x, y, z, w);
}

void GLApiShell::glUniform4fvFn(
    GLint location, GLsizei count, const GLfloat* v) {
  glUniform4fv(location, count, v);
}

void GLApiShell::glUniform4iFn(
    GLint location, GLint x, GLint y, GLint z, GLint w) {
  glUniform4i(location, x, y, z, w);
}

void GLApiShell::glUniform4ivFn(GLint location, GLsizei count, const GLint* v) {
  glUniform4iv(location, count, v);
}

void GLApiShell::glUniformMatrix2fvFn(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
  glUniformMatrix2fv(location, count, transpose, value);
}

void GLApiShell::glUniformMatrix3fvFn(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
  glUniformMatrix3fv(location, count, transpose, value);
}

void GLApiShell::glUniformMatrix4fvFn(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
  glUniformMatrix4fv(location, count, transpose, value);
}

GLboolean GLApiShell::glUnmapBufferFn(GLenum target) {
  return glUnmapBuffer(target);
}

void GLApiShell::glUseProgramFn(GLuint program) {
  glUseProgram(program);
}

void GLApiShell::glValidateProgramFn(GLuint program) {
  glValidateProgram(program);
}

void GLApiShell::glVertexAttrib1fFn(GLuint indx, GLfloat x) {
  glVertexAttrib1f(indx, x);
}

void GLApiShell::glVertexAttrib1fvFn(GLuint indx, const GLfloat* values) {
  glVertexAttrib1fv(indx, values);
}

void GLApiShell::glVertexAttrib2fFn(GLuint indx, GLfloat x, GLfloat y) {
  glVertexAttrib2f(indx, x, y);
}

void GLApiShell::glVertexAttrib2fvFn(GLuint indx, const GLfloat* values) {
  glVertexAttrib2fv(indx, values);
}

void GLApiShell::glVertexAttrib3fFn(
    GLuint indx, GLfloat x, GLfloat y, GLfloat z) {
  glVertexAttrib3f(indx, x, y, z);
}

void GLApiShell::glVertexAttrib3fvFn(GLuint indx, const GLfloat* values) {
  glVertexAttrib3fv(indx, values);
}

void GLApiShell::glVertexAttrib4fFn(
    GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  glVertexAttrib4f(indx, x, y, z, w);
}

void GLApiShell::glVertexAttrib4fvFn(GLuint indx, const GLfloat* values) {
  glVertexAttrib4fv(indx, values);
}

void GLApiShell::glVertexAttribPointerFn(
    GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
    const void* ptr) {
  glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
}

void GLApiShell::glViewportFn(GLint x, GLint y, GLsizei width, GLsizei height) {
  glViewport(x, y, width, height);
}

void GLApiShell::glGenFencesNVFn(GLsizei n, GLuint* fences) {
  NOTIMPLEMENTED();
}

void GLApiShell::glDeleteFencesNVFn(GLsizei n, const GLuint* fences) {
  NOTIMPLEMENTED();
}

void GLApiShell::glSetFenceNVFn(GLuint fence, GLenum condition) {
  NOTIMPLEMENTED();
}

GLboolean GLApiShell::glTestFenceNVFn(GLuint fence) {
  NOTIMPLEMENTED();
  return false;
}

void GLApiShell::glFinishFenceNVFn(GLuint fence) {
  NOTIMPLEMENTED();
}

GLboolean GLApiShell::glIsFenceNVFn(GLuint fence) {
  NOTIMPLEMENTED();
  return false;
}

void GLApiShell::glGetFenceivNVFn(GLuint fence, GLenum pname, GLint* params) {
  NOTIMPLEMENTED();
}

GLsync GLApiShell::glFenceSyncFn(GLenum condition, GLbitfield flags) {
  return glFenceSync(condition, flags);
}

void GLApiShell::glDeleteSyncFn(GLsync sync) {
  glDeleteSync(sync);
}

void GLApiShell::glGetSyncivFn(
    GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length,
    GLint* values) {
  glGetSynciv(sync, pname, bufSize, length, values);
}

void GLApiShell::glDrawArraysInstancedANGLEFn(
    GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
  NOTIMPLEMENTED();
}

void GLApiShell::glDrawElementsInstancedANGLEFn(
    GLenum mode, GLsizei count, GLenum type, const void* indices,
    GLsizei primcount) {
  NOTIMPLEMENTED();
}

void GLApiShell::glVertexAttribDivisorANGLEFn(GLuint index, GLuint divisor) {
  NOTIMPLEMENTED();
}

void GLApiShell::glGenVertexArraysOESFn(GLsizei n, GLuint* arrays) {
  NOTIMPLEMENTED();
}

void GLApiShell::glDeleteVertexArraysOESFn(GLsizei n, const GLuint* arrays) {
  NOTIMPLEMENTED();
}

void GLApiShell::glBindVertexArrayOESFn(GLuint array) {
  NOTIMPLEMENTED();
}

GLboolean GLApiShell::glIsVertexArrayOESFn(GLuint array) {
  NOTIMPLEMENTED();
  return false;
}

void GLApiShell::glDiscardFramebufferEXTFn(
    GLenum target, GLsizei numAttachments, const GLenum* attachments) {
  NOTIMPLEMENTED();
}


}  // namespace gfx
