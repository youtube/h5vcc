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
// Provides a platform-independent JavaScript extension object.  This works
// in concert with LBScriptableNPClass and LBDevicePlugin to map JavaScript
// properties queried by the client to this object where they can be executed
// in native C++ and the results marshalled back to JavaScriptCore.

#ifndef _SRC_LBPS3_LBPS3_LB_NATIVE_JAVASCRIPT_OBJECT_H_
#define _SRC_LBPS3_LBPS3_LB_NATIVE_JAVASCRIPT_OBJECT_H_

#include <string.h>

#include "external/chromium/third_party/WebKit/Source/WebCore/plugins/npapi.h"
#include "external/chromium/third_party/WebKit/Source/WebCore/plugins/npruntime.h"

namespace WebKit {
  class WebFrame;
}

class LBNativeJavaScriptObject {
 public:
  typedef bool(*JSPropertyType)(NPVariant *);
  typedef bool(*JSMethodType)(WebKit::WebFrame *, const NPVariant*,
                              const uint32_t, NPVariant *);

  inline bool HasProperty(const char* name) {
    return findProperty(name) != NULL;
  }

  inline bool GetProperty(const char* name, NPVariant* result) {
    JSPropertyType propertyFn = findProperty(name);
    if (propertyFn)
      return propertyFn(result);
    return false;
  }

  inline bool HasMethod(const char* name) {
    return findMethod(name) != NULL;
  }

  inline bool Invoke(const char* name,
                     WebKit::WebFrame* frame,
                     const NPVariant* args,
                     uint32_t argCount,
                     NPVariant* result) {
    JSMethodType methodFn = findMethod(name);
    if (methodFn)
      return methodFn(frame, args, argCount, result);
    return false;
  }

  struct DispatchEntry {
    const char *name;
    union {
      LBNativeJavaScriptObject::JSPropertyType propertyFn;
      LBNativeJavaScriptObject::JSMethodType methodFn;
    };
    bool method;
    const char *helpString;

    DispatchEntry(const char *name,
                  JSPropertyType propertyFn,
                  const char *helpString)
      : name(name)
      , propertyFn(propertyFn)
      , method(false)
      , helpString(helpString) {
    }

    DispatchEntry(const char *name,
                  JSMethodType methodFn,
                  const char *helpString)
      : name(name)
      , methodFn(methodFn)
      , method(true)
      , helpString(helpString) {
    }

    // Used as a list terminator.
    DispatchEntry()
      : name(NULL)
      , propertyFn(NULL)
      , method(false)
      , helpString(NULL) {
    }
  };

  static DispatchEntry dispatchList[];

 protected:
  inline JSPropertyType findProperty(const char *name) {
    for (int i = 0; dispatchList[i].name; i++) {
      if (dispatchList[i].method == false &&
          strcmp(name, dispatchList[i].name) == 0) {
        return dispatchList[i].propertyFn;
      }
    }
    return NULL;
  }

  inline JSMethodType findMethod(const char *name) {
    for (int i = 0; dispatchList[i].name; i++) {
      if (dispatchList[i].method == true &&
          strcmp(name, dispatchList[i].name) == 0) {
        return dispatchList[i].methodFn;
      }
    }
    return NULL;
  }
};

#endif  // _SRC_LBPS3_LBPS3_LB_NATIVE_JAVASCRIPT_OBJECT_H_
