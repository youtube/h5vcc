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

#if ENABLE_INSPECTOR

#include "external/chromium/base/bind.h"
#include "external/chromium/base/threading/thread.h"
#include "external/chromium/net/server/http_server.h"
#include "net/url_request/url_request.h"

#include "lb_console_values.h"
#include "lb_devtools_agent.h"
#include "lb_generated_resources_types.h"
#include "lb_web_view_host.h"


namespace net {
class TCPListenSocketFactory;
}

class LBHttpHandler : public net::HttpServer::Delegate {
 public:
  explicit LBHttpHandler(LBWebViewHost *host);
  virtual ~LBHttpHandler();

  void SendOverWebSocket(
      const int connection_id, const std::string &data) const;

 private:
  void CreateServer();
  void DoClose(const int connection_id);
  void DoSendOverWebSocket(
      const int connection_id, const std::string &data) const;
  void SendFile(const int connection_id, const std::string &path) const;
  bool GetWebInspectorPort(int* port);

  // net::HttpServer::Delegate implementation.
  virtual void OnHttpRequest(const int connection_id,
                             const net::HttpServerRequestInfo& info) OVERRIDE;

  virtual void OnWebSocketRequest(
      const int connection_id,
      const net::HttpServerRequestInfo& info) OVERRIDE;

  virtual void OnWebSocketMessage(const int connection_id,
                                  const std::string& data) OVERRIDE;

  virtual void OnClose(const int connection_id) OVERRIDE;

  scoped_ptr<base::Thread> thread_;
  scoped_ptr<net::StreamListenSocketFactory> factory_;
  scoped_refptr<net::HttpServer> server_;  // can only be deleted via ref count

  LBWebViewHost *host_;  // not owned
  scoped_refptr<LBDevToolsAgent> agent_;

  GeneratedResourceMap inspector_resource_map_;

  LB::CVal<std::string> console_display_;
};

#endif  // ENABLE_INSPECTOR
#endif  // SRC_LB_HTTP_HANDLER_H_
