// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/heads_up_display_layer.h"

#include "base/debug/trace_event.h"
#include "cc/heads_up_display_layer_impl.h"
#include "cc/layer_tree_host.h"

namespace cc {

scoped_refptr<HeadsUpDisplayLayer> HeadsUpDisplayLayer::create()
{
    return make_scoped_refptr(new HeadsUpDisplayLayer());
}

HeadsUpDisplayLayer::HeadsUpDisplayLayer()
    : Layer()
    , m_hasFontAtlas(false)
{
    setBounds(gfx::Size(256, 128));
#if defined(ENABLE_LB_SHELL_CSS_EXTENSIONS) && ENABLE_LB_SHELL_CSS_EXTENSIONS
    setH5vccTargetScreen(WebKit::ScreenAll);
#endif
}

HeadsUpDisplayLayer::~HeadsUpDisplayLayer()
{
}

void HeadsUpDisplayLayer::update(ResourceUpdateQueue&, const OcclusionTracker*, RenderingStats&)
{
    const LayerTreeDebugState& debugState = layerTreeHost()->debugState();
    int maxTextureSize = layerTreeHost()->rendererCapabilities().maxTextureSize;

    gfx::Size bounds;
    gfx::Transform matrix;
    matrix.MakeIdentity();

    if (debugState.showPlatformLayerTree || debugState.showHudRects()) {
        int width = std::min(maxTextureSize, layerTreeHost()->layoutViewportSize().width());
        int height = std::min(maxTextureSize, layerTreeHost()->layoutViewportSize().height());
        bounds = gfx::Size(width, height);
    } else {
        bounds = gfx::Size(256, 128);
#if defined(__LB_SHELL__)
        // Bring HUD in a bit so you can still see it despite TV overscan
        const int overscan_x = 64;
        const int overscan_y = 32;
        matrix.Translate(
            layerTreeHost()->layoutViewportSize().width() - 256 - overscan_x,
            overscan_y);
#else
        matrix.Translate(layerTreeHost()->layoutViewportSize().width() - 256, 0);
#endif
    }

    setBounds(bounds);
    setTransform(matrix);
}

bool HeadsUpDisplayLayer::drawsContent() const
{
    return true;
}

void HeadsUpDisplayLayer::setFontAtlas(scoped_ptr<FontAtlas> fontAtlas)
{
    m_fontAtlas = fontAtlas.Pass();
    m_hasFontAtlas = true;
}

scoped_ptr<LayerImpl> HeadsUpDisplayLayer::createLayerImpl(LayerTreeImpl* treeImpl)
{
    return HeadsUpDisplayLayerImpl::create(treeImpl, m_layerId).PassAs<LayerImpl>();
}

void HeadsUpDisplayLayer::pushPropertiesTo(LayerImpl* layerImpl)
{
    Layer::pushPropertiesTo(layerImpl);

    if (!m_fontAtlas)
        return;

    HeadsUpDisplayLayerImpl* hudLayerImpl = static_cast<HeadsUpDisplayLayerImpl*>(layerImpl);
    hudLayerImpl->setFontAtlas(m_fontAtlas.Pass());
}

}  // namespace cc
