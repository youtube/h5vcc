// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/bitmap_content_layer_updater.h"

#include "cc/layer_painter.h"
#include "cc/rendering_stats.h"
#include "cc/resource_update.h"
#include "cc/resource_update_queue.h"
#include "skia/ext/platform_canvas.h"

namespace cc {

BitmapContentLayerUpdater::Resource::Resource(BitmapContentLayerUpdater* updater, scoped_ptr<PrioritizedResource> texture)
    : LayerUpdater::Resource(texture.Pass())
    , m_updater(updater)
{
}

BitmapContentLayerUpdater::Resource::~Resource()
{
}

void BitmapContentLayerUpdater::Resource::update(ResourceUpdateQueue& queue, const gfx::Rect& sourceRect, const gfx::Vector2d& destOffset, bool partialUpdate, RenderingStats&)
{
    updater()->updateTexture(queue, texture(), sourceRect, destOffset, partialUpdate);
}

scoped_refptr<BitmapContentLayerUpdater> BitmapContentLayerUpdater::create(scoped_ptr<LayerPainter> painter)
{
    return make_scoped_refptr(new BitmapContentLayerUpdater(painter.Pass()));
}

BitmapContentLayerUpdater::BitmapContentLayerUpdater(scoped_ptr<LayerPainter> painter)
    : ContentLayerUpdater(painter.Pass())
    , m_opaque(false)
{
}

BitmapContentLayerUpdater::~BitmapContentLayerUpdater()
{
}

scoped_ptr<LayerUpdater::Resource> BitmapContentLayerUpdater::createResource(PrioritizedResourceManager* manager)
{
    return scoped_ptr<LayerUpdater::Resource>(new Resource(this, PrioritizedResource::create(manager)));
}

void BitmapContentLayerUpdater::prepareToUpdate(const gfx::Rect& contentRect, const gfx::Size& tileSize, float contentsWidthScale, float contentsHeightScale, gfx::Rect& resultingOpaqueRect, RenderingStats& stats)
{
#if defined(__LB_SHELL__)
    if (m_canvasSize != contentRect.size()) {
        m_canvasSize = contentRect.size();
        // If we need a bigger buffer, recreate, otherwise keep the previous buffer
        gfx::Size newSize(std::max(contentRect.width(), m_internalSize.width()),
                          std::max(contentRect.height(), m_internalSize.height()));
        if (m_internalSize != newSize) {
            m_internalSize = newSize;
            // Destroy the old one before creating a new one, to reduce memory pressure.
            m_canvas.reset();
            m_canvas = make_scoped_ptr(skia::CreateBitmapCanvas(m_internalSize.width(), m_internalSize.height(), m_opaque));
        }
    }

    stats.totalPixelsRasterized += contentRect.width() * contentRect.height();

    // We need to pass content rect to paintContents() with correct canvas size
    gfx::Rect canvasRect = contentRect;
    canvasRect.set_size(m_internalSize);
    paintContents(m_canvas.get(), canvasRect, contentsWidthScale, contentsHeightScale, resultingOpaqueRect, stats);
#else
    if (m_canvasSize != contentRect.size()) {
        m_canvasSize = contentRect.size();
        // Destroy the old one before creating a new one, to reduce memory pressure.
        m_canvas.reset();
        m_canvas = make_scoped_ptr(skia::CreateBitmapCanvas(m_canvasSize.width(), m_canvasSize.height(), m_opaque));
    }

    stats.totalPixelsRasterized += contentRect.width() * contentRect.height();

    paintContents(m_canvas.get(), contentRect, contentsWidthScale, contentsHeightScale, resultingOpaqueRect, stats);
#endif
}

void BitmapContentLayerUpdater::updateTexture(ResourceUpdateQueue& queue, PrioritizedResource* texture, const gfx::Rect& sourceRect, const gfx::Vector2d& destOffset, bool partialUpdate)
{
    ResourceUpdate upload = ResourceUpdate::Create(
        texture,
        &m_canvas->getDevice()->accessBitmap(false),
        contentRect(),
        sourceRect,
        destOffset);
    if (partialUpdate)
        queue.appendPartialUpload(upload);
    else
        queue.appendFullUpload(upload);
}

void BitmapContentLayerUpdater::setOpaque(bool opaque)
{
    if (opaque != m_opaque) {
        m_canvas.reset();
        m_canvasSize = gfx::Size();
#if defined(__LB_SHELL__)
        m_internalSize = gfx::Size();
#endif
    }
    m_opaque = opaque;
}

}  // namespace cc
