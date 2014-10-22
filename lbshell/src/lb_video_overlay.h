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
#ifndef SRC_LB_VIDEO_OVERLAY_H_
#define SRC_LB_VIDEO_OVERLAY_H_

#include <vector>
#include "base/synchronization/lock.h"
#include "base/memory/scoped_ptr.h"
#include "lb_gl_image_utils.h"
#include "lb_graphics.h"
#include "lb_web_graphics_context_3d.h"
#include "media/base/video_frame.h"

namespace LB {

class VideoOverlay {
 public:
  VideoOverlay(LBGraphics* graphics, LBWebGraphicsContext3D* context);
  ~VideoOverlay();

  static VideoOverlay* Instance();

  // Makes GL calls to the graphics context to compose a display frame
  void Render();

  void AddFrame(const scoped_refptr<media::VideoFrame>& frame);
  void ClearFrames(bool stopped);

 private:
  void DrawCurrentFrame();

  LBGraphics* graphics_;
  LBWebGraphicsContext3D* context_;  // The context we write our commands to
  scoped_ptr<LB::QuadDrawer> quad_drawer_;

  base::Lock frames_lock_;
  std::vector<scoped_refptr<media::VideoFrame> > frames_;
  scoped_refptr<media::VideoFrame> current_frame_;

#if !defined(__LB_SHELL__FOR_RELEASE__)
  int dropped_frames_;
  int max_delay_in_microseconds_;
#endif  // !defined(__LB_SHELL__FOR_RELEASE__)
};

}  // namespace LB

#endif  // SRC_LB_VIDEO_OVERLAY_H_
