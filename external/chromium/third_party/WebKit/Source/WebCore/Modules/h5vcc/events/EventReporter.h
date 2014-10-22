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

#ifndef SOURCE_WEBCORE_MODULES_H5VCC_EVENTS_EVENTREPORTER_H_
#define SOURCE_WEBCORE_MODULES_H5VCC_EVENTS_EVENTREPORTER_H_

#include <stdint.h>

#include "config.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace H5vcc {

class EventReporter {
 public:
  virtual ~EventReporter() { }

  virtual void traceMediaPlaybackSessionStart() = 0;
  virtual void traceMediaPlaybackSessionEnd() = 0;
  virtual void traceMediaPlaybackSessionPause() = 0;
  virtual void traceMediaPlaybackSessionResume() = 0;

  virtual void traceChannelSubscribed() = 0;
  virtual void tracePairingComplete() = 0;
  virtual void traceSignedIn() = 0;
  virtual void traceVideoLiked() = 0;
  virtual void traceVideoUploaded() = 0;
  virtual void traceVideoWatched(uint32_t minutes,
                                 bool throughRemote,
                                 const String& genre,
                                 bool viewsBelowThreshold) = 0;
};
}  // namespace H5vcc

#if ENABLE(LB_SHELL_EVENT_REPORTING)
namespace WebCore {

class EventReporter : public RefCounted<EventReporter> {
 public:
  static PassRefPtr<EventReporter> create();

  void traceMediaPlaybackSessionStart();
  void traceMediaPlaybackSessionEnd();
  void traceMediaPlaybackSessionPause();
  void traceMediaPlaybackSessionResume();

  void traceChannelSubscribed();
  void tracePairingComplete();
  void traceSignedIn();
  void traceVideoLiked();
  void traceVideoUploaded();
  void traceVideoWatched(uint32_t minutes,
                         bool throughRemote,
                         const String& genre,
                         bool viewsBelowThreshold);

 private:
  EventReporter();

  H5vcc::EventReporter *impl_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_EVENT_REPORTING)
#endif  // SOURCE_WEBCORE_MODULES_H5VCC_EVENTS_EVENTREPORTER_H_
