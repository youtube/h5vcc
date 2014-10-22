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

#ifndef RemoteMediaInfo_h
#define RemoteMediaInfo_h

#include "config.h"

#include <wtf/OSAllocator.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

#include "RemoteMediaScrollEventCallback.h"

namespace JSC { class JSObject; }

namespace WebCore {
class RemoteMediaInfo;
class RemoteMediaScrollEventCallback;
class ScriptExecutionContext;
}  // namespace WebCore

namespace H5vcc {

class RemoteController {
 public:
  virtual ~RemoteController() { }

  virtual void setMediaInfo(WebCore::RemoteMediaInfo* info) = 0;
  virtual void setCapabilitiesEnabled(unsigned short bitmask) = 0;
  virtual void clearMediaInfo() = 0;

#if ENABLE(LB_SHELL_REMOTE_CONTROL)
  void setSeekCallback(
      WebCore::ScriptExecutionContext* context,
      PassRefPtr<WebCore::RemoteMediaScrollEventCallback> callback) {
    m_context = context;
    m_seekCallback = callback;
  }

  WEBKIT_EXPORT void InvokeSeekCallback(unsigned seekTime);

 private:
  WebCore::ScriptExecutionContext* m_context;
  RefPtr<WebCore::RemoteMediaScrollEventCallback> m_seekCallback;
#endif // ENABLE(LB_SHELL_REMOTE_CONTROL)
};

}  // namespace H5vcc

#if ENABLE(LB_SHELL_REMOTE_CONTROL)

namespace WebCore {

class RemoteMediaInfo : public RefCounted<RemoteMediaInfo> {
 public:
  // The following constants, although defined also in the IDL, are kept in
  // sync via COMPILE_ASSERT in generated code.

  // constants for RemoteMediaInfo status:
  static const unsigned short PLAYBACK_CLOSED = 0;
  static const unsigned short PLAYBACK_CHANGING = 1;
  static const unsigned short PLAYBACK_STOPPED = 2;
  static const unsigned short PLAYBACK_PLAYING = 3;
  static const unsigned short PLAYBACK_PAUSED = 4;

  // constants for RemoteMediaInfo capabilities:
  static const unsigned short ENABLED_NONE = 0;
  static const unsigned short ENABLED_PLAY = 1;
  static const unsigned short ENABLED_PAUSE = 2;
  static const unsigned short ENABLED_PREVIOUS = 4;
  static const unsigned short ENABLED_NEXT = 8;
  static const unsigned short ENABLED_STOP = 16;
  static const unsigned short ENABLED_FASTFORWARD = 32;
  static const unsigned short ENABLED_REWIND = 64;
  static const unsigned short ENABLED_CHANNELUP = 128;
  static const unsigned short ENABLED_CHANNELDOWN = 256;
  static const unsigned short ENABLED_BACK = 512;
  static const unsigned short ENABLED_VIEW = 1024;
  static const unsigned short ENABLED_MENU = 2048;

  virtual ~RemoteMediaInfo() { }

  static void setMediaInfo(RemoteMediaInfo* info);
  static void setCapabilitiesEnabled(unsigned short bitmask);
  static void setSeekCallback(
      ScriptExecutionContext*, PassRefPtr<RemoteMediaScrollEventCallback>);
  static void clearMediaInfo();

  static PassRefPtr<RemoteMediaInfo> create();

  unsigned short status() const { return m_status; }
  WTF::String appMediaId() const { return m_appMediaId; }
  WTF::String videoTitle() const { return m_videoTitle; }
  WTF::String videoSubtitle() const { return m_videoSubtitle; }
  WTF::String thumbnailUrl() const { return m_thumbnailUrl; }
  float playbackRate() const { return m_playbackRate; }
  long playbackPosition() const { return m_playbackPosition; }
  long mediaStartTime() const { return m_mediaStartTime; }
  long mediaEndTime() const { return m_mediaEndTime; }
  long minSeekTime() const { return m_minSeekTime; }
  long maxSeekTime() const { return m_maxSeekTime; }

  void setStatus(short status) { m_status = status; }
  void setAppMediaId(const WTF::String& appMediaId) { m_appMediaId = appMediaId; }
  void setVideoTitle(const WTF::String& videoTitle) { m_videoTitle = videoTitle; }
  void setVideoSubtitle(const WTF::String& videoSubtitle) { m_videoSubtitle = videoSubtitle; }
  void setThumbnailUrl(const WTF::String& thumbnailUrl) { m_thumbnailUrl = thumbnailUrl; }
  void setPlaybackRate(float playbackRate) { m_playbackRate = playbackRate; }
  void setPlaybackPosition(long playbackPosition) { m_playbackPosition = playbackPosition; }
  void setMediaStartTime(long mediaStartTime) { m_mediaStartTime = mediaStartTime; }
  void setMediaEndTime(long mediaEndTime) { m_mediaEndTime = mediaEndTime; }
  void setMinSeekTime(long minSeekTime) { m_minSeekTime = minSeekTime; }
  void setMaxSeekTime(long maxSeekTime) { m_maxSeekTime = maxSeekTime; }

 private:
  static void init();

  RemoteMediaInfo();

  // This pointer is owned by LBShellWebKitInit.
  static H5vcc::RemoteController* impl_;

  unsigned short m_status;
  WTF::String m_appMediaId;
  WTF::String m_videoTitle;
  WTF::String m_videoSubtitle;
  WTF::String m_thumbnailUrl;
  float m_playbackRate;
  long m_playbackPosition;
  long m_mediaStartTime;
  long m_mediaEndTime;
  long m_minSeekTime;
  long m_maxSeekTime;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_REMOTE_CONTROL)

#endif  // RemoteMediaInfo_h
