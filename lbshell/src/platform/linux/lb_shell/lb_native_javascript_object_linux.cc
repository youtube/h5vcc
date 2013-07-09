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
// Provides a Linux-specific JavaScript extension object.  This allows
// JavaScript access to native methods on Linux.
#include "lb_native_javascript_object.h"

#include <stdlib.h>

#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "lb_app_counters.h"
#include "lb_savegame_syncer.h"
#include "steel_build_id.h"

static bool platform(NPVariant *result) {
  char *data = strdup("Linux");
  STRINGZ_TO_NPVARIANT(data, *result);
  return true;
}

static bool buildID(NPVariant *result) {
  char *data = strdup(STEEL_BUILD_ID);
  STRINGZ_TO_NPVARIANT(data, *result);
  return true;
}

static bool gc(WebKit::WebFrame *frame, const NPVariant* args,
               const uint32_t argCount, NPVariant *result) {
  frame->collectGarbage();
  return true;
}

static bool programStartupCount(NPVariant *result) {
  INT32_TO_NPVARIANT(LBAppCounters::Startups(), *result);
  return true;
}

static bool programCleanExitCount(NPVariant *result) {
  INT32_TO_NPVARIANT(LBAppCounters::CleanExits(), *result);
  return true;
}

static bool createdTableCount(NPVariant *result) {
  INT32_TO_NPVARIANT(LBSavegameSyncer::GetCreatedTableCount(), *result);
  return true;
}

// A helper to avoid having to (mis-)type the function name twice.
#define declareDispatchEntry(function, helpString) \
    DispatchEntry(#function, function, helpString)

// static
LBNativeJavaScriptObject::DispatchEntry
    LBNativeJavaScriptObject::dispatchList[] = {

  declareDispatchEntry(platform,
      "The string 'Linux'."),
  declareDispatchEntry(buildID,
      "A string uniquely identifying a build of the application."),
  declareDispatchEntry(gc,
      "Force JavaScriptCore's garbage collector to run."),
  declareDispatchEntry(programStartupCount,
      "An integer; the number of times the program has been executed."),
  declareDispatchEntry(programCleanExitCount,
      "An integer; the number of times the program has exited cleanly."),
  declareDispatchEntry(createdTableCount,
      "An integer; the number of database tables created in this session."),

  DispatchEntry(),  // list terminator
};

