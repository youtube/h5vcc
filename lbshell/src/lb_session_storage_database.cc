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

// This file is based off of chromium/webkit/dom_storage/dom_storage_database.cc
// and provides symbols in that associated .h file.

#include "external/chromium/base/logging.h"
#include "external/chromium/webkit/dom_storage/session_storage_database.h"

namespace dom_storage {

SessionStorageDatabase::SessionStorageDatabase(const FilePath& file_path) {
  NOTIMPLEMENTED();
}

SessionStorageDatabase::~SessionStorageDatabase() {}

void SessionStorageDatabase::ReadAreaValues(const std::string& namespace_id,
                                            const GURL& origin,
                                            ValuesMap* result) {
  NOTIMPLEMENTED();
}

bool SessionStorageDatabase::CommitAreaChanges(
    const std::string& namespace_id, const GURL& origin,
    bool clear_all_first, const ValuesMap& changes) {
  NOTIMPLEMENTED();
  return false;
}

bool SessionStorageDatabase::CloneNamespace(
    const std::string& namespace_id, const std::string& new_namespace_id) {
  NOTIMPLEMENTED();
  return false;
}

bool SessionStorageDatabase::DeleteArea(const std::string& namespace_id,
                                        const GURL& origin) {
  NOTIMPLEMENTED();
  return false;
}

bool SessionStorageDatabase::DeleteNamespace(const std::string& namespace_id) {
  NOTIMPLEMENTED();
  return false;
}

bool SessionStorageDatabase::ReadNamespacesAndOrigins(
    std::map<std::string, std::vector<GURL> >* namespaces_and_origins) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace dom_storage
