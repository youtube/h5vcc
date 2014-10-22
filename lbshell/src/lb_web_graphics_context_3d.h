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

#ifndef SRC_LB_WEB_GRAPHICS_CONTEXT_3D_H_
#define SRC_LB_WEB_GRAPHICS_CONTEXT_3D_H_

// Defining this as 1 enables detailed instrumentation about what commands are
// inserted into the WebKit graphics command buffer.
#define LB_WEBKIT_CONTEXT_TRACE 0
// Defining this as 1 enables detailed instrumentation about what GL commands
// the WebKit/Chromium graphics pipeline are calling
#define LB_WEBKIT_GL_TRACE 0
// Defining this as 1 will save every frame WebKit generates to a PNG in the
// content folder
#define LB_WEBKIT_TEXTURE_SAVE 0

#include <list>
#include <map>
#include <string>
#include <deque>

#include "external/chromium/base/message_loop.h"
#include "external/chromium/base/synchronization/lock.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebGraphicsContext3D.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebString.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebView.h"

// Follow suite with how GraphicsContext3D.h deals with the NO_ERROR name
// conflict.
#ifdef NO_ERROR
#undef NO_ERROR
#endif

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

using WebKit::WebGLId;

using WebKit::WebString;
using WebKit::WebView;

using WebKit::WebGraphicsContext3D;

class LBWebGraphicsContext3D : public WebKit::WebGraphicsContext3D {
 public:
#if defined(__LB_WIIU__) || defined(__LB_PS3__) || defined(__LB_PS4__)
  // Support for loading shaders by binary instead of source code
  virtual void shaderBinary(WGC3Dsizei n,
                            const WGC3Duint* shaders,
                            WGC3Denum binaryFormat,
                            const void* binary,
                            WGC3Dsizei length) = 0;
#endif
};


// __LB_PS3__FIX_ME__
// This enum was copied from
// external/chromium/third_party/WebKit/Source/WebCore/platform/graphics/
// GraphicsContext3D.h, which we'd like to include but that brings in a
// whole rat's nest of dependencies. Figure out a less rebase-brittle way to
// do this.
namespace GraphicsContext3D {
  enum {
    DEPTH_BUFFER_BIT = 0x00000100,
    STENCIL_BUFFER_BIT = 0x00000400,
    COLOR_BUFFER_BIT = 0x00004000,
    POINTS = 0x0000,
    LINES = 0x0001,
    LINE_LOOP = 0x0002,
    LINE_STRIP = 0x0003,
    TRIANGLES = 0x0004,
    TRIANGLE_STRIP = 0x0005,
    TRIANGLE_FAN = 0x0006,
    ZERO = 0,
    ONE = 1,
    SRC_COLOR = 0x0300,
    ONE_MINUS_SRC_COLOR = 0x0301,
    SRC_ALPHA = 0x0302,
    ONE_MINUS_SRC_ALPHA = 0x0303,
    DST_ALPHA = 0x0304,
    ONE_MINUS_DST_ALPHA = 0x0305,
    DST_COLOR = 0x0306,
    ONE_MINUS_DST_COLOR = 0x0307,
    SRC_ALPHA_SATURATE = 0x0308,
    FUNC_ADD = 0x8006,
    BLEND_EQUATION = 0x8009,
    BLEND_EQUATION_RGB = 0x8009,
    BLEND_EQUATION_ALPHA = 0x883D,
    FUNC_SUBTRACT = 0x800A,
    FUNC_REVERSE_SUBTRACT = 0x800B,
    BLEND_DST_RGB = 0x80C8,
    BLEND_SRC_RGB = 0x80C9,
    BLEND_DST_ALPHA = 0x80CA,
    BLEND_SRC_ALPHA = 0x80CB,
    CONSTANT_COLOR = 0x8001,
    ONE_MINUS_CONSTANT_COLOR = 0x8002,
    CONSTANT_ALPHA = 0x8003,
    ONE_MINUS_CONSTANT_ALPHA = 0x8004,
    BLEND_COLOR = 0x8005,
    ARRAY_BUFFER = 0x8892,
    ELEMENT_ARRAY_BUFFER = 0x8893,
    ARRAY_BUFFER_BINDING = 0x8894,
    ELEMENT_ARRAY_BUFFER_BINDING = 0x8895,
    STREAM_DRAW = 0x88E0,
    STATIC_DRAW = 0x88E4,
    DYNAMIC_DRAW = 0x88E8,
    BUFFER_SIZE = 0x8764,
    BUFFER_USAGE = 0x8765,
    CURRENT_VERTEX_ATTRIB = 0x8626,
    FRONT = 0x0404,
    BACK = 0x0405,
    FRONT_AND_BACK = 0x0408,
    TEXTURE_2D = 0x0DE1,
    CULL_FACE = 0x0B44,
    BLEND = 0x0BE2,
    DITHER = 0x0BD0,
    STENCIL_TEST = 0x0B90,
    DEPTH_TEST = 0x0B71,
    SCISSOR_TEST = 0x0C11,
    POLYGON_OFFSET_FILL = 0x8037,
    SAMPLE_ALPHA_TO_COVERAGE = 0x809E,
    SAMPLE_COVERAGE = 0x80A0,
    NO_ERROR = 0,
    INVALID_ENUM = 0x0500,
    INVALID_VALUE = 0x0501,
    INVALID_OPERATION = 0x0502,
    OUT_OF_MEMORY = 0x0505,
    CW = 0x0900,
    CCW = 0x0901,
    LINE_WIDTH = 0x0B21,
    ALIASED_POINT_SIZE_RANGE = 0x846D,
    ALIASED_LINE_WIDTH_RANGE = 0x846E,
    CULL_FACE_MODE = 0x0B45,
    FRONT_FACE = 0x0B46,
    DEPTH_RANGE = 0x0B70,
    DEPTH_WRITEMASK = 0x0B72,
    DEPTH_CLEAR_VALUE = 0x0B73,
    DEPTH_FUNC = 0x0B74,
    STENCIL_CLEAR_VALUE = 0x0B91,
    STENCIL_FUNC = 0x0B92,
    STENCIL_FAIL = 0x0B94,
    STENCIL_PASS_DEPTH_FAIL = 0x0B95,
    STENCIL_PASS_DEPTH_PASS = 0x0B96,
    STENCIL_REF = 0x0B97,
    STENCIL_VALUE_MASK = 0x0B93,
    STENCIL_WRITEMASK = 0x0B98,
    STENCIL_BACK_FUNC = 0x8800,
    STENCIL_BACK_FAIL = 0x8801,
    STENCIL_BACK_PASS_DEPTH_FAIL = 0x8802,
    STENCIL_BACK_PASS_DEPTH_PASS = 0x8803,
    STENCIL_BACK_REF = 0x8CA3,
    STENCIL_BACK_VALUE_MASK = 0x8CA4,
    STENCIL_BACK_WRITEMASK = 0x8CA5,
    VIEWPORT = 0x0BA2,
    SCISSOR_BOX = 0x0C10,
    COLOR_CLEAR_VALUE = 0x0C22,
    COLOR_WRITEMASK = 0x0C23,
    UNPACK_ALIGNMENT = 0x0CF5,
    PACK_ALIGNMENT = 0x0D05,
    MAX_TEXTURE_SIZE = 0x0D33,
    MAX_VIEWPORT_DIMS = 0x0D3A,
    SUBPIXEL_BITS = 0x0D50,
    RED_BITS = 0x0D52,
    GREEN_BITS = 0x0D53,
    BLUE_BITS = 0x0D54,
    ALPHA_BITS = 0x0D55,
    DEPTH_BITS = 0x0D56,
    STENCIL_BITS = 0x0D57,
    POLYGON_OFFSET_UNITS = 0x2A00,
    POLYGON_OFFSET_FACTOR = 0x8038,
    TEXTURE_BINDING_2D = 0x8069,
    SAMPLE_BUFFERS = 0x80A8,
    SAMPLES = 0x80A9,
    SAMPLE_COVERAGE_VALUE = 0x80AA,
    SAMPLE_COVERAGE_INVERT = 0x80AB,
    NUM_COMPRESSED_TEXTURE_FORMATS = 0x86A2,
    COMPRESSED_TEXTURE_FORMATS = 0x86A3,
    DONT_CARE = 0x1100,
    FASTEST = 0x1101,
    NICEST = 0x1102,
    GENERATE_MIPMAP_HINT = 0x8192,
    BYTE = 0x1400,
    UNSIGNED_BYTE = 0x1401,
    SHORT = 0x1402,
    UNSIGNED_SHORT = 0x1403,
    INT = 0x1404,
    UNSIGNED_INT = 0x1405,
    FLOAT = 0x1406,
    FIXED = 0x140C,
    DEPTH_COMPONENT = 0x1902,
    ALPHA = 0x1906,
    RGB = 0x1907,
    RGBA = 0x1908,
    BGRA = 0x80E1,
    LUMINANCE = 0x1909,
    LUMINANCE_ALPHA = 0x190A,
    UNSIGNED_SHORT_4_4_4_4 = 0x8033,
    UNSIGNED_SHORT_5_5_5_1 = 0x8034,
    UNSIGNED_SHORT_5_6_5 = 0x8363,
    FRAGMENT_SHADER = 0x8B30,
    VERTEX_SHADER = 0x8B31,
    MAX_VERTEX_ATTRIBS = 0x8869,
    MAX_VERTEX_UNIFORM_VECTORS = 0x8DFB,
    MAX_VARYING_VECTORS = 0x8DFC,
    MAX_COMBINED_TEXTURE_IMAGE_UNITS = 0x8B4D,
    MAX_VERTEX_TEXTURE_IMAGE_UNITS = 0x8B4C,
    MAX_TEXTURE_IMAGE_UNITS = 0x8872,
    MAX_FRAGMENT_UNIFORM_VECTORS = 0x8DFD,
    SHADER_TYPE = 0x8B4F,
    DELETE_STATUS = 0x8B80,
    LINK_STATUS = 0x8B82,
    VALIDATE_STATUS = 0x8B83,
    ATTACHED_SHADERS = 0x8B85,
    ACTIVE_UNIFORMS = 0x8B86,
    ACTIVE_UNIFORM_MAX_LENGTH = 0x8B87,
    ACTIVE_ATTRIBUTES = 0x8B89,
    ACTIVE_ATTRIBUTE_MAX_LENGTH = 0x8B8A,
    SHADING_LANGUAGE_VERSION = 0x8B8C,
    CURRENT_PROGRAM = 0x8B8D,
    NEVER = 0x0200,
    LESS = 0x0201,
    EQUAL = 0x0202,
    LEQUAL = 0x0203,
    GREATER = 0x0204,
    NOTEQUAL = 0x0205,
    GEQUAL = 0x0206,
    ALWAYS = 0x0207,
    KEEP = 0x1E00,
    REPLACE = 0x1E01,
    INCR = 0x1E02,
    DECR = 0x1E03,
    INVERT = 0x150A,
    INCR_WRAP = 0x8507,
    DECR_WRAP = 0x8508,
    VENDOR = 0x1F00,
    RENDERER = 0x1F01,
    VERSION = 0x1F02,
    EXTENSIONS = 0x1F03,
    NEAREST = 0x2600,
    LINEAR = 0x2601,
    NEAREST_MIPMAP_NEAREST = 0x2700,
    LINEAR_MIPMAP_NEAREST = 0x2701,
    NEAREST_MIPMAP_LINEAR = 0x2702,
    LINEAR_MIPMAP_LINEAR = 0x2703,
    TEXTURE_MAG_FILTER = 0x2800,
    TEXTURE_MIN_FILTER = 0x2801,
    TEXTURE_WRAP_S = 0x2802,
    TEXTURE_WRAP_T = 0x2803,
    TEXTURE = 0x1702,
    TEXTURE_CUBE_MAP = 0x8513,
    TEXTURE_BINDING_CUBE_MAP = 0x8514,
    TEXTURE_CUBE_MAP_POSITIVE_X = 0x8515,
    TEXTURE_CUBE_MAP_NEGATIVE_X = 0x8516,
    TEXTURE_CUBE_MAP_POSITIVE_Y = 0x8517,
    TEXTURE_CUBE_MAP_NEGATIVE_Y = 0x8518,
    TEXTURE_CUBE_MAP_POSITIVE_Z = 0x8519,
    TEXTURE_CUBE_MAP_NEGATIVE_Z = 0x851A,
    MAX_CUBE_MAP_TEXTURE_SIZE = 0x851C,
    TEXTURE0 = 0x84C0,
    TEXTURE1 = 0x84C1,
    TEXTURE2 = 0x84C2,
    TEXTURE3 = 0x84C3,
    TEXTURE4 = 0x84C4,
    TEXTURE5 = 0x84C5,
    TEXTURE6 = 0x84C6,
    TEXTURE7 = 0x84C7,
    TEXTURE8 = 0x84C8,
    TEXTURE9 = 0x84C9,
    TEXTURE10 = 0x84CA,
    TEXTURE11 = 0x84CB,
    TEXTURE12 = 0x84CC,
    TEXTURE13 = 0x84CD,
    TEXTURE14 = 0x84CE,
    TEXTURE15 = 0x84CF,
    TEXTURE16 = 0x84D0,
    TEXTURE17 = 0x84D1,
    TEXTURE18 = 0x84D2,
    TEXTURE19 = 0x84D3,
    TEXTURE20 = 0x84D4,
    TEXTURE21 = 0x84D5,
    TEXTURE22 = 0x84D6,
    TEXTURE23 = 0x84D7,
    TEXTURE24 = 0x84D8,
    TEXTURE25 = 0x84D9,
    TEXTURE26 = 0x84DA,
    TEXTURE27 = 0x84DB,
    TEXTURE28 = 0x84DC,
    TEXTURE29 = 0x84DD,
    TEXTURE30 = 0x84DE,
    TEXTURE31 = 0x84DF,
    ACTIVE_TEXTURE = 0x84E0,
    REPEAT = 0x2901,
    CLAMP_TO_EDGE = 0x812F,
    MIRRORED_REPEAT = 0x8370,
    FLOAT_VEC2 = 0x8B50,
    FLOAT_VEC3 = 0x8B51,
    FLOAT_VEC4 = 0x8B52,
    INT_VEC2 = 0x8B53,
    INT_VEC3 = 0x8B54,
    INT_VEC4 = 0x8B55,
    BOOL = 0x8B56,
    BOOL_VEC2 = 0x8B57,
    BOOL_VEC3 = 0x8B58,
    BOOL_VEC4 = 0x8B59,
    FLOAT_MAT2 = 0x8B5A,
    FLOAT_MAT3 = 0x8B5B,
    FLOAT_MAT4 = 0x8B5C,
    SAMPLER_2D = 0x8B5E,
    SAMPLER_CUBE = 0x8B60,
    VERTEX_ATTRIB_ARRAY_ENABLED = 0x8622,
    VERTEX_ATTRIB_ARRAY_SIZE = 0x8623,
    VERTEX_ATTRIB_ARRAY_STRIDE = 0x8624,
    VERTEX_ATTRIB_ARRAY_TYPE = 0x8625,
    VERTEX_ATTRIB_ARRAY_NORMALIZED = 0x886A,
    VERTEX_ATTRIB_ARRAY_POINTER = 0x8645,
    VERTEX_ATTRIB_ARRAY_BUFFER_BINDING = 0x889F,
    COMPILE_STATUS = 0x8B81,
    INFO_LOG_LENGTH = 0x8B84,
    SHADER_SOURCE_LENGTH = 0x8B88,
    SHADER_COMPILER = 0x8DFA,
    SHADER_BINARY_FORMATS = 0x8DF8,
    NUM_SHADER_BINARY_FORMATS = 0x8DF9,
    LOW_FLOAT = 0x8DF0,
    MEDIUM_FLOAT = 0x8DF1,
    HIGH_FLOAT = 0x8DF2,
    LOW_INT = 0x8DF3,
    MEDIUM_INT = 0x8DF4,
    HIGH_INT = 0x8DF5,
    FRAMEBUFFER = 0x8D40,
    RENDERBUFFER = 0x8D41,
    RGBA4 = 0x8056,
    RGB5_A1 = 0x8057,
    RGB565 = 0x8D62,
    DEPTH_COMPONENT16 = 0x81A5,
    STENCIL_INDEX = 0x1901,
    STENCIL_INDEX8 = 0x8D48,
    DEPTH_STENCIL = 0x84F9,
    UNSIGNED_INT_24_8 = 0x84FA,
    DEPTH24_STENCIL8 = 0x88F0,
    RENDERBUFFER_WIDTH = 0x8D42,
    RENDERBUFFER_HEIGHT = 0x8D43,
    RENDERBUFFER_INTERNAL_FORMAT = 0x8D44,
    RENDERBUFFER_RED_SIZE = 0x8D50,
    RENDERBUFFER_GREEN_SIZE = 0x8D51,
    RENDERBUFFER_BLUE_SIZE = 0x8D52,
    RENDERBUFFER_ALPHA_SIZE = 0x8D53,
    RENDERBUFFER_DEPTH_SIZE = 0x8D54,
    RENDERBUFFER_STENCIL_SIZE = 0x8D55,
    FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE = 0x8CD0,
    FRAMEBUFFER_ATTACHMENT_OBJECT_NAME = 0x8CD1,
    FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL = 0x8CD2,
    FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE = 0x8CD3,
    COLOR_ATTACHMENT0 = 0x8CE0,
    DEPTH_ATTACHMENT = 0x8D00,
    STENCIL_ATTACHMENT = 0x8D20,
    DEPTH_STENCIL_ATTACHMENT = 0x821A,
    NONE = 0,
    FRAMEBUFFER_COMPLETE = 0x8CD5,
    FRAMEBUFFER_INCOMPLETE_ATTACHMENT = 0x8CD6,
    FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7,
    FRAMEBUFFER_INCOMPLETE_DIMENSIONS = 0x8CD9,
    FRAMEBUFFER_UNSUPPORTED = 0x8CDD,
    FRAMEBUFFER_BINDING = 0x8CA6,
    RENDERBUFFER_BINDING = 0x8CA7,
    MAX_RENDERBUFFER_SIZE = 0x84E8,
    INVALID_FRAMEBUFFER_OPERATION = 0x0506,

    // WebGL-specific enums
    UNPACK_FLIP_Y_WEBGL = 0x9240,
    UNPACK_PREMULTIPLY_ALPHA_WEBGL = 0x9241,
    CONTEXT_LOST_WEBGL = 0x9242,
    UNPACK_COLORSPACE_CONVERSION_WEBGL = 0x9243,
    BROWSER_DEFAULT_WEBGL = 0x9244
  };
}  // namespace GraphicsContext3D

class LBWebViewHost;

#if !defined(__LB_DISABLE_SKIA_GPU__)

#define GRAPHICS_CONTEXT_3D_SKIA_METHOD_OVERRIDES \
virtual GrGLInterface* onCreateGrGLInterface() OVERRIDE; \

#else

#define GRAPHICS_CONTEXT_3D_SKIA_METHOD_OVERRIDES

#endif


#define GRAPHICS_CONTEXT_3D_METHOD_OVERRIDES \
virtual bool makeContextCurrent() OVERRIDE; \
  \
virtual int width() OVERRIDE; \
virtual int height() OVERRIDE; \
  \
virtual void reshape(int width, int height) OVERRIDE; \
  \
virtual bool isGLES2Compliant() OVERRIDE; \
  \
virtual bool setParentContext(WebGraphicsContext3D* parentContext) OVERRIDE; \
  \
virtual bool readBackFramebuffer(unsigned char* pixels, \
                                 size_t bufferSize, \
                                 WebGLId framebuffer, \
                                 int width, \
                                 int height) OVERRIDE; \
  \
virtual WebGLId getPlatformTextureId() OVERRIDE; \
  \
virtual void prepareTexture() OVERRIDE; \
  \
virtual void synthesizeGLError(WGC3Denum) OVERRIDE; \
  \
virtual bool isContextLost() OVERRIDE; \
  \
virtual void* mapBufferSubDataCHROMIUM(WGC3Denum target, \
                                       WGC3Dintptr offset, \
                                       WGC3Dsizeiptr size, \
                                       WGC3Denum access) OVERRIDE; \
virtual void unmapBufferSubDataCHROMIUM(const void*) OVERRIDE; \
virtual void* mapTexSubImage2DCHROMIUM(WGC3Denum target, \
                                       WGC3Dint level, \
                                       WGC3Dint xoffset, \
                                       WGC3Dint yoffset, \
                                       WGC3Dsizei width, \
                                       WGC3Dsizei height, \
                                       WGC3Denum format, \
                                       WGC3Denum type, \
                                       WGC3Denum access) OVERRIDE; \
virtual void unmapTexSubImage2DCHROMIUM(const void*) OVERRIDE; \
  \
virtual WebString getRequestableExtensionsCHROMIUM() OVERRIDE; \
virtual void requestExtensionCHROMIUM(const char*) OVERRIDE; \
  \
virtual void blitFramebufferCHROMIUM(WGC3Dint srcX0, \
                                     WGC3Dint srcY0, \
                                     WGC3Dint srcX1, \
                                     WGC3Dint srcY1, \
                                     WGC3Dint dstX0, \
                                     WGC3Dint dstY0, \
                                     WGC3Dint dstX1, \
                                     WGC3Dint dstY1, \
                                     WGC3Dbitfield mask, \
                                     WGC3Denum filter) OVERRIDE; \
virtual void renderbufferStorageMultisampleCHROMIUM( \
    WGC3Denum target, \
    WGC3Dsizei samples, \
    WGC3Denum internalformat, \
    WGC3Dsizei width, \
    WGC3Dsizei height) OVERRIDE; \
  \
virtual void setSwapBuffersCompleteCallbackCHROMIUM( \
  WebGraphicsSwapBuffersCompleteCallbackCHROMIUM* callback) OVERRIDE; \
  \
virtual void rateLimitOffscreenContextCHROMIUM() OVERRIDE; \
  \
virtual void activeTexture(WGC3Denum texture); \
virtual void attachShader(WebGLId program, WebGLId shader) OVERRIDE; \
virtual void bindAttribLocation(WebGLId program, \
                                WGC3Duint index, \
                                const WGC3Dchar* name) OVERRIDE; \
virtual void bindBuffer(WGC3Denum target, \
                        WebGLId buffer) OVERRIDE; \
virtual void bindFramebuffer(WGC3Denum target, \
                             WebGLId framebuffer) OVERRIDE; \
virtual void bindRenderbuffer(WGC3Denum target, \
                              WebGLId renderbuffer) OVERRIDE; \
virtual void bindTexture(WGC3Denum target, \
                         WebGLId texture) OVERRIDE; \
virtual void blendColor(WGC3Dclampf red, \
                        WGC3Dclampf green, \
                        WGC3Dclampf blue, \
                        WGC3Dclampf alpha) OVERRIDE; \
virtual void blendEquation(WGC3Denum mode) OVERRIDE; \
virtual void blendEquationSeparate(WGC3Denum modeRGB, \
                                   WGC3Denum modeAlpha) OVERRIDE; \
virtual void blendFunc(WGC3Denum sfactor, WGC3Denum dfactor) OVERRIDE; \
virtual void blendFuncSeparate(WGC3Denum srcRGB, \
                               WGC3Denum dstRGB, \
                               WGC3Denum srcAlpha, \
                               WGC3Denum dstAlpha) OVERRIDE; \
  \
virtual void bufferData(WGC3Denum target, \
                        WGC3Dsizeiptr size, \
                        const void* data, \
                        WGC3Denum usage) OVERRIDE; \
virtual void bufferSubData(WGC3Denum target, \
                           WGC3Dintptr offset, \
                           WGC3Dsizeiptr size, \
                           const void* data) OVERRIDE; \
  \
virtual WGC3Denum checkFramebufferStatus(WGC3Denum target) OVERRIDE; \
virtual void clear(WGC3Dbitfield mask) OVERRIDE; \
virtual void clearColor(WGC3Dclampf red, \
                        WGC3Dclampf green, \
                        WGC3Dclampf blue, \
                        WGC3Dclampf alpha) OVERRIDE; \
virtual void clearDepth(WGC3Dclampf depth) OVERRIDE; \
virtual void clearStencil(WGC3Dint s) OVERRIDE; \
virtual void colorMask(WGC3Dboolean red, \
                       WGC3Dboolean green, \
                       WGC3Dboolean blue, \
                       WGC3Dboolean alpha) OVERRIDE; \
virtual void compileShader(WebGLId shader) OVERRIDE; \
  \
virtual void compressedTexImage2D(WGC3Denum target, \
                                  WGC3Dint level, \
                                  WGC3Denum internalformat, \
                                  WGC3Dsizei width, \
                                  WGC3Dsizei height, \
                                  WGC3Dint border, \
                                  WGC3Dsizei imageSize, \
                                  const void* data) OVERRIDE; \
virtual void compressedTexSubImage2D(WGC3Denum target, \
                                     WGC3Dint level, \
                                     WGC3Dint xoffset, \
                                     WGC3Dint yoffset, \
                                     WGC3Dsizei width, \
                                     WGC3Dsizei height, \
                                     WGC3Denum format, \
                                     WGC3Dsizei imageSize, \
                                     const void* data) OVERRIDE; \
virtual void copyTexImage2D(WGC3Denum target, \
                            WGC3Dint level, \
                            WGC3Denum internalformat, \
                            WGC3Dint x, \
                            WGC3Dint y, \
                            WGC3Dsizei width, \
                            WGC3Dsizei height, \
                            WGC3Dint border) OVERRIDE; \
virtual void copyTexSubImage2D(WGC3Denum target, \
                               WGC3Dint level, \
                               WGC3Dint xoffset, \
                               WGC3Dint yoffset, \
                               WGC3Dint x, \
                               WGC3Dint y, \
                               WGC3Dsizei width, \
                               WGC3Dsizei height) OVERRIDE; \
virtual void cullFace(WGC3Denum mode) OVERRIDE; \
virtual void depthFunc(WGC3Denum func) OVERRIDE; \
virtual void depthMask(WGC3Dboolean flag) OVERRIDE; \
virtual void depthRange(WGC3Dclampf zNear, WGC3Dclampf zFar) OVERRIDE; \
virtual void detachShader(WebGLId program, WebGLId shader) OVERRIDE; \
virtual void disable(WGC3Denum cap) OVERRIDE; \
virtual void disableVertexAttribArray(WGC3Duint index) OVERRIDE; \
virtual void drawArrays(WGC3Denum mode, \
                        WGC3Dint first, \
                        WGC3Dsizei count) OVERRIDE; \
virtual void drawElements(WGC3Denum mode, \
                          WGC3Dsizei count, \
                          WGC3Denum type, \
                          WGC3Dintptr offset) OVERRIDE; \
  \
virtual void enable(WGC3Denum cap) OVERRIDE; \
virtual void enableVertexAttribArray(WGC3Duint index) OVERRIDE; \
virtual void finish() OVERRIDE; \
virtual void flush() OVERRIDE; \
virtual void framebufferRenderbuffer(WGC3Denum target, \
                                     WGC3Denum attachment, \
                                     WGC3Denum renderbuffertarget, \
                                     WebGLId renderbuffer) OVERRIDE; \
virtual void framebufferTexture2D(WGC3Denum target, \
                                  WGC3Denum attachment, \
                                  WGC3Denum textarget, \
                                  WebGLId texture, \
                                  WGC3Dint level) OVERRIDE; \
virtual void frontFace(WGC3Denum mode) OVERRIDE; \
virtual void generateMipmap(WGC3Denum target) OVERRIDE; \
  \
virtual bool getActiveAttrib(WebGLId program, \
                             WGC3Duint index, \
                             ActiveInfo&) OVERRIDE; \
virtual bool getActiveUniform(WebGLId program, \
                              WGC3Duint index, \
                              ActiveInfo&) OVERRIDE; \
virtual void getAttachedShaders(WebGLId program, \
                                WGC3Dsizei maxCount, \
                                WGC3Dsizei* count, \
                                WebGLId* shaders) OVERRIDE; \
virtual WGC3Dint getAttribLocation(WebGLId program, \
                                   const WGC3Dchar* name) OVERRIDE; \
virtual void getBooleanv(WGC3Denum pname, WGC3Dboolean* value) OVERRIDE; \
virtual void getBufferParameteriv(WGC3Denum target, \
                                  WGC3Denum pname, \
                                  WGC3Dint* value) OVERRIDE; \
virtual Attributes getContextAttributes() OVERRIDE; \
virtual WGC3Denum getError() OVERRIDE; \
virtual void getFloatv(WGC3Denum pname, WGC3Dfloat* value) OVERRIDE; \
virtual void getFramebufferAttachmentParameteriv(WGC3Denum target, \
                                                 WGC3Denum attachment, \
                                                 WGC3Denum pname, \
                                                 WGC3Dint* value) OVERRIDE; \
virtual void getIntegerv(WGC3Denum pname, \
                         WGC3Dint* value) OVERRIDE; \
virtual void getProgramiv(WebGLId program, \
                          WGC3Denum pname, \
                          WGC3Dint* value) OVERRIDE; \
virtual WebString getProgramInfoLog(WebGLId program) OVERRIDE; \
virtual void getRenderbufferParameteriv(WGC3Denum target, \
                                        WGC3Denum pname, \
                                        WGC3Dint* value) OVERRIDE; \
virtual void getShaderiv(WebGLId shader, \
                         WGC3Denum pname, \
                         WGC3Dint* value) OVERRIDE; \
virtual WebString getShaderInfoLog(WebGLId shader) OVERRIDE; \
virtual void getShaderPrecisionFormat(WGC3Denum shadertype, \
                                      WGC3Denum precisiontype, \
                                      WGC3Dint* range, \
                                      WGC3Dint* precision) OVERRIDE; \
  \
virtual WebString getShaderSource(WebGLId shader) OVERRIDE; \
virtual WebString getString(WGC3Denum name) OVERRIDE; \
virtual void getTexParameterfv(WGC3Denum target, \
                               WGC3Denum pname, \
                               WGC3Dfloat* value) OVERRIDE; \
virtual void getTexParameteriv(WGC3Denum target, \
                               WGC3Denum pname, \
                               WGC3Dint* value) OVERRIDE; \
virtual void getUniformfv(WebGLId program, \
                          WGC3Dint location, \
                          WGC3Dfloat* value) OVERRIDE; \
virtual void getUniformiv(WebGLId program, \
                          WGC3Dint location, \
                          WGC3Dint* value) OVERRIDE; \
virtual WGC3Dint getUniformLocation(WebGLId program, \
                                    const WGC3Dchar* name) OVERRIDE; \
virtual void getVertexAttribfv(WGC3Duint index, \
                               WGC3Denum pname, \
                               WGC3Dfloat* value) OVERRIDE; \
virtual void getVertexAttribiv(WGC3Duint index, \
                               WGC3Denum pname, \
                               WGC3Dint* value) OVERRIDE; \
virtual WGC3Dsizeiptr getVertexAttribOffset(WGC3Duint index, \
                                            WGC3Denum pname) OVERRIDE; \
  \
virtual void hint(WGC3Denum target, WGC3Denum mode) OVERRIDE; \
virtual WGC3Dboolean isBuffer(WebGLId buffer) OVERRIDE; \
virtual WGC3Dboolean isEnabled(WGC3Denum cap) OVERRIDE; \
virtual WGC3Dboolean isFramebuffer(WebGLId framebuffer) OVERRIDE; \
virtual WGC3Dboolean isProgram(WebGLId program) OVERRIDE; \
virtual WGC3Dboolean isRenderbuffer(WebGLId renderbuffer) OVERRIDE; \
virtual WGC3Dboolean isShader(WebGLId shader) OVERRIDE; \
virtual WGC3Dboolean isTexture(WebGLId texture) OVERRIDE; \
virtual void lineWidth(WGC3Dfloat) OVERRIDE; \
virtual void linkProgram(WebGLId program) OVERRIDE; \
virtual void pixelStorei(WGC3Denum pname, WGC3Dint param) OVERRIDE; \
virtual void polygonOffset(WGC3Dfloat factor, WGC3Dfloat units) OVERRIDE; \
  \
virtual void readPixels(WGC3Dint x, \
                        WGC3Dint y, \
                        WGC3Dsizei width, \
                        WGC3Dsizei height, \
                        WGC3Denum format, \
                        WGC3Denum type, \
                        void* pixels) OVERRIDE; \
  \
virtual void releaseShaderCompiler() OVERRIDE; \
  \
virtual void renderbufferStorage(WGC3Denum target, \
                                 WGC3Denum internalformat, \
                                 WGC3Dsizei width, \
                                 WGC3Dsizei height) OVERRIDE; \
virtual void sampleCoverage(WGC3Dclampf value, WGC3Dboolean invert) OVERRIDE; \
virtual void scissor(WGC3Dint x, \
                     WGC3Dint y, \
                     WGC3Dsizei width, \
                     WGC3Dsizei height) OVERRIDE; \
virtual void shaderSource(WebGLId shader, \
                          const WGC3Dchar* string) OVERRIDE; \
virtual void stencilFunc(WGC3Denum func, \
                         WGC3Dint ref, \
                         WGC3Duint mask) OVERRIDE; \
virtual void stencilFuncSeparate(WGC3Denum face, \
                                 WGC3Denum func, \
                                 WGC3Dint ref, \
                                 WGC3Duint mask) OVERRIDE; \
virtual void stencilMask(WGC3Duint mask) OVERRIDE; \
virtual void stencilMaskSeparate(WGC3Denum face, \
                                 WGC3Duint mask) OVERRIDE; \
virtual void stencilOp(WGC3Denum fail, \
                       WGC3Denum zfail, \
                       WGC3Denum zpass) OVERRIDE; \
virtual void stencilOpSeparate(WGC3Denum face, \
                               WGC3Denum fail, \
                               WGC3Denum zfail, \
                               WGC3Denum zpass) OVERRIDE; \
  \
virtual void texImage2D(WGC3Denum target, \
                        WGC3Dint level, \
                        WGC3Denum internalformat, \
                        WGC3Dsizei width, \
                        WGC3Dsizei height, \
                        WGC3Dint border, \
                        WGC3Denum format, \
                        WGC3Denum type, \
                        const void* pixels) OVERRIDE; \
  \
virtual void texParameterf(WGC3Denum target, \
                           WGC3Denum pname, \
                           WGC3Dfloat param) OVERRIDE; \
virtual void texParameteri(WGC3Denum target, \
                           WGC3Denum pname, \
                           WGC3Dint param) OVERRIDE; \
  \
virtual void texSubImage2D(WGC3Denum target, \
                           WGC3Dint level, \
                           WGC3Dint xoffset, \
                           WGC3Dint yoffset, \
                           WGC3Dsizei width, \
                           WGC3Dsizei height, \
                           WGC3Denum format, \
                           WGC3Denum type, \
                           const void* pixels) OVERRIDE; \
  \
virtual void uniform1f(WGC3Dint location, WGC3Dfloat x) OVERRIDE; \
virtual void uniform1fv(WGC3Dint location, \
                      WGC3Dsizei count, \
                      const WGC3Dfloat* v) OVERRIDE; \
virtual void uniform1i(WGC3Dint location, WGC3Dint x) OVERRIDE; \
virtual void uniform1iv(WGC3Dint location, \
                        WGC3Dsizei count, \
                        const WGC3Dint* v) OVERRIDE; \
virtual void uniform2f(WGC3Dint location, \
                       WGC3Dfloat x, \
                       WGC3Dfloat y) OVERRIDE; \
virtual void uniform2fv(WGC3Dint location, \
                        WGC3Dsizei count, \
                        const WGC3Dfloat* v) OVERRIDE; \
virtual void uniform2i(WGC3Dint location, \
                       WGC3Dint x, \
                       WGC3Dint y) OVERRIDE; \
virtual void uniform2iv(WGC3Dint location, \
                        WGC3Dsizei count, \
                        const WGC3Dint* v) OVERRIDE; \
virtual void uniform3f(WGC3Dint location, \
                       WGC3Dfloat x, \
                       WGC3Dfloat y, \
                       WGC3Dfloat z) OVERRIDE; \
virtual void uniform3fv(WGC3Dint location, \
                        WGC3Dsizei count, \
                        const WGC3Dfloat* v) OVERRIDE; \
virtual void uniform3i(WGC3Dint location, \
                       WGC3Dint x, \
                       WGC3Dint y, \
                       WGC3Dint z) OVERRIDE; \
virtual void uniform3iv(WGC3Dint location, \
                        WGC3Dsizei count, \
                        const WGC3Dint* v) OVERRIDE; \
virtual void uniform4f(WGC3Dint location, \
                       WGC3Dfloat x, \
                       WGC3Dfloat y, \
                       WGC3Dfloat z, \
                       WGC3Dfloat w) OVERRIDE; \
virtual void uniform4fv(WGC3Dint location, \
                        WGC3Dsizei count, \
                        const WGC3Dfloat* v) OVERRIDE; \
virtual void uniform4i(WGC3Dint location, \
                       WGC3Dint x, \
                       WGC3Dint y, \
                       WGC3Dint z, \
                       WGC3Dint w) OVERRIDE; \
virtual void uniform4iv(WGC3Dint location, \
                        WGC3Dsizei count, \
                        const WGC3Dint* v) OVERRIDE; \
virtual void uniformMatrix2fv(WGC3Dint location, \
                              WGC3Dsizei count, \
                              WGC3Dboolean transpose, \
                              const WGC3Dfloat* value) OVERRIDE; \
virtual void uniformMatrix3fv(WGC3Dint location, \
                              WGC3Dsizei count, \
                              WGC3Dboolean transpose, \
                              const WGC3Dfloat* value) OVERRIDE; \
virtual void uniformMatrix4fv(WGC3Dint location, \
                              WGC3Dsizei count, \
                              WGC3Dboolean transpose, \
                              const WGC3Dfloat* value) OVERRIDE; \
  \
virtual void useProgram(WebGLId program) OVERRIDE; \
virtual void validateProgram(WebGLId program) OVERRIDE; \
  \
virtual void vertexAttrib1f(WGC3Duint index, \
                            WGC3Dfloat x) OVERRIDE; \
virtual void vertexAttrib1fv(WGC3Duint index, \
                             const WGC3Dfloat* values) OVERRIDE; \
virtual void vertexAttrib2f(WGC3Duint index, \
                            WGC3Dfloat x, \
                            WGC3Dfloat y) OVERRIDE; \
virtual void vertexAttrib2fv(WGC3Duint index, \
                             const WGC3Dfloat* values) OVERRIDE; \
virtual void vertexAttrib3f(WGC3Duint index, \
                            WGC3Dfloat x, \
                            WGC3Dfloat y, \
                            WGC3Dfloat z) OVERRIDE; \
virtual void vertexAttrib3fv(WGC3Duint index, \
                             const WGC3Dfloat* values) OVERRIDE; \
virtual void vertexAttrib4f(WGC3Duint index, \
                            WGC3Dfloat x, \
                            WGC3Dfloat y, \
                            WGC3Dfloat z, \
                            WGC3Dfloat w) OVERRIDE; \
virtual void vertexAttrib4fv(WGC3Duint index, \
                             const WGC3Dfloat* values) OVERRIDE; \
virtual void vertexAttribPointer(WGC3Duint index, \
                                 WGC3Dint size, \
                                 WGC3Denum type, \
                                 WGC3Dboolean normalized, \
                                 WGC3Dsizei stride, \
                                 WGC3Dintptr offset) OVERRIDE; \
  \
virtual void viewport(WGC3Dint x, \
                      WGC3Dint y, \
                      WGC3Dsizei width, \
                      WGC3Dsizei height) OVERRIDE; \
  \
virtual WebGLId createBuffer() OVERRIDE; \
virtual WebGLId createFramebuffer() OVERRIDE; \
virtual WebGLId createProgram() OVERRIDE; \
virtual WebGLId createRenderbuffer() OVERRIDE; \
virtual WebGLId createShader(WGC3Denum) OVERRIDE; \
virtual WebGLId createTexture() OVERRIDE; \
  \
virtual void deleteBuffer(WebGLId) OVERRIDE; \
virtual void deleteFramebuffer(WebGLId) OVERRIDE; \
virtual void deleteProgram(WebGLId) OVERRIDE; \
virtual void deleteRenderbuffer(WebGLId) OVERRIDE; \
virtual void deleteShader(WebGLId) OVERRIDE; \
virtual void deleteTexture(WebGLId) OVERRIDE; \
  \
virtual void setVisibilityCHROMIUM(bool visible) OVERRIDE; \
virtual void postSubBufferCHROMIUM(int x, \
                                   int y, \
                                   int width, \
                                   int height) OVERRIDE; \
  \
  \
virtual void setMemoryAllocationChangedCallbackCHROMIUM( \
WebGraphicsMemoryAllocationChangedCallbackCHROMIUM* callback) OVERRIDE; \
virtual void sendManagedMemoryStatsCHROMIUM( \
const WebKit::WebGraphicsManagedMemoryStats* stats) OVERRIDE; \
virtual void discardFramebufferEXT(WGC3Denum target, \
                                   WGC3Dsizei numAttachments, \
                                   const WGC3Denum* attachments) OVERRIDE; \
virtual void discardBackbufferCHROMIUM() OVERRIDE; \
virtual void ensureBackbufferCHROMIUM() OVERRIDE; \
virtual unsigned insertSyncPoint() OVERRIDE; \
virtual void waitSyncPoint(unsigned syncPoint) OVERRIDE; \
virtual WebGLId createStreamTextureCHROMIUM(WebGLId texture) OVERRIDE; \
virtual void destroyStreamTextureCHROMIUM(WebGLId texture) OVERRIDE; \
virtual WebString getTranslatedShaderSourceANGLE(WebGLId shader) OVERRIDE; \
virtual void texImageIOSurface2DCHROMIUM(WGC3Denum target, \
                                         WGC3Dint width, \
                                         WGC3Dint height, \
                                         WGC3Duint ioSurfaceId, \
                                         WGC3Duint plane) OVERRIDE; \
virtual void texStorage2DEXT(WGC3Denum target, \
                             WGC3Dint levels, \
                             WGC3Duint internalformat, \
                             WGC3Dint width, \
                             WGC3Dint height) OVERRIDE; \
virtual WebGLId createQueryEXT() OVERRIDE; \
virtual void deleteQueryEXT(WebGLId query) OVERRIDE; \
virtual WGC3Dboolean isQueryEXT(WebGLId query) OVERRIDE; \
virtual void beginQueryEXT(WGC3Denum target, WebGLId query) OVERRIDE; \
virtual void endQueryEXT(WGC3Denum target) OVERRIDE; \
virtual void getQueryivEXT(WGC3Denum target, \
                           WGC3Denum pname, \
                           WGC3Dint* params) OVERRIDE; \
virtual void getQueryObjectuivEXT(WebGLId query, \
                                  WGC3Denum pname, \
                                  WGC3Duint* params) OVERRIDE; \
virtual void bindUniformLocationCHROMIUM(WebGLId program, \
                                         WGC3Dint location, \
                                         const WGC3Dchar* uniform) OVERRIDE; \
virtual void copyTextureCHROMIUM(WGC3Denum target, \
                                 WGC3Duint sourceId, \
                                 WGC3Duint destId, \
                                 WGC3Dint level, \
                                 WGC3Denum internalFormat) OVERRIDE; \
virtual void shallowFlushCHROMIUM() OVERRIDE; \
virtual void genMailboxCHROMIUM(WebKit::WGC3Dbyte* mailbox); \
virtual void produceTextureCHROMIUM( \
    WGC3Denum target, const WebKit::WGC3Dbyte* mailbox) OVERRIDE; \
virtual void consumeTextureCHROMIUM( \
    WGC3Denum target, const WebKit::WGC3Dbyte* mailbox) OVERRIDE; \
virtual void insertEventMarkerEXT(const WGC3Dchar* marker) OVERRIDE; \
virtual void pushGroupMarkerEXT(const WGC3Dchar* marker) OVERRIDE; \
virtual void popGroupMarkerEXT(void) OVERRIDE; \
virtual WebGLId createVertexArrayOES() OVERRIDE; \
virtual void deleteVertexArrayOES(WebGLId array) OVERRIDE; \
virtual WGC3Dboolean isVertexArrayOES(WebGLId array) OVERRIDE; \
virtual void bindVertexArrayOES(WebGLId array) OVERRIDE; \
virtual void bindTexImage2DCHROMIUM(WGC3Denum target, \
                                    WGC3Dint imageId) OVERRIDE; \
virtual void releaseTexImage2DCHROMIUM(WGC3Denum target, \
                                       WGC3Dint imageId) OVERRIDE; \
virtual void* mapBufferCHROMIUM(WGC3Denum target, \
                                WGC3Denum access) OVERRIDE; \
virtual WGC3Dboolean unmapBufferCHROMIUM(WGC3Denum target) OVERRIDE; \
virtual void asyncTexImage2DCHROMIUM(WGC3Denum target, \
                                     WGC3Dint level, \
                                     WGC3Denum internalformat, \
                                     WGC3Dsizei width, \
                                     WGC3Dsizei height, \
                                     WGC3Dint border, \
                                     WGC3Denum format, \
                                     WGC3Denum type, \
                                     const void* pixels) OVERRIDE; \
virtual void asyncTexSubImage2DCHROMIUM(WGC3Denum target, \
                                        WGC3Dint level, \
                                        WGC3Dint xoffset, \
                                        WGC3Dint yoffset, \
                                        WGC3Dsizei width, \
                                        WGC3Dsizei height, \
                                        WGC3Denum format, \
                                        WGC3Denum type, \
                                        const void* pixels) OVERRIDE; \
  \
virtual void setContextLostCallback(WebGraphicsContextLostCallback* callback) \
OVERRIDE; \
  \
virtual WGC3Denum getGraphicsResetStatusARB() \
OVERRIDE; \
  \
/* LB_Shell specific. */ \
  \
/* Chromium often wishes to load a subimage from a skia buffer into */ \
/* a part of a texture. It does this by first copying the subimage into */ \
/* its own buffer (unless the strides align) and then calling */ \
/* texSubImage2D.  The strides almost never align. Since we are doing a */ \
/* simple memcpy to upload to our textures anyway, let's skip the */ \
/* allocation of the buffer and just upload from the sub image directly. */ \
virtual void texSubImageSub(WGC3Denum format, \
                            int dstOffX,  /* destination offset */ \
                            int dstOffY, \
                            int dstWidth, /* destination subtexture width */ \
                            int dstHeight, \
                            int srcX,     /* source offset */ \
                            int srcY, \
                            int srcWidth, /* source image stride (pixels) */ \
                            const void* image) OVERRIDE; \
GRAPHICS_CONTEXT_3D_SKIA_METHOD_OVERRIDES

#endif  // SRC_LB_WEB_GRAPHICS_CONTEXT_3D_H_
