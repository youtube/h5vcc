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

// The RequestAnimationFrameCallback idl calls for a DOMTimeStamp argument
// to the handleEvent method.  The JavaScript bindings automatically generate
// an include for a JSDOMTimeStamp.h file as a result.  I think in V8 that a
// DOMTimeStamp is actually an object and so JS bindings get build for it
// automatically.  However in WebCore/dom a DOMTimeStamp is typedefed to
// a long long.  As a result there's no DOMTimeStamp.idl file and no
// JSDOMTimeStamp wrapper being generated.  We make this file to include
// the unwrapped native type to fix the build.  It lives here in
// platform/shell for lack of a better place to put it.
#include "DOMTimeStamp.h"
