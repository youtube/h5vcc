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
// Manages telnet connections to the debug console.
// To use, create an LBNetworkConsole object and call Poll() periodically
// to query new input and connections

#ifndef SRC_LB_NETWORK_CONSOLE_H_
#define SRC_LB_NETWORK_CONSOLE_H_

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

#include "lb_console_values.h"
#include "lb_console_connection.h"
#include "lb_debug_console.h"
#include "net/base/stream_listen_socket.h"

class LBNetworkConsole;

// Manages buffering and state for a single network connection.
// Also executes commands.
class LBNetworkConnection : public LBConsoleConnection {
 public:
  LBNetworkConnection(LBDebugConsole *debug_console,
                      LBNetworkConsole *network_console,
                      scoped_refptr<net::StreamListenSocket> socket);
  ~LBNetworkConnection();

  void OnInput(const char *data, int len);

  // LBConsoleConnection:
  virtual void Output(const std::string& data) OVERRIDE;
  virtual void Close() OVERRIDE;

 private:
  LBDebugConsole *debug_console_;
  LBNetworkConsole *network_console_;
  scoped_refptr<net::StreamListenSocket> socket_;
  std::string command_;
};

// Manages incoming connections and socket I/O.  Creates and destroys
// LBNetworkConnections, which manage the state of each stream.
class LBNetworkConsole : public net::StreamListenSocket::Delegate {
 public:
  static void Initialize(LBDebugConsole *console);
  static void Teardown();
  static LBNetworkConsole* GetInstance() { return instance_; }

  static void CaptureLogging(bool mode) {
    DCHECK(!logging::GetLogMessageHandler() ||
           logging::GetLogMessageHandler() == TelnetLogHandler);
    logging::SetLogMessageHandler(mode ? TelnetLogHandler : NULL);
  }

  // net::StreamListenSocket::Delegate implementation:
  virtual void DidAccept(net::StreamListenSocket* listener,
                         net::StreamListenSocket* client) OVERRIDE;
  virtual void DidRead(net::StreamListenSocket* client,
                       const char* data,
                       int len) OVERRIDE;
  virtual void DidClose(net::StreamListenSocket* client) OVERRIDE;

 private:
  explicit LBNetworkConsole(LBDebugConsole *console);
  virtual ~LBNetworkConsole();

  static bool TelnetLogHandler(int severity, const char* file, int line,
                               size_t message_start, const std::string& str);
  bool GetTelnetPort(int* port);

  static LBNetworkConsole *instance_;

  LBDebugConsole *debug_console_;
  typedef std::map<net::StreamListenSocket *, LBNetworkConnection *>
      ConnectionMap;
  ConnectionMap connections_;
  scoped_ptr<net::StreamListenSocketFactory> factory_;
  scoped_refptr<net::StreamListenSocket> listen_socket_;

  LB::CVal<std::string> console_display_;
};

#endif  // __LB_SHELL__ENABLE_CONSOLE__

#endif  // SRC_LB_NETWORK_CONSOLE_H_
