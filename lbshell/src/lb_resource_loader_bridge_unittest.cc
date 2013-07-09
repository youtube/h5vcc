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

#include "lb_resource_loader_bridge.h"

#include "external/chromium/base/file_path.h"
#include "external/chromium/base/lazy_instance.h"
#include "external/chromium/base/memory/ref_counted.h"
#include "external/chromium/base/message_loop_proxy.h"
#include "external/chromium/base/threading/thread.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/net/base/net_errors.h"
#include "external/chromium/net/base/tcp_listen_socket.h"
#include "external/chromium/net/base/test_completion_callback.h"
#include "external/chromium/net/http/http_cache.h"
#include "external/chromium/net/http/http_response_headers.h"
#include "external/chromium/net/http/http_status_code.h"
#include "external/chromium/net/server/http_server.h"
#include "external/chromium/net/server/http_server_request_info.h"

#include "external/chromium/webkit/glue/resource_loader_bridge.h"
#include "external/chromium/webkit/glue/resource_request_body.h"
#include "external/chromium/webkit/user_agent/user_agent.h"

#include "external/chromium/testing/gmock/include/gmock/gmock.h"
#include "external/chromium/testing/gtest/include/gtest/gtest.h"

using namespace webkit_glue;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

struct ResourceLoaderTestParam {
  const char* url;
  int response_code;
  int content_length;
  const char* mime_type;

  bool expected_result;
} whitelist_tests[] = {
  // Perfect responses
  { "https://www.youtube.com/htm", 200, 13, "text/html", true },
  { "https://www.youtube.com/ajs", 200, 13, "text/javascript", true },

  // SSL needed
  { "http://www.youtube.com/htm", 200, 13, "text/html", false },
  { "http://www.youtube.com/ajs", 200, 13, "text/javascript", false },
  { "http://www.youtube.com/blb", 200, 13, "application/octet-stream", false },

  // Falls under the whitelisted Mime-list
  { "http://www.youtube.com/xml", 200, 13, "text/xml", true },
  { "http://www.youtube.com/blb", 200, 13, "binary/octet-stream", true },
  { "http://www.youtube.com/img", 200, 13, "image/weird-img-mime", true },
  { "http://www.youtube.com/vid", 200, 13, "video/weird-video-mime", true },

  // Sends a 204 but with non-zero content-length
  { "http://www.youtube.com/htm", 204, 13, "text/html", false },
  { "https://www.youtube.com/js", 204, 13, "text/javascript", false },

  // Fails the whitelist.
  { "http://www.example.org/",  200, 3, "text/xml", false },
  { "https://www.example.org/", 200, 3, "video/mp4", false },

  // Fails whitelist but still passes because of 204
  { "http://www.example.org/",  204, 0, "text/html", true },
  { "https://www.example.org/", 204, 0, "text/javascript", true },

  // Local url's always pass
  { "local:///var/log/syslog",  200, 15, "text/html", true },
  { "local:///usr/bin/rm",  200, 40, "application/octet-stream", true },

};

class ResourceLoaderCheckTest :
    public ::testing::TestWithParam<ResourceLoaderTestParam> {
  virtual void SetUp() OVERRIDE {
    LBResourceLoaderBridge::SetPerimeterCheckLogging(false);
    LBResourceLoaderBridge::SetPerimeterCheckEnabled(true);
  }
};

TEST_P(ResourceLoaderCheckTest, Behaves) {
  const ResourceLoaderTestParam& instance = GetParam();
  ResourceResponseInfo info;
  info.headers = make_scoped_refptr(new net::HttpResponseHeaders(StringPrintf(
      "HTTP/1.1 %d OK\r\n", instance.response_code)));
  ASSERT_EQ(instance.response_code, info.headers->response_code());

  info.mime_type = instance.mime_type;
  info.content_length = instance.content_length;

  EXPECT_EQ(instance.expected_result,
            LBResourceLoaderBridge::DoesHttpResponsePassSecurityCheck(
                GURL(instance.url), info));
}

INSTANTIATE_TEST_CASE_P(ResourceLoaderCheckTestInstances,
                        ResourceLoaderCheckTest,
                        ::testing::ValuesIn(whitelist_tests));

