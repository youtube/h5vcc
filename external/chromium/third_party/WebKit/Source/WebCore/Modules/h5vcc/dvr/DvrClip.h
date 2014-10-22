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

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRCLIP_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRCLIP_H_

#include "config.h"

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

#include "DvrManager.h"

#if ENABLE(LB_SHELL_DVR)

namespace WebCore {

class DvrClipContext;
class ScriptExecutionContext;

class DvrClip : public RefCounted<DvrClip> {
 public:
  virtual ~DvrClip() {}

  static PassRefPtr<DvrClip> create(PassRefPtr<H5vcc::DvrClip> h5vcc_clip);

  // IDL methods
  PassRefPtr<DvrClipContext> acquireContext(ScriptExecutionContext* context);

  // IDL attribute getters
  const String& clipId() const { return clip_id_; }
  const String& clipName() const { return clip_name_; }
  const String& clipVisibility() const { return clip_visibility_; }
  const double dateRecorded() const { return date_recorded_; }
  const unsigned int durationSec() const { return duration_sec_; }
  const bool isRemote() const { return is_remote_; }
  const String& titleName() const { return title_name_; }

  // Candidates for removal in the final version
  const String& userCaption() const { return user_caption_; }
  const String& titleData() const { return title_data_; }

 private:
  DvrClip();

  String clip_id_;
  String clip_name_;
  String clip_visibility_;
  double date_recorded_;
  unsigned int duration_sec_;
  bool is_remote_;
  String title_name_;

  // Candidates for removal in the final version
  String user_caption_;
  String title_data_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_DVR)

#endif  // SOURCE_WEBCORE_MODULES_H5VCC_DVR_DVRCLIP_H_
