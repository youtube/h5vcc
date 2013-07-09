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
//
// For enumeration of methods, a meta-object can be invoked through the "_help"
// property.  WebKit does not seem to allow the direct enumeration of a plugin
// object's C methods.  If you try to enumerate them directly, you just get
// inherited HTMLObject properties.

#include <stdlib.h>

#include "external/chromium/base/logging.h"
#include "lb_memory_manager.h"
#include "lb_native_javascript_object.h"
#include "lb_scriptable_np_object.h"

LBScriptableNPObject::LBScriptableNPObject(NPP npp)
    : NPObject()
    , npp(npp)
    , nativeJSObject(LB_NEW LBNativeJavaScriptObject) {
}

// static
NPClass LBScriptableNPObject::metaClass;

// static
void LBScriptableNPObject::InitNPClassTable(NPClass* npClass) {
  memset(npClass, 0, sizeof(NPClass));
  npClass->structVersion = NP_CLASS_STRUCT_VERSION;
  npClass->allocate = LBScriptableNPObject::_Native_Allocate;
  npClass->deallocate = LBScriptableNPObject::_Native_Deallocate;
  npClass->hasProperty = LBScriptableNPObject::_Native_HasProperty;
  npClass->getProperty = LBScriptableNPObject::_Native_GetProperty;
  npClass->hasMethod = LBScriptableNPObject::_Native_HasMethod;
  npClass->invoke = LBScriptableNPObject::_Native_Invoke;

  memset(&metaClass, 0, sizeof(NPClass));
  metaClass.structVersion = NP_CLASS_STRUCT_VERSION;
  metaClass.hasProperty = LBScriptableNPObject::_Meta_HasProperty;
  metaClass.getProperty = LBScriptableNPObject::_Meta_GetProperty;
  metaClass.enumerate = LBScriptableNPObject::_Meta_Enumerate;
}

// static
NPObject* LBScriptableNPObject::_Native_Allocate(NPP npp, NPClass* npClass) {
  LBScriptableNPObject* obj = LB_NEW LBScriptableNPObject(npp);
  return static_cast<NPObject*>(obj);
}

// static
void LBScriptableNPObject::_Native_Deallocate(NPObject* npobj) {
  LBScriptableNPObject* obj = static_cast<LBScriptableNPObject*>(npobj);
  delete obj;
}

// static
bool LBScriptableNPObject::_Native_HasProperty(NPObject* npobj,
                                               NPIdentifier name) {
  LBScriptableNPObject* obj = static_cast<LBScriptableNPObject*>(npobj);
  DCHECK(obj);
  DCHECK(obj->nativeJSObject);
  if (NPN_IdentifierIsString(name)) {
    char* propertyName = NPN_UTF8FromIdentifier(name);
    bool rv;
#if !defined(__LB_SHELL__FOR_RELEASE__)
    if (strcmp(propertyName, "_help") == 0) {
      rv = true;
    } else
#endif
    {
      rv = obj->nativeJSObject->HasProperty(propertyName);
    }
    free(propertyName);
    return rv;
  }
  return false;
}

// static
bool LBScriptableNPObject::_Native_GetProperty(NPObject* npobj,
                                               NPIdentifier name,
                                               NPVariant* result) {
  LBScriptableNPObject* obj = static_cast<LBScriptableNPObject*>(npobj);
  DCHECK(obj);
  DCHECK(obj->nativeJSObject);
  if (NPN_IdentifierIsString(name)) {
    char* propertyName = NPN_UTF8FromIdentifier(name);
    bool rv;
#if !defined(__LB_SHELL__FOR_RELEASE__)
    if (strcmp(propertyName, "_help") == 0) {
      NPObject *meta = NPN_CreateObject(obj->npp, &metaClass);
      OBJECT_TO_NPVARIANT(meta, *result);
      rv = true;
    } else
#endif
    {
      rv = obj->nativeJSObject->GetProperty(propertyName, result);
    }
    free(propertyName);
    return rv;
  }
  return false;
}

// static
bool LBScriptableNPObject::_Native_HasMethod(NPObject* npobj,
                                             NPIdentifier name) {
  LBScriptableNPObject* obj = static_cast<LBScriptableNPObject*>(npobj);
  DCHECK(obj);
  DCHECK(obj->nativeJSObject);
  if (NPN_IdentifierIsString(name)) {
    char* propertyName = NPN_UTF8FromIdentifier(name);
    bool rv = obj->nativeJSObject->HasMethod(propertyName);
    free(propertyName);
    return rv;
  }
  return false;
}

// static
bool LBScriptableNPObject::_Native_Invoke(NPObject* npobj,
                                          NPIdentifier name,
                                          const NPVariant* args,
                                          uint32_t argCount,
                                          NPVariant* result) {
  LBScriptableNPObject* obj = static_cast<LBScriptableNPObject*>(npobj);
  DCHECK(obj);
  DCHECK(obj->nativeJSObject);
  WebKit::WebFrame* frame = static_cast<WebKit::WebFrame*>(obj->npp->pdata);
  DCHECK(frame);
  if (NPN_IdentifierIsString(name)) {
    char* propertyName = NPN_UTF8FromIdentifier(name);
    bool rv = obj->nativeJSObject->Invoke(propertyName, frame, args, argCount,
                                          result);
    free(propertyName);
    return rv;
  }
  return false;
}

// static
bool LBScriptableNPObject::_Meta_HasProperty(NPObject* npobj,
                                             NPIdentifier name) {
  bool rv = false;
  if (NPN_IdentifierIsString(name)) {
    char* propertyName = NPN_UTF8FromIdentifier(name);
    for (int i = 0; LBNativeJavaScriptObject::dispatchList[i].name; ++i) {
      const LBNativeJavaScriptObject::DispatchEntry &entry =
          LBNativeJavaScriptObject::dispatchList[i];
      if (strcmp(propertyName, entry.name) == 0) {
        rv = true;
        break;
      }
    }
    free(propertyName);
  }
  return rv;
}

// static
bool LBScriptableNPObject::_Meta_GetProperty(NPObject* npobj,
                                             NPIdentifier name,
                                             NPVariant* result) {
  bool rv = false;
  if (NPN_IdentifierIsString(name)) {
    char* propertyName = NPN_UTF8FromIdentifier(name);
    for (int i = 0; LBNativeJavaScriptObject::dispatchList[i].name; ++i) {
      const LBNativeJavaScriptObject::DispatchEntry &entry =
          LBNativeJavaScriptObject::dispatchList[i];
      if (strcmp(propertyName, entry.name) == 0) {
        char *helpString = strdup(entry.helpString);
        STRINGZ_TO_NPVARIANT(helpString, *result);
        rv = true;
        break;
      }
    }
    free(propertyName);
  }
  return rv;
}

// static
bool LBScriptableNPObject::_Meta_Enumerate(NPObject* npobj,
                                           NPIdentifier** identifier,
                                           uint32_t *count) {
  *count = 0;
  for (int i = 0; LBNativeJavaScriptObject::dispatchList[i].name; ++i) {
    ++(*count);
  }

  *identifier = static_cast<NPIdentifier*>(LB_MALLOC(sizeof(NPIdentifier*) * *count));
  for (int i = 0; LBNativeJavaScriptObject::dispatchList[i].name; ++i) {
    const LBNativeJavaScriptObject::DispatchEntry &entry =
        LBNativeJavaScriptObject::dispatchList[i];
    (*identifier)[i] = NPN_GetStringIdentifier(entry.name);
  }
  return true;
}
