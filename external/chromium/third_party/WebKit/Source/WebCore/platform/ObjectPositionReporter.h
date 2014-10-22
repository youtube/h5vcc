/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ObjectPositionReporter_h
#define ObjectPositionReporter_h

#include "config.h"

#include <vector>
#include "public/Platform.h"
#include "public/WebCommon.h"

#if ENABLE(LB_SHELL_CSS_EXTENSIONS)

namespace WebCore {

class LayoutRect;
class RenderBoxModelObject;
class RenderLayer;
class RenderObject;
class RenderView;

class WEBKIT_EXPORT ObjectPositionReporter {
  public:
    // An exact copy of H5VCCGesturable (sync ensured via compile time asserts).
    enum GesturableState {
      GestureIgnored,
      GestureClickable,
      GestureScrollHorizontal,
      GestureScrollVertical,
      GestureScrollBoth,
      MaxGesturableState = GestureScrollBoth,
    };

    static ObjectPositionReporter* create() {
        return WebKit::Platform::current()->createObjectPositionReporter();
    }

    void WalkRenderView(RenderView* view);

    struct ObjectPosition {
        RenderObject* key;
        bool is_video;
        GesturableState gesture_style;
        int x;
        int y;
        int width;
        int height;
    };

    virtual void ReportGesturableElements(const std::vector<ObjectPosition>&) = 0;
    virtual void ReportVideoElements(const std::vector<ObjectPosition>&) = 0;

    virtual ~ObjectPositionReporter() { }
  protected:
    ObjectPositionReporter() { }

    static ObjectPosition GetObjectPositionFromRenderObject(
        RenderBoxModelObject* object);

    static LayoutRect GetTransformedBoundingBox(LayoutRect rect,
                                                RenderLayer* layer);
    static LayoutRect GetAbsolutePosition(RenderBoxModelObject* object);
    static RenderLayer* GetRenderLayer(RenderBoxModelObject* object);
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_CSS_EXTENSIONS)
#endif  // ObjectPositionReporter_h
