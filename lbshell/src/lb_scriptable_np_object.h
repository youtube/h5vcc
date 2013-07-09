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
// For JavaScript interaction with the WebPlugin defined by LBDevicePlugin the
// plugin will be asked to provide a NPScriptableObject, an object derived from
// the NPObject, the root object in the Netscape Plugin API.  This object
// provides the appropriate platform-independent marshalling to call a
// LBNativeJavaScriptObject, which actually does the platform-specific work.

#ifndef _SRC_LBPS3_LBPS3_LB_SCRIPTABLE_NP_OBJECT_H_
#define _SRC_LBPS3_LBPS3_LB_SCRIPTABLE_NP_OBJECT_H_

#include "external/chromium/third_party/WebKit/Source/WebCore/plugins/npapi.h"
#include "external/chromium/third_party/WebKit/Source/WebCore/plugins/npruntime.h"

class LBNativeJavaScriptObject;

class LBScriptableNPObject : public NPObject {
 public:
  // fills the provided NPClass table with pointers to our static methods
  static void InitNPClassTable(NPClass* npClass);

 private:
  // these are not called explicitly but are rather called by the NPAPI runtime
  static NPObject* _Native_Allocate(NPP npp, NPClass* npClass);
  static void _Native_Deallocate(NPObject* npobj);
  static bool _Native_HasProperty(NPObject* npobj, NPIdentifier name);
  static bool _Native_GetProperty(NPObject* npobj, NPIdentifier name,
                                  NPVariant* result);
  static bool _Native_HasMethod(NPObject* npobj, NPIdentifier name);
  static bool _Native_Invoke(NPObject* npobj,
                             NPIdentifier name,
                             const NPVariant* args,
                             uint32_t argCount,
                             NPVariant* result);

  static bool _Meta_HasProperty(NPObject* npobj, NPIdentifier name);
  static bool _Meta_GetProperty(NPObject* npobj, NPIdentifier name,
                                NPVariant* result);
  static bool _Meta_Enumerate(NPObject* npobj, NPIdentifier** identifier,
                              uint32_t *count);

  explicit LBScriptableNPObject(NPP npp);

  NPP npp;
  static NPClass metaClass;
  LBNativeJavaScriptObject* nativeJSObject;
};

#endif  // _SRC_LBPS3_LBPS3_LB_SCRIPTABLE_NP_OBJECT_H_
