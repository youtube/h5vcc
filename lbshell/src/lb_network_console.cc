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

#include "external/chromium/base/stringprintf.h"

#include "lb_memory_manager.h"
#include "lb_platform.h"
#include "lb_shell.h"
#include "lb_web_graphics_context_3d.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "lb_network_console.h"

static int kPort = 1713;

///////////////////////////////////////////////////////////////////////////////
// LBNetworkConnection
LBNetworkConnection::LBNetworkConnection(
    LBDebugConsole *console,
    int socket) :
    debug_console_(console),
    socket_(socket),
    request_close_(false) {
  DLOG(INFO) << "Connection established.";
}

LBNetworkConnection::~LBNetworkConnection() {
  if (socket_ > 0) {
    LB::Platform::close_socket(socket_);
    debug_console_->shell()->webViewHost()->SetTelnetConnection(NULL);
    DLOG(INFO) << "Connection closed.";
  }
}

// Checks the connection for input and executes it if there is any
bool LBNetworkConnection::Poll() {
  char buffer[1024];
  int bytes;

  // Receive some data. Commands that are split up into multiple buffers
  // are processed across multiple Poll() calls.
  bytes = recv(socket_, buffer, sizeof(buffer), MSG_DONTWAIT);
  if (bytes <= 0) {
    // Either an error occurred or the client disconnected.
    return false;
  }

  std::string input;
  input.assign(buffer, bytes);
  size_t begin = 0;
  size_t end = 0;

  // Within the input, keep searching for newline/carriage returns. Each
  // time we find one, we have a complete line to execute.
  while (end != std::string::npos && begin != std::string::npos) {
    end = input.find_first_of("\n\r", begin);
    if (end != std::string::npos) {
      // Found newline/carriage return
      command_.append(input, begin, end - begin);
      debug_console_->shell()->webViewHost()->SetTelnetConnection(this);
      debug_console_->ParseAndExecuteCommand(command_);

      begin = input.find_first_not_of("\n\r", end);
      command_.clear();
    } else {
      // Commmand is split into multiple buffers
      command_.append(input, begin, input.size() - begin);
    }
  }

  return true;
}

// Sends a string of text to the telnet client
void LBNetworkConnection::Output(const std::string &text) const {
  send(socket_, text.c_str(), text.size(), MSG_DONTWAIT);
}

void LBNetworkConnection::Close() {
  request_close_ = true;
}

///////////////////////////////////////////////////////////////////////////////
// LBNetworkConsole
LBNetworkConsole* LBNetworkConsole::instance_ = NULL;

// static
void LBNetworkConsole::Initialize(LBDebugConsole *console) {
  if (!LBNetworkConsole::instance_) {
    LBNetworkConsole::instance_ = LB_NEW LBNetworkConsole(console);
  }
}

// static
void LBNetworkConsole::Teardown() {
  delete instance_;
  instance_ = NULL;
}

LBNetworkConsole::LBNetworkConsole(LBDebugConsole *console)
    : debug_console_(console)
    , listen_socket_(-1) {
}

LBNetworkConsole::~LBNetworkConsole() {
  // Shutdown listening socket
  if (listen_socket_ != -1) {
    LB::Platform::close_socket(listen_socket_);
  }

  // Delete/close all connections
  connection_map::iterator itr;
  for (itr = connections_.begin(); itr != connections_.end(); itr++) {
    delete itr->second;
  }
}

// Runs once per frame. Checks for incoming connections, checks for
// input from each connection, and removes dead connections.
void LBNetworkConsole::Poll() {
  std::vector<int> dead_connections;

  // Figure out which connections have new data to be read
  SelectConnections();

  // Check for new connections
  ProcessIncomingConnections();

  // Poll each connection for input
  connection_map::iterator itr;
  for (itr = connections_.begin(); itr != connections_.end(); itr++) {
    bool should_delete = false;
    if (itr->second->request_close_) {
      should_delete = true;
    } else if (FD_ISSET(itr->second->socket_, &fds_)) {
      if (!itr->second->Poll()) {
        // Connection has been terminated. Mark for deletion
        should_delete = true;
      }
    }

    if (should_delete) {
        delete itr->second;
        dead_connections.push_back(itr->first);
    }
  }

  // Cull dead connections
  for (int i = 0; i < dead_connections.size(); i++) {
    connections_.erase(dead_connections[i]);
  }
}

// Checks the listening port for new connections
void LBNetworkConsole::ProcessIncomingConnections() {
  // Check for new connections, and add if we have one
  struct sockaddr_in sin;

  // Create the listening socket if it is invalid.
  if (listen_socket_ < 0) {
    listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_ < 0) {
      DLOG(ERROR) << "Failed to create listening socket.";
      return;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(kPort);

    if (bind(listen_socket_, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      DLOG(ERROR) << "Failed to bind listening socket.";
      return;
    }
    if (listen(listen_socket_, 5) < 0) {
      DLOG(ERROR) << "Failed to listen for incoming connections.";
      return;
    }
    struct in_addr addr;
    LB::Platform::GetLocalIpAddress(&addr);
    char ip_address[128];
    snprintf(ip_address, sizeof(ip_address), "%d.%d.%d.%d:%d",
             (addr.s_addr >> 24) & 0xff,
             (addr.s_addr >> 16) & 0xff,
             (addr.s_addr >> 8) & 0xff,
             (addr.s_addr) & 0xff,
             htons(sin.sin_port));
    DLOG(INFO) << "Listening for connections on " << ip_address;
  }

  // if there's activity on the listening socket, check for new connection
  if (FD_ISSET(listen_socket_, &fds_)) {
    socklen_t len = sizeof(sin);
    int new_socket = accept(listen_socket_, (struct sockaddr *)&sin, &len);
    if (new_socket >= 0) {
      // Connection is successful; store it.
      LBNetworkConnection *connection = LB_NEW LBNetworkConnection(
          debug_console_,
          new_socket);
      connections_[new_socket] = connection;
    } else {
      DLOG(ERROR) << "Failed to establish incoming connection.";

      // Listening socket has died.  Reset the socket and clean up any open
      // telnet connections.
      close(listen_socket_);
      listen_socket_ = -1;
      connection_map::iterator itr;
      for (itr = connections_.begin(); itr != connections_.end(); itr++) {
        itr->second->request_close_ = true;
      }
    }
  }
}

// Calls select on all of the connections (to determine which sockets are
// ready for read and write)
void LBNetworkConsole::SelectConnections() {
  FD_ZERO(&fds_);
  if (listen_socket_ >= 0) {
    FD_SET(listen_socket_, &fds_);
  }

  std::map<int,LBNetworkConnection*>::iterator itr;
  for (itr = connections_.begin(); itr != connections_.end(); itr++) {
    FD_SET(itr->first, &fds_);
  }

  timeval tv;
  memset(&tv, 0, sizeof(tv));   // timeout of 0 == don't block on select
  int result = select(FD_SETSIZE, &fds_, NULL, NULL, &tv);
  if (result < 0) {
    DLOG(ERROR) << "Failed to read socket status.";
    return;
  } else if (result == 0) {
    // No ready sockets.
    FD_ZERO(&fds_);
  }
}

// static
bool LBNetworkConsole::TelnetLogHandler(
    int severity, const char* file, int line, size_t message_start,
    const std::string& str) {
  LBNetworkConsole *console = GetInstance();

  // Send output to each connection
  if (console) {
    connection_map::iterator itr;
    for (itr = console->connections_.begin();
         itr != console->connections_.end();
         itr++) {
      itr->second->Output(str);
    }
  }

  // Don't eat this event
  return false;
}

#endif  // __LB_SHELL__ENABLE_CONSOLE__
