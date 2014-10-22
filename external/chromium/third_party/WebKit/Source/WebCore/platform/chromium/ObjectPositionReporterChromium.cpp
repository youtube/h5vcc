/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ObjectPositionReporter.h"

#if !ENABLE(LB_SHELL_CSS_EXTENSIONS)
#error This file should not be included unless the above is true.
#endif

#include <wtf/Vector.h>
#include <vector>

#include "FloatPoint.h"
#include "FloatQuad.h"
#include "RenderBoxModelObject.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "RenderStyleConstants.h"
#include "RenderView.h"
#include "TransformationMatrix.h"

using namespace WTF;

namespace WebCore {

COMPILE_ASSERT(static_cast<int>(MaxH5VCCGesturable) ==
               static_cast<int>(ObjectPositionReporter::MaxGesturableState),
               Enum_sizes_must_match);

// This class walks the render tree in descending z-order, and fills up it's
// internal list with all RenderObjects it finds that have the particular
// -h5vcc-gesturable tag on.
class RenderTreeWalker {
  public:
    void WalkLayer(RenderLayer* layer, int zOffset);

    const Vector<RenderBoxModelObject*>& gesturable_candidates() const {
        return gesturable_candidates_;
    }
    const Vector<RenderBoxModelObject*>& video_elements() const {
        return video_elements_;
    }

  private:
    void WalkLayerList(Vector<RenderLayer*>* layers, int zOffset);
    void WalkLayerObjects(RenderBoxModelObject* obj);

    Vector<RenderBoxModelObject*> gesturable_candidates_;
    Vector<RenderBoxModelObject*> video_elements_;
};


ObjectPositionReporter::ObjectPosition
ObjectPositionReporter::GetObjectPositionFromRenderObject(
    RenderBoxModelObject* object)
{
    RenderLayer* layer = GetRenderLayer(object);

    // Don't work on a case where there is no layer.
    ASSERT(layer);

    LayoutRect pos = GetAbsolutePosition(object);

    // Apply transform styles (if any)
    pos = GetTransformedBoundingBox(pos, layer);

    ObjectPosition rect = {
        object,
        object->isVideo(),
        static_cast<GesturableState>(object->style()->h5vccGesturable()),
        pos.pixelSnappedX(),
        pos.pixelSnappedY(),
        pos.pixelSnappedWidth(),
        pos.pixelSnappedHeight()
    };
    return rect;
}

void ObjectPositionReporter::WalkRenderView(RenderView* view)
{
    ASSERT(!view->needsLayout());
    ASSERT(view->layer());

    // Get all the elements that have the h5vcc-gesturable tag on.
    RenderTreeWalker walker;
    walker.WalkLayer(view->layer(), 0 /* initial zOffset */);

    std::vector<ObjectPosition> gesturable_bounds;
    for (int i = 0; i < walker.gesturable_candidates().size(); ++i) {
        RenderBoxModelObject* obj = walker.gesturable_candidates().at(i);
        if (!GetRenderLayer(obj)) {
            continue;
        }
        gesturable_bounds.push_back(GetObjectPositionFromRenderObject(obj));
    }
    ReportGesturableElements(gesturable_bounds);

    std::vector<ObjectPosition> video_bounds;
    for (int i = 0; i < walker.video_elements().size(); ++i) {
        RenderBoxModelObject* obj = walker.video_elements().at(i);
        if (!GetRenderLayer(obj)) {
            continue;
        }
        video_bounds.push_back(GetObjectPositionFromRenderObject(obj));
    }
    ReportVideoElements(video_bounds);
}

// |offsetLeft| / |offsetTop| refer to positions relative to the parent, so we
// walk up the RenderObject tree and add up all the offsets to get the absolute
// position.
LayoutRect ObjectPositionReporter::GetAbsolutePosition(
        RenderBoxModelObject* object)
{
    ASSERT(object);
    LayoutRect pos(object->offsetLeft(), object->offsetTop(),
                   object->offsetWidth(), object->offsetHeight());
    while ((object = object->offsetParent()) != NULL) {
        pos.move(object->offsetLeft(), object->offsetTop());
    }
    return pos;
}

// Walk up the tree and find the parent object that has a RenderLayer. Return
// that. In case of a problem, return NULL.
RenderLayer* ObjectPositionReporter::GetRenderLayer(
        RenderBoxModelObject* object)
{
    if (!object)
        return NULL;

    if (object->hasLayer() && object->layer())
        return object->layer();

    if (!object->parent() || !object->parent()->isBoxModelObject())
        return NULL;

    return GetRenderLayer(toRenderBoxModelObject(object->parent()));
}

// This deals with the -webkit-transform css style. For more details on it, see:
// http://dev.opera.com/articles/view/understanding-the-css-transforms-matrix/.
//
// LayoutRect and FloatRect are rectangular position co-ordinates containing
// location(left, top) and size(width, height). From there, we get a FloatQuad,
// a set of 4 points that represent 4 end-points of the rectangle. We transform
// this particular set of points, to get a new quad representing the 4 end-pts
// of the resulting quadrilateral, and then get the bounding-box (box containing
// all points of the quadrilateral), which is then returned.
//
// This has a small caveat: The transformation matrix expects the quad co-ords
// such that it's top-left is (0,0). However, LayoutRect/FloatRect have co-ord
// system as (0,0) the top-left of the screen. Hence, we need to 'move' the
// LayoutRect via its location offset to align it at (0,0), and then
// apply the transformation matrix to the quad. We then 'un-move' the bounding
// box by the same offset to go back to the same (relative) position as before.
//
// Okay, enough talk. Here's some code:
LayoutRect ObjectPositionReporter::GetTransformedBoundingBox(
        LayoutRect pos, RenderLayer* layer)
{
    ASSERT(layer);
    if (!layer->hasTransform())
        return pos;

    // This matrix includes the canonical transformation matrix at the
    // present animation frame. We then reduce it to 2D since we
    // are interested in 2D bounding box only.
    // TODO: verify that this takes in account ancestor transform matrices.
    TransformationMatrix matrix = layer->currentTransform();
    matrix = matrix.to2dTransform();

    if (matrix.isIdentity())
        return pos;

    // Remember the original position, and then reset to point (0,0).
    LayoutPoint offset = pos.location();
    pos.setLocation(LayoutPoint::zero());

    // Get the quad, multiply the matrix with it.
    FloatQuad original = FloatQuad(FloatRect(pos));
    FloatQuad final = matrix.mapQuad(original);

    // Check that if the matrix was not identity, something must have changed.
    ASSERT((original != final) ^ (matrix.isIdentity()));

    // This is the final bounding-box relative to the center of the original
    // box.
    pos = LayoutRect(final.boundingBox());

    // Now move back by the offset amount. Note: we don't simply
    // setLocation(offset) since the new boundingBox can have a different
    // location set.
    pos.moveBy(offset);

    return pos;
}

// For a particular layer, start walking it and it's children. Note that this
// has to happen in a particular order, very similar to the HitTesting algorithm
// in WebCore. The tree-walk considers all elements in descending z-order. Note
// that in a particular RenderLayer all it's elements are in the same z-order.
void RenderTreeWalker::WalkLayer(RenderLayer* layer, int zOffset)
{
    if (!layer->isSelfPaintingLayer() &&
            !layer->hasSelfPaintingLayerDescendant())
        return; // skip, it or it's children do not draw anything.

    if (layer->preserves3D() || layer->has3DTransform())
        return; // ignore, too compilcated.

    // Make sure the child lists are in-order.
    layer->updateLayerListsIfNeeded();

    // First report on positive z-index layers, if any.
    WalkLayerList(layer->posZOrderList(), zOffset);
    // Then overflow lists.
    WalkLayerList(layer->normalFlowList(), zOffset);

    ASSERT(layer->renderer());
    // Now check on this layer and render objects inside it.
    if (layer->renderer() && layer->renderer()->isBoxModelObject()) {
        WalkLayerObjects(toRenderBoxModelObject(layer->renderer()));
    }

    // Finally the negative z-index layers, if any.
    WalkLayerList(layer->negZOrderList(), zOffset);
}

void RenderTreeWalker::WalkLayerList(Vector<RenderLayer*>* layers, int zOffset)
{
    if (!layers || layers->isEmpty())
        return;

    // Go in reverse order. TODO: Verify if reverse matters or not.
    for (int i = layers->size() - 1; i >= 0; --i) {
        RenderLayer* layer = layers->at(i);
        ASSERT(!layer->isPaginated());
        if (layer->isPaginated())
            continue;
        WalkLayer(layer, zOffset + layer->zIndex());
    }
}

// For a given layer, start walking all elements in the layer. We start with
// the renderer() object (kind-a like the parent of the RenderLayer), and
// iterate all elements that do not have a separate layer.
void RenderTreeWalker::WalkLayerObjects(RenderBoxModelObject* obj)
{
    // Iterate over all children recursively. Skip children with a layer.
    // PostOrder so the parent hits last.
    for (RenderObject* child = obj->firstChild(); child;
            child = child->nextSibling()) {
        if (!child->hasLayer() && child->isBoxModelObject()) {
            WalkLayerObjects(toRenderBoxModelObject(child));
        }
    }

    if (obj->isVideo()) {
        video_elements_.append(obj);
    }
    if (obj->visibleToHitTesting() &&
        obj->style()->h5vccGesturable() != WebCore::GestureIgnored) {
        gesturable_candidates_.append(obj);
    }
}

}  // namespace WebCore
