// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/delegated_renderer_layer_impl.h"

#include "cc/append_quads_data.h"
#include "cc/layer_tree_host_impl.h"
#include "cc/quad_sink.h"
#include "cc/render_pass_draw_quad.h"
#include "cc/scoped_ptr_vector.h"
#include "cc/single_thread_proxy.h"
#include "cc/solid_color_draw_quad.h"
#include "cc/solid_color_layer_impl.h"
#include "cc/test/fake_layer_tree_host_impl.h"
#include "cc/test/fake_layer_tree_host_impl_client.h"
#include "cc/test/fake_output_surface.h"
#include "cc/test/fake_proxy.h"
#include "cc/test/fake_web_graphics_context_3d.h"
#include "cc/test/geometry_test_utils.h"
#include "cc/test/mock_quad_culler.h"
#include "cc/test/render_pass_test_common.h"
#include "cc/test/render_pass_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/transform.h"

namespace cc {
namespace {

class DelegatedRendererLayerImplTest : public testing::Test {
public:
    DelegatedRendererLayerImplTest()
        : m_proxy(scoped_ptr<Thread>(NULL))
        , m_alwaysImplThreadAndMainThreadBlocked(&m_proxy)
    {
        LayerTreeSettings settings;
        settings.minimumOcclusionTrackingSize = gfx::Size();

        m_hostImpl = LayerTreeHostImpl::create(settings, &m_client, &m_proxy);
        m_hostImpl->initializeRenderer(createFakeOutputSurface());
        m_hostImpl->setViewportSize(gfx::Size(10, 10), gfx::Size(10, 10));
    }

protected:
    FakeProxy m_proxy;
    FakeLayerTreeHostImplClient m_client;
    DebugScopedSetImplThreadAndMainThreadBlocked m_alwaysImplThreadAndMainThreadBlocked;
    scoped_ptr<LayerTreeHostImpl> m_hostImpl;
};

class DelegatedRendererLayerImplTestSimple : public DelegatedRendererLayerImplTest {
public:
    DelegatedRendererLayerImplTestSimple()
        : DelegatedRendererLayerImplTest()
    {
        scoped_ptr<LayerImpl> rootLayer = SolidColorLayerImpl::create(m_hostImpl->activeTree(), 1).PassAs<LayerImpl>();
        scoped_ptr<LayerImpl> layerBefore = SolidColorLayerImpl::create(m_hostImpl->activeTree(), 2).PassAs<LayerImpl>();
        scoped_ptr<LayerImpl> layerAfter = SolidColorLayerImpl::create(m_hostImpl->activeTree(), 3).PassAs<LayerImpl>();
        scoped_ptr<DelegatedRendererLayerImpl> delegatedRendererLayer = DelegatedRendererLayerImpl::create(m_hostImpl->activeTree(), 4);

        m_hostImpl->setViewportSize(gfx::Size(100, 100), gfx::Size(100, 100));
        rootLayer->setBounds(gfx::Size(100, 100));

        layerBefore->setPosition(gfx::Point(20, 20));
        layerBefore->setBounds(gfx::Size(14, 14));
        layerBefore->setContentBounds(gfx::Size(14, 14));
        layerBefore->setDrawsContent(true);
        layerBefore->setForceRenderSurface(true);

        layerAfter->setPosition(gfx::Point(5, 5));
        layerAfter->setBounds(gfx::Size(15, 15));
        layerAfter->setContentBounds(gfx::Size(15, 15));
        layerAfter->setDrawsContent(true);
        layerAfter->setForceRenderSurface(true);

        delegatedRendererLayer->setPosition(gfx::Point(3, 3));
        delegatedRendererLayer->setBounds(gfx::Size(10, 10));
        delegatedRendererLayer->setContentBounds(gfx::Size(10, 10));
        delegatedRendererLayer->setDrawsContent(true);
        gfx::Transform transform;
        transform.Translate(1, 1);
        delegatedRendererLayer->setTransform(transform);

        ScopedPtrVector<RenderPass> delegatedRenderPasses;
        TestRenderPass* pass1 = addRenderPass(delegatedRenderPasses, RenderPass::Id(9, 6), gfx::Rect(6, 6, 6, 6), gfx::Transform());
        addQuad(pass1, gfx::Rect(0, 0, 6, 6), 33u);
        TestRenderPass* pass2 = addRenderPass(delegatedRenderPasses, RenderPass::Id(9, 7), gfx::Rect(7, 7, 7, 7), gfx::Transform());
        addQuad(pass2, gfx::Rect(0, 0, 7, 7), 22u);
        addRenderPassQuad(pass2, pass1);
        TestRenderPass* pass3 = addRenderPass(delegatedRenderPasses, RenderPass::Id(9, 8), gfx::Rect(8, 8, 8, 8), gfx::Transform());
        addRenderPassQuad(pass3, pass2);
        delegatedRendererLayer->setRenderPasses(delegatedRenderPasses);

        // The RenderPasses should be taken by the layer.
        EXPECT_EQ(0u, delegatedRenderPasses.size());

        m_rootLayerPtr = rootLayer.get();
        m_layerBeforePtr = layerBefore.get();
        m_layerAfterPtr = layerAfter.get();
        m_delegatedRendererLayerPtr = delegatedRendererLayer.get();

        // Force the delegated RenderPasses to come before the RenderPass from layerAfter.
        layerAfter->addChild(delegatedRendererLayer.PassAs<LayerImpl>());
        rootLayer->addChild(layerAfter.Pass());

        // Get the RenderPass generated by layerBefore to come before the delegated RenderPasses.
        rootLayer->addChild(layerBefore.Pass());

        m_hostImpl->setRootLayer(rootLayer.Pass());
    }

protected:
    LayerImpl* m_rootLayerPtr;
    LayerImpl* m_layerBeforePtr;
    LayerImpl* m_layerAfterPtr;
    DelegatedRendererLayerImpl* m_delegatedRendererLayerPtr;
};

TEST_F(DelegatedRendererLayerImplTestSimple, AddsContributingRenderPasses)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // Each non-DelegatedRendererLayer added one RenderPass. The DelegatedRendererLayer added two contributing passes.
    ASSERT_EQ(5u, frame.renderPasses.size());

    // The DelegatedRendererLayer should have added its contributing RenderPasses to the frame.
    EXPECT_EQ(4, frame.renderPasses[1]->id.layer_id);
    EXPECT_EQ(1, frame.renderPasses[1]->id.index);
    EXPECT_EQ(4, frame.renderPasses[2]->id.layer_id);
    EXPECT_EQ(2, frame.renderPasses[2]->id.index);
    // And all other RenderPasses should be non-delegated.
    EXPECT_NE(4, frame.renderPasses[0]->id.layer_id);
    EXPECT_EQ(0, frame.renderPasses[0]->id.index);
    EXPECT_NE(4, frame.renderPasses[3]->id.layer_id);
    EXPECT_EQ(0, frame.renderPasses[3]->id.index);
    EXPECT_NE(4, frame.renderPasses[4]->id.layer_id);
    EXPECT_EQ(0, frame.renderPasses[4]->id.index);

    // The DelegatedRendererLayer should have added its RenderPasses to the frame in order.
    EXPECT_RECT_EQ(gfx::Rect(6, 6, 6, 6), frame.renderPasses[1]->output_rect);
    EXPECT_RECT_EQ(gfx::Rect(7, 7, 7, 7), frame.renderPasses[2]->output_rect);

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

TEST_F(DelegatedRendererLayerImplTestSimple, AddsQuadsToContributingRenderPasses)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // Each non-DelegatedRendererLayer added one RenderPass. The DelegatedRendererLayer added two contributing passes.
    ASSERT_EQ(5u, frame.renderPasses.size());

    // The DelegatedRendererLayer should have added its contributing RenderPasses to the frame.
    EXPECT_EQ(4, frame.renderPasses[1]->id.layer_id);
    EXPECT_EQ(1, frame.renderPasses[1]->id.index);
    EXPECT_EQ(4, frame.renderPasses[2]->id.layer_id);
    EXPECT_EQ(2, frame.renderPasses[2]->id.index);

    // The DelegatedRendererLayer should have added copies of its quads to contributing RenderPasses.
    ASSERT_EQ(1u, frame.renderPasses[1]->quad_list.size());
    EXPECT_RECT_EQ(gfx::Rect(0, 0, 6, 6), frame.renderPasses[1]->quad_list[0]->rect);

    // Verify it added the right quads.
    ASSERT_EQ(2u, frame.renderPasses[2]->quad_list.size());
    EXPECT_RECT_EQ(gfx::Rect(0, 0, 7, 7), frame.renderPasses[2]->quad_list[0]->rect);
    EXPECT_RECT_EQ(gfx::Rect(6, 6, 6, 6), frame.renderPasses[2]->quad_list[1]->rect);
    ASSERT_EQ(1u, frame.renderPasses[1]->quad_list.size());
    EXPECT_RECT_EQ(gfx::Rect(0, 0, 6, 6), frame.renderPasses[1]->quad_list[0]->rect);

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

TEST_F(DelegatedRendererLayerImplTestSimple, AddsQuadsToTargetRenderPass)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // Each non-DelegatedRendererLayer added one RenderPass. The DelegatedRendererLayer added two contributing passes.
    ASSERT_EQ(5u, frame.renderPasses.size());

    // The layer's target is the RenderPass from m_layerAfter.
    EXPECT_EQ(RenderPass::Id(3, 0), frame.renderPasses[3]->id);

    // The DelegatedRendererLayer should have added copies of quads in its root RenderPass to its target RenderPass.
    // The m_layerAfter also adds one quad.
    ASSERT_EQ(2u, frame.renderPasses[3]->quad_list.size());

    // Verify it added the right quads.
    EXPECT_RECT_EQ(gfx::Rect(7, 7, 7, 7), frame.renderPasses[3]->quad_list[0]->rect);

    // Its target layer should have a quad as well.
    EXPECT_RECT_EQ(gfx::Rect(0, 0, 15, 15), frame.renderPasses[3]->quad_list[1]->rect);

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

TEST_F(DelegatedRendererLayerImplTestSimple, QuadsFromRootRenderPassAreModifiedForTheTarget)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // Each non-DelegatedRendererLayer added one RenderPass. The DelegatedRendererLayer added two contributing passes.
    ASSERT_EQ(5u, frame.renderPasses.size());

    // The DelegatedRendererLayer is at position 3,3 compared to its target, and has a translation transform of 1,1.
    // So its root RenderPass' quads should all be transformed by that combined amount.
    gfx::Transform transform;
    transform.Translate(4, 4);
    EXPECT_TRANSFORMATION_MATRIX_EQ(transform, frame.renderPasses[3]->quad_list[0]->quadTransform());

    // Quads from non-root RenderPasses should not be shifted though.
    ASSERT_EQ(2u, frame.renderPasses[2]->quad_list.size());
    EXPECT_TRANSFORMATION_MATRIX_EQ(gfx::Transform(), frame.renderPasses[2]->quad_list[0]->quadTransform());
    EXPECT_TRANSFORMATION_MATRIX_EQ(gfx::Transform(), frame.renderPasses[2]->quad_list[1]->quadTransform());
    ASSERT_EQ(1u, frame.renderPasses[1]->quad_list.size());
    EXPECT_TRANSFORMATION_MATRIX_EQ(gfx::Transform(), frame.renderPasses[1]->quad_list[0]->quadTransform());

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

TEST_F(DelegatedRendererLayerImplTestSimple, DoesNotOwnARenderSurface)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // If the DelegatedRendererLayer is axis aligned and has opacity 1, then
    // it has no need to be a renderSurface for the quads it carries.
    EXPECT_FALSE(m_delegatedRendererLayerPtr->renderSurface());

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

TEST_F(DelegatedRendererLayerImplTestSimple, DoesOwnARenderSurface)
{
    m_delegatedRendererLayerPtr->setOpacity(0.5f);

    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // This test case has quads from multiple layers in the delegated renderer,
    // so if the DelegatedRendererLayer has opacity < 1, it should end up with
    // a render surface.
    EXPECT_TRUE(m_delegatedRendererLayerPtr->renderSurface());

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

class DelegatedRendererLayerImplTestOwnSurface : public DelegatedRendererLayerImplTestSimple {
public:
    DelegatedRendererLayerImplTestOwnSurface()
        : DelegatedRendererLayerImplTestSimple()
    {
        m_delegatedRendererLayerPtr->setForceRenderSurface(true);
    }
};

TEST_F(DelegatedRendererLayerImplTestOwnSurface, AddsRenderPasses)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // Each non-DelegatedRendererLayer added one RenderPass. The DelegatedRendererLayer added two contributing passes and its owned surface added one pass.
    ASSERT_EQ(6u, frame.renderPasses.size());

    // The DelegatedRendererLayer should have added its contributing RenderPasses to the frame.
    EXPECT_EQ(4, frame.renderPasses[1]->id.layer_id);
    EXPECT_EQ(1, frame.renderPasses[1]->id.index);
    EXPECT_EQ(4, frame.renderPasses[2]->id.layer_id);
    EXPECT_EQ(2, frame.renderPasses[2]->id.index);
    // The DelegatedRendererLayer should have added a RenderPass for its surface to the frame.
    EXPECT_EQ(4, frame.renderPasses[1]->id.layer_id);
    EXPECT_EQ(0, frame.renderPasses[3]->id.index);
    // And all other RenderPasses should be non-delegated.
    EXPECT_NE(4, frame.renderPasses[0]->id.layer_id);
    EXPECT_EQ(0, frame.renderPasses[0]->id.index);
    EXPECT_NE(4, frame.renderPasses[4]->id.layer_id);
    EXPECT_EQ(0, frame.renderPasses[4]->id.index);
    EXPECT_NE(4, frame.renderPasses[5]->id.layer_id);
    EXPECT_EQ(0, frame.renderPasses[5]->id.index);

    // The DelegatedRendererLayer should have added its RenderPasses to the frame in order.
    EXPECT_RECT_EQ(gfx::Rect(6, 6, 6, 6), frame.renderPasses[1]->output_rect);
    EXPECT_RECT_EQ(gfx::Rect(7, 7, 7, 7), frame.renderPasses[2]->output_rect);

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

TEST_F(DelegatedRendererLayerImplTestOwnSurface, AddsQuadsToContributingRenderPasses)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // Each non-DelegatedRendererLayer added one RenderPass. The DelegatedRendererLayer added two contributing passes and its owned surface added one pass.
    ASSERT_EQ(6u, frame.renderPasses.size());

    // The DelegatedRendererLayer should have added its contributing RenderPasses to the frame.
    EXPECT_EQ(4, frame.renderPasses[1]->id.layer_id);
    EXPECT_EQ(1, frame.renderPasses[1]->id.index);
    EXPECT_EQ(4, frame.renderPasses[2]->id.layer_id);
    EXPECT_EQ(2, frame.renderPasses[2]->id.index);

    // The DelegatedRendererLayer should have added copies of its quads to contributing RenderPasses.
    ASSERT_EQ(1u, frame.renderPasses[1]->quad_list.size());
    EXPECT_RECT_EQ(gfx::Rect(0, 0, 6, 6), frame.renderPasses[1]->quad_list[0]->rect);

    // Verify it added the right quads.
    ASSERT_EQ(2u, frame.renderPasses[2]->quad_list.size());
    EXPECT_RECT_EQ(gfx::Rect(0, 0, 7, 7), frame.renderPasses[2]->quad_list[0]->rect);
    EXPECT_RECT_EQ(gfx::Rect(6, 6, 6, 6), frame.renderPasses[2]->quad_list[1]->rect);
    ASSERT_EQ(1u, frame.renderPasses[1]->quad_list.size());
    EXPECT_RECT_EQ(gfx::Rect(0, 0, 6, 6), frame.renderPasses[1]->quad_list[0]->rect);

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

TEST_F(DelegatedRendererLayerImplTestOwnSurface, AddsQuadsToTargetRenderPass)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // Each non-DelegatedRendererLayer added one RenderPass. The DelegatedRendererLayer added two contributing passes and its owned surface added one pass.
    ASSERT_EQ(6u, frame.renderPasses.size());

    // The layer's target is the RenderPass owned by itself.
    EXPECT_EQ(RenderPass::Id(4, 0), frame.renderPasses[3]->id);

    // The DelegatedRendererLayer should have added copies of quads in its root RenderPass to its target RenderPass.
    // The m_layerAfter also adds one quad.
    ASSERT_EQ(1u, frame.renderPasses[3]->quad_list.size());

    // Verify it added the right quads.
    EXPECT_RECT_EQ(gfx::Rect(7, 7, 7, 7), frame.renderPasses[3]->quad_list[0]->rect);

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

TEST_F(DelegatedRendererLayerImplTestOwnSurface, QuadsFromRootRenderPassAreNotModifiedForTheTarget)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    // Each non-DelegatedRendererLayer added one RenderPass. The DelegatedRendererLayer added two contributing passes and its owned surface added one pass.
    ASSERT_EQ(6u, frame.renderPasses.size());

    // Because the DelegatedRendererLayer owns a RenderSurfaceImpl, its root RenderPass' quads do not need to be
    // modified at all.
    EXPECT_TRANSFORMATION_MATRIX_EQ(gfx::Transform(), frame.renderPasses[3]->quad_list[0]->quadTransform());

    // Quads from non-root RenderPasses should not be shifted though.
    ASSERT_EQ(2u, frame.renderPasses[2]->quad_list.size());
    EXPECT_TRANSFORMATION_MATRIX_EQ(gfx::Transform(), frame.renderPasses[2]->quad_list[0]->quadTransform());
    EXPECT_TRANSFORMATION_MATRIX_EQ(gfx::Transform(), frame.renderPasses[2]->quad_list[1]->quadTransform());
    ASSERT_EQ(1u, frame.renderPasses[1]->quad_list.size());
    EXPECT_TRANSFORMATION_MATRIX_EQ(gfx::Transform(), frame.renderPasses[1]->quad_list[0]->quadTransform());

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

class DelegatedRendererLayerImplTestSharedData : public DelegatedRendererLayerImplTest {
public:
    DelegatedRendererLayerImplTestSharedData()
        : DelegatedRendererLayerImplTest()
    {
        scoped_ptr<LayerImpl> rootLayer = LayerImpl::create(m_hostImpl->activeTree(), 1);
        scoped_ptr<DelegatedRendererLayerImpl> delegatedRendererLayer = DelegatedRendererLayerImpl::create(m_hostImpl->activeTree(), 2);

        m_hostImpl->setViewportSize(gfx::Size(100, 100), gfx::Size(100, 100));
        rootLayer->setBounds(gfx::Size(100, 100));

        delegatedRendererLayer->setPosition(gfx::Point(20, 20));
        delegatedRendererLayer->setBounds(gfx::Size(20, 20));
        delegatedRendererLayer->setContentBounds(gfx::Size(20, 20));
        delegatedRendererLayer->setDrawsContent(true);
        gfx::Transform transform;
        transform.Translate(10, 10);
        delegatedRendererLayer->setTransform(transform);

        ScopedPtrVector<RenderPass> delegatedRenderPasses;
        gfx::Rect passRect(0, 0, 50, 50);
        TestRenderPass* pass = addRenderPass(delegatedRenderPasses, RenderPass::Id(9, 6), passRect, gfx::Transform());
        MockQuadCuller quadSink(pass->quad_list, pass->shared_quad_state_list);
        AppendQuadsData data(pass->id);
        SharedQuadState* sharedState = quadSink.useSharedQuadState(SharedQuadState::Create());
        sharedState->SetAll(gfx::Transform(), passRect, passRect, passRect, false, 1);
        scoped_ptr<SolidColorDrawQuad> colorQuad;

        colorQuad = SolidColorDrawQuad::Create();
        colorQuad->SetNew(sharedState, gfx::Rect(0, 0, 10, 10), 1u);
        quadSink.append(colorQuad.PassAs<DrawQuad>(), data);

        colorQuad = SolidColorDrawQuad::Create();
        colorQuad->SetNew(sharedState, gfx::Rect(0, 10, 10, 10), 2u);
        quadSink.append(colorQuad.PassAs<DrawQuad>(), data);

        colorQuad = SolidColorDrawQuad::Create();
        colorQuad->SetNew(sharedState, gfx::Rect(10, 0, 10, 10), 3u);
        quadSink.append(colorQuad.PassAs<DrawQuad>(), data);

        colorQuad = SolidColorDrawQuad::Create();
        colorQuad->SetNew(sharedState, gfx::Rect(10, 10, 10, 10), 4u);
        quadSink.append(colorQuad.PassAs<DrawQuad>(), data);

        delegatedRendererLayer->setRenderPasses(delegatedRenderPasses);

        // The RenderPasses should be taken by the layer.
        EXPECT_EQ(0u, delegatedRenderPasses.size());

        m_rootLayerPtr = rootLayer.get();
        m_delegatedRendererLayerPtr = delegatedRendererLayer.get();

        rootLayer->addChild(delegatedRendererLayer.PassAs<LayerImpl>());

        m_hostImpl->setRootLayer(rootLayer.Pass());
    }

protected:
    LayerImpl* m_rootLayerPtr;
    DelegatedRendererLayerImpl* m_delegatedRendererLayerPtr;
};

TEST_F(DelegatedRendererLayerImplTestSharedData, SharedData)
{
    LayerTreeHostImpl::FrameData frame;
    EXPECT_TRUE(m_hostImpl->prepareToDraw(frame));

    ASSERT_EQ(1u, frame.renderPasses.size());
    EXPECT_EQ(1, frame.renderPasses[0]->id.layer_id);
    EXPECT_EQ(0, frame.renderPasses[0]->id.index);

    const QuadList& quadList = frame.renderPasses[0]->quad_list;
    ASSERT_EQ(4u, quadList.size());

    // All quads should share the same state.
    const SharedQuadState* sharedState = quadList[0]->shared_quad_state;
    EXPECT_EQ(sharedState, quadList[1]->shared_quad_state);
    EXPECT_EQ(sharedState, quadList[2]->shared_quad_state);
    EXPECT_EQ(sharedState, quadList[3]->shared_quad_state);

    // The state should be transformed only once.
    EXPECT_RECT_EQ(gfx::Rect(30, 30, 50, 50), sharedState->clipped_rect_in_target);
    gfx::Transform expected;
    expected.Translate(30, 30);
    EXPECT_TRANSFORMATION_MATRIX_EQ(expected, sharedState->content_to_target_transform);

    m_hostImpl->drawLayers(frame);
    m_hostImpl->didDrawAllLayers(frame);
}

}  // namespace
}  // namespace cc
