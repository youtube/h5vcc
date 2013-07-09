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
#include "lb_debug_console.h"

// Manages a single network connection
class LBNetworkConsole;
class LBNetworkConnection {
  friend class LBNetworkConsole;
 public:
  void Output(const std::string &text) const;
  void Close();

 private:
  LBNetworkConnection(LBDebugConsole *console, int socket);
  ~LBNetworkConnection();

  bool Poll();

  LBDebugConsole *debug_console_;
  int socket_;
  std::string command_;
  bool request_close_;
};

// Listens for incoming connections and is responsible for creation
// deletion of LBNetworkConnections
class LBNetworkConsole {
 public:
  static void Initialize(LBDebugConsole *console);
  static void Teardown();
  static LBNetworkConsole* GetInstance() { return instance_; }

  void Poll();
  static void CaptureLogging(bool mode) {
    DCHECK(!logging::GetLogMessageHandler() ||
           logging::GetLogMessageHandler() == TelnetLogHandler);
    logging::SetLogMessageHandler(mode ? TelnetLogHandler : NULL);
  }

 private:
  LBNetworkConsole(LBDebugConsole *console);
  ~LBNetworkConsole();

  void ProcessIncomingConnections();
  void SelectConnections();

  static bool TelnetLogHandler(int severity, const char* file, int line,
                               size_t message_start, const std::string& str);

  LBDebugConsole *debug_console_;
  static LBNetworkConsole *instance_;

  typedef std::map<int, LBNetworkConnection*> connection_map;
  connection_map connections_;
  int listen_socket_;
  fd_set fds_;  // used by select() to query the read/write readiness of sockets
};
#endif  // __LB_SHELL__ENABLE_CONSOLE__

#endif  // SRC_LB_NETWORK_CONSOLE_H_
