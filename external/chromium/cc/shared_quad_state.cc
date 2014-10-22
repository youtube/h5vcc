// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/shared_quad_state.h"

#if defined(__LB_SHELL__)
#include <vector>

static std::vector<void*> s_quad_state_reuse_buffer;
#endif

namespace cc {

SharedQuadState::SharedQuadState() : opacity(0) {}

SharedQuadState::~SharedQuadState() {}

#if defined(__LB_SHELL__)
void SharedQuadState::operator delete(void* p) {
  s_quad_state_reuse_buffer.push_back(p);
}
#endif

scoped_ptr<SharedQuadState> SharedQuadState::Create() {
#if defined(__LB_SHELL__)
  if (s_quad_state_reuse_buffer.size() > 0) {
    void* block = s_quad_state_reuse_buffer.back();
    s_quad_state_reuse_buffer.pop_back();
    return make_scoped_ptr(new (block) SharedQuadState);
  }
#endif
  return make_scoped_ptr(new SharedQuadState);
}

scoped_ptr<SharedQuadState> SharedQuadState::Copy() const {
#if defined(__LB_SHELL__)
  if (s_quad_state_reuse_buffer.size() > 0) {
    void* block = s_quad_state_reuse_buffer.back();
    s_quad_state_reuse_buffer.pop_back();
    return make_scoped_ptr(new (block) SharedQuadState(*this));
  }
#endif
  return make_scoped_ptr(new SharedQuadState(*this));
}

void SharedQuadState::SetAll(
    const gfx::Transform& content_to_target_transform,
    const gfx::Rect& visible_content_rect,
    const gfx::Rect& clipped_rect_in_target,
    const gfx::Rect& clip_rect,
    bool is_clipped,
    float opacity) {
  this->content_to_target_transform = content_to_target_transform;
  this->visible_content_rect = visible_content_rect;
  this->clipped_rect_in_target = clipped_rect_in_target;
  this->clip_rect = clip_rect;
  this->is_clipped = is_clipped;
  this->opacity = opacity;
}

#if defined(ENABLE_LB_SHELL_CSS_EXTENSIONS) && ENABLE_LB_SHELL_CSS_EXTENSIONS
void SharedQuadState::SetAll(
    const gfx::Transform& content_to_target_transform,
    const gfx::Rect& visible_content_rect,
    const gfx::Rect& clipped_rect_in_target,
    const gfx::Rect& clip_rect,
    bool is_clipped,
    float opacity,
    WebKit::H5VCCTargetScreen h5vcc_target_screen) {
  SetAll(content_to_target_transform,
         visible_content_rect,
         clipped_rect_in_target,
         clip_rect,
         is_clipped,
         opacity);
  this->h5vcc_target_screen = h5vcc_target_screen;
}
#endif

}  // namespace cc
