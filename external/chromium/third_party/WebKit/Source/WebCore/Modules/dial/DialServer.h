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

#ifndef DialServer_h
#define DialServer_h

#include "Dictionary.h"
#include "ExceptionCode.h"
#include "DialHttpRequestCallback.h"
#include "ScriptExecutionContext.h"

#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

#include "net/dial/dial_service_handler.h"

namespace WebCore {

class DialHttpRequest;
class DialHttpResponse;

class DialServer : public RefCounted<DialServer>,
    public net::DialServiceHandler, public ActiveDOMObject {
 public:
  static PassRefPtr<DialServer> create(ScriptExecutionContext*,
                                       const String&,
                                       ExceptionCode&);
  virtual ~DialServer();

  const String& serviceName() const { return m_serviceName; }

  bool onGet(const String& path, PassRefPtr<DialHttpRequestCallback> callback) {
    return AddService(path, "GET", callback);
  }

  bool onPost(const String& path,
              PassRefPtr<DialHttpRequestCallback> callback) {
    return AddService(path, "POST", callback);
  }

  // net::DialServiceHandler implementation.
  virtual bool handleRequest(const std::string& path,
      const net::HttpServerRequestInfo& req,
      net::HttpServerResponseInfo* resp,
      const base::Callback<void(bool)>& on_completion) OVERRIDE;
  virtual const std::string service_name() const OVERRIDE;

  // ActiveDOMObject implementation.
  virtual void stop() OVERRIDE;

 private:
  DialServer(ScriptExecutionContext*, const String&);

  bool AddService(const String& path, const std::string& method,
                  PassRefPtr<DialHttpRequestCallback>);

  const String m_serviceName;

  // |CallbackRegistry| is a map to store the registered paths. It's value
  // is a specific JS callback, the key is a pair of path, http_method.
  // Note: wtf does not have hash functions for CString or std::string, so we
  // store the HTTP method in String instead of a simpler storage container.
  typedef WTF::HashMap<std::pair<String, String>,
                       RefPtr<DialHttpRequestCallback> > CallbackRegistry;
  CallbackRegistry m_callbackMap;
};

} // namespace WebCore

#endif // DialServer_h

