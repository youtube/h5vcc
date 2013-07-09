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

#ifndef _LB_GRAPHICS_LINUX_H_
#define _LB_GRAPHICS_LINUX_H_

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#ifndef GLX_GLXEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

#include <string>

#include "external/chromium/base/message_loop.h"
#include "external/chromium/base/synchronization/condition_variable.h"
#include "external/chromium/base/synchronization/lock.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
#include "lb_gl_text_printer.h"
#endif
#include "lb_graphics.h"

typedef struct _XDisplay Display;

class LBWebGraphicsContext3DLinux;

class LBGraphicsLinux : public LBGraphics {
 public:
  static LBGraphicsLinux* GetPtr() {
    return static_cast<LBGraphicsLinux*>(LBGraphics::GetPtr());
  }

  virtual LBWebGraphicsContext3D* GetCompositorContext() OVERRIDE;

  // Fair to call any time after SetupGraphics() returns, this method sends
  // sufficient commands to render one frame.
  virtual void UpdateAndDrawFrame() OVERRIDE;
  // This method will not return until a buffer flip occurs.
  virtual void BlockUntilFlip() OVERRIDE;

  // show or hide the spinner
  virtual void ShowSpinner() OVERRIDE;
  virtual void HideSpinner() OVERRIDE;

  // enable or disable screen-dimming
  virtual void EnableDimming() OVERRIDE;
  virtual void DisableDimming() OVERRIDE;

  // Returns the number of composite buffers (to be filled by a WebKit
  // composition) used by the rendering system.
  virtual int NumCompositeBuffers() const OVERRIDE;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // ==== texture tracking capabilities

  // return a report of WebKit textures
  // draw the WebKit texture onscreen on top of all else but the console.
  // 0 or another invalid texture handle will disable the watch.
  virtual const std::string PrintTextureSummary() OVERRIDE;

  virtual void WatchTexture(uint32_t handle) OVERRIDE;
  // save the texture to disk, or all WebKit textures if handle is 0xffffffff,
  // with the provided filename prefix.
  virtual void SaveTexture(uint32_t handle, const char* prefix) OVERRIDE;
#endif

  virtual void SetWebViewHost(LBWebViewHost* host) OVERRIDE;
  virtual LBWebViewHost* GetWebViewHost() const OVERRIDE;

  // If turning on WebKit compositing you will need to be calling from the
  // WebKit thread.
  virtual void SetWebKitCompositeEnable(bool enable) OVERRIDE;
  virtual bool WebKitCompositeEnabled() OVERRIDE;

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  virtual void TakeScreenshot(const std::string& filename) OVERRIDE;
#endif

  virtual int GetDeviceWidth() const OVERRIDE;
  virtual int GetDeviceHeight() const OVERRIDE;

  void InitializeGraphics();

  void initializeDevice();
  void initializeRenderSurfaces();
  void initializeRenderTextures();
  void initializeRenderGeometry();
  void initializeShaders();
  void initializeSpinner();
  void clearWebKitBuffers();
  void bindVertexData();

  // main draw loop
  void updateWebKitTexture();
  void setDrawState();
  void drawWebKitQuad();
  void updateSpinnerState();
  void drawSpinner();

  Display* GetXDisplay() const {
    return x_display_;
  }
  const Window& GetXWindow() const {
    return x_window_;
  }

  XVisualInfo* GetXVisualInfo() { return x_visual_info_; }

  const GLXContext& GetMasterContext() { return gl_context_; }

  static const int kGLMaxVertexAttribs = 16;
  static const int kGLMaxFragmentUniformVectors = 16;
  static const int kGLMaxTextureSize = 8192;
  static const int kGLMaxActiveTextureUnits = 16;

 private:
  LBGraphicsLinux();
  ~LBGraphicsLinux();

  scoped_ptr<LBWebGraphicsContext3DLinux> compositor_context_;

    // spinner
  enum SpinnerState {
    VISIBLE,
    INVISIBLE,
  };

  enum {
    kPositionAttrib = 0,
    kTexcoordAttrib = 1,
  };

  int device_width_;
  int device_height_;
  int device_color_pitch_;
  Display* x_display_;
  Window x_window_;
  MessageLoop* webkit_message_loop_;

  GLuint webkit_main_texture_;

  SpinnerState spinner_state_;
  GLuint spinner_texture_;
  float spinner_rotation_;
  float spinner_offset_[2];
  float spinner_scale_[2];
  double spinner_frame_time_;

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  bool screenshot_requested_;
  std::string screenshot_filename_;
  base::Lock screenshot_lock_;
  base::ConditionVariable screenshot_taken_cond_;
  void checkAndPossiblyTakeScreenshot();
  void saveTextureToDisk(void* pixels,
                         const char* file_path,
                         int width,
                         int height,
                         int pitch);
#endif


#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  void drawOnScreenDisplay();

  // Code that deals with rendering debug on-screen display information
  scoped_ptr<LBGLTextPrinter> gl_text_printer_;

  // All times are in seconds
  double last_webkit_frame_time_;
  double longest_webkit_frame_time_;
  double average_webkit_start_time_;
  int webkit_frame_counter_;
  int frame_counter_;
  double webkit_fps_;
  double worst_webkit_fps_;
  int num_samples_for_fps_;
  std::string frame_rate_string_;
#endif

  // Vertex buffer for drawing full-screen quads.
  GLuint full_screen_vbo_;

  // Our own custom shaders.
  GLuint program_webquad_;
  GLuint program_spinner_;

  GLint vert_spinner_scale_param_;
  GLint vert_spinner_offset_param_;
  GLint vert_spinner_rotation_param_;

  LBWebViewHost* web_view_host_;
  bool webkit_composite_enabled_;

  GLXContext gl_context_;

  XVisualInfo* x_visual_info_;

  pthread_t flip_thread_;
  bool flip_thread_assigned_;

  friend class LBGraphics;
};

#endif  // _LB_GRAPHICS_LINUX_H_
