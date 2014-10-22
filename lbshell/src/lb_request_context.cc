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
// external/chromium/webkit/tools/test_shell/test_shell_request_context.cc

#include "lb_request_context.h"

#include "build/build_config.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/threading/worker_pool.h"
#include "net/base/cert_verify_proc.h"
#include "net/base/default_server_bound_cert_store.h"
#include "net/base/host_resolver.h"
#include "net/base/multi_threaded_cert_verifier.h"
#include "net/base/server_bound_cert_service.h"
#include "net/base/ssl_config_service_defaults.h"
#include "net/cookies/cookie_monster.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties_impl.h"
#include "net/proxy/proxy_config_service.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/proxy/proxy_service.h"
#include "net/socket/client_socket_pool.h"
#include "net/socket/client_socket_pool_manager.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebKitPlatformSupport.h"
#include "webkit/blob/blob_storage_controller.h"
#include "webkit/glue/webkit_glue.h"

#if __LB_ENABLE_NATIVE_HTTP_STACK__
#include "net/http/shell/http_transaction_factory_shell.h"
#endif

#include "chromium/net/proxy/proxy_config_service_shell.h"
#include "lb_network_helpers.h"
#include "lb_resource_loader_bridge.h"
#include "lb_shell_switches.h"
#include "lb_webblobregistry_impl.h"

LBRequestContext::LBRequestContext()
    : ALLOW_THIS_IN_INITIALIZER_LIST(storage_(this)) {
  Init(NULL, false);
}

LBRequestContext::LBRequestContext(
    net::CookieMonster::PersistentCookieStore *persistent_cookie_store,
    bool no_proxy)
    : ALLOW_THIS_IN_INITIALIZER_LIST(storage_(this)) {
  Init(persistent_cookie_store, no_proxy);
}

void LBRequestContext::Init(
    net::CookieMonster::PersistentCookieStore *persistent_cookie_store,
    bool no_proxy) {
  persistent_cookie_store_ = persistent_cookie_store;
  storage_.set_cookie_store(new net::CookieMonster(persistent_cookie_store,
      NULL));
  storage_.set_server_bound_cert_service(new net::ServerBoundCertService(
      new net::DefaultServerBoundCertStore(NULL),
      base::WorkerPool::GetTaskRunner(true)));

  blob_storage_controller_.reset(new webkit_blob::BlobStorageController());

  // LBRequestContext must be created on the I/O thread
  LBWebBlobRegistryImpl::InitFromIOThread(blob_storage_controller_.get());

  scoped_ptr<net::ProxyConfigService> proxy_config_service;

  std::string address;
#if !defined(__LB_SHELL__FOR_RELEASE__)
  CommandLine *cl = CommandLine::ForCurrentProcess();
  address = cl->GetSwitchValueASCII(LB::switches::kProxy);
  if (!address.empty()) {
    DLOG(INFO) << "Using fixed proxy address: " << address;
    net::ProxyConfig pc;
    pc.proxy_rules().ParseFromString(address);
    proxy_config_service.reset(new net::ProxyConfigServiceFixed(pc));
    LBResourceLoaderBridge::SetPerimeterCheckEnabled(false);
    LBResourceLoaderBridge::SetPerimeterCheckLogging(false);
  }
#endif
  if (address.empty()) {
    proxy_config_service.reset(new net::ProxyConfigServiceShell());
  }

#if !__LB_ENABLE_NATIVE_HTTP_STACK__
  net::HostResolver::Options options;
  options.max_concurrent_resolves = net::HostResolver::kDefaultParallelism;
  options.max_retry_attempts = net::HostResolver::kDefaultRetryAttempts;
  options.enable_caching = true;
  storage_.set_host_resolver(
      net::HostResolver::CreateSystemResolver(options, NULL));

  storage_.set_cert_verifier(new net::MultiThreadedCertVerifier(
      net::CertVerifyProc::CreateDefault()));
  storage_.set_ssl_config_service(
      new net::SSLConfigServiceDefaults);

  storage_.set_http_auth_handler_factory(
      net::HttpAuthHandlerFactory::CreateDefault(host_resolver()));
  storage_.set_http_server_properties(
      new net::HttpServerPropertiesImpl);
#endif
  storage_.set_proxy_service(net::ProxyService::CreateUsingSystemProxyResolver(
      proxy_config_service.release(), 0, NULL));
  net::HttpNetworkSession::Params params;
  params.client_socket_factory = NULL;  // TODO
  params.host_resolver = host_resolver();
  params.cert_verifier = cert_verifier();
  params.server_bound_cert_service = server_bound_cert_service();
  params.transport_security_state = NULL;
  params.proxy_service = proxy_service();
  params.ssl_session_cache_shard = "";
  params.ssl_config_service = ssl_config_service();
  params.http_auth_handler_factory = http_auth_handler_factory();
  params.network_delegate  = NULL;
  params.http_server_properties = http_server_properties();
  params.net_log = NULL;
  params.trusted_spdy_proxy = "";

#if defined(__LB_WIIU__)
  LB::Platform::ConfigureSocketPools();
#endif

#if !__LB_ENABLE_NATIVE_HTTP_STACK__
  // disable caching
  net::HttpCache::DefaultBackend* backend = NULL;
  net::HttpCache* cache = new net::HttpCache(params, backend);
  cache->set_mode(net::HttpCache::DISABLE);
  storage_.set_http_transaction_factory(cache);
#else
  net::HttpTransactionFactory* transaction_factory =
      new net::HttpTransactionFactoryShell(params);
  storage_.set_http_transaction_factory(transaction_factory);
#endif
  net::URLRequestJobFactory* job_factory = new net::URLRequestJobFactoryImpl;

  storage_.set_job_factory(job_factory);
}

void LBRequestContext::ResetCookieMonster() {
  storage_.set_cookie_store(
      new net::CookieMonster(persistent_cookie_store_, NULL));
}

LBRequestContext::~LBRequestContext() {
  if (blob_storage_controller_) {
    // LBRequestContext must be destroyed on the I/O thread
    LBWebBlobRegistryImpl::CleanUpFromIOThread();
  }
}

