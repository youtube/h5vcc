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
#if defined(__LB_SHELL__ENABLE_CONSOLE__)

#include "lb_http_handler.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base/stringprintf.h"
#include "base/string_util.h"
#include "net/base/tcp_listen_socket.h"
#include "net/server/http_server_request_info.h"

#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDevToolsAgent.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDevToolsFrontend.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebString.h"

#include "lb_inspector_resources.h"
#include "lb_memory_manager.h"

LBHttpHandler::LBHttpHandler(LBWebViewHost *host) : host_(host),
    agent_(NULL) {
  // Create thread for http server
  thread_.reset(LB_NEW base::Thread("http_handler"));
  thread_->StartWithOptions(base::Thread::Options(MessageLoop::TYPE_IO, 0));

  thread_->message_loop()->PostTask(FROM_HERE, base::Bind(
      &LBHttpHandler::CreateServer, this));
}

LBHttpHandler::~LBHttpHandler() {
  thread_->Stop();
  DCHECK(!agent_) << "Agent should have been cleaned up by now!";
}

void LBHttpHandler::CreateServer() {
  DCHECK_EQ(MessageLoop::current(), thread_->message_loop());

  // Populate Map
  LBInspectorResource::GenerateMap(inspector_resource_map_);

  // Create http server
  factory_.reset(LB_NEW net::TCPListenSocketFactory("0.0.0.0", 9222));
  server_ = LB_NEW net::HttpServer(*factory_, this);
}

// From external/chromium/content/browser/debugger/devtools_http_handler.cc
static std::string GetMimeType(const std::string& filename) {
  if (EndsWith(filename, ".html", false)) {
    return "text/html";
  } else if (EndsWith(filename, ".css", false)) {
    return "text/css";
  } else if (EndsWith(filename, ".js", false)) {
    return "application/javascript";
  } else if (EndsWith(filename, ".png", false)) {
    return "image/png";
  } else if (EndsWith(filename, ".gif", false)) {
    return "image/gif";
  }
  NOTREACHED();
  return "text/plain";
}


void LBHttpHandler::SendFile(const int connection_id,
                             const std::string &path) const {
  DCHECK_EQ(MessageLoop::current(), thread_->message_loop());
  extern std::string *global_game_content_path;
  std::string full_path = *global_game_content_path;
  full_path.append(path);

  // If path is in the "inspector" path, use the special embedded files
  // located in inspector_resources.h
  if (path.find("/inspector/") == 0) {
    std::string filename = path.substr(strlen("/inspector/"));
    GeneratedResourceMap::const_iterator itr =
        inspector_resource_map_.find(filename);
    if (itr != inspector_resource_map_.end()) {
      std::string data(itr->second.data, itr->second.size);
      server_->Send200(connection_id, data, GetMimeType(path));
    }
    return;
  }

  // try to open the file
  int fd = open(full_path.c_str(), O_RDONLY, 0);
  if (fd == -1) {
    server_->Send404(connection_id);
    return;
  }

  DLOG(WARNING) << "Files should be served off the resource pack, "
                   "and not from individual files.";

  // get the file size
  struct stat file_stats;
  {
    int ret = fstat(fd, &file_stats);
    if (ret < 0) {
      server_->Send404(connection_id);
      close(fd);
      return;
    }
    DCHECK_LT(0, file_stats.st_size);
  }
  size_t size = file_stats.st_size;

  // copy the file contents to a string
  std::string contents;
  {
    contents.reserve(size);
    ssize_t ret = read(fd, const_cast<char*>(contents.data()), size);
    DCHECK_EQ(ret, size);
    DCHECK_EQ(size, contents.length());
    close(fd);
  }
  // send that as the HTTP response.
  server_->Send200(connection_id, contents, GetMimeType(path));
}

void LBHttpHandler::OnHttpRequest(const int connection_id,
                                  const net::HttpServerRequestInfo& info) {
  DCHECK_EQ(MessageLoop::current(), thread_->message_loop());
  std::string path;

  // Discard any URL variables from the path
  size_t query_position = info.path.find("?");
  if (query_position != std::string::npos)
    path = info.path.substr(0, query_position);
  else
    path = info.path;

  if (path == "" || path == "/") {
    // Default page is simply a frame containing the WebInspector
    // Note: "?page=0" is due to the WebInspector requiring the "page" variable
    // to be set in order to open a WebSocket (see DevTools.js:2939)
    std::string default_page =
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
        "<head>\n"
        "<frameset>\n"
        "<frame src = \"/inspector/devtools.html?page=0\" />\n"
        "</frameset>\n"
        "</head>\n";

    server_->Send200(connection_id, default_page, "text/html");
  } else {
    SendFile(connection_id, path);
  }
}

void LBHttpHandler::OnWebSocketRequest(const int connection_id,
                                       const net::HttpServerRequestInfo& info) {
  DCHECK_EQ(MessageLoop::current(), thread_->message_loop());
  // Allow only one debugging client at a time
  if (agent_)
    return;

  server_->AcceptWebSocket(connection_id, info);

  // Connect to the backend
  agent_ = LB_NEW LBDevToolsAgent(connection_id, host_, this);
  agent_->Attach();
  return;
}

void LBHttpHandler::OnWebSocketMessage(const int connection_id,
                                       const std::string& data) {
  DCHECK_EQ(MessageLoop::current(), thread_->message_loop());
  if (agent_ && agent_->connection_id() == connection_id) {
    WebKit::WebString webstring;
    webstring = WebKit::WebString::fromUTF8(data.c_str(), data.length());
    agent_->sendMessageToInspectorBackend(webstring);
  }
}

void LBHttpHandler::OnClose(const int connection_id) {
  DCHECK_EQ(MessageLoop::current(), thread_->message_loop());
  if (agent_ && agent_->connection_id() == connection_id) {
    agent_->Detach();
    agent_->Release();
    agent_ = NULL;
  }
}

void LBHttpHandler::DoSendOverWebSocket(const int connection_id,
                                        const std::string &data) const {
  DCHECK_EQ(MessageLoop::current(), thread_->message_loop());
  server_->SendOverWebSocket(connection_id, data);
}

void LBHttpHandler::SendOverWebSocket(const int connection_id,
                                      const std::string &data) const {
  thread_->message_loop()->PostTask(FROM_HERE, base::Bind(
      &LBHttpHandler::DoSendOverWebSocket, this, connection_id, data));
}

#endif // __LB_SHELL__ENABLE_CONSOLE__
