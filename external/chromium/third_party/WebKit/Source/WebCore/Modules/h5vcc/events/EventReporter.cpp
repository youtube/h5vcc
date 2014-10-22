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
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) H./OWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "EventReporter.h"

#include <public/Platform.h>
#include <wtf/Assertions.h>

#if ENABLE(LB_SHELL_EVENT_REPORTING)

namespace WebCore {

// static
PassRefPtr<EventReporter> EventReporter::create() {
  return adoptRef(new EventReporter());
}

EventReporter::EventReporter()
    : impl_(WebKit::Platform::current()->h5vccEventReporter()) {
  ASSERT(impl_);
}

void EventReporter::traceMediaPlaybackSessionStart() {
  impl_->traceMediaPlaybackSessionStart();
}

void EventReporter::traceMediaPlaybackSessionEnd() {
  impl_->traceMediaPlaybackSessionEnd();
}

void EventReporter::traceMediaPlaybackSessionPause() {
  impl_->traceMediaPlaybackSessionPause();
}

void EventReporter::traceMediaPlaybackSessionResume() {
  impl_->traceMediaPlaybackSessionResume();
}

void EventReporter::traceChannelSubscribed() {
  impl_->traceChannelSubscribed();
}

void EventReporter::tracePairingComplete() {
  impl_->tracePairingComplete();
}

void EventReporter::traceSignedIn() {
  impl_->traceSignedIn();
}

void EventReporter::traceVideoLiked() {
  impl_->traceVideoLiked();
}

void EventReporter::traceVideoUploaded() {
  impl_->traceVideoUploaded();
}

void EventReporter::traceVideoWatched(uint32_t minutes,
                                      bool throughRemote,
                                      const String& genre,
                                      bool viewsBelowThreshold) {
  impl_->traceVideoWatched(minutes, throughRemote, genre, viewsBelowThreshold);
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_EVENT_REPORTING)
