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
#ifndef SRC_TCP_CLIENT_SOCKET_SHELL_H_
#define SRC_TCP_CLIENT_SOCKET_SHELL_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/threading/non_thread_safe.h"
#include "object_watcher_shell.h"
#include "net/base/address_list.h"
#include "net/base/completion_callback.h"
#include "net/base/net_log.h"
#include "net/socket/stream_socket.h"

#include "lb_shell/lb_shell_constants.h"

namespace net {

// A client socket that uses TCP as the transport layer.
class TCPClientSocketShell : public StreamSocket, base::NonThreadSafe {
 public:
  static const int kReceiveBufferSize = kNetworkReceiveBufferSize;
  // The IP address(es) and port number to connect to.  The TCP socket will try
  // each IP address in the list until it succeeds in establishing a
  // connection.
  TCPClientSocketShell(const AddressList& addresses,
                       net::NetLog* net_log,
                       const net::NetLog::Source& source);

  virtual ~TCPClientSocketShell();

  // AdoptSocket causes the given, connected socket to be adopted as a TCP
  // socket. This object must not be connected. This object takes ownership of
  // the given socket and then acts as if Connect() had been called. This
  // function is used by TCPServerSocket() to adopt accepted connections
  // and for testing.
  int AdoptSocket(int socket);

  // Binds the socket to a local IP address and port.
  int Bind(const IPEndPoint& address);

  // StreamSocket implementation.
  virtual int Connect(const CompletionCallback& callback) OVERRIDE;
  virtual void Disconnect() OVERRIDE;
  virtual bool IsConnected() const OVERRIDE;
  virtual bool IsConnectedAndIdle() const OVERRIDE;
  virtual int GetPeerAddress(IPEndPoint* address) const OVERRIDE;
  virtual int GetLocalAddress(IPEndPoint* address) const OVERRIDE;
  virtual const BoundNetLog& NetLog() const OVERRIDE;
  virtual void SetSubresourceSpeculation() OVERRIDE;
  virtual void SetOmniboxSpeculation() OVERRIDE;
  virtual bool WasEverUsed() const OVERRIDE;
  virtual bool UsingTCPFastOpen() const OVERRIDE;
  virtual int64 NumBytesRead() const OVERRIDE;
  virtual base::TimeDelta GetConnectTimeMicros() const OVERRIDE;
  virtual bool WasNpnNegotiated() const OVERRIDE;
  virtual NextProto GetNegotiatedProtocol() const OVERRIDE;
  virtual bool GetSSLInfo(SSLInfo* ssl_info) OVERRIDE;

  // Socket methods:
  // Multiple outstanding requests are not supported.
  // Full duplex mode (reading and writing at the same time) is supported
  virtual int Read(IOBuffer* buf,
                   int buf_len,
                   const CompletionCallback& callback) OVERRIDE;
  virtual int Write(IOBuffer* buf,
                    int buf_len,
                    const CompletionCallback& callback) OVERRIDE;
  virtual bool SetReceiveBufferSize(int32 size) OVERRIDE;
  virtual bool SetSendBufferSize(int32 size) OVERRIDE;

  virtual bool SetKeepAlive(bool enable, int delay);
  virtual bool SetNoDelay(bool no_delay);
#if defined(__LB_WIIU__)
  virtual bool SetWindowScaling(bool enable);
#endif

 private:
  // State machine states for connecting the socket asynchronously.
  enum ConnectState {
    CONNECT_STATE_CONNECT,
    CONNECT_STATE_CONNECT_COMPLETE,
    CONNECT_STATE_NONE,
  };

  // Returns true if a Connect() is in progress.
  bool waiting_connect() const {
    return next_connect_state_ != CONNECT_STATE_NONE;
  }

  // State machine used by Connect().
  int DoConnectLoop(int result);
  int DoConnect();
  int DoConnectComplete(int result);

  // Helper to add a TCP_CONNECT (end) event to the NetLog.
  void LogConnectCompletion(int net_error);

  // Helper used by Disconnect(), which disconnects minus the logging and
  // resetting of current_ai_.
  void DoDisconnect();

  void DoReadCallback(int rv);
  void DoWriteCallback(int rv);
  void DidCompleteRead();
  void DidCompleteWrite();
  void DidCompleteConnect();

  // current state of the connect state machine
  ConnectState next_connect_state_;
  // The OS error that CONNECT_STATE_CONNECT last completed with.
  int connect_os_error_;
  // This socket was previously disconnected and has not been re-connected.
  bool previously_disconnected_;

  // Record of connectivity and transmissions, for use in speculative
  // connection histograms.
  UseHistory use_history_;

  // forward declaration of our delegate objects for callbacks
  // from their respective watchers
  class ReadDelegate;
  class WriteDelegate;
  // |read_watcher_| watches for events from Connect() and Read().
  base::steel::ObjectWatcher read_watcher_;
  // |write_watcher_| watches for events from Write();
  base::steel::ObjectWatcher write_watcher_;
  // |reader_| handles the signals from |read_watcher_|.
  ReadDelegate * reader_;
  // |writer_| handles the signals from |write_watcher_|.
  WriteDelegate * writer_;
    // External callback; called when connect or read is complete.
  CompletionCallback read_callback_;
  // External callback; called when write is complete.
  CompletionCallback write_callback_;
  // The various read/write states that a full-duplex socket can be in.
  bool waiting_read_;
  bool waiting_write_;
  // The buffer used by OnSocketReady to retry Read requests
  scoped_refptr<IOBuffer> read_buf_;
  int read_buf_len_;

  // The buffer used by OnSocketReady to retry Write requests
  scoped_refptr<IOBuffer> write_buf_;
  int write_buf_len_;

  BoundNetLog net_log_;

  // the socket descriptor used by this object
  int socket_;

  // Local IP address and port we are bound to. Set to NULL if Bind()
  // was't called (in that cases OS chooses address/port).
  scoped_ptr<IPEndPoint> bind_address_;

  // Stores bound socket between Bind() and Connect() calls.
  int bound_socket_;

  // The list of addresses we should try in order to establish a connection.
  AddressList addresses_;
  // Where we are in above list. Set to -1 if uninitialized.
  int current_address_index_;

  base::TimeTicks connect_start_time_;
  base::TimeDelta connect_time_micros_;
  int64 num_bytes_read_;

  DISALLOW_COPY_AND_ASSIGN(TCPClientSocketShell);
};

}  // namespace net

#endif  // SRC_TCP_CLIENT_SOCKET_SHELL_H_
