/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_LB_HTTP_HANDLER_H_
#define SRC_LB_HTTP_HANDLER_H_

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

#include "external/chromium/base/bind.h"
#include "external/chromium/base/threading/thread.h"
#include "external/chromium/net/server/http_server.h"
#include "net/url_request/url_request.h"

#include "lb_devtools_agent.h"
#include "lb_generated_resources_types.h"
#include "lb_web_view_host.h"


namespace net {
class TCPListenSocketFactory;
}

class LBHttpHandler
    : public net::HttpServer::Delegate,
      public base::RefCountedThreadSafe<LBHttpHandler> {
 public:
  LBHttpHandler(LBWebViewHost *host);
  virtual ~LBHttpHandler();

  void SendOverWebSocket(const int connection_id, const std::string &data) const;

 private:
  void CreateServer();
  void DoSendOverWebSocket(const int connection_id, const std::string &data) const;
  void SendFile(const int connection_id, const std::string &path) const;

  // net::HttpServer::Delegate implementation.
  virtual void OnHttpRequest(const int connection_id,
                             const net::HttpServerRequestInfo& info) OVERRIDE;

  virtual void OnWebSocketRequest(const int connection_id,
                                  const net::HttpServerRequestInfo& info) OVERRIDE;

  virtual void OnWebSocketMessage(const int connection_id,
                                  const std::string& data) OVERRIDE;

  virtual void OnClose(const int connection_id) OVERRIDE;

  scoped_ptr<base::Thread> thread_;
  scoped_ptr<net::TCPListenSocketFactory> factory_;
  scoped_refptr<net::HttpServer> server_;  // can only be deleted via ref count

  LBWebViewHost *host_;  // not owned
  LBDevToolsAgent *agent_;  // owned, but cleaned up by calls from outside

  GeneratedResourceMap inspector_resource_map_;
};

#endif // __LB_SHELL__ENABLE_CONSOLE__
#endif // SRC_LB_HTTP_HANDLER_H_
