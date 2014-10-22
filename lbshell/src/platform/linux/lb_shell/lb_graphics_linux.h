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

#ifndef SRC_PLATFORM_LINUX_LB_SHELL_LB_GRAPHICS_LINUX_H_
#define SRC_PLATFORM_LINUX_LB_SHELL_LB_GRAPHICS_LINUX_H_

#include <X11/xpm.h>

#include <string>

#include "external/chromium/base/message_loop.h"
#include "external/chromium/base/synchronization/condition_variable.h"
#include "external/chromium/base/synchronization/lock.h"
#include "external/chromium/base/threading/thread.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebGraphicsContext3D.h"

#include "lb_graphics.h"
#include "lb_web_graphics_context_3d_command_buffer.h"

namespace LB {
class OnScreenDisplay;
class QuadDrawer;
class SpinnerOverlay;
}

typedef struct __GLXFBConfigRec *GLXFBConfig;

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

  virtual void SetWebViewHost(LBWebViewHost* host) OVERRIDE;
  virtual LBWebViewHost* GetWebViewHost() const OVERRIDE;

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  virtual void TakeScreenshot(const std::string& filename) OVERRIDE;
#endif

  virtual int GetDeviceWidth() const OVERRIDE;
  virtual int GetDeviceHeight() const OVERRIDE;

  Display* GetXDisplay() const {
    return x_display_;
  }
  const Window& GetXWindow() const {
    return x_window_;
  }

  XVisualInfo* GetXVisualInfo() { return x_visual_info_; }
  const GLXFBConfig& GetFBConfig() const { return fb_config_; }

 private:
  LBGraphicsLinux();
  ~LBGraphicsLinux();
  void Initialize();

  void GraphicsThreadInitialize();
  void GraphicsThreadShutdown();

  scoped_ptr<LBWebGraphicsContext3DCommandBuffer>
      lb_screen_context_;

  scoped_ptr<LBWebGraphicsContext3DCommandBuffer>
      compositor_context_;

  scoped_ptr<LB::SpinnerOverlay> spinner_overlay_;
  scoped_ptr<LB::QuadDrawer> quad_drawer_;

  base::Thread graphics_thread_;
  MessageLoop* graphics_message_loop_;

  LBWebViewHost* web_view_host_;

  Display* x_display_;
  Window x_window_;
  GLXFBConfig fb_config_;
  XVisualInfo* x_visual_info_;

  int device_width_;
  int device_height_;
  int device_color_pitch_;

  friend class LBGraphics;
};

#endif  // SRC_PLATFORM_LINUX_LB_SHELL_LB_GRAPHICS_LINUX_H_
