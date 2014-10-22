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
// The LBDevToolsAgent communicates with the base WebDevToolsAgentImpl class,
// which in turn makes the appropriate calls to communicate with WebInspector.
//
// sendMessageToInspectorBackend() passes WebSocket data from the WebInspector
// frontend to the backend
//
// sendMessageToInspectorFrontend() is a callback that is triggered when some
// data needs to be pushed to the WebInspector frontend.

#ifndef SRC_LB_DEVTOOLS_AGENT_H_
#define SRC_LB_DEVTOOLS_AGENT_H_

#if ENABLE_INSPECTOR

#include "base/memory/ref_counted.h"
#include "base/synchronization/waitable_event.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDevToolsAgentClient.h"

#include "lb_memory_manager.h"

namespace WebKit {
class WebDevToolsAgent;
class WebView;
}  // namespace WebKit

// Forward declarations
class LBHttpHandler;
class LBWebViewHost;

class LBDevToolsAgent
    : public WebKit::WebDevToolsAgentClient,
      public base::RefCountedThreadSafe<LBDevToolsAgent> {
 public:
  LBDevToolsAgent(const int connection_id, LBWebViewHost *host,
                  LBHttpHandler *http_handler);
  virtual ~LBDevToolsAgent();

  void sendMessageToInspectorBackend(const WebKit::WebString& data);

  // WebDevToolsAgentClient implementation.
  virtual void sendMessageToInspectorFrontend(
      const WebKit::WebString& data) OVERRIDE;
  virtual int hostIdentifier() OVERRIDE { return 1; }

  virtual WebKit::WebDevToolsAgentClient::WebKitClientMessageLoop*
      createClientMessageLoop() OVERRIDE;

  void Attach();
  void Detach();

  inline const int connection_id() const { return connection_id_; }

 private:
  // Tasks are members to ensure that this class is not destroyed
  // before the tasks have finished on the main message loop, as they may
  // result in calls to sendMessageToInspectorFrontend() method
  void InitializeTask();
  void DispatchTask(const WebKit::WebString& data);
  void AttachTask();
  void DetachTask();

  int connection_id_;
  WebKit::WebDevToolsAgent *agent_;
  LBWebViewHost *host_;
  LBHttpHandler *http_handler_;
  base::WaitableEvent detach_event_;
};

#endif  // ENABLE_INSPECTOR
#endif  // SRC_LB_DEVTOOLS_AGENT_H_
