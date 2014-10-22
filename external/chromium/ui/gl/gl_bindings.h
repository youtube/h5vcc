// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_BINDINGS_H_
#define UI_GL_GL_BINDINGS_H_

// Includes the platform independent and platform dependent GL headers.
// Only include this in cc files. It pulls in system headers, including
// the X11 headers on linux, which define all kinds of macros that are
// liable to cause conflicts.

#include <GL/gl.h>
#include <GL/glext.h>
#if !defined(__LB_SHELL__) || defined(__LB_ANDROID__)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#include "base/logging.h"
#include "build/build_config.h"
#include "ui/gl/gl_export.h"

// The standard OpenGL native extension headers are also included.
#if defined(OS_WIN)
#include <GL/wglext.h>
#elif defined(OS_MACOSX)
#include <OpenGL/OpenGL.h>
#elif defined(__LB_LINUX__)
#include <X11/xpm.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#elif defined(__LB_SHELL__)
#elif defined(USE_X11)
#include <GL/glx.h>
#include <GL/glxext.h>

// Undefine some macros defined by X headers. This is why this file should only
// be included in .cc files.
#undef Bool
#undef None
#undef Status
#endif

#if defined(OS_WIN)
#define GL_BINDING_CALL WINAPI
#else
#define GL_BINDING_CALL
#endif

#define GL_SERVICE_LOG(args) DLOG(INFO) << args;
#if defined(NDEBUG)
  #define GL_SERVICE_LOG_CODE_BLOCK(code)
#else
  #define GL_SERVICE_LOG_CODE_BLOCK(code) code
#endif

// Forward declare OSMesa types.
typedef struct osmesa_context *OSMesaContext;
typedef void (*OSMESAproc)();

// Forward declare EGL types.

#if defined(__LB_SHELL__) && !defined(__LB_ANDROID__)
// We might be able to get rid of all of this after a refactor to using
// EGL on Linux, and just simply including egl.h and eglext.h, as above.
// This code is already removed in upstream Chromium and we are relying
// on these types to be properly defined within the Khronos EGL headers.
typedef uint64 EGLTimeKHR;
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef int EGLint;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLImageKHR;
typedef void *EGLSurface;
typedef void *EGLClientBuffer;
typedef void *EGLSyncKHR;
typedef void (*__eglMustCastToProperFunctionPointerType)(void);
typedef void* GLeglImageOES;

#if defined(__LB_LINUX__)
typedef Display *EGLNativeDisplayType;
typedef Pixmap   EGLNativePixmapType;
typedef Window   EGLNativeWindowType;
#elif defined(__LB_SHELL__)
typedef void*    EGLNativeDisplayType;
typedef void*    EGLNativePixmapType;
typedef void*    EGLNativeWindowType;
#endif
#endif

#include "gl_bindings_autogen_gl.h"
#include "gl_bindings_autogen_osmesa.h"

#if defined(OS_WIN)
#include "gl_bindings_autogen_egl.h"
#include "gl_bindings_autogen_wgl.h"
#elif defined(USE_X11)
#include "gl_bindings_autogen_egl.h"
#include "gl_bindings_autogen_glx.h"
#elif defined(OS_ANDROID)
#include "gl_bindings_autogen_egl.h"
#elif defined(__LB_LINUX__)
#include "gl_bindings_autogen_egl.h"
#include "gl_bindings_autogen_glx.h"
#elif defined(__LB_SHELL__)
#include "gl_bindings_autogen_egl.h"
#endif

namespace gfx {

#if defined(__LB_SHELL__) && !defined(__LB_ANDROID__)
// We don't compile the cpps that have these.
// But they are needed to link DLLs.
inline OSMESAApi::~OSMESAApi() {
  NOTREACHED();
}
inline EGLApi::~EGLApi() {
  NOTREACHED();
}
#endif

GL_EXPORT extern GLApi* g_current_gl_context;
GL_EXPORT extern OSMESAApi* g_current_osmesa_context;
GL_EXPORT extern DriverGL g_driver_gl;
GL_EXPORT extern DriverOSMESA g_driver_osmesa;

#if defined(OS_WIN)

GL_EXPORT extern EGLApi* g_current_egl_context;
GL_EXPORT extern WGLApi* g_current_wgl_context;
GL_EXPORT extern DriverEGL g_driver_egl;
GL_EXPORT extern DriverWGL g_driver_wgl;

#elif defined(USE_X11)

GL_EXPORT extern EGLApi* g_current_egl_context;
GL_EXPORT extern GLXApi* g_current_glx_context;
GL_EXPORT extern DriverEGL g_driver_egl;
GL_EXPORT extern DriverGLX g_driver_glx;

#elif defined(OS_ANDROID) || defined(__LB_ANDROID__)

GL_EXPORT extern EGLApi* g_current_egl_context;
GL_EXPORT extern DriverEGL g_driver_egl;

#endif

// Find an entry point to the mock GL implementation.
void* GL_BINDING_CALL GetMockGLProcAddress(const char* name);

}  // namespace gfx

#endif  // UI_GL_GL_BINDINGS_H_
