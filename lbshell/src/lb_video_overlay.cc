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

#include "lb_video_overlay.h"

#include <math.h>

#include "base/logging.h"
#include "lb_globals.h"
#include "media/base/pipeline.h"

namespace {

LB::VideoOverlay* s_instance;

void FillTextureCoords(const gfx::Rect& visible_rect,
                       const gfx::Size& coded_size, LB::Coord (&coords)[4]) {
  float coded_width = coded_size.width();
  float coded_height = coded_size.height();
  gfx::RectF normalized_rect(visible_rect.x() / coded_width,
                             visible_rect.y() / coded_height,
                             visible_rect.width() / coded_width,
                             visible_rect.height() / coded_height);
  coords[0].x = normalized_rect.x();
  coords[0].y = normalized_rect.bottom();

  coords[1].x = normalized_rect.x();
  coords[1].y = normalized_rect.y();

  coords[2].x = normalized_rect.right();
  coords[2].y = normalized_rect.bottom();

  coords[3].x = normalized_rect.right();
  coords[3].y = normalized_rect.y();
}

void CalculateScale(float context_width, float context_height,
                    float frame_width, float frame_height, float (&scale)[2]) {
  // Set a default value for invalid sizes to avoid divide by 0.
  if (frame_width == 0 || frame_height == 0 ||
      context_width == 0 || context_height == 0) {
    scale[0] = 1;
    scale[1] = 1;
    return;
  }
  float context_aspect_ratio = context_width / context_height;
  float frame_aspect_ratio = frame_width / frame_height;
  // Use default scale if the aspect ratios are the same.
  if (frame_aspect_ratio <= context_aspect_ratio) {
    scale[0] = frame_aspect_ratio / context_aspect_ratio;
    scale[1] = 1;
    return;
  }
  scale[0] = 1;
  scale[1] = context_aspect_ratio / frame_aspect_ratio;
}

}  // namespace

namespace LB {

VideoOverlay::VideoOverlay(LBGraphics* graphics,
                           LBWebGraphicsContext3D* context) {
  DCHECK(!s_instance);
  s_instance = this;
  graphics_ = graphics;
  context_ = context;
  quad_drawer_.reset(new QuadDrawer(graphics_, context_));

#if !defined(__LB_SHELL__FOR_RELEASE__)
  dropped_frames_ = 0;
  max_delay_in_microseconds_ = 0;
#endif  // !defined(__LB_SHELL__FOR_RELEASE__)
}

VideoOverlay::~VideoOverlay() {
  DCHECK_EQ(s_instance, this);
  s_instance = NULL;
}

VideoOverlay* VideoOverlay::Instance() {
  DCHECK(s_instance);
  return s_instance;
}

void VideoOverlay::Render() {
  const int kEpsilonInMicroseconds =
      base::Time::kMicrosecondsPerSecond / 60 / 2;

  base::AutoLock auto_lock(frames_lock_);

  base::TimeDelta media_time = media::Pipeline::GetCurrentTime();
  while (!frames_.empty()) {
    int64_t frame_time = frames_[0]->GetTimestamp().InMicroseconds();
    if (frame_time >= media_time.InMicroseconds())
      break;
    if (current_frame_ != frames_[0] &&
        frame_time + kEpsilonInMicroseconds >= media_time.InMicroseconds())
      break;
#if !defined(__LB_SHELL__FOR_RELEASE__)
    if (current_frame_ != frames_[0]) {
      ++dropped_frames_;
      if (media_time.InMicroseconds() - frame_time > max_delay_in_microseconds_)
        max_delay_in_microseconds_ = media_time.InMicroseconds() - frame_time;
      DLOG(WARNING)
          << "VideoOverlay::Render() : dropped one frame with timestamp "
          << frames_[0]->GetTimestamp().InMicroseconds()
          << " at media time " << media_time.InMicroseconds()
          << " total dropped " << dropped_frames_
          << " frames with a max delay of " << max_delay_in_microseconds_
          << " ms";
    }
#endif  // !defined(__LB_SHELL__FOR_RELEASE__)

    frames_.erase(frames_.begin());
  }
  if (!frames_.empty())
    current_frame_ = frames_[0];
  if (current_frame_)
    DrawCurrentFrame();
}

void VideoOverlay::AddFrame(const scoped_refptr<media::VideoFrame>& frame) {
  base::AutoLock auto_lock(frames_lock_);
  frames_.push_back(frame);
}

void VideoOverlay::ClearFrames(bool stopped) {
  base::AutoLock auto_lock(frames_lock_);
  frames_.clear();
  if (stopped)
    current_frame_ = NULL;
}

void VideoOverlay::DrawCurrentFrame() {
  DCHECK(current_frame_);

  context_->viewport(0, 0, context_->width(), context_->height());
  context_->scissor(0, 0, context_->width(), context_->height());

  context_->blendFuncSeparate(GraphicsContext3D::SRC_ALPHA,
                              GraphicsContext3D::ONE_MINUS_SRC_ALPHA,
                              GraphicsContext3D::ONE,
                              GraphicsContext3D::ONE);

  Coord coords[4];
  FillTextureCoords(current_frame_->visible_rect(),
                    current_frame_->coded_size(), coords);

  float scales[2];
  CalculateScale(context_->width(), context_->height(),
                 current_frame_->visible_rect().width(),
                 current_frame_->visible_rect().height(),
                 scales);

  quad_drawer_->DrawQuadTex(current_frame_->texture_id(), 0, 0,
                            scales[0], scales[1], 0, coords);
}

}  // namespace LB
