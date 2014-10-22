// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_DOM_STORAGE_DOM_STORAGE_DATABASE_ADAPTER_H_
#define WEBKIT_DOM_STORAGE_DOM_STORAGE_DATABASE_ADAPTER_H_

// Database interface used by DomStorageArea. Abstracts the differences between
// the per-origin DomStorageDatabases for localStorage and
// SessionStorageDatabase which stores multiple origins.

#include "base/logging.h"
#include "webkit/dom_storage/dom_storage_types.h"
#include "webkit/storage/webkit_storage_export.h"

namespace dom_storage {

#if defined (__LB_SHELL__)
class DomStorageDatabaseAdapter;
typedef DomStorageDatabaseAdapter*
    (*DomStorageDatabaseAdapterFactory)(const std::string& id);
#endif

class WEBKIT_STORAGE_EXPORT DomStorageDatabaseAdapter {
 public:
  virtual ~DomStorageDatabaseAdapter() {}
  virtual void ReadAllValues(ValuesMap* result) = 0;
  virtual bool CommitChanges(
      bool clear_all_first, const ValuesMap& changes) = 0;
  virtual void DeleteFiles() {}
  virtual void Reset() {}

#if defined (__LB_SHELL__)
  static void SetClassFactory(DomStorageDatabaseAdapterFactory f) {
    class_factory_ = f;
  }
  static DomStorageDatabaseAdapterFactory ClassFactory() {
    DCHECK(class_factory_);
    return class_factory_;
  }
 private:
  static DomStorageDatabaseAdapterFactory class_factory_;
#endif
};

}  // namespace dom_storage

#endif  // WEBKIT_DOM_STORAGE_DOM_STORAGE_DATABASE_ADAPTER_H_
