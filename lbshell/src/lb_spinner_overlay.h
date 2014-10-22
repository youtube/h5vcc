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
#ifndef SRC_LB_SPINNER_OVERLAY_H_
#define SRC_LB_SPINNER_OVERLAY_H_

#include "external/chromium/base/memory/scoped_ptr.h"

#include "lb_gl_image_utils.h"
#include "lb_graphics.h"
#include "lb_memory_manager.h"
#include "lb_web_graphics_context_3d.h"

namespace LB {

class SpinnerOverlay {
 public:
  SpinnerOverlay(LBGraphics* graphics, LBWebGraphicsContext3D* context);

  // Makes GL calls to the graphics context to compose a display frame
  void Render();

  // Increase or decrease the "enabled" counter.  When the enabled counter
  // is 0, the spinner is invisible.  Enabling it increases the count,
  // disabling it decreases the count.  The count is initialized to 0.
  void EnableSpinner(bool enable);

 private:
  void UpdateSpinnerState();

  LBGraphics* graphics_;
  LBWebGraphicsContext3D* context_;  // The context we write our commands to

  // Stored so we can initialize graphics objects on the first call to
  // Compose() instead of in the constructor.
  bool initialized_;

  scoped_ptr<LB::QuadDrawer> quad_drawer_;

  int spinner_texture_;

  float spinner_scale_[2];
  float spinner_offset_[2];
  float spinner_rotation_;
  double spinner_frame_time_;

  int spinner_count_;
};

}  // namespace LB

#endif  // SRC_LB_SPINNER_OVERLAY_H_
