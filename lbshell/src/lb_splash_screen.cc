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
#include "lb_splash_screen.h"

#include "base/command_line.h"
#include "base/message_loop.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"

#include "lb_web_graphics_context_3d_command_buffer.h"
#include "lb_web_view_host.h"
#include "lb_shell_switches.h"

static gfx::PointF ImageCenter(const gfx::RectF& position,
                               const gfx::SizeF& viewport) {
  // At first, get the center of the image.
  gfx::PointF pos(position.width() / 2.0f, position.height() / 2.0f);

  // Now move this by the offset
  pos.Offset(position.x(), position.y());

  // Then normalize by viewport size. Our scale is 2.0 (-1, 1).
  pos.Scale(2.0f / viewport.width(), 2.0f / viewport.height());

  // Finally, our origin was top-left.
  // Move the origin to the center of the viewport.
  pos.Offset(-1.0f, -1.0f);

  return pos;
}

static gfx::SizeF ImageScale(const gfx::RectF& position,
                             const gfx::SizeF& viewport) {
  return gfx::SizeF(position.width() / viewport.width(),
                    position.height() / viewport.height());
}

namespace LB {

SplashScreen::SplashScreen(LBWebViewHost* host,
    const base::TimeDelta& timeout) {
  DCHECK(host);
  DCHECK(host->webkit_message_loop());

  base::Closure cl = base::Bind(&SplashScreen::HideTrampoline,
      base::Unretained(this));

  DCHECK_GT(timeout.InSeconds(), 1) << "Must be set to some value.";
  DCHECK_LT(timeout.InSeconds(), 15) << "Must not be too high.";

  if (CommandLine::ForCurrentProcess()->HasSwitch(
      LB::switches::kHideSplashScreenAtInit)) {
#if defined(__LB_SHELL__FOR_RELEASE__)
    NOTREACHED() << "This should not be reachable in Gold.";
#endif

    host->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(&SplashScreen::HideTrampoline, base::Unretained(this)));
  } else {
    host->webkit_message_loop()->PostDelayedTask(FROM_HERE,
        base::Bind(&SplashScreen::HideTrampoline, base::Unretained(this)),
        timeout);
  }
}

void SplashScreen::HideTrampoline() {
  ignore_result(Hide());
}

ExtendedSplashScreen::ExtendedSplashScreen(
    LBWebViewHost* host,
    const base::TimeDelta& timeout,
    LBGraphics* graphics,
    LBWebGraphicsContext3D* context,
    const std::string& image_path,
    const gfx::RectF& position,
    const gfx::SizeF& viewport_size)
  : SplashScreen(host, timeout)
  , context_(context)
  , showing_splash_screen_(1)
  , splash_screen_texture_(-1)
  , image_position_(0.0f, 0.0f)
  , image_scale_(1.0f, 1.0f) {
  DCHECK(graphics);
  DCHECK(context);
  drawer_.reset(new LB::QuadDrawer(graphics, context));

  LB::Image2D image(image_path);
  splash_screen_texture_ = image.ToGLTexture(context_);
  DCHECK_NE(-1, splash_screen_texture_);

  image_position_ = ImageCenter(position, viewport_size);
  image_scale_ = ImageScale(position, viewport_size);

#if defined(__LB_XB1__)
  if (viewport_size.width() < 480.1f && viewport_size.height() < 1080.1f) {
    // In snap-mode, XB1 seems to have a bug where the reported value seems to
    // be too less. Fix manually. Ugh.
    image_scale_.Scale(1.55f);
  }
#endif
}

ExtendedSplashScreen::~ExtendedSplashScreen() {
  DCHECK(context_);
  if (splash_screen_texture_ >= 0) {
    context_->deleteTexture(splash_screen_texture_);
  }
}

bool ExtendedSplashScreen::Hide() {
  DCHECK_EQ(MessageLoop::current(),
            LBWebViewHost::Get()->webkit_message_loop());
  return base::subtle::NoBarrier_AtomicExchange(
      &showing_splash_screen_, 0) == 1;
}

void ExtendedSplashScreen::Draw() {
  DCHECK(context_);
  DCHECK(drawer_);

  if (base::subtle::NoBarrier_Load(&showing_splash_screen_) != 1) {
    // Draw a blank screen and exit.
#if defined(__LB_XB1__)
    // Since XB1 has layer composition, we need to set this transparent.
    context_->clearColor(0.0f, 0.0f, 0.0f, 0.0f);
    context_->clear(GraphicsContext3D::COLOR_BUFFER_BIT);
#endif
    return;
  }

  DCHECK_NE(-1, splash_screen_texture_);

  // Set the background
  context_->enable(GraphicsContext3D::BLEND);
  context_->clearColor(0.902f, 0.176f, 0.153f, 1.0f);
  context_->clear(GraphicsContext3D::COLOR_BUFFER_BIT);

  // And set the splash screen.
  context_->blendFuncSeparate(GraphicsContext3D::SRC_ALPHA,
                              GraphicsContext3D::ONE_MINUS_SRC_ALPHA,
                              GraphicsContext3D::ONE,
                              GraphicsContext3D::ONE);
  drawer_->DrawQuad(splash_screen_texture_,
                    image_position_.x(), image_position_.y(),
                    image_scale_.width(), image_scale_.height(),
                    0.0f /* rotation */);
}

// static
gfx::Rect ExtendedSplashScreen::CenterPosition(const gfx::Size& image_size,
                                               const gfx::Size& viewport_size) {
  gfx::Rect rect(image_size);
  rect.Offset((viewport_size.width() - image_size.width()) / 2,
              (viewport_size.height() - image_size.height()) / 2);
  return rect;
}

}  // namespace LB
