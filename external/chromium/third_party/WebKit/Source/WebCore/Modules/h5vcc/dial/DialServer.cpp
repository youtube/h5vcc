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
#include "DialServer.h"

#if ENABLE(DIAL_SERVER)

#include "ScriptExecutionContext.h"
#include "DialHttpRequestCallback.h"
#include "DialHttpRequest.h"
#include "DialHttpResponse.h"
#include <wtf/Assertions.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

#include "base/callback.h"
#include "net/dial/dial_service_handler.h"
#include "net/server/http_server_request_info.h"

namespace net {
class DialService {
 public:
  static DialService* GetInstance();
  bool Register(DialServiceHandler*);
  bool Deregister(DialServiceHandler*);
  std::string GetHttpHostAddress() const;
};
}

std::string WTFToStdString(const String& str) {
  CString cstr = str.utf8();
  return std::string(cstr.data(), cstr.length());
}

String StdToWTFString(const std::string& str) {
  return String::fromUTF8(str.data(), str.length());
}

namespace WebCore {

class HttpRequestHandlerCallbackTask : public ScriptExecutionContext::Task {
 public:
  static PassOwnPtr<HttpRequestHandlerCallbackTask> create(
      PassRefPtr<DialHttpRequestCallback> callback,
      PassRefPtr<DialHttpRequest> request,
      PassRefPtr<DialHttpResponse> response,
      net::HttpServerResponseInfo* server_response,
      base::Callback<void(bool)> on_completion) {
    return adoptPtr(new HttpRequestHandlerCallbackTask(callback,
                                                       request,
                                                       response,
                                                       server_response,
                                                       on_completion));
  }

  virtual void performTask(ScriptExecutionContext*) OVERRIDE {
    // FIXME: The handleEvent return does not correspond to the
    // Javascript return value, but to whether an exception has been raised.
    // We need to fix this misleading code path, but doing that requires
    // too many changes for now.
    bool ret = m_callback->handleEvent(m_request.get(), m_response.get());
    FillResponse();
    m_on_completion.Run(ret);
  }

 private:
  HttpRequestHandlerCallbackTask(
      PassRefPtr<DialHttpRequestCallback> callback,
      PassRefPtr<DialHttpRequest> request,
      PassRefPtr<DialHttpResponse> response,
      net::HttpServerResponseInfo* server_response,
      base::Callback<void(bool)> on_completion)
    : m_callback(callback)
    , m_request(request)
    , m_response(response)
    , m_server_response(server_response)
    , m_on_completion(on_completion) {
  }

  // copy from DialHttpResponse to net::HttpServerResponseInfo
  void FillResponse() {
    m_server_response->response_code = m_response->responseCode();
    m_server_response->body = WTFToStdString(m_response->body());
    m_server_response->mime_type = WTFToStdString(m_response->mimeType());

    for (Vector<String>::const_iterator it = m_response->headers().begin();
         it != m_response->headers().end();
         ++it) {
      m_server_response->headers.push_back(WTFToStdString(*it));
    }
  }

  RefPtr<DialHttpRequestCallback> m_callback;
  RefPtr<DialHttpRequest> m_request;
  RefPtr<DialHttpResponse> m_response;
  net::HttpServerResponseInfo* m_server_response;
  base::Callback<void(bool)> m_on_completion;
};

// static
PassRefPtr<DialServer> DialServer::create(
    ScriptExecutionContext* context,
    const String& serviceName,
    ExceptionCode& ec) {
  if (serviceName.isEmpty()) {
    ec = NOT_SUPPORTED_ERR;
    return 0;
  }
  RefPtr<DialServer> server = adoptRef(new DialServer(context, serviceName));
  if (!net::DialService::GetInstance()->Register(server.get())) {
    ec = NOT_SUPPORTED_ERR;
    return 0;
  }
  server->suspendIfNeeded();
  return server.release();
}

DialServer::DialServer(ScriptExecutionContext* context,
                       const String& serviceName)
    : ActiveDOMObject(context, this)
    , m_serviceName(serviceName) {
}

DialServer::~DialServer() {
}

void DialServer::stop() {
  net::DialService::GetInstance()->Deregister(this);
}

const std::string DialServer::service_name() const {
  return WTFToStdString(serviceName());
}

bool DialServer::AddService(const String& path, const std::string& method,
                            PassRefPtr<DialHttpRequestCallback> callback) {
  if (path.isEmpty() || !callback.get()) {
    return false;
  }

  // Does not replace the key, and accordingly returns value.
  CallbackRegistry::AddResult result =
      m_callbackMap.add(std::make_pair(path, StdToWTFString(method)),
                        callback);
  return result.isNewEntry;
}

// called by DialService. Return true if DialServer can handle it, and then
// call the callback when it is actually done.
bool DialServer::handleRequest(const std::string& path,
     const net::HttpServerRequestInfo& req,
     net::HttpServerResponseInfo* resp,
     const base::Callback<void(bool)>& on_completion) {

  PassRefPtr<DialHttpRequestCallback> result =
      m_callbackMap.get(std::make_pair(StdToWTFString(path),
                                       StdToWTFString(req.method)));

  if (!result.get()) {
    return false;
  }

  RefPtr<DialHttpRequest> request = adoptRef(new DialHttpRequest(
      StdToWTFString(req.path),
      StdToWTFString(req.method),
      StdToWTFString(req.data),
      StdToWTFString(net::DialService::GetInstance()->GetHttpHostAddress())));
  RefPtr<DialHttpResponse> response = adoptRef(new DialHttpResponse());

  ASSERT(scriptExecutionContext());
  scriptExecutionContext()->postTask(
      HttpRequestHandlerCallbackTask::create(result,
                                             request.release(),
                                             response.release(),
                                             resp,
                                             on_completion));
  return true;
}

}
#endif // ENABLE(DIAL_SERVER)
