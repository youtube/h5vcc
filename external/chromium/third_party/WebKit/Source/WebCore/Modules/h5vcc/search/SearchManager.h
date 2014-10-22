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

#ifndef SearchManager_h
#define SearchManager_h

#include "config.h"

#include <wtf/OSAllocator.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#include "DOMStringList.h"
#include "ScriptExecutionContext.h"

namespace WebCore {
class SearchManager;
class StringCallback;
}  // namespace WebCore

namespace H5vcc {

class SearchManager {
 public:
  virtual ~SearchManager() {}
  virtual void DoBindSearchBarService() = 0;
  virtual void DoUnbindSearchBarService() = 0;
  virtual void FocusToSearchBar() = 0;
  virtual void SendSearchTerm(const WTF::String& searchTerm) = 0;
  virtual void SetSuggestions(
      const PassRefPtr<WebCore::DOMStringList> suggestions) = 0;

  // Called by WebKit whenever a new instance of WebCore::SearchManager is
  // created. Can be called before a previous instance was garbage collected.
  virtual void AddWebKitInstance(WebCore::SearchManager* inst) = 0;

  // Called by WebKit whenever a instance of WebCore::SearchManager is removed.
  // Will be called after the instance was garbage collected.
  virtual void RemoveWebKitInstance(WebCore::SearchManager* inst) = 0;

};

}  // namespace H5vcc

#if ENABLE(LB_SHELL_SEARCH_BAR)

namespace WebCore {

class SearchManager : public RefCounted<SearchManager> {
 public:
  virtual ~SearchManager();

  static PassRefPtr<SearchManager> create(ScriptExecutionContext* context);

  void doBindSearchBarService(PassRefPtr<StringCallback> callback);
  void doUnbindSearchBarService();
  void focusToSearchBar();
  void setSuggestions(const PassRefPtr<WebCore::DOMStringList> suggestions);

  WEBKIT_EXPORT void invokeSearchResultCallback(const WTF::String& searchTerm);
  WEBKIT_EXPORT void releaseReferences();

 private:
  explicit SearchManager(ScriptExecutionContext* context);

  RefPtr<StringCallback> searchResultCallback_;

  // This pointer is owned by LBShellWebKitInit.
  H5vcc::SearchManager* search_impl_;
  ScriptExecutionContext* context_;
};

}  // namespace WebCore

#endif  // ENABLE(LB_SHELL_SEARCH_BAR)

#endif  // SearchManager_h
