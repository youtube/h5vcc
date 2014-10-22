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
#include "RemoteMediaInfo.h"

#if ENABLE(LB_SHELL_REMOTE_CONTROL)

#include <public/Platform.h>
#include <wtf/RefPtr.h>

#include "CrossThreadEventDispatcher.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

// static
H5vcc::RemoteController* RemoteMediaInfo::impl_ = NULL;

// static
void RemoteMediaInfo::init()
{
    if (!impl_) {
        impl_ = WebKit::Platform::current()->h5vccRemoteController();
    }
    ASSERT(impl_);
}

// static
void RemoteMediaInfo::setMediaInfo(RemoteMediaInfo* info)
{
    init();
    impl_->setMediaInfo(info);
}

// static
void RemoteMediaInfo::setCapabilitiesEnabled(unsigned short bitmask)
{
    init();
    impl_->setCapabilitiesEnabled(bitmask);
}

// static
void RemoteMediaInfo::setSeekCallback(
        ScriptExecutionContext* context,
        PassRefPtr<RemoteMediaScrollEventCallback> callback)
{
    init();
    impl_->setSeekCallback(context, callback);
}

// static
void RemoteMediaInfo::clearMediaInfo()
{
    init();
    impl_->clearMediaInfo();
}

// static
PassRefPtr<RemoteMediaInfo> RemoteMediaInfo::create() {
    return adoptRef(new RemoteMediaInfo());
}

RemoteMediaInfo::RemoteMediaInfo()
    : m_status(0)
    , m_playbackRate(0.0f)
    , m_playbackPosition(0L)
    , m_mediaStartTime(0L)
    , m_mediaEndTime(0L)
    , m_minSeekTime(0L)
    , m_maxSeekTime(0L)
{
}

}  // namespace WebCore

namespace H5vcc {

void RemoteController::InvokeSeekCallback(unsigned seekTime)
{
    if (!m_seekCallback || !m_context)
        return;
    m_context->postTask(WebCore::createEventDispatcherTask(
        m_seekCallback, seekTime));
}

}  // namespace H5vcc

#endif  // ENABLE(LB_SHELL_REMOTE_CONTROL)
