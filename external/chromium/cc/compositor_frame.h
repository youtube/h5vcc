// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_COMPOSITOR_FRAME_H_
#define CC_COMPOSITOR_FRAME_H_

#include "base/memory/scoped_ptr.h"
#include "cc/cc_export.h"
#include "cc/compositor_frame_metadata.h"
#include "cc/delegated_frame_data.h"
#include "cc/gl_frame_data.h"

namespace cc {

class CC_EXPORT CompositorFrame {
 public:
  CompositorFrame();
  ~CompositorFrame();

  CompositorFrameMetadata metadata;
  scoped_ptr<DelegatedFrameData> delegated_frame_data;
  scoped_ptr<GLFrameData> gl_frame_data;
};

}  // namespace cc

#endif  // CC_COMPOSITOR_FRAME_H_
