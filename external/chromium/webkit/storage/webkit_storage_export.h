// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_STORAGE_STORAGE_EXPORT_H_
#define WEBKIT_STORAGE_STORAGE_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(_MSC_VER)

#if defined(WEBKIT_STORAGE_IMPLEMENTATION)
#define WEBKIT_STORAGE_EXPORT __declspec(dllexport)
#define WEBKIT_STORAGE_EXPORT_PRIVATE __declspec(dllexport)
#else
#define WEBKIT_STORAGE_EXPORT __declspec(dllimport)
#define WEBKIT_STORAGE_EXPORT_PRIVATE __declspec(dllimport)
#endif  // defined(STORAGE_IMPLEMENTATION)

#else // defined(WIN32)
#if defined(WEBKIT_STORAGE_IMPLEMENTATION)
#define WEBKIT_STORAGE_EXPORT __attribute__((visibility("default")))
#define WEBKIT_STORAGE_EXPORT_PRIVATE __attribute__((visibility("default")))
#else
#define WEBKIT_STORAGE_EXPORT
#define WEBKIT_STORAGE_EXPORT_PRIVATE
#endif
#endif

#else // defined(COMPONENT_BUILD)
#define WEBKIT_STORAGE_EXPORT
#define WEBKIT_STORAGE_EXPORT_PRIVATE
#endif

#endif  // WEBKIT_STORAGE_STORAGE_EXPORT_H_
