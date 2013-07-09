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

// WebKit defines a complicated plugin API in which the externally-visible
// API calls (those without an underscore starting their name) are bound via
// a function table to the correct internal (underscore-prefixed) functions.
// For the gritty details of the binding that we aren't doing see 
// WebCore/plugins/PluginPackage.cpp.  Since the external Netscape Plugin API
// is defined in npruntime.h and npapi.h the code will compile but not link,
// as none of the non-underscore functions have been defined.  So this file
// has to live inside of WebKit, where it can see the internal functions, and
// acts as a simple static bridge, calling the private function for each call
// of the public function.

#include "config.h"

#if defined(__LB_SHELL__)
#if ENABLE(NETSCAPE_PLUGIN_API)

#include "npruntime.h"
#include "npapi.h"
#include "npruntime_impl.h"

NPObject *NPN_CreateObject(NPP npp, NPClass *aClass) {
  return _NPN_CreateObject(npp, aClass);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8* name) {
  return _NPN_GetStringIdentifier(name);
}

bool NPN_IdentifierIsString(NPIdentifier identifier) {
  return _NPN_IdentifierIsString(identifier);
}

NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier) {
  return _NPN_UTF8FromIdentifier(identifier);
}

#endif // ENABLE(NETSCAPE_PLUGIN_API)
#endif // __LB_SHELL__
