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
#include "net/socket/tcp_client_socket.h"
#include "tcp_client_socket_shell.h"

#include "base/logging.h"
#include "base/message_loop.h"
#include "base/message_pump_shell.h"
#include "base/metrics/stats_counters.h"
#include "base/posix/eintr_wrapper.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "net/base/connection_type_histograms.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/net_log.h"
#include "net/base/net_util.h"
#include "net/base/network_change_notifier.h"
#include "net/socket/socket_net_log_params.h"

#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "lb_memory_manager.h"
#include "lb_network_helpers.h"

#if defined(__LB_XB360__)
# define CAST_OPTVAL(x) ((char *)(x))
#else
# define CAST_OPTVAL(x) (x)
#endif

namespace net {

namespace {

const int kInvalidSocket = -1;
const int kTCPKeepAliveSeconds = 45;

#if defined(__LB_PS4__)
const int kDefaultMsgFlags = MSG_NOSIGNAL | MSG_DONTWAIT;
#else
const int kDefaultMsgFlags = MSG_DONTWAIT;
#endif

// ps3 network code stores network error codes in thread-specific storage
// this function gets that error and maps it to the internal chrome errors
int MapLastNetworkError() {
  int last_error = LB::Platform::net_errno();
  switch (last_error) {
    // invalid socket number specified
    case LB_NET_EBADF:
      return ERR_INVALID_HANDLE;

    // invalid value specified
    case LB_NET_EINVAL:
    // invalid combination of level and option (for setsockopt)
    case LB_NET_ENOPROTOOPT:
    // bad pointer to argument
    case LB_NET_EADDRNOTAVAIL:
      return ERR_INVALID_ARGUMENT;

    // a tcp connection was reset
    case LB_NET_ERROR_ECONNRESET:
      return ERR_CONNECTION_RESET;

    // insufficent resources to allocate socket
    case LB_NET_EMFILE:
      return ERR_INSUFFICIENT_RESOURCES;

    // error code returned for non-blocking sockets
    case LB_NET_EWOULDBLOCK:
      return ERR_IO_PENDING;

    default:
      return ERR_FAILED;
  }
}

int MapConnectError(int os_error) {
  switch (os_error) {
    case EACCES:
      return ERR_NETWORK_ACCESS_DENIED;
    case ETIMEDOUT:
      return ERR_CONNECTION_TIMED_OUT;
    default: {
      int net_error = MapSystemError(os_error);
      if (net_error == ERR_FAILED)
        return ERR_CONNECTION_FAILED;  // More specific than ERR_FAILED.

      // Give a more specific error when the user is offline.
      if (net_error == ERR_ADDRESS_UNREACHABLE &&
          NetworkChangeNotifier::IsOffline()) {
        return ERR_INTERNET_DISCONNECTED;
      }
      return net_error;
    }
  }
}

bool SetTCPNoDelay(int fd, bool no_delay) {
  int on = no_delay ? 1 : 0;
  int error = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, CAST_OPTVAL(&on),
      sizeof(on));
  return error == 0;
}

// SetTCPKeepAlive sets SO_KEEPALIVE.
bool  SetTCPKeepAlive(int fd, bool enable, int delay) {
  int on = enable ? 1 : 0;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, CAST_OPTVAL(&on), sizeof(on))) {
    DLOG(ERROR) << base::StringPrintf("Failed to set SO_KEEPALIVE on fd: %d",
                                      fd);
    return false;
  }
  return true;
}

bool SetTCPReceiveBufferSize(int fd, int32 size) {
  DCHECK_LT(size, 512 * 1024);
  int rv = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, CAST_OPTVAL(&size), sizeof(size));
  DCHECK_EQ(rv, 0);
  return (rv == 0);
}

#if defined(__LB_WIIU__)
bool SetTCPWindowScaling(int fd, bool enable) {
  int on = enable ? 1 : 0;
  int error = setsockopt(fd, SOL_SOCKET, SO_WINSCALE, &on, sizeof(on));
  return error == 0;
}
#endif

int SetupSocket(int socket) {
  if (SetNonBlocking(socket))
    return LB::Platform::net_errno();

  // This mirrors the behaviour on Windows. See the comment in
  // tcp_client_socket_win.cc after searching for "NODELAY".
  SetTCPNoDelay(socket, true);  // If SetTCPNoDelay fails, we don't care.
  SetTCPKeepAlive(socket, true, kTCPKeepAliveSeconds);

#if defined(__LB_WIIU__)
  // TCP scaling
  SetTCPWindowScaling(socket, true);

  SetTCPReceiveBufferSize(socket, TCPClientSocketShell::kReceiveBufferSize);
#endif
  return 0;
}

// Creates a new socket and sets default parameters for it. Returns
// the OS error code (or 0 on success).
int CreateSocket(int family, int* socket) {
  DCHECK(family == AF_INET || family == AF_INET6);
  *socket = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
  if (*socket == kInvalidSocket)
    return LB::Platform::net_errno();
  int error = SetupSocket(*socket);
  if (error) {
    if (HANDLE_EINTR(LB::Platform::close_socket(*socket)) < 0)
      DLOG(ERROR) << "close_socket";
    *socket = kInvalidSocket;
    return error;
  }
  return 0;
}

}  // namespace

//-----------------------------------------------------------------------------

class TCPClientSocketShell::ReadDelegate
    : public base::steel::ObjectWatcher::Delegate {
 public:
  explicit ReadDelegate(TCPClientSocketShell * socket) : socket_(socket) {}
  virtual ~ReadDelegate() {}

  virtual void OnObjectSignaled(int object) {
    socket_->DidCompleteRead();
  }

 private:
  TCPClientSocketShell * socket_;
};

class TCPClientSocketShell::WriteDelegate
    : public base::steel::ObjectWatcher::Delegate {
 public:
  explicit WriteDelegate(TCPClientSocketShell * socket) : socket_(socket) {}
  virtual ~WriteDelegate() {}

  virtual void OnObjectSignaled(int object) {
    // is this a connection callback?
    if (socket_->waiting_connect()) {
      socket_->DidCompleteConnect();
    } else {
      socket_->DidCompleteWrite();
    }
  }

 private:
  TCPClientSocketShell * socket_;
};

//-----------------------------------------------------------------------------

TCPClientSocketShell::TCPClientSocketShell(const AddressList& addresses,
                                           net::NetLog* net_log,
                                           const net::NetLog::Source& source) :
      addresses_(addresses),
      current_address_index_(-1),
      net_log_(BoundNetLog::Make(net_log, NetLog::SOURCE_SOCKET)),
      next_connect_state_(CONNECT_STATE_NONE),
      previously_disconnected_(true),
      reader_(new ReadDelegate(this)),
      socket_(kInvalidSocket),
      waiting_read_(false),
      waiting_write_(false),
      writer_(new WriteDelegate(this)),
      num_bytes_read_(0) {
  net_log_.BeginEvent(NetLog::TYPE_SOCKET_ALIVE,
                      source.ToEventParametersCallback());
}

TCPClientSocketShell::~TCPClientSocketShell() {
  Disconnect();
  net_log_.EndEvent(NetLog::TYPE_SOCKET_ALIVE);
  delete reader_;
  delete writer_;
}

int TCPClientSocketShell::Connect(const CompletionCallback& callback) {
  DCHECK(CalledOnValidThread());

  // if socket is already valid (non-negative) then just return OK
  if (socket_ >= 0)
    return OK;

  base::StatsCounter connects("tcp.connect");
  connects.Increment();

  // we should not have another call to Connect() already in flight
  DCHECK(next_connect_state_ == CONNECT_STATE_NONE);

  net_log_.BeginEvent(NetLog::TYPE_TCP_CONNECT,
                      addresses_.CreateNetLogCallback());

  next_connect_state_ = CONNECT_STATE_CONNECT;
  current_address_index_ = 0;

  int rv = DoConnectLoop(OK);
  if (rv == ERR_IO_PENDING) {
    write_callback_ = callback;
  } else {
    LogConnectCompletion(rv);
  }

  return rv;
}

// pretty much cribbed directly from TCPClientSocketLibevent and/or
// TCPClientSocketWin, take your pick, they're identical
int TCPClientSocketShell::DoConnectLoop(int result) {
  DCHECK_NE(next_connect_state_, CONNECT_STATE_NONE);

  int rv = result;
  do {
    ConnectState state = next_connect_state_;
    next_connect_state_ = CONNECT_STATE_NONE;
    switch (state) {
      case CONNECT_STATE_CONNECT:
        DCHECK_EQ(OK, rv);
        rv = DoConnect();
        break;
      case CONNECT_STATE_CONNECT_COMPLETE:
        rv = DoConnectComplete(rv);
        break;
      default:
        DLOG(DFATAL) << "bad state";
        rv = ERR_UNEXPECTED;
        break;
    }
  } while (rv != ERR_IO_PENDING && next_connect_state_ != CONNECT_STATE_NONE);

  return rv;
}

int TCPClientSocketShell::DoConnect() {
  DCHECK_GE(current_address_index_, 0);
  DCHECK_LT(current_address_index_, static_cast<int>(addresses_.size()));

  const IPEndPoint& endpoint = addresses_[current_address_index_];

  if (previously_disconnected_) {
    use_history_.Reset();
    previously_disconnected_ = false;
  }

  net_log_.BeginEvent(NetLog::TYPE_TCP_CONNECT_ATTEMPT,
                      CreateNetLogIPEndPointCallback(&endpoint));

  next_connect_state_ = CONNECT_STATE_CONNECT_COMPLETE;

  // create a non-blocking socket
  connect_os_error_ = CreateSocket(endpoint.GetSockAddrFamily(), &socket_);
  if (connect_os_error_)
    return MapSystemError(connect_os_error_);

  connect_start_time_ = base::TimeTicks::Now();

  SockaddrStorage storage;
  if (!endpoint.ToSockAddr(storage.addr, &storage.addr_len))
    return ERR_INVALID_ARGUMENT;

  // now we call connect() with the expectation that it will return
  // LB_NET_EINPROGRESS, since we set nonblocking option above.
  // returning no error is considered a fatal error condition in the other
  // tcp_client_socket_* implementations, so we follow that pattern
  int rv = connect(socket_, storage.addr, storage.addr_len);
  DCHECK_LT(rv, 0);

  rv = LB::Platform::net_errno();
  if (rv != LB_NET_EINPROGRESS)
    return MapLastNetworkError();

  // poll() will report sockets that finish connecting as ready for _writes_
  // so we set up a write poll for these sockets
  write_watcher_.StartWatching(
      socket_, base::MessagePumpShell::WATCH_WRITE, writer_);
  return ERR_IO_PENDING;
}

int TCPClientSocketShell::DoConnectComplete(int result) {
  // Log the end of this attempt (and any OS error it threw).
  int os_error = connect_os_error_;
  connect_os_error_ = 0;
  if (result != OK) {
    net_log_.EndEvent(NetLog::TYPE_TCP_CONNECT_ATTEMPT,
                      NetLog::IntegerCallback("os_error", os_error));
  } else {
    net_log_.EndEvent(NetLog::TYPE_TCP_CONNECT_ATTEMPT);
  }

  if (result == OK) {
    connect_time_micros_ = base::TimeTicks::Now() - connect_start_time_;
    use_history_.set_was_ever_connected();
    return OK;  // Done!
  }

  // ok, didn't work, clean up
  DoDisconnect();

  // Try to fall back to the next address in the list.
  if (current_address_index_ + 1 < static_cast<int>(addresses_.size())) {
    next_connect_state_ = CONNECT_STATE_CONNECT;
    ++current_address_index_;
    return OK;
  }
  // Otherwise there is nothing to fall back to, so give up.
  return result;
}

void TCPClientSocketShell::DidCompleteConnect() {
  DCHECK_EQ(next_connect_state_, CONNECT_STATE_CONNECT_COMPLETE);

  // Get the error that connect() completed with.
  connect_os_error_ = LB::Platform::GetSocketError(socket_);
  int rv = DoConnectLoop(MapConnectError(connect_os_error_));
  if (rv != ERR_IO_PENDING) {
    LogConnectCompletion(rv);
    DoWriteCallback(rv);
  }
}

void TCPClientSocketShell::LogConnectCompletion(int net_error) {
  if (net_error == OK)
    UpdateConnectionTypeHistograms(CONNECTION_ANY);

  if (net_error != OK) {
    net_log_.EndEventWithNetErrorCode(NetLog::TYPE_TCP_CONNECT, net_error);
    return;
  }

  SockaddrStorage storage;
  int rv = getsockname(socket_, storage.addr, &storage.addr_len);
  if (rv != 0) {
    DLOG(ERROR) << base::StringPrintf("getsockname() [rv: %d] error",
                                      LB::Platform::net_errno());
    NOTREACHED();
    net_log_.EndEventWithNetErrorCode(NetLog::TYPE_TCP_CONNECT, rv);
    return;
  }

  net_log_.EndEvent(NetLog::TYPE_TCP_CONNECT,
                    CreateNetLogSourceAddressCallback(storage.addr,
                                                      storage.addr_len));
}

void TCPClientSocketShell::Disconnect() {
  DoDisconnect();
  current_address_index_ = -1;
}

void TCPClientSocketShell::DoDisconnect() {
  if (socket_ < 0)
    return;

  shutdown(socket_, SHUT_RDWR);
  LB::Platform::close_socket(socket_);
  socket_ = kInvalidSocket;

  waiting_read_ = false;
  waiting_write_ = false;

  previously_disconnected_ = true;
}

bool TCPClientSocketShell::IsConnected() const {
  if (socket_ < 0 || waiting_connect())
    return false;

  // Check if connection is alive.
  char c;
  int rv = recv(socket_, &c, 1, MSG_PEEK | kDefaultMsgFlags);
  if (rv == 0)
    return false;
  if (rv < 0 && LB::Platform::net_errno() != LB_NET_EWOULDBLOCK)
    return false;

  return true;
}

bool TCPClientSocketShell::IsConnectedAndIdle() const {
  if (socket_ < 0 || waiting_connect())
    return false;

  // Check if connection is alive and we haven't received any data
  // unexpectedly.
  char c;
  int rv = recv(socket_, &c, 1, MSG_PEEK | kDefaultMsgFlags);
  // if 0, we got a TCP FIN.  If > 0, socket is not idle.
  if (rv >= 0)
    return false;
  if (LB::Platform::net_errno() != LB_NET_EWOULDBLOCK)
    return false;

  return true;
}

int TCPClientSocketShell::GetPeerAddress(IPEndPoint* address) const {
  DCHECK(address);
  if (!IsConnected())
    return ERR_SOCKET_NOT_CONNECTED;
  *address = addresses_[current_address_index_];
  return OK;
}

int TCPClientSocketShell::GetLocalAddress(IPEndPoint* address) const {
  DCHECK(address);
  if (!IsConnected())
    return ERR_SOCKET_NOT_CONNECTED;

  struct sockaddr_storage addr_storage;
  socklen_t addr_len = sizeof(addr_storage);
  struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&addr_storage);
  if (getsockname(socket_, addr, &addr_len))
    return MapLastNetworkError();
  if (!address->FromSockAddr(addr, addr_len))
    return ERR_FAILED;
  return OK;
}

const BoundNetLog& TCPClientSocketShell::NetLog() const {
  return net_log_;
}

void TCPClientSocketShell::SetSubresourceSpeculation() {
  use_history_.set_subresource_speculation();
}

void TCPClientSocketShell::SetOmniboxSpeculation() {
  use_history_.set_omnibox_speculation();
}

bool TCPClientSocketShell::WasEverUsed() const {
  return use_history_.was_used_to_convey_data();
}

bool TCPClientSocketShell::UsingTCPFastOpen() const {
  // not supported on PS3
  return false;
}

int64 TCPClientSocketShell::NumBytesRead() const {
  return num_bytes_read_;
}

base::TimeDelta TCPClientSocketShell::GetConnectTimeMicros() const {
  return connect_time_micros_;
}

bool TCPClientSocketShell::WasNpnNegotiated() const {
  return false;
}

NextProto TCPClientSocketShell::GetNegotiatedProtocol() const {
  return kProtoUnknown;
}

bool TCPClientSocketShell::GetSSLInfo(SSLInfo* ssl_info) {
  return false;
}

int TCPClientSocketShell::Read(IOBuffer* buf, int buf_len,
                               const CompletionCallback& callback) {
  DCHECK_GE(socket_, 0);
  DCHECK(!waiting_connect());
  DCHECK(!waiting_read_);
  DCHECK_GT(buf_len, 0);


  ssize_t nread = recv(socket_, buf->data(), buf_len, 0);
  // maybe there's data available to read right now? if so do stats and return
  if (nread >= 0) {
    base::StatsCounter read_bytes("tcp.read_bytes");
    read_bytes.Add(nread);
    num_bytes_read_ += nread;
    if (nread > 0)
      use_history_.set_was_used_to_convey_data();
    net_log_.AddByteTransferEvent(
      NetLog::TYPE_SOCKET_BYTES_RECEIVED, nread, buf->data());
    return nread;
  }
  // nread < 0, we got some kind of error
  if (LB::Platform::net_errno() != LB_NET_EWOULDBLOCK)
    return MapLastNetworkError();

  // ok we'll do a callback for this read
  waiting_read_ = true;
  read_buf_ = buf;
  read_buf_len_ = buf_len;
  read_callback_ = callback;
  read_watcher_.StartWatching(
      socket_, base::MessagePumpShell::WATCH_READ, reader_);
  return ERR_IO_PENDING;
}

void TCPClientSocketShell::DidCompleteRead() {
  DCHECK(waiting_read_);
  int nread = recv(socket_, read_buf_->data(), read_buf_len_, 0);
  int result;
  if (nread >= 0) {
    result = nread;
    base::StatsCounter read_bytes("tcp.read_bytes");
    read_bytes.Add(nread);
    num_bytes_read_ += nread;
    if (nread > 0)
      use_history_.set_was_used_to_convey_data();
    net_log_.AddByteTransferEvent(NetLog::TYPE_SOCKET_BYTES_SENT, result,
      read_buf_->data());
  } else {
    result = MapLastNetworkError();
  }

  if (result != ERR_IO_PENDING) {
    read_buf_ = NULL;
    read_buf_len_ = 0;
    waiting_read_ = false;
    DoReadCallback(result);
  }
}

void TCPClientSocketShell::DoReadCallback(int rv) {
  DCHECK_NE(rv, ERR_IO_PENDING);

  // since Run may result in Read being called, clear read_callback_ up front.
  CompletionCallback c = read_callback_;
  read_callback_ = CompletionCallback();
  c.Run(rv);
}

int TCPClientSocketShell::Write(IOBuffer* buf, int buf_len,
                                const CompletionCallback& callback) {
  DCHECK_GE(socket_, 0);
  DCHECK(!waiting_connect());
  DCHECK(!waiting_write_);
  DCHECK_GT(buf_len, 0);

  base::StatsCounter writes("tcp.writes");
  writes.Increment();

  write_buf_ = buf;
  write_buf_len_ = buf_len;

  int rv = send(socket_, buf->data(), buf_len, kDefaultMsgFlags);
  // did it happen right away?
  if (rv >= 0) {
    base::StatsCounter write_bytes("tcp.write_bytes");
    write_bytes.Add(rv);
    if (rv > 0)
      use_history_.set_was_used_to_convey_data();
    net_log_.AddByteTransferEvent(
        NetLog::TYPE_SOCKET_BYTES_SENT, rv, buf->data());
    return rv;
  }

  if (LB::Platform::net_errno() != LB_NET_EWOULDBLOCK)
    return MapLastNetworkError();

  // set up for callback
  waiting_write_ = true;
  write_buf_ = buf;
  write_buf_len_ = buf_len;
  write_callback_ = callback;
  write_watcher_.StartWatching(
      socket_, base::MessagePumpShell::WATCH_WRITE, writer_);
  // no bytes were written currently, we return the EWOULDBLOCK mapped
  // to standard network errors
  return ERR_IO_PENDING;
}

void TCPClientSocketShell::DidCompleteWrite() {
  DCHECK(waiting_write_);
  int nwrite = send(socket_, write_buf_->data(), write_buf_len_, kDefaultMsgFlags);
  int result;
  if (nwrite >= 0) {
    result = nwrite;
    base::StatsCounter write_bytes("tcp.write_bytes");
    write_bytes.Add(nwrite);
    if (nwrite > 0)
      use_history_.set_was_used_to_convey_data();
    net_log_.AddByteTransferEvent(NetLog::TYPE_SOCKET_BYTES_SENT, result,
      write_buf_->data());
  } else {
    result = MapLastNetworkError();
  }

  if (result != ERR_IO_PENDING) {
    write_buf_ = NULL;
    write_buf_len_ = 0;
    waiting_write_ = false;
    DoWriteCallback(result);
  }
}

void TCPClientSocketShell::DoWriteCallback(int rv) {
  DCHECK_NE(rv, ERR_IO_PENDING);

  // since Run may result in Write being called, clear write_callback_ up front.
  CompletionCallback c = write_callback_;
  write_callback_ = CompletionCallback();
  c.Run(rv);
}

bool TCPClientSocketShell::SetReceiveBufferSize(int32 size) {
  return SetTCPReceiveBufferSize(socket_, size);
}

bool TCPClientSocketShell::SetSendBufferSize(int32 size) {
  // 512K is OS-dependent limit for send buffers
  DCHECK_LT(size, 512 * 1024);
  int rv = setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, CAST_OPTVAL(&size),
                      sizeof(size));
  DCHECK_EQ(rv, 0);
  return (rv == 0);
}

bool TCPClientSocketShell::SetKeepAlive(bool enable, int delay) {
  int socket = socket_ != kInvalidSocket ? socket_ : bound_socket_;
  return SetTCPKeepAlive(socket, enable, delay);
}

bool TCPClientSocketShell::SetNoDelay(bool no_delay) {
  int socket = socket_ != kInvalidSocket ? socket_ : bound_socket_;
  return SetTCPNoDelay(socket, no_delay);
}

#if defined(__LB_WIIU__)
bool TCPClientSocketShell::SetWindowScaling(bool enable) {
  int socket = socket_ != kInvalidSocket ? socket_ : bound_socket_;
  return SetTCPWindowScaling(socket, enable);
}
#endif

}  // namespace net
