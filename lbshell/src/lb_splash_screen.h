/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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
#ifndef SRC_LB_SPLASH_SCREEN_H_
#define SRC_LB_SPLASH_SCREEN_H_

#include <string>

#include "base/atomicops.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "ui/gfx/point_f.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/rect_f.h"
#include "ui/gfx/size.h"
#include "ui/gfx/size_f.h"

#include "lb_gl_image_utils.h"

class LBGraphics;
class LBWebGraphicsContext3D;
class LBWebViewHost;
class MessageLoop;

namespace LB {

class SplashScreen {
 public:
  SplashScreen(LBWebViewHost* host, const base::TimeDelta& timeout);
  virtual ~SplashScreen() {}

  // This command is called every frame to draw the splash screen.
  virtual void Draw() = 0;

  // This command is sometimes called from the webkit thread to hide the splash
  // screen.
  virtual bool Hide() = 0;

 private:
  void HideTrampoline();

  DISALLOW_COPY_AND_ASSIGN(SplashScreen);
};

class ExtendedSplashScreen : public SplashScreen {
 public:
  ExtendedSplashScreen(
      LBWebViewHost* host,
      const base::TimeDelta& timeout,
      LBGraphics* graphics,
      LBWebGraphicsContext3D* context,
      const std::string& image_path,
      const gfx::RectF& position,
      const gfx::SizeF& viewport_size);

  virtual ~ExtendedSplashScreen();

  // This command is called every frame to draw the splash screen.
  virtual void Draw() OVERRIDE;

  // This command is sometimes called from the webkit thread to hide the splash
  // screen.
  virtual bool Hide() OVERRIDE;

  // Utility function to position the image in the center of the viewport.
  static gfx::Rect CenterPosition(const gfx::Size& image_size,
                                  const gfx::Size& viewport_size);

 private:
  volatile base::subtle::Atomic32 showing_splash_screen_;
  int splash_screen_texture_;
  LBWebGraphicsContext3D* context_;
  scoped_ptr<LB::QuadDrawer> drawer_;
  gfx::PointF image_position_;
  gfx::SizeF image_scale_;

  DISALLOW_COPY_AND_ASSIGN(ExtendedSplashScreen);
};

}  // namespace LB

#endif  // SRC_LB_SPLASH_SCREEN_H_
