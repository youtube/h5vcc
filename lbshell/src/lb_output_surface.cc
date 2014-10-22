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

#include "lb_graphics.h"
#include "lb_output_surface.h"
#include "lb_web_graphics_context_3d.h"

//------------------------------------------------------------------------------

LBOutputSurface::LBOutputSurface() {
  context_ = LBGraphics::GetPtr()->GetCompositorContext();
}

LBOutputSurface::~LBOutputSurface() {
}

const struct cc::OutputSurface::Capabilities&
    LBOutputSurface::Capabilities() const {
  static struct OutputSurface::Capabilities capabilities;
  return capabilities;
}

bool LBOutputSurface::BindToClient(
    cc::OutputSurfaceClient* client) {
  return context_->makeContextCurrent();
}

WebGraphicsContext3D* LBOutputSurface::Context3D() const {
  return context_;
}
