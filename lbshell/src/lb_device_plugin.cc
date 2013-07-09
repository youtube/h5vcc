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
// Implements the WebKit plugin interface requirements.  As the sole function
// of this plugin is to provide the scriptable object most functions return
// defaults.

#include "lb_device_plugin.h"

#include "external/chromium/base/logging.h"
#include "lb_memory_manager.h"
#include "lb_scriptable_np_object.h"

LBDevicePlugin::LBDevicePlugin(WebKit::WebFrame* frame)
    : WebPlugin(), npObject_(NULL) {
  npp_ = LB_NEW NPP_t();
  npp_->pdata = frame;
  LBScriptableNPObject::InitNPClassTable(&scriptableNPClass_);
}

LBDevicePlugin::~LBDevicePlugin() {
  delete npp_;
}

NPObject *LBDevicePlugin::scriptableObject() {
  if (npObject_) return npObject_;
  npObject_ = NPN_CreateObject(npp_, &scriptableNPClass_);
  DCHECK(npObject_);
  return npObject_;
}
