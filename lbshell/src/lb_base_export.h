/*
 * Copyright 2013 Google Inc. All Rights Reserved.
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

#ifndef SRC_LB_BASE_EXPORT_H_
#define SRC_LB_BASE_EXPORT_H_

// Macros for exporting/importing from posix_emulation.h
// Note that posix_emulation is a DLL regardless of COMPONENT_BUILD.
// This is to allow us to use some DLLs that link against
// posix_emulation even on a static build.

#if defined (__LB_BASE_SHARED__)
#if defined(_MSC_VER)

#if defined(LB_BASE_IMPLEMENTATION)
#define LB_BASE_EXPORT __declspec(dllexport)
#define LB_BASE_EXPORT_PRIVATE __declspec(dllexport)
#else
#define LB_BASE_EXPORT __declspec(dllimport)
#define LB_BASE_EXPORT_PRIVATE __declspec(dllimport)
#endif  // defined(LB_BASE_IMPLEMENTATION)

// Macro for importing data, either via extern or dllimport.
#define LB_BASE_EXTERN LB_BASE_EXPORT extern

#else  // _MSC_VER
#error Unhandled compiler
#endif  // _MSC_VER

#else  // __LB_BASE_SHARED__

#define LB_BASE_EXPORT
#define LB_BASE_EXPORT_PRIVATE
#define LB_BASE_EXTERN extern
#endif  // __LB_BASE_SHARED__

#endif  // SRC_LB_BASE_EXPORT_H_

