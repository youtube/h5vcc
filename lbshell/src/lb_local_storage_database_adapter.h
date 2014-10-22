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
// Implements dom_storage::DomStorageDatabaseAdapter.  This is a sibling of
// and a replacement for dom_storage::LocalStorageDatabaseAdapter.

#ifndef SRC_LB_LOCAL_STORAGE_DATABASE_ADAPTER_H_
#define SRC_LB_LOCAL_STORAGE_DATABASE_ADAPTER_H_

#include "external/chromium/base/compiler_specific.h"  // OVERRIDE
#include "external/chromium/base/synchronization/lock.h"
#include "external/chromium/webkit/dom_storage/dom_storage_database_adapter.h"

class LBConsoleConnection;

class LBLocalStorageDatabaseAdapter
    : public dom_storage::DomStorageDatabaseAdapter {
 public:
  // We will be fed an ID that is backward-compatible with that of our
  // original localstorage implementation.
  explicit LBLocalStorageDatabaseAdapter(const std::string& id);
  virtual ~LBLocalStorageDatabaseAdapter();
  virtual void ReadAllValues(dom_storage::ValuesMap* result) OVERRIDE;
  virtual bool CommitChanges(bool clear_all_first,
                             const dom_storage::ValuesMap& changes) OVERRIDE;
  virtual void Reset() OVERRIDE;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  // For the debug console's use:
  static void ClearAll();
  static void Dump(LBConsoleConnection *connection);
#endif

  static void Flush();

  static void Register();

  static dom_storage::DomStorageDatabaseAdapter* Create(const std::string& id);

  // Re-initialize the tables in the savegame database needed for localStorage
  static void Reinitialize();

 private:
  static void Init();

  static base::Lock init_lock_;
  static bool initialized_;
  std::string id_;
};

#endif  // SRC_LB_LOCAL_STORAGE_DATABASE_ADAPTER_H_
