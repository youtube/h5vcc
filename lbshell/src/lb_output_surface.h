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

#ifndef SRC_LB_OUTPUT_SURFACE_H_
#define SRC_LB_OUTPUT_SURFACE_H_

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "cc/output_surface.h"


class LBOutputSurface
    : NON_EXPORTED_BASE(public cc::OutputSurface) {
 public:
  LBOutputSurface();
  virtual ~LBOutputSurface();

  // cc::OutputSurface implementation.
  virtual bool BindToClient(cc::OutputSurfaceClient* client) OVERRIDE;
  virtual const struct Capabilities& Capabilities() const OVERRIDE;
  virtual WebKit::WebGraphicsContext3D* Context3D() const OVERRIDE;
  virtual cc::SoftwareOutputDevice* SoftwareDevice() const OVERRIDE {
    return NULL;
  }
  virtual void SendFrameToParentCompositor(
      const cc::CompositorFrame&) OVERRIDE {
    NOTIMPLEMENTED();
  }

 private:
  WebKit::WebGraphicsContext3D* context_;
};

#endif  // SRC_LB_OUTPUT_SURFACE_H_
