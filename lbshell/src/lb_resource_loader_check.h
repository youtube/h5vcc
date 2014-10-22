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

#ifndef SRC_LB_RESOURCE_LOADER_CHECK_H_
#define SRC_LB_RESOURCE_LOADER_CHECK_H_

#include "external/chromium/net/url_request/url_request.h"
#include "external/chromium/webkit/glue/resource_loader_bridge.h"
#include "external/chromium/webkit/glue/webkit_glue.h"
#include "lb_shell_export.h"

// Steel has its own set of rules on what URL should be allowed and what not.
// Do this check here, and return true if everything is ok and the URL should
// be loaded.
bool LB_SHELL_EXPORT DoesHttpResponsePassSecurityCheck(
    const GURL& url,
    const webkit_glue::ResourceResponseInfo& info);

void HandleSSLCertificateError(net::URLRequest* request);

// Require SSL for all connections, regardless of mime-type whitelist.
bool LB_SHELL_EXPORT EnforceSSL();

#endif  // SRC_LB_RESOURCE_LOADER_CHECK_H_
