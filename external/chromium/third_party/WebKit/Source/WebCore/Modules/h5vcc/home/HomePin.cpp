/*
 * Copyright (C) 2014 Google Inc.  All rights reserved.
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
#include "HomePin.h"

#include <public/Platform.h>
#include "BoolCallback.h"
#include "CrossThreadEventDispatcher.h"
#include "ScriptExecutionContext.h"

#if ENABLE(LB_SHELL_HOME_PINNING)

namespace WebCore {

// static
String HomePin::s_providerName = "";

// static
String HomePin::s_providerLogoUrl = "";

// static
void HomePin::ScheduleCallback(ScriptExecutionContext* context,
    const RefPtr<BoolCallback>& callback, bool value) {
  ASSERT(context);
  context->postTask(createEventDispatcherTask(callback, value));
}

// static
PassRefPtr<HomePin> HomePin::create(const String& contentId) {
  return adoptRef(new HomePin(contentId));
}

HomePin::HomePin(const String& contentId)
    : m_contentId(contentId)
    , manager_(WebKit::Platform::current()->h5vccHomePinManager()) {
  ASSERT(manager_);
}

void HomePin::add() {
  if (manager_) {
    manager_->AddToHome(this);
  }
}

void HomePin::remove() {
  if (manager_) {
    manager_->RemoveFromHome(this);
  }
}

void HomePin::isPinned(ScriptExecutionContext* context,
                       const RefPtr<BoolCallback>& callback) {
  if (manager_) {
    manager_->CheckIfPinned(context, this, callback);
  }
}

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_HOME_PINNING)
