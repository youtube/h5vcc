/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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
#include "DvrClip.h"

#if ENABLE(LB_SHELL_DVR)

#include "DvrClipContext.h"

namespace WebCore {

// static
PassRefPtr<DvrClip> DvrClip::create(PassRefPtr<H5vcc::DvrClip> h5vcc_clip) {
  ASSERT(h5vcc_clip);

  RefPtr<DvrClip> clip = adoptRef(new DvrClip());
  clip->clip_id_ = h5vcc_clip->clip_id();
  clip->clip_name_ = h5vcc_clip->clip_name();
  clip->clip_visibility_ = h5vcc_clip->clip_visibility();
  clip->date_recorded_ = h5vcc_clip->date_recorded();
  clip->duration_sec_ = h5vcc_clip->duration_sec();
  clip->is_remote_ = h5vcc_clip->is_remote();
  clip->title_name_ = h5vcc_clip->title_name();
  clip->user_caption_ = h5vcc_clip->user_caption();
  clip->title_data_ = h5vcc_clip->title_data();

  return clip;
}

DvrClip::DvrClip()
    : date_recorded_(0)
    , duration_sec_(0)
    , is_remote_(false) {}

PassRefPtr<DvrClipContext> DvrClip::acquireContext(
    ScriptExecutionContext* context) {
  return DvrClipContext::create(context, clip_id_);
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_DVR)
