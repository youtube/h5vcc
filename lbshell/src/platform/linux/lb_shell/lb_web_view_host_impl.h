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
#ifndef SRC_PLATFORM_LINUX_LB_SHELL_LB_WEB_VIEW_HOST_IMPL_H_
#define SRC_PLATFORM_LINUX_LB_SHELL_LB_WEB_VIEW_HOST_IMPL_H_

#include "lb_web_view_host.h"

class LBShell;

class LBWebViewHostImpl : public LBWebViewHost {
 public:
  explicit LBWebViewHostImpl(LBShell* shell);
  virtual void RunLoop() OVERRIDE;
  virtual ~LBWebViewHostImpl() {}

 private:
  void UpdateIO();
};

#endif  // SRC_PLATFORM_LINUX_LB_SHELL_LB_WEB_VIEW_HOST_IMPL_H_
