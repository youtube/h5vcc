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
// This file is a fork of:
// external/chromium/webkit/tools/test_shell/simple_resource_loader_bridge.cc

// This file contains an implementation of the ResourceLoaderBridge class.
// The class is implemented using net::URLRequest, meaning it is a "simple"
// version that directly issues requests. The more complicated one used in the
// browser uses IPC.
//
// Because net::URLRequest only provides an asynchronous resource loading API,
// this file makes use of net::URLRequest from a background IO thread. Requests
// for cookies and synchronously loaded resources result in the main thread of
// the application blocking until the IO thread completes the operation.  (See
// GetCookies and SyncLoad)
//
// Main thread                          IO thread
// -----------                          ---------
// ResourceLoaderBridge <---o---------> RequestProxy (normal case)
//                           \            -> net::URLRequest
//                            o-------> SyncRequestProxy (synchronous case)
//                                        -> net::URLRequest
// SetCookie <------------------------> CookieSetter
//                                        -> net_util::SetCookie
// GetCookies <-----------------------> CookieGetter
//                                        -> net_util::GetCookies
//
// NOTE: The implementation in this file may be used to have WebKit fetch
// resources in-process.  For example, it is handy for building a single-
// process WebKit embedding (e.g., test_shell) that can use net::URLRequest to
// perform URL loads.  See renderer/resource_dispatcher.h for details on an
// alternate implementation that defers fetching to another process.

#include "lb_resource_loader_bridge.h"

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/debug/trace_event.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "base/time.h"
#include "base/timer.h"
#include "lb_http_user_agent_settings.h"
#include "lb_memory_manager.h"
#include "lb_on_screen_display.h"
#include "lb_request_context.h"
#include "lb_resource_loader_check.h"
#include "lb_shell/lb_shell_constants.h"
#if defined(__LB_XB1__)
#include "lb_shell/lb_user_manager_xb1.h"
#endif
#include "lb_web_view_host.h"
#include "lb_webblobregistry_impl.h"
#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "net/base/static_cookie_policy.h"
#include "net/base/upload_data.h"
#include "net/cookies/cookie_store.h"
#include "net/http/http_cache.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_job_factory.h"
#include "tcp_client_socket_shell.h"
#include "webkit/blob/blob_storage_controller.h"
#include "webkit/blob/blob_url_request_job_factory.h"
#include "webkit/glue/resource_loader_bridge.h"
#include "webkit/glue/resource_request_body.h"
#include "webkit/glue/webkit_glue.h"


using webkit_glue::ResourceLoaderBridge;
using webkit_glue::ResourceRequestBody;
using webkit_glue::ResourceResponseInfo;
using net::StaticCookiePolicy;
using net::HttpResponseHeaders;

bool LBResourceLoaderBridge::cookies_enabled_ = true;
#if !defined(__LB_SHELL__FOR_RELEASE__)
bool LBResourceLoaderBridge::perimeter_log_enabled_ = true;
bool LBResourceLoaderBridge::perimeter_check_enabled_ = true;
#endif

namespace {
struct LBRequestContextParams {
  LBRequestContextParams(
      net::CookieMonster::PersistentCookieStore *in_persistent_cookie_store,
      std::string in_preferred_language,
      bool in_no_proxy)
      : persistent_cookie_store(in_persistent_cookie_store),
        preferred_language(in_preferred_language),
        no_proxy(in_no_proxy) {}

  net::CookieMonster::PersistentCookieStore *persistent_cookie_store;
  std::string preferred_language;
  bool no_proxy;
};

bool g_accept_all_cookies = false;

StaticCookiePolicy::Type ComputeCookiePolicyType() {
  if (!LBResourceLoaderBridge::GetCookiesEnabled()) {
    return StaticCookiePolicy::BLOCK_ALL_COOKIES;
  }
  if (g_accept_all_cookies) {
    return StaticCookiePolicy::ALLOW_ALL_COOKIES;
  }
  return StaticCookiePolicy::BLOCK_SETTING_THIRD_PARTY_COOKIES;
}

class LBNetworkDelegate : public net::NetworkDelegate {
 public:
  virtual ~LBNetworkDelegate() {}

 protected:
  // net::NetworkDelegate implementation.
  virtual int OnBeforeURLRequest(net::URLRequest* request,
                                 const net::CompletionCallback& callback,
                                 GURL* new_url) OVERRIDE {
    return net::OK;
  }
  virtual int OnBeforeSendHeaders(net::URLRequest* request,
                                  const net::CompletionCallback& callback,
                                  net::HttpRequestHeaders* headers) OVERRIDE {
    return net::OK;
  }
  virtual void OnSendHeaders(net::URLRequest* request,
                             const net::HttpRequestHeaders& headers) OVERRIDE {}
  virtual int OnHeadersReceived(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>*
          override_response_headers) OVERRIDE {
    return net::OK;
  }
  virtual void OnBeforeRedirect(net::URLRequest* request,
                                const GURL& new_location) OVERRIDE {}
  virtual void OnResponseStarted(net::URLRequest* request) OVERRIDE {}
  virtual void OnRawBytesRead(const net::URLRequest& request,
                              int bytes_read) OVERRIDE {}
  virtual void OnCompleted(net::URLRequest* request, bool started) OVERRIDE {}
  virtual void OnURLRequestDestroyed(net::URLRequest* request) OVERRIDE {}

  virtual void OnPACScriptError(int line_number,
                                const string16& error) OVERRIDE {
  }
  virtual AuthRequiredResponse OnAuthRequired(
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      const AuthCallback& callback,
      net::AuthCredentials* credentials) OVERRIDE {
    return AUTH_REQUIRED_RESPONSE_NO_ACTION;
  }
  virtual bool OnCanGetCookies(const net::URLRequest& request,
                               const net::CookieList& cookie_list) OVERRIDE {
    StaticCookiePolicy policy(ComputeCookiePolicyType());
    int rv = policy.CanGetCookies(
        request.url(), request.first_party_for_cookies());
    return rv == net::OK;
  }
  virtual bool OnCanSetCookie(const net::URLRequest& request,
                              const std::string& cookie_line,
                              net::CookieOptions* options) OVERRIDE {
    StaticCookiePolicy policy(ComputeCookiePolicyType());
    int rv = policy.CanSetCookie(
        request.url(), request.first_party_for_cookies());
    return rv == net::OK;
  }
  virtual bool OnCanAccessFile(const net::URLRequest& request,
                               const FilePath& path) const OVERRIDE {
    return true;
  }
  virtual bool OnCanThrottleRequest(
      const net::URLRequest& request) const OVERRIDE {
    return false;
  }

  virtual int OnBeforeSocketStreamConnect(
      net::SocketStream* stream,
      const net::CompletionCallback& callback) OVERRIDE {
    return net::OK;
  }

  virtual void OnRequestWaitStateChange(const net::URLRequest& request,
                                        RequestWaitState state) OVERRIDE {}
};


LBRequestContextParams* g_request_context_params = NULL;
LBHttpUserAgentSettings* g_user_agent_settings = NULL;
LBRequestContext* g_request_context = NULL;
LBNetworkDelegate* g_network_delegate = NULL;

struct FileOverHTTPParams {
  FileOverHTTPParams(std::string in_file_path_template, GURL in_http_prefix)
      : file_path_template(in_file_path_template),
        http_prefix(in_http_prefix) {}

  std::string file_path_template;
  GURL http_prefix;
};

FileOverHTTPParams* g_file_over_http_params = NULL;

//-----------------------------------------------------------------------------

class IOThread : public base::Thread {
 public:
  IOThread() : base::Thread("IOThread") {
  }

  ~IOThread() {
    Stop();
  }

  virtual void Init() {
    if (g_request_context_params) {
      g_request_context = new LBRequestContext(
          g_request_context_params->persistent_cookie_store,
          g_request_context_params->no_proxy);
      g_user_agent_settings = new LBHttpUserAgentSettings(
          g_request_context_params->preferred_language);
      delete g_request_context_params;
      g_request_context_params = NULL;
    } else {
      g_request_context = new LBRequestContext();
      g_user_agent_settings = new LBHttpUserAgentSettings();
    }
    g_network_delegate = new LBNetworkDelegate();
    g_request_context->set_network_delegate(g_network_delegate);
    g_request_context->set_http_user_agent_settings(g_user_agent_settings);

    RegisterBlobProtocol();
  }

  static const int kWaitTimeoutMilliseconds = 1500;
  static const int kWaitPollIntervalMilliseconds = 100;

  void LetRemainingRequestsFinish() {
    // This is generally run on the main thread during app shutdown.
    // It must never be run on the IO thread itself.
    DCHECK(MessageLoop::current() != message_loop());

    std::set<const net::URLRequest*> *requests =
        g_request_context->url_requests();

    // Give last-minute requests a short time to complete.
    int num_requests = requests->size();
    int ms = 0;
    while (ms < kWaitTimeoutMilliseconds && requests->size()) {
      usleep(kWaitPollIntervalMilliseconds * 1000);
      ms += kWaitPollIntervalMilliseconds;
    }
    DLOG(INFO) << base::StringPrintf("Last %d requests took %d ms to finish.",
                                     num_requests, ms);
  }

  void RegisterBlobProtocol() {
    const char* kBlobScheme = "blob";

    net::URLRequestJobFactory* job_factory =
        const_cast<net::URLRequestJobFactory*>(
            g_request_context->job_factory());

    if (!job_factory->IsHandledProtocol(kBlobScheme)) {
      job_factory->SetProtocolHandler(
          kBlobScheme,
          new webkit_blob::BlobProtocolHandler(
              g_request_context->blob_storage_controller(),
              NULL,  // We don't support the FileSystem API
              message_loop_proxy()));
    } else {
      DLOG(WARNING) << "Blob protocol handler was already installed";
    }
  }

  virtual void CleanUp() {
    // In reverse order of initialization.
    if (g_request_context) {
      g_request_context->set_network_delegate(NULL);
      delete g_request_context;
      g_request_context = NULL;
    }
    if (g_user_agent_settings) {
      delete g_user_agent_settings;
      g_user_agent_settings = NULL;
    }
    delete g_network_delegate;
    g_network_delegate = NULL;
  }
};

IOThread* g_io_thread = NULL;

//-----------------------------------------------------------------------------

struct RequestParams {
  std::string method;
  GURL url;
  GURL first_party_for_cookies;
  GURL referrer;
  std::string headers;
  int load_flags;
  ResourceType::Type request_type;
  int appcache_host_id;
  bool download_to_file;
  scoped_refptr<ResourceRequestBody> request_body;
};

// The interval for calls to RequestProxy::MaybeUpdateUploadProgress
static const int kUpdateUploadProgressIntervalMsec = 33;

// The RequestProxy does most of its work on the IO thread.  The Start and
// Cancel methods are proxied over to the IO thread, where an net::URLRequest
// object is instantiated.
class RequestProxy : public net::URLRequest::Delegate,
                     public base::RefCountedThreadSafe<RequestProxy> {
 public:
  // Takes ownership of the params.
  RequestProxy()
     : owner_loop_(NULL),
       peer_(NULL),
       buf_(new ProxyIOBuffer<kDataSize>),
       last_upload_position_(0),
       internal_buffer_data_size_(0) {
    TRACE_EVENT0("lb_net", "RequestProxy::RequestProxy");
  }

  void DropPeer() {
    TRACE_EVENT0("lb_net", "RequestProxy::DropPeer");
    peer_ = NULL;
  }

  void Start(ResourceLoaderBridge::Peer* peer, RequestParams* params) {
    TRACE_EVENT0("lb_net", "RequestProxy::Start");

    peer_ = peer;
    owner_loop_ = MessageLoop::current();

    ConvertRequestParamsForFileOverHTTPIfNeeded(params);
    // proxy over to the io thread
    g_io_thread->message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&RequestProxy::AsyncStart, this, params));
  }

  void Cancel() {
    TRACE_EVENT0("lb_net", "RequestProxy::Cancel");

    // proxy over to the io thread
    g_io_thread->message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&RequestProxy::AsyncCancel, this));
  }

 protected:
  friend class base::RefCountedThreadSafe<RequestProxy>;

  virtual ~RequestProxy() {
    TRACE_EVENT0("lb_net", "RequestProxy::~RequestProxy");
    // If we have a request, then we'd better be on the io thread!
    DCHECK(!request_.get() ||
           MessageLoop::current() == g_io_thread->message_loop());
  }

  // --------------------------------------------------------------------------
  // The following methods are called on the owner's thread in response to
  // various net::URLRequest callbacks.  The event hooks, defined below, trigger
  // these methods asynchronously.

  void NotifyReceivedRedirect(const GURL& new_url,
                              const ResourceResponseInfo& info) {
    TRACE_EVENT0("lb_net", "RequestProxy::NotifyReceivedRedirect");
    bool has_new_first_party_for_cookies = false;
    GURL new_first_party_for_cookies;
    if (peer_ && peer_->OnReceivedRedirect(new_url, info,
                                           &has_new_first_party_for_cookies,
                                           &new_first_party_for_cookies)) {
      g_io_thread->message_loop()->PostTask(
          FROM_HERE,
          base::Bind(&RequestProxy::AsyncFollowDeferredRedirect, this,
                     has_new_first_party_for_cookies,
                     new_first_party_for_cookies));
    } else {
      Cancel();
    }
  }

  void NotifyReceivedResponse(const ResourceResponseInfo& info) {
    TRACE_EVENT0("lb_net", "RequestProxy::NotifyReceivedResponse");
    if (peer_)
      peer_->OnReceivedResponse(info);
  }

  void NotifyReceivedData(int bytes_read) {
    TRACE_EVENT0("lb_net", "RequestProxy::NotifyReceivedData");

    if (!peer_)
      return;

    // Swap buffer
    DCHECK_LE(bytes_read, kDataSize);
    buf_->Swap();

    // Continue reading more data into buf_
    // Note: Doing this before notifying our peer ensures our load events get
    // dispatched in a manner consistent with DumpRenderTree (and also avoids a
    // race condition).  If the order of the next 2 functions were reversed, the
    // peer could generate new requests in reponse to the received data, which
    // when run on the io thread, could race against this function in doing
    // another InvokeLater.
    g_io_thread->message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&RequestProxy::AsyncReadData, this));

    // Copy to buffer. Send to peer when buffer is full.
    int size_to_copy = std::min(bytes_read,
                                kDataSize - internal_buffer_data_size_);
    memcpy(internal_buffer_ + internal_buffer_data_size_, buf_->BackData(),
           size_to_copy);
    internal_buffer_data_size_ += size_to_copy;
    // If buffer is full, send it out.
    if (internal_buffer_data_size_ == kDataSize) {
      peer_->OnReceivedData(internal_buffer_, kDataSize, -1);

      // copy the data left in incoming buffer
      internal_buffer_data_size_ = bytes_read - size_to_copy;
      DCHECK_LT(internal_buffer_data_size_, kDataSize);
      if (internal_buffer_data_size_) {
        memcpy(internal_buffer_, buf_->BackData() + size_to_copy,
               internal_buffer_data_size_);
      }
    }
  }

  void NotifyCompletedRequest(int error_code,
                              const std::string& security_info,
                              const base::TimeTicks& complete_time) {
    TRACE_EVENT0("lb_net", "RequestProxy::NotifyCompletedRequest");
    if (peer_) {
      // Send the cached data
      if (internal_buffer_data_size_) {
        peer_->OnReceivedData(internal_buffer_, internal_buffer_data_size_, -1);
        internal_buffer_data_size_ = 0;
      }

      peer_->OnCompletedRequest(
          error_code, false, security_info, complete_time);
      DropPeer();  // ensure no further notifications
    }
  }

  void NotifyUploadProgress(uint64 position, uint64 size) {
    if (peer_)
      peer_->OnUploadProgress(position, size);
  }

  // --------------------------------------------------------------------------
  // The following methods are called on the io thread.  They correspond to
  // actions performed on the owner's thread.

  void AsyncStart(RequestParams* params) {
    TRACE_EVENT0("lb_net", "RequestProxy::AsyncStart");
    request_.reset(new net::URLRequest(params->url, this, g_request_context));
    request_->set_method(params->method);
    request_->set_first_party_for_cookies(params->first_party_for_cookies);
    request_->set_referrer(params->referrer.spec());
    net::HttpRequestHeaders headers;
    headers.AddHeadersFromString(params->headers);
    request_->SetExtraRequestHeaders(headers);
    request_->set_load_flags(params->load_flags);
    if (params->request_body) {
      request_->set_upload(make_scoped_ptr(
          params->request_body->ResolveElementsAndCreateUploadDataStream(
              g_request_context->blob_storage_controller())));
    }

    request_->Start();

    if (request_->has_upload() &&
        params->load_flags & net::LOAD_ENABLE_UPLOAD_PROGRESS) {
      upload_progress_timer_.reset(new base::RepeatingTimer<RequestProxy>());
      upload_progress_timer_->Start(FROM_HERE,
          base::TimeDelta::FromMilliseconds(kUpdateUploadProgressIntervalMsec),
          this, &RequestProxy::MaybeUpdateUploadProgress);
    }

    delete params;
  }

  void AsyncCancel() {
    TRACE_EVENT0("lb_net", "RequestProxy::AsyncCancel");

    // This can be null in cases where the request is already done.
    if (!request_.get())
      return;

    request_->Cancel();
    Done();
  }

  void AsyncFollowDeferredRedirect(bool has_new_first_party_for_cookies,
                                   const GURL& new_first_party_for_cookies) {
    TRACE_EVENT0("lb_net", "RequestProxy::AsyncFollowDeferredRedirect");

    // This can be null in cases where the request is already done.
    if (!request_.get())
      return;

    if (has_new_first_party_for_cookies)
      request_->set_first_party_for_cookies(new_first_party_for_cookies);
    request_->FollowDeferredRedirect();
  }

  void AsyncReadData() {
    TRACE_EVENT0("lb_net", "RequestProxy::AsyncReadData");

    // This can be null in cases where the request is already done.
    if (!request_.get())
      return;

    if (request_->status().is_success()) {
      int bytes_read;
      if (request_->Read(buf_, kDataSize, &bytes_read) && bytes_read) {
        OnReceivedData(bytes_read);
      } else if (!request_->status().is_io_pending()) {
        Done();
      }  // else wait for OnReadCompleted
    } else {
      Done();
    }
  }

  // --------------------------------------------------------------------------
  // The following methods are event hooks (corresponding to net::URLRequest
  // callbacks) that run on the IO thread.  They are designed to be overridden
  // by the SyncRequestProxy subclass.

  virtual void OnReceivedRedirect(
      const GURL& new_url,
      const ResourceResponseInfo& info,
      bool* defer_redirect) {
    TRACE_EVENT0("lb_net", "RequestProxy::OnReceivedRedirect");

    *defer_redirect = true;  // See AsyncFollowDeferredRedirect
    owner_loop_->PostTask(
        FROM_HERE,
        base::Bind(&RequestProxy::NotifyReceivedRedirect, this, new_url, info));
  }

  virtual void OnReceivedResponse(
      const ResourceResponseInfo& info) {
    TRACE_EVENT0("lb_net", "RequestProxy::OnReceivedResponse");
    owner_loop_->PostTask(
        FROM_HERE,
        base::Bind(&RequestProxy::NotifyReceivedResponse, this, info));
  }

  virtual void OnReceivedData(int bytes_read) {
    TRACE_EVENT0("lb_net", "RequestProxy::OnReceivedData");
    owner_loop_->PostTask(
        FROM_HERE,
        base::Bind(&RequestProxy::NotifyReceivedData, this, bytes_read));
  }

  virtual void OnCompletedRequest(int error_code,
                                  const std::string& security_info,
                                  const base::TimeTicks& complete_time) {
    TRACE_EVENT0("lb_net", "RequestProxy::OnCompletedRequest");
    // Error information can be obtained from
    //  external/chromium/net/base/net_error_list.h
    if (error_code && error_code != net::ERR_ABORTED &&
        error_code != net::ERR_FILE_NOT_FOUND) {
      DCHECK(request_.get());
      DLOG(INFO) << base::StringPrintf(
          "Network connection failed with : %d:%s on request %s\n",
          error_code, net::ErrorToString(error_code),
          request_->url().spec().c_str());
    }

    owner_loop_->PostTask(
        FROM_HERE,
        base::Bind(&RequestProxy::NotifyCompletedRequest, this, error_code,
                   security_info, complete_time));
  }

  // --------------------------------------------------------------------------
  // net::URLRequest::Delegate implementation:

  virtual void OnReceivedRedirect(net::URLRequest* request,
                                  const GURL& new_url,
                                  bool* defer_redirect) OVERRIDE {
    TRACE_EVENT0("lb_net", "RequestProxy::OnReceivedRedirect");

    DCHECK(request->status().is_success());
    ResourceResponseInfo info;
    PopulateResponseInfo(request, &info);
    // For file protocol, should never have the redirect situation.
    DCHECK(!ConvertResponseInfoForFileOverHTTPIfNeeded(request, &info));
    OnReceivedRedirect(new_url, info, defer_redirect);
  }

  virtual void OnResponseStarted(net::URLRequest* request) OVERRIDE {
    TRACE_EVENT0("lb_net", "RequestProxy::OnResponseStarted");
    if (request->status().is_success()) {
      ResourceResponseInfo info;
      PopulateResponseInfo(request, &info);
      const GURL& url = request->url();
      bool error = false;
      // If encountering error when requesting the file, cancel the request.
      if (ConvertResponseInfoForFileOverHTTPIfNeeded(request, &info) &&
          failed_file_request_status_.get()) {
        LBResourceLoaderBridge::OutputError(base::StringPrintf(
            "File request status failed for URL %s!\n", url.spec().c_str()));
        error = true;
      } else {
        error = !DoesHttpResponsePassSecurityCheck(url, info);
      }
      if (error) {
        AsyncCancel();
      } else {
        OnReceivedResponse(info);
        AsyncReadData();  // start reading
      }
    } else {  // if (request->status().is_success())
      Done();
    }  // if (request->status().is_success())
  }

  virtual void OnSSLCertificateError(net::URLRequest* request,
                                     const net::SSLInfo& ssl_info,
                                     bool fatal) OVERRIDE {
    TRACE_EVENT0("lb_net", "RequestProxy::OnSSLCertificateError");
    HandleSSLCertificateError(request);
  }

  virtual void OnReadCompleted(net::URLRequest* request,
                               int bytes_read) OVERRIDE {
    TRACE_EVENT0("lb_net", "RequestProxy::OnReadCompleted");
    if (request->status().is_success() && bytes_read > 0) {
      OnReceivedData(bytes_read);
    } else {
      Done();
    }
  }

  // --------------------------------------------------------------------------
  // Helpers and data:

  void Done() {
    TRACE_EVENT0("lb_net", "RequestProxy::Done");
    if (upload_progress_timer_) {
      MaybeUpdateUploadProgress();
      upload_progress_timer_.reset();
    }
    DCHECK(request_.get());
    // If |failed_file_request_status_| is not empty, which means the request
    // was a file request and encountered an error, then we need to use the
    // |failed_file_request_status_|. Otherwise use request_'s status.
    OnCompletedRequest(failed_file_request_status_.get() ?
                       failed_file_request_status_->error() :
                       request_->status().error(),
                       std::string(), base::TimeTicks());
    request_.reset();  // destroy on the io thread
  }

  // Called on the IO thread.
  void MaybeUpdateUploadProgress() {
    TRACE_EVENT0("lb_net", "RequestProxy::MaybeUpdateUploadProgress");
    // If a redirect is received upload is cancelled in net::URLRequest, we
    // should destroy the |upload_progress_timer_| timer and return.
    if (!request_->has_upload()) {
      upload_progress_timer_.reset();
      return;
    }

    net::UploadProgress progress = request_->GetUploadProgress();
    if (progress.position() == last_upload_position_)
      return;  // no progress made since last time

    const uint64 kHalfPercentIncrements = 200;
    const base::TimeDelta kOneSecond = base::TimeDelta::FromMilliseconds(1000);

    uint64 amt_since_last = progress.position() - last_upload_position_;
    base::TimeDelta time_since_last = base::TimeTicks::Now() -
                                      last_upload_ticks_;

    bool is_finished = (progress.size() == progress.position());
    bool enough_new_progress = (amt_since_last > (progress.size() /
                                                  kHalfPercentIncrements));
    bool too_much_time_passed = time_since_last > kOneSecond;

    if (is_finished || enough_new_progress || too_much_time_passed) {
      owner_loop_->PostTask(
          FROM_HERE,
          base::Bind(&RequestProxy::NotifyUploadProgress, this,
                     progress.position(), progress.size()));
      last_upload_ticks_ = base::TimeTicks::Now();
      last_upload_position_ = progress.position();
    }
  }

  void PopulateResponseInfo(net::URLRequest* request,
                            ResourceResponseInfo* info) const {
    TRACE_EVENT0("lb_net", "RequestProxy::PopulateResponseInfo");
    info->request_time = request->request_time();
    info->response_time = request->response_time();
    info->headers = request->response_headers();
    request->GetMimeType(&info->mime_type);
    request->GetCharset(&info->charset);
    info->content_length = request->GetExpectedContentSize();
  }

  // Called on owner thread
  void ConvertRequestParamsForFileOverHTTPIfNeeded(RequestParams* params) {
    TRACE_EVENT0("lb_net",
                 "RequestProxy::ConvertRequestParamsForFileOverHTTPIfNeeded");
    // Reset the status.
    file_url_prefix_ .clear();
    failed_file_request_status_.reset();
    // Only do this when enabling file-over-http and request is file scheme.
    if (!g_file_over_http_params || !params->url.SchemeIsFile())
      return;

    // For file protocol, method must be GET, POST or NULL.
    DCHECK(params->method == "GET" || params->method == "POST" ||
           params->method.empty());
    DCHECK(!params->download_to_file);

    if (params->method.empty())
      params->method = "GET";
    std::string original_request = params->url.spec();
    std::string::size_type found =
        original_request.find(g_file_over_http_params->file_path_template);
    if (found == std::string::npos)
      return;
    found += g_file_over_http_params->file_path_template.size();
    file_url_prefix_ = original_request.substr(0, found);
    original_request.replace(0, found,
        g_file_over_http_params->http_prefix.spec());
    params->url = GURL(original_request);
    params->first_party_for_cookies = params->url;
    // For file protocol, nerver use cache.
    params->load_flags = net::LOAD_BYPASS_CACHE;
  }

  // Called on IO thread.
  bool ConvertResponseInfoForFileOverHTTPIfNeeded(net::URLRequest* request,
      ResourceResponseInfo* info) {
    TRACE_EVENT0("lb_net",
                 "RequestProxy::ConvertResponseInfoForFileOverHTTPIfNeeded");
    // Only do this when enabling file-over-http and request url
    // matches the http prefix for file-over-http feature.
    if (!g_file_over_http_params || file_url_prefix_.empty())
      return false;
    std::string original_request = request->url().spec();
    std::string http_prefix = g_file_over_http_params->http_prefix.spec();
    DCHECK(!original_request.empty() &&
           StartsWithASCII(original_request, http_prefix, true));
    // Get the File URL.
    original_request.replace(0, http_prefix.size(), file_url_prefix_);

    FilePath file_path;
    if (!net::FileURLToFilePath(GURL(original_request), &file_path)) {
      NOTREACHED();
    }

    info->mime_type.clear();
    DCHECK(info->headers);
    int status_code = info->headers->response_code();
    // File protocol does not support response headers.
    info->headers = NULL;
    if (200 == status_code) {
      // Don't use the MIME type from HTTP server, use net::GetMimeTypeFromFile
      // instead.
      net::GetMimeTypeFromFile(file_path, &info->mime_type);
    } else {
      // If the file does not exist, immediately call OnCompletedRequest with
      // setting URLRequestStatus to FAILED.
      DCHECK(status_code == 404 || status_code == 403);
      if (status_code == 404) {
        failed_file_request_status_.reset(
            new net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                      net::ERR_FILE_NOT_FOUND));
      } else {
        failed_file_request_status_.reset(
            new net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                      net::ERR_ACCESS_DENIED));
      }
    }
    return true;
  }

  // Using double buffer IOBuffer to avoid memcpy.
  template<int buffer_size>
  class ProxyIOBuffer : public net::IOBuffer {
   public:
    ProxyIOBuffer() : net::IOBuffer(AlignedBuffer(0)), buffer_index_(0) {
    }

    ~ProxyIOBuffer() {
      data_ = NULL;  // To avoid delete of memory blocks
    }

    void Swap() {
      buffer_index_ = 1 - buffer_index_;
      data_ = AlignedBuffer(buffer_index_);
    }

    const char* BackData() const {
      return (const char*)AlignedBuffer(1 - buffer_index_);
    }

   private:
    char* AlignedBuffer(int index) const {
      uintptr_t buffer = (uintptr_t)(buffers_[index]);
      uintptr_t aligned_buf =
          buffer + (kNetworkIOBufferAlign - 1) &
          ~((uintptr_t)kNetworkIOBufferAlign - 1);
      DCHECK_EQ(aligned_buf % kNetworkIOBufferAlign, 0);
      DCHECK_GE(aligned_buf, buffer);
      DCHECK_LT(aligned_buf, buffer + kNetworkIOBufferAlign);
      return reinterpret_cast<char*>(aligned_buf);
    }
    int buffer_index_;
    char buffers_[2][buffer_size + kNetworkIOBufferAlign];
  };

  scoped_ptr<net::URLRequest> request_;

  // Size of our async IO data buffers
  static const int kDataSize =
      net::TCPClientSocketShell::kReceiveBufferSize &
      ~(kNetworkIOBufferAlign - 1);
  COMPILE_ASSERT((kDataSize % kNetworkIOBufferAlign) == 0,
      _IO_Buffer_size_not_multiple_of_alignment_);

  // read buffer for async IO
  scoped_refptr<ProxyIOBuffer<kDataSize> > buf_;

  MessageLoop* owner_loop_;

  // This is our peer in WebKit (implemented as ResourceHandleInternal). We do
  // not manage its lifetime, and we may only access it from the owner's
  // message loop (owner_loop_).
  ResourceLoaderBridge::Peer* peer_;

  // Timer used to pull upload progress info. Must be destroyed in the same
  // thread that was used to create it.
  scoped_ptr<base::RepeatingTimer<RequestProxy> > upload_progress_timer_;

  // Info used to determine whether or not to send an upload progress update.
  uint64 last_upload_position_;
  base::TimeTicks last_upload_ticks_;

  // Save the real FILE URL prefix for the FILE URL which converts to HTTP URL.
  std::string file_url_prefix_;
  // Save a failed file request status to pass it to webkit.
  scoped_ptr<net::URLRequestStatus> failed_file_request_status_;

  // Only notify peer when we have a full buffer, to reduce number of memory
  // blocks and make it easier to reuse the blocks.
  int internal_buffer_data_size_;
  char internal_buffer_[kDataSize];
};

//-----------------------------------------------------------------------------

class SyncRequestProxy : public RequestProxy {
 public:
  explicit SyncRequestProxy(ResourceLoaderBridge::SyncLoadResponse* result)
      : result_(result), event_(true, false) {
  }

  void WaitForCompletion() {
    event_.Wait();
  }

  // --------------------------------------------------------------------------
  // Event hooks that run on the IO thread:

  virtual void OnReceivedRedirect(
      const GURL& new_url,
      const ResourceResponseInfo& info,
      bool* defer_redirect) {
    // TODO(darin): It would be much better if this could live in WebCore, but
    // doing so requires API changes at all levels.  Similar code exists in
    // WebCore/platform/network/cf/ResourceHandleCFNet.cpp :-(
    if (new_url.GetOrigin() != result_->url.GetOrigin()) {
      DLOG(WARNING) << "Cross origin redirect denied";
      Cancel();
      return;
    }
    result_->url = new_url;
  }

  virtual void OnReceivedResponse(const ResourceResponseInfo& info) {
    *static_cast<ResourceResponseInfo*>(result_) = info;
  }

  virtual void OnReceivedData(int bytes_read) {
      result_->data.append(buf_->data(), bytes_read);
    AsyncReadData();  // read more (may recurse)
  }

  virtual void OnCompletedRequest(
      int error_code,
      const std::string& security_info,
      const base::TimeTicks& complete_time) OVERRIDE {
    result_->error_code = error_code;
    event_.Signal();
  }

 private:
  ResourceLoaderBridge::SyncLoadResponse* result_;
  base::WaitableEvent event_;
};

//-----------------------------------------------------------------------------

class ResourceLoaderBridgeImpl : public ResourceLoaderBridge {
 public:
  ResourceLoaderBridgeImpl(
      const webkit_glue::ResourceLoaderBridge::RequestInfo& request_info)
      : params_(new RequestParams),
        proxy_(NULL) {
    TRACE_EVENT0("lb_net",
                 "ResourceLoaderBridgeImpl::ResourceLoaderBridgeImpl");
    params_->method = request_info.method;
    params_->url = request_info.url;
    params_->first_party_for_cookies = request_info.first_party_for_cookies;
    params_->referrer = request_info.referrer;
    params_->headers = request_info.headers;
    params_->request_type = request_info.request_type;
    params_->appcache_host_id = request_info.appcache_host_id;
    params_->download_to_file = request_info.download_to_file;

    int seriously_bad_flags =
        net::LOAD_IGNORE_ALL_CERT_ERRORS |
        net::LOAD_IGNORE_CERT_AUTHORITY_INVALID |
        net::LOAD_IGNORE_CERT_COMMON_NAME_INVALID |
        net::LOAD_IGNORE_CERT_WRONG_USAGE |
        net::LOAD_DISABLE_CERT_REVOCATION_CHECKING |
        net::LOAD_DISABLE_INTERCEPT;
    // catch these flags in a debug build, remove them by force in release.
    DCHECK_EQ(0, request_info.load_flags & seriously_bad_flags);
    params_->load_flags = request_info.load_flags & ~seriously_bad_flags;
    // always ignore date-related errors:
    params_->load_flags |= net::LOAD_IGNORE_CERT_DATE_INVALID;
  }

  virtual ~ResourceLoaderBridgeImpl() {
    TRACE_EVENT0("lb_net",
                 "ResourceLoaderBridgeImpl::~ResourceLoaderBridgeImpl");
    if (proxy_) {
      proxy_->DropPeer();
      // Let the proxy die on the IO thread
      g_io_thread->message_loop()->ReleaseSoon(FROM_HERE, proxy_);
    }
  }

  // --------------------------------------------------------------------------
  // ResourceLoaderBridge implementation:

  virtual void SetRequestBody(ResourceRequestBody* request_body) {
    TRACE_EVENT0("lb_net", "ResourceLoaderBridgeImpl::SetRequestBody");
    DCHECK(params_.get());
    DCHECK(!params_->request_body);
    params_->request_body = request_body;
  }

  virtual bool Start(Peer* peer) {
    TRACE_EVENT0("lb_net", "ResourceLoaderBridgeImpl::Start");
    DCHECK(!proxy_);

    if (!LBResourceLoaderBridge::EnsureIOThread())
      return false;

    proxy_ = new RequestProxy();
    proxy_->AddRef();

    proxy_->Start(peer, params_.release());

    return true;  // Any errors will be reported asynchronously.
  }

  virtual void Cancel() {
    TRACE_EVENT0("lb_net", "ResourceLoaderBridgeImpl::Cancel");
    DCHECK(proxy_);
    proxy_->Cancel();
  }

  virtual void SetDefersLoading(bool value) {
    // TODO(darin): implement me
  }

  virtual void SyncLoad(SyncLoadResponse* response) {
    TRACE_EVENT0("lb_net", "ResourceLoaderBridgeImpl::SyncLoad");
    DCHECK(!proxy_);

    if (!LBResourceLoaderBridge::EnsureIOThread())
      return;

    // this may change as the result of a redirect
    response->url = params_->url;

    proxy_ = new SyncRequestProxy(response);
    proxy_->AddRef();

    proxy_->Start(NULL, params_.release());

    static_cast<SyncRequestProxy*>(proxy_)->WaitForCompletion();
  }

  virtual void UpdateRoutingId(int new_routing_id) { }

 private:
  // Ownership of params_ is transfered to the proxy when the proxy is created.
  scoped_ptr<RequestParams> params_;

  // The request proxy is allocated when we start the request, and then it
  // sticks around until this ResourceLoaderBridge is destroyed.  It is
  // dereferenced via the MessageLoop's ReleaseSoon method.
  RequestProxy* proxy_;
};

//-----------------------------------------------------------------------------

class CookieSetter : public base::RefCountedThreadSafe<CookieSetter> {
 public:
  void Set(const GURL& url, const std::string& cookie) {
    TRACE_EVENT0("lb_net", "CookieSetter::Set");
    DCHECK(MessageLoop::current() == g_io_thread->message_loop());
    g_request_context->cookie_store()->SetCookieWithOptionsAsync(
        url, cookie, net::CookieOptions(),
        net::CookieStore::SetCookiesCallback());
  }

 private:
  friend class base::RefCountedThreadSafe<CookieSetter>;

  ~CookieSetter() {}
};

class CookieGetter : public base::RefCountedThreadSafe<CookieGetter> {
 public:
  CookieGetter() : event_(false, false) {
  }

  void Get(const GURL& url) {
    TRACE_EVENT0("lb_net", "CookieGetter::Get");
    g_request_context->cookie_store()->GetCookiesWithOptionsAsync(
        url, net::CookieOptions(),
        base::Bind(&CookieGetter::OnGetCookies, this));
  }

  std::string GetResult() {
    TRACE_EVENT0("lb_net", "CookieGetter::GetResult");
    event_.Wait();
    return result_;
  }

 private:
  void OnGetCookies(const std::string& cookie_line) {
    TRACE_EVENT0("lb_net", "CookieGetter::OnGetCookies");
    result_ = cookie_line;
    event_.Signal();
  }
  friend class base::RefCountedThreadSafe<CookieGetter>;

  ~CookieGetter() {}

  base::WaitableEvent event_;
  std::string result_;
};

class CookieKiller : public base::RefCountedThreadSafe<CookieKiller> {
 public:
  void Kill() {
    TRACE_EVENT0("lb_net", "CookieKiller::Kill");
    DCHECK(MessageLoop::current() == g_io_thread->message_loop());
    g_request_context->cookie_store()->GetCookieMonster()->DeleteAllAsync(
        base::Callback<void(int)>());
  }

 private:
  friend class base::RefCountedThreadSafe<CookieKiller>;

  ~CookieKiller() {}
};

class CookiePurger : public base::RefCountedThreadSafe<CookiePurger> {
 public:
  explicit CookiePurger(const base::Closure& purge_complete_cb)
      : completed_cb_(purge_complete_cb) {
  }

  void Purge() {
    TRACE_EVENT0("lb_net", "CookiePurger::Purge");
    DCHECK(MessageLoop::current() == g_io_thread->message_loop());
    g_request_context->ResetCookieMonster();
    if (!completed_cb_.is_null()) {
      completed_cb_.Run();
    }
  }

 private:
  friend class base::RefCountedThreadSafe<CookiePurger>;

  ~CookiePurger() {}

  base::Closure completed_cb_;
};

}  // anonymous namespace

//-----------------------------------------------------------------------------

// static
void LBResourceLoaderBridge::Init(
    net::CookieMonster::PersistentCookieStore *persistent_cookie_store,
    std::string preferred_language,
    bool no_proxy) {
  TRACE_EVENT0("lb_net", "LBResourceLoaderBridge::Init");
  // Make sure to stop any existing IO thread since it may be using the
  // current request context.
  Shutdown();

  DCHECK(!g_request_context_params);
  DCHECK(!g_request_context);
  DCHECK(!g_network_delegate);
  DCHECK(!g_io_thread);

  g_request_context_params = new LBRequestContextParams(
      persistent_cookie_store,
      preferred_language,
      no_proxy);
}

// static
void LBResourceLoaderBridge::Shutdown() {
  TRACE_EVENT0("lb_net", "LBResourceLoaderBridge::Shutdown");
  if (g_io_thread) {
    g_io_thread->LetRemainingRequestsFinish();

    delete g_io_thread;
    g_io_thread = NULL;

    DCHECK(!g_request_context) << "should have been nulled by thread dtor";
    DCHECK(!g_network_delegate) << "should have been nulled by thread dtor";
  } else {
    delete g_request_context_params;
    g_request_context_params = NULL;

    if (g_file_over_http_params) {
      delete g_file_over_http_params;
      g_file_over_http_params = NULL;
    }
  }
}

// static
void LBResourceLoaderBridge::WaitForAllRequests() {
  if (g_io_thread) {
    g_io_thread->LetRemainingRequestsFinish();
  }
}

// static
void LBResourceLoaderBridge::SetCookie(const GURL& url,
                                       const GURL& first_party_for_cookies,
                                       const std::string& cookie) {
  // Proxy to IO thread to synchronize w/ network loading.
  TRACE_EVENT0("lb_net", "LBResourceLoaderBridge::SetCookie");

  if (!EnsureIOThread()) {
    NOTREACHED();
    return;
  }

  if (GetCookiesEnabled()) {
    scoped_refptr<CookieSetter> cookie_setter(new CookieSetter());
    g_io_thread->message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&CookieSetter::Set, cookie_setter.get(), url, cookie));
  }
}

// static
std::string LBResourceLoaderBridge::GetCookies(
    const GURL& url, const GURL& first_party_for_cookies) {
  // Proxy to IO thread to synchronize w/ network loading
  TRACE_EVENT0("lb_net", "LBResourceLoaderBridge::GetCookies");

  if (!EnsureIOThread()) {
    NOTREACHED();
    return std::string();
  }

  if (GetCookiesEnabled()) {
    scoped_refptr<CookieGetter> getter(new CookieGetter());

    g_io_thread->message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&CookieGetter::Get, getter.get(), url));

    return getter->GetResult();
  }
  return "";
}

// static
void LBResourceLoaderBridge::ClearCookies() {
  TRACE_EVENT0("lb_net", "LBResourceLoaderBridge::ClearCookies");
  // Proxy to IO thread to synchronize w/ network loading

  if (!EnsureIOThread()) {
    NOTREACHED();
    return;
  }

  scoped_refptr<CookieKiller> killer(new CookieKiller());

  g_io_thread->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&CookieKiller::Kill, killer.get()));
}

// static
void LBResourceLoaderBridge::PurgeCookies(
    const base::Closure& cookies_purged_cb) {
  if (!EnsureIOThread()) {
    NOTREACHED();
    return;
  }

  scoped_refptr<CookiePurger> purger(new CookiePurger(cookies_purged_cb));

  g_io_thread->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&CookiePurger::Purge, purger.get()));
}

// static
bool LBResourceLoaderBridge::GetCookiesEnabled() {
#if defined(__LB_XB1__)
  cookies_enabled_ = LBUserManager::GetCookiesEnabled();
#endif
  return cookies_enabled_;
}

// static
void LBResourceLoaderBridge::SetCookiesEnabled(bool value) {
#if defined(__LB_XB1__)
  LBUserManager::SetCookiesEnabled(value);
#endif
  cookies_enabled_ = value;
}

// static
bool LBResourceLoaderBridge::EnsureIOThread() {
  if (g_io_thread)
    return true;

  g_io_thread = new IOThread();

  return g_io_thread->StartWithOptions(
      base::Thread::Options(MessageLoop::TYPE_IO,
                            kIOThreadStackSize,
                            kIOThreadPriority,
                            kNetworkIOThreadAffinity));
}

// static
void LBResourceLoaderBridge::SetAcceptAllCookies(bool accept_all_cookies) {
  g_accept_all_cookies = accept_all_cookies;
}

// static
scoped_refptr<base::MessageLoopProxy>
    LBResourceLoaderBridge::GetCacheThread() {
  NOTREACHED() << "caching is disabled";
  return NULL;
}

// static
scoped_refptr<base::MessageLoopProxy>
    LBResourceLoaderBridge::GetIoThread() {
  if (!EnsureIOThread()) {
    LOG(DFATAL) << "Failed to create IO thread.";
    return NULL;
  }
  return g_io_thread->message_loop_proxy();
}

// static
void LBResourceLoaderBridge::AllowFileOverHTTP(
    const std::string& file_path_template, const GURL& http_prefix) {
  DCHECK(!file_path_template.empty());
  DCHECK(http_prefix.is_valid() &&
         (http_prefix.SchemeIs("http") || http_prefix.SchemeIs("https")));
  g_file_over_http_params = new FileOverHTTPParams(file_path_template,
                                                   http_prefix);
}

// static
webkit_glue::ResourceLoaderBridge* LBResourceLoaderBridge::Create(
    const webkit_glue::ResourceLoaderBridge::RequestInfo& request_info) {
  return new ResourceLoaderBridgeImpl(request_info);
}

// static
void LBResourceLoaderBridge::ChangeLanguage(const std::string& lang) {
  if (g_user_agent_settings) {
    // we already initialized the context
    g_user_agent_settings->set_accept_language(lang);
  } else if (g_request_context_params) {
    // we haven't initialized it yet, but we've already set the lang param
    g_request_context_params->preferred_language = lang;
  }
}

// static
void LBResourceLoaderBridge::OutputError(const std::string& error) {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  // QA builds don't print log warnings.
  printf("%s\n", error.c_str());
#endif
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  LB::OnScreenDisplay *osd = LB::OnScreenDisplay::GetPtr();
  if (osd) {
    osd->GetConsole()->Output(error);
    osd->ShowConsole();
  }
#endif
  DLOG(WARNING) << error;
}
