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

#include "external/chromium/build/build_config.h"
#include "external/chromium/base/compiler_specific.h"
#include "external/chromium/base/file_path.h"
#include "external/chromium/base/threading/worker_pool.h"
#include "external/chromium/net/base/cert_verify_proc.h"
#include "external/chromium/net/base/default_server_bound_cert_store.h"
#include "external/chromium/net/base/host_resolver.h"
#include "external/chromium/net/base/multi_threaded_cert_verifier.h"
#include "external/chromium/net/base/server_bound_cert_service.h"
#include "external/chromium/net/base/ssl_config_service_defaults.h"
#include "external/chromium/net/cookies/cookie_monster.h"
#include "external/chromium/net/http/http_auth_handler_factory.h"
#include "external/chromium/net/http/http_network_session.h"
#include "external/chromium/net/http/http_server_properties_impl.h"
#include "external/chromium/net/proxy/proxy_config_service.h"
#include "external/chromium/net/proxy/proxy_config_service_fixed.h"
#include "external/chromium/net/proxy/proxy_service.h"
#include "external/chromium/net/socket/client_socket_pool.h"
#include "external/chromium/net/socket/client_socket_pool_manager.h"
#include "external/chromium/net/url_request/url_request_job_factory.h"
#include "external/chromium/net/url_request/url_request_job_factory_impl.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebKitPlatformSupport.h"
#include "external/chromium/webkit/blob/blob_storage_controller.h"
#include "external/chromium/webkit/glue/webkit_glue.h"

#include "lb_memory_manager.h"
#include "lb_platform.h"
#include "lb_resource_loader_bridge.h"

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
  storage_.set_cookie_store(LB_NEW net::CookieMonster(persistent_cookie_store,
      NULL));
  storage_.set_server_bound_cert_service(LB_NEW net::ServerBoundCertService(
      LB_NEW net::DefaultServerBoundCertStore(NULL),
      base::WorkerPool::GetTaskRunner(true)));

  // Use the system proxy settings.
  scoped_ptr<net::ProxyConfigService> proxy_config_service(
      net::ProxyService::CreateSystemProxyConfigService(
           NULL, MessageLoop::current()));

  net::HostResolver::Options options;
  options.max_concurrent_resolves = net::HostResolver::kDefaultParallelism;
  options.max_retry_attempts = net::HostResolver::kDefaultRetryAttempts;
  options.enable_caching = true;
  storage_.set_host_resolver(
      net::HostResolver::CreateSystemResolver(options, NULL));

  storage_.set_cert_verifier(LB_NEW net::MultiThreadedCertVerifier(
      net::CertVerifyProc::CreateDefault()));
  storage_.set_proxy_service(net::ProxyService::CreateUsingSystemProxyResolver(
      proxy_config_service.release(), 0, NULL));
  storage_.set_ssl_config_service(
      LB_NEW net::SSLConfigServiceDefaults);

  storage_.set_http_auth_handler_factory(
      net::HttpAuthHandlerFactory::CreateDefault(host_resolver()));
  storage_.set_http_server_properties(
      LB_NEW net::HttpServerPropertiesImpl);

  net::HttpNetworkSession::Params params;
  params.client_socket_factory = NULL; // TODO
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

  // disable caching
  net::HttpCache::DefaultBackend* backend = NULL;
  net::HttpCache* cache = LB_NEW net::HttpCache(params, backend);
  cache->set_mode(net::HttpCache::DISABLE);
  storage_.set_http_transaction_factory(cache);

  net::URLRequestJobFactory* job_factory = LB_NEW net::URLRequestJobFactoryImpl;

  storage_.set_job_factory(job_factory);
}

LBRequestContext::~LBRequestContext() {
}

