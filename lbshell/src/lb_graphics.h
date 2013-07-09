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

#ifndef _LB_GRAPHICS_H_
#define _LB_GRAPHICS_H_

#include <string>

#include "external/chromium/base/callback.h"
#include "external/chromium/base/memory/ref_counted.h"
#include "external/chromium/base/memory/scoped_ptr.h"
#include "external/chromium/media/base/shell_filter_graph_log.h"

class LBWebGraphicsContext3D;
class LBWebViewHost;

class LBGraphics {
 public:
  // called exactly once per program instance.
  static void SetupGraphics();
  static void TeardownGraphics();
  // singleton instance access
  static LBGraphics* GetPtr() {
    return instance_;
  }

  virtual LBWebGraphicsContext3D* GetCompositorContext() = 0;

  // Fair to call any time after SetupGraphics() returns, this method sends
  // sufficient commands to render one frame.
  virtual void UpdateAndDrawFrame() = 0;
  // This method will not return until a buffer flip occurs.
  virtual void BlockUntilFlip() = 0;

  // show or hide the spinner
  virtual void ShowSpinner() = 0;
  virtual void HideSpinner() = 0;

  // enable or disable screen-dimming
  virtual void EnableDimming() = 0;
  virtual void DisableDimming() = 0;

  // Returns the number of composite buffers (to be filled by a WebKit
  // composition) used by the rendering system.
  virtual int NumCompositeBuffers() const = 0;


#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // ==== texture tracking capabilities

  // return a report of WebKit textures
  // draw the WebKit texture onscreen on top of all else but the console.
  // 0 or another invalid texture handle will disable the watch.
  virtual const std::string PrintTextureSummary() = 0;

  virtual void WatchTexture(uint32_t handle) = 0;
  // save the texture to disk, or all WebKit textures if handle is 0xffffffff,
  // with the provided filename prefix.
  virtual void SaveTexture(uint32_t handle, const char* prefix) = 0;

  // Console/OSD toggling functions
  virtual void HideConsole() { console_visible_ = false; }
  virtual void ShowConsole() { console_visible_ = true; }
  virtual void ToggleConsole() { console_visible_ = !console_visible_; }
  virtual bool ConsoleVisible() const { return console_visible_; }

  virtual void HideOSD() { osd_visible_ = false; }
  virtual void ShowOSD() { osd_visible_ = true; }
  virtual void ToggleOSD() { osd_visible_ = !osd_visible_; }
  virtual bool OSDVisible() const { return osd_visible_; }
  virtual void SetOSDURL(const std::string& url) { osd_url_ = url; }
#endif

  virtual void SetWebViewHost(LBWebViewHost* host) = 0;
  virtual LBWebViewHost* GetWebViewHost() const = 0;

  // If turning on WebKit compositing you will need to be calling from the
  // WebKit thread.
  virtual void SetWebKitCompositeEnable(bool enable) = 0;
  virtual bool WebKitCompositeEnabled() = 0;

#if !defined(__LB_SHELL__FOR_RELEASE__)
  void SetFilterGraphLog(
      scoped_refptr<media::ShellFilterGraphLog> filter_graph_log) {
    filter_graph_log_ = filter_graph_log;
  }
#endif

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  virtual void TakeScreenshot(const std::string& filename) = 0;
#endif

  virtual int GetDeviceWidth() const = 0;
  virtual int GetDeviceHeight() const = 0;

 private:
  static LBGraphics* instance_;

 protected:
  LBGraphics()
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    : console_visible_(false)
    , osd_visible_(true)
#endif
    {}

  virtual ~LBGraphics() {}

#if !defined(__LB_SHELL__FOR_RELEASE__)
  scoped_refptr<media::ShellFilterGraphLog> filter_graph_log_;
#endif

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  bool console_visible_;
  bool osd_visible_;
  std::string osd_url_;
#endif
};

#endif  // _LB_GRAPHICS_H_
