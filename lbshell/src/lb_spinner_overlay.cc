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

#include "lb_spinner_overlay.h"

#include <math.h>

#include "lb_gl_image_utils.h"
#include "lb_globals.h"

namespace LB {

namespace {
  // Offset from the bottom right corner of the screen, both x and y in units of
  // screen height.
  const float kSpinnerOffsetFromCornerX = 0.22f;
  const float kSpinnerOffsetFromCornerY = 0.18f;
  const float kSpinnerScale = 0.15f;  // Where 1.0f is the height of the screen.
  const double kSpinnerAngularVelocity = M_PI;  // in radians/sec
}

SpinnerOverlay::SpinnerOverlay(LBGraphics* graphics,
                                   LBWebGraphicsContext3D* context) {
  graphics_ = graphics;
  context_ = context;
  initialized_ = false;
  spinner_texture_ = -1;
  spinner_count_ = 0;

  // Initialize timing
  spinner_frame_time_ = base::Time::Now().ToDoubleT();
  spinner_rotation_ = 0.0f;
  UpdateSpinnerState();
}


void SpinnerOverlay::Render() {
  if (!initialized_) {
    quad_drawer_.reset(new LB::QuadDrawer(graphics_, context_));
    const std::string game_content_path(GetGlobalsPtr()->game_content_path);

    // Load spinner texture from disk in to a texture handle
    std::string file_path = game_content_path + "/spinner.png";
    LB::Image2D spinner_image(file_path);
    spinner_texture_ = spinner_image.ToGLTexture(context_);

    initialized_ = true;
  }

  context_->viewport(0, 0, context_->width(), context_->height());
  context_->scissor(0, 0, context_->width(), context_->height());

  UpdateSpinnerState();

  context_->enable(GraphicsContext3D::BLEND);

  if (spinner_count_) {
    context_->blendFuncSeparate(GraphicsContext3D::SRC_ALPHA,
                                GraphicsContext3D::ONE_MINUS_SRC_ALPHA,
                                GraphicsContext3D::ONE,
                                GraphicsContext3D::ONE);

    quad_drawer_->DrawQuad(spinner_texture_,
                           spinner_offset_[0], spinner_offset_[1],
                           spinner_scale_[0], spinner_scale_[1],
                           spinner_rotation_);
  }
}

void SpinnerOverlay::EnableSpinner(bool enable) {
  if (enable) {
    ++spinner_count_;
  } else {
    --spinner_count_;
    DCHECK_GE(spinner_count_, 0);
  }
}

void SpinnerOverlay::UpdateSpinnerState() {
  double current_time = base::Time::Now().ToDoubleT();
  double diff = current_time - spinner_frame_time_;
  spinner_frame_time_ = current_time;

  // Adjust scale for aspect ratio to keep things circular, as the width
  // and height of the target surface may change over time
  float aspect_ratio = static_cast<float>(context_->height()) /
                       static_cast<float>(context_->width());
  spinner_scale_[0] = kSpinnerScale * aspect_ratio;
  spinner_scale_[1] = kSpinnerScale;

  // Set spinner in the bottom right corner.  kSpinnerOffsetFromCorner ranges
  // from [0,1], so we multiply by 2 since the display is from [-1,1].
  spinner_offset_[0] = 1 - 2 * kSpinnerOffsetFromCornerX * aspect_ratio;
  spinner_offset_[1] = -1 + 2 * kSpinnerOffsetFromCornerY;

  spinner_rotation_ -= static_cast<float>(kSpinnerAngularVelocity * diff);
}

}  // namespace LB
