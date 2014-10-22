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

#ifndef SRC_LB_GRAPHICS_H_
#define SRC_LB_GRAPHICS_H_

#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"

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
  // Get the context used for skia GPU rendering.
  // When Skia GPU painting is disabled, sharing the compositor context
  // is fine.
  virtual LBWebGraphicsContext3D* GetSkiaContext() {
    return GetCompositorContext();
  }

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

  virtual void SetWebViewHost(LBWebViewHost* host) = 0;
  virtual LBWebViewHost* GetWebViewHost() const = 0;

  // Sets (or clears) the platform player into the graphics subsystem so it can
  // extract the video texture frame. Optionally implemented by platforms that
  // require this.
  virtual void SetPlayer(void *player) { }

#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  virtual void TakeScreenshot(const std::string& filename) = 0;
#endif

  virtual int GetDeviceWidth() const = 0;
  virtual int GetDeviceHeight() const = 0;

 private:
  static LBGraphics* instance_;

 protected:
  LBGraphics() {}
  virtual ~LBGraphics() {}
};

#endif  // SRC_LB_GRAPHICS_H_
