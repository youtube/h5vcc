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

#include "lb_devtools_agent.h"

#include "base/bind.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDevToolsAgent.h"

#include "lb_http_handler.h"
#include "lb_web_view_host.h"

class WebKitClientMessageLoopImpl
    : public WebKit::WebDevToolsAgentClient::WebKitClientMessageLoop {
 public:
  WebKitClientMessageLoopImpl() : message_loop_(MessageLoop::current()) { }
  virtual ~WebKitClientMessageLoopImpl() {
    message_loop_ = NULL;
  }
  virtual void run() {
    MessageLoop::ScopedNestableTaskAllower allow(message_loop_);
    message_loop_->Run();
  }
  virtual void quitNow() {
    message_loop_->QuitNow();
  }
 private:
  MessageLoop* message_loop_;
};



LBDevToolsAgent::LBDevToolsAgent(const int connection_id,
                                 LBWebViewHost *host,
                                 LBHttpHandler *http_handler)
    : connection_id_(connection_id), host_(host), http_handler_(http_handler) {
  host_->main_message_loop()->PostTask(FROM_HERE, base::Bind(
      &LBDevToolsAgent::InitializeTask, this));
  AddRef();
}

LBDevToolsAgent::~LBDevToolsAgent() {
}

void LBDevToolsAgent::InitializeTask() {
  // All calls involving the WebDevToolsAgentImpl class must be done on the
  // main message loop
  host_->webview()->setDevToolsAgentClient(this);
  agent_ = host_->webview()->devToolsAgent();
}

void LBDevToolsAgent::AttachTask() {
  // This must be done on the main message loop
  agent_->attach();
}

void LBDevToolsAgent::Attach() {
  host_->main_message_loop()->PostTask(FROM_HERE, base::Bind(
      &LBDevToolsAgent::AttachTask, this));
}

void LBDevToolsAgent::DetachTask() {
  // This must be done on the main message loop
  agent_->detach();
}

void LBDevToolsAgent::Detach() {
  host_->main_message_loop()->PostTask(FROM_HERE, base::Bind(
      &LBDevToolsAgent::DetachTask, this));
}

void LBDevToolsAgent::sendMessageToInspectorFrontend(
    const WebKit::WebString& data) {
  http_handler_->SendOverWebSocket(connection_id_, data.utf8());
}

void LBDevToolsAgent::DispatchTask(const WebKit::WebString& data) {
  // This must be done on the main message loop
  agent_->dispatchOnInspectorBackend(data);
}

void LBDevToolsAgent::sendMessageToInspectorBackend(
    const WebKit::WebString& data) {
  host_->main_message_loop()->PostTask(FROM_HERE, base::Bind(
      &LBDevToolsAgent::DispatchTask, this, data));
}

WebKit::WebDevToolsAgentClient::WebKitClientMessageLoop*
    LBDevToolsAgent::createClientMessageLoop() {
  return new WebKitClientMessageLoopImpl();
}

#endif // __LB_SHELL__ENABLE_CONSOLE__
