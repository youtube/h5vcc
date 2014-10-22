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

#ifndef HomePin_h
#define HomePin_h

#include "config.h"
#include <wtf/OSAllocator.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore{
class BoolCallback;
class HomePin;
class ScriptExecutionContext;
}  // namespace WebCore

namespace H5vcc {

class HomePinManager {
 public:
  virtual ~HomePinManager() {}
  virtual void AddToHome(WTF::PassRefPtr<WebCore::HomePin> pin) = 0;
  virtual void RemoveFromHome(WTF::PassRefPtr<WebCore::HomePin> pin) = 0;
  virtual void CheckIfPinned(WebCore::ScriptExecutionContext* context,
      WTF::PassRefPtr<WebCore::HomePin> pin,
      WTF::PassRefPtr<WebCore::BoolCallback> callback) = 0;
};

}  // namespace H5vcc

#if ENABLE(LB_SHELL_HOME_PINNING)

namespace WebCore {

class HomePin : public RefCounted<HomePin> {
 public:
  static PassRefPtr<HomePin> create(const String& contentId);

  // getters
  const String& contentId() const { return m_contentId; }
  const String& contentType() const { return m_contentType; }
  const String& imageUrl() const { return m_imageUrl; }
  const String& title() const { return m_title; }
  const String& subTitle() const { return m_subTitle; }

  // setters
  void setContentType(const String& contentType) {
    m_contentType = contentType;
  }
  void setImageUrl(const String& imageUrl) { m_imageUrl = imageUrl; }
  void setTitle(const String& title) { m_title = title; }
  void setSubTitle(const String& subTitle) { m_subTitle = subTitle; }

  void add();
  void remove();
  void isPinned(ScriptExecutionContext* context,
                const RefPtr<BoolCallback>& callback);

  // static getters
  WEBKIT_EXPORT static const String& providerName() { return s_providerName; }
  WEBKIT_EXPORT static const String& providerLogoUrl() {
    return s_providerLogoUrl;
  }

  // static setters
  WEBKIT_EXPORT static void setProviderName(const String& providerName) {
    s_providerName = providerName;
  }

  WEBKIT_EXPORT static void setProviderLogoUrl(const String& providerLogoUrl) {
    s_providerLogoUrl = providerLogoUrl;
  }

  WEBKIT_EXPORT static void ScheduleCallback(
      ScriptExecutionContext* context,
      const RefPtr<BoolCallback>& callback,
      bool value);

 private:
  explicit HomePin(const String& contentId);

  const String m_contentId;
  String m_contentType;
  String m_imageUrl;
  String m_title;
  String m_subTitle;
  String m_locale;

  static String s_providerName;
  static String s_providerLogoUrl;

  H5vcc::HomePinManager* manager_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_HOME_PINNING)
#endif  // HomePin_h
