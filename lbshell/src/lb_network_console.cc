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

#include "lb_network_console.h"

#include "base/debug/trace_event.h"
#include "base/stringprintf.h"
#include "lb_resource_loader_bridge.h"
#include "lb_shell.h"
#include "lb_web_view_host.h"
#include "net/base/tcp_listen_socket.h"

#if defined(__LB_XB1__)
// non-standard port number
static int kPort = 4600;
#elif defined(__LB_PS4__)
// get the port number automatically
static int kPort = 0;
#else
static int kPort = 1713;
#endif

///////////////////////////////////////////////////////////////////////////////
// LBNetworkConnection
LBNetworkConnection::LBNetworkConnection(
    LBDebugConsole *debug_console,
    LBNetworkConsole *network_console,
    scoped_refptr<net::StreamListenSocket> socket)
    : debug_console_(debug_console)
    , network_console_(network_console)
    , socket_(socket) {
  DLOG(INFO) << "Connection established.";
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
}

LBNetworkConnection::~LBNetworkConnection() {
  DLOG(INFO) << "Connection closed.";
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());

  // Shutdown socket.
  socket_ = NULL;
}

// Processes input and executes it if complete.
void LBNetworkConnection::OnInput(const char *data, int len) {
  TRACE_EVENT0("lb_shell", "LBNetworkConnection::OnInput");
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());

  std::string input(data, len);
  size_t begin = 0;
  size_t end = 0;

  // Within the input, keep searching for newline/carriage returns. Each
  // time we find one, we have a complete line to execute.
  while (end != std::string::npos && begin != std::string::npos) {
    end = input.find_first_of("\n\r", begin);
    if (end != std::string::npos) {
      // Found newline/carriage return
      command_.append(input, begin, end - begin);

      // Post ParseAndExecuteCommand as a task in case that command wants to
      // terminate this connection.
      MessageLoop::current()->PostTask(FROM_HERE, base::Bind(
          &LBDebugConsole::ParseAndExecuteCommand,
          base::Unretained(debug_console_), this, command_));

      begin = input.find_first_not_of("\n\r", end);
      command_.clear();
    } else {
      // Commmand is split into multiple buffers
      command_.append(input, begin, input.size() - begin);
    }
  }
}

void LBNetworkConnection::Output(const std::string& data) {
  scoped_refptr<base::MessageLoopProxy> io_message_loop =
      LBResourceLoaderBridge::GetIoThread();

  if (io_message_loop->BelongsToCurrentThread()) {
    if (socket_) {
      socket_->Send(data);
    }
  } else {
    io_message_loop->PostTask(FROM_HERE,
        base::Bind(&LBNetworkConnection::Output, base::Unretained(this), data));
  }
}

void LBNetworkConnection::Close() {
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
  // Called when the debug console gets a command to kill the connction.
  // NOTE: Simply destroying the socket does not cause DidClose to fire.
  // Therefore, we call it directly ourselves, which leads to |this|
  // being deleted.
  network_console_->DidClose(socket_.get());
  // PLEASE DO NOT ADD CODE HERE.
}


///////////////////////////////////////////////////////////////////////////////
// LBNetworkConsole
LBNetworkConsole* LBNetworkConsole::instance_ = NULL;

// static
void LBNetworkConsole::Initialize(LBDebugConsole *console) {
  if (!LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread()) {
    LBResourceLoaderBridge::GetIoThread()->PostTask(FROM_HERE,
        base::Bind(&LBNetworkConsole::Initialize, console));
    return;
  }

  if (!LBNetworkConsole::instance_) {
    LBNetworkConsole::instance_ = new LBNetworkConsole(console);
  }
}

// static
void LBNetworkConsole::Teardown() {
  if (!LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread()) {
    LBResourceLoaderBridge::GetIoThread()->PostTask(FROM_HERE,
        base::Bind(&LBNetworkConsole::Teardown));
    return;
  }

  delete instance_;
  instance_ = NULL;
}

LBNetworkConsole::LBNetworkConsole(LBDebugConsole *console)
    : debug_console_(console)
    , console_display_("Telnet.Port", "0", "Telnet port number.") {
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
  factory_.reset(new net::TCPListenSocketFactory("0.0.0.0", kPort));
  listen_socket_ = factory_->CreateAndListen(this);
  int port;
  if (GetTelnetPort(&port)) {
    // Note: Don't remove this line. It's needed for tests that
    // connect via telnet.
    DLOG(INFO) << "Listening for connections on port: " << port;
  }
  // Add the CVal string to the system display.
  console_display_ = base::StringPrintf("%d", port);
}

LBNetworkConsole::~LBNetworkConsole() {
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
  // Shutdown listening socket.
  listen_socket_ = NULL;

  // Delete/close all connections
  ConnectionMap::iterator itr;
  for (itr = connections_.begin(); itr != connections_.end(); itr++) {
    delete itr->second;
  }
}

void LBNetworkConsole::DidAccept(net::StreamListenSocket* listener,
                                 net::StreamListenSocket* client) {
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
  LBNetworkConnection *connection =
      new LBNetworkConnection(debug_console_, this, client);
  connections_[client] = connection;
}

void LBNetworkConsole::DidRead(net::StreamListenSocket* client,
                               const char* data,
                               int len) {
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
  ConnectionMap::iterator itr = connections_.find(client);
  DCHECK(itr != connections_.end());
  if (itr != connections_.end()) {
    itr->second->OnInput(data, len);
  }
}

void LBNetworkConsole::DidClose(net::StreamListenSocket* client) {
  DCHECK(LBResourceLoaderBridge::GetIoThread()->BelongsToCurrentThread());
  ConnectionMap::iterator itr = connections_.find(client);
  DCHECK(itr != connections_.end());
  if (itr != connections_.end()) {
    delete itr->second;
    connections_.erase(itr);
  }
}

// static
bool LBNetworkConsole::TelnetLogHandler(
    int severity, const char* file, int line, size_t message_start,
    const std::string& str) {
  LBNetworkConsole *console = GetInstance();

  // Send output to each connection
  if (console) {
    ConnectionMap::iterator itr;
    for (itr = console->connections_.begin();
         itr != console->connections_.end();
         itr++) {
      itr->second->Output(str);
    }
  }

  // Don't eat this event
  return false;
}

bool LBNetworkConsole::GetTelnetPort(int* port) {
#if defined(__LB_PS4__)
  net::IPEndPoint port_addr;
  int ret = listen_socket_->GetLocalAddress(&port_addr);
  *port = (ret == 0) ? port_addr.port() : 0;
  return (ret == 0);
#else  // defined(__LB_PS4__)
  *port = kPort;
  return true;
#endif  // defined(__LB_PS4__)
}

#endif  // __LB_SHELL__ENABLE_CONSOLE__
