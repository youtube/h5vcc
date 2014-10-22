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

#include "lb_local_storage_database_adapter.h"

#include "base/logging.h"
#include "base/stringprintf.h"
#include "lb_console_connection.h"
#include "lb_savegame_syncer.h"
#include "sql/statement.h"
#include "webkit/tools/test_shell/simple_dom_storage_system.h"

static const int kOriginalLocalStorageSchemaVersion = 1;
static const int kLatestLocalStorageSchemaVersion = 1;

// static
base::Lock LBLocalStorageDatabaseAdapter::init_lock_;

// static
bool LBLocalStorageDatabaseAdapter::initialized_ = false;

// static
void LBLocalStorageDatabaseAdapter::Init() {
  base::AutoLock init_lock(init_lock_);

  if (initialized_)
    return;

  Reinitialize();
  initialized_ = true;
}

// static
void LBLocalStorageDatabaseAdapter::Reinitialize() {
  LBSavegameSyncer::WaitForLoad();

  sql::Connection *conn = LBSavegameSyncer::connection();

  // Check the table's schema version.
  int version = LBSavegameSyncer::GetSchemaVersion("LocalStorageTable");
  if (version == LBSavegameSyncer::kSchemaTableIsNew) {
    // This savegame predates the existence of the schema table.
    // Since the local-storage table did not change between the initial
    // release of the app and the introduction of the schema table, assume
    // that this existing local-storage table is schema version 1.  This
    // avoids a loss of data on upgrade.
    LBSavegameSyncer::UpdateSchemaVersion("LocalStorageTable",
                                          kOriginalLocalStorageSchemaVersion);
    version = kOriginalLocalStorageSchemaVersion;
  } else if (version == LBSavegameSyncer::kSchemaVersionLost) {
    // Since there has only been one schema so far, treat this the same as
    // kSchemaTableIsNew.  When there are multiple schemas in the wild,
    // we may want to drop the table instead.
    LBSavegameSyncer::UpdateSchemaVersion("LocalStorageTable",
                                          kOriginalLocalStorageSchemaVersion);
    version = kOriginalLocalStorageSchemaVersion;
  }

  if (version == LBSavegameSyncer::kNoSuchTable) {
    // The table does not exist, so create it in its latest form.
    sql::Statement create_table(conn->GetUniqueStatement(
        "CREATE TABLE LocalStorageTable ("
        "  site_identifier TEXT, "
        "  key TEXT, "
        "  value TEXT NOT NULL ON CONFLICT FAIL, "
        "  UNIQUE(site_identifier, key) ON CONFLICT REPLACE"
        ")"));
    bool ok = create_table.Run();
    DCHECK(ok);
    LBSavegameSyncer::ReportCreatedTable();
    LBSavegameSyncer::UpdateSchemaVersion("LocalStorageTable",
                                          kLatestLocalStorageSchemaVersion);
  }
  // On schema change, update kLatestLocalStorageSchemaVersion and write
  // upgrade logic for old tables.  For example:
  //
  // else if (version == 1) {
  //  sql::Statement update_table(conn->GetUniqueStatement(
  //      "ALTER TABLE LocalStorageTable ADD COLUMN xyz INTEGER default 10"));
  //  bool ok = update_table.Run();
  //  DCHECK(ok);
  //  LBSavegameSyncer::UpdateSchemaVersion("LocalStorageTable",
  //                                        kLatestLocalStorageSchemaVersion);
  // }
}

// static
void LBLocalStorageDatabaseAdapter::Flush() {
  SimpleDomStorageSystem::instance().Flush();
}


LBLocalStorageDatabaseAdapter::LBLocalStorageDatabaseAdapter(
    const std::string& id) : id_(id) {
  DLOG(INFO) << base::StringPrintf("Creating LocalStorage adapter for %s",
                                   id_.c_str());
}

LBLocalStorageDatabaseAdapter::~LBLocalStorageDatabaseAdapter() {
}

void LBLocalStorageDatabaseAdapter::ReadAllValues(
    dom_storage::ValuesMap* result) {
  Init();

  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement get_values(conn->GetCachedStatement(SQL_FROM_HERE,
      "SELECT key, value FROM LocalStorageTable WHERE site_identifier = ?"));
  get_values.BindString(0, id_);
  while (get_values.Step()) {
    string16 key = get_values.ColumnString16(0);
    NullableString16 value(get_values.ColumnString16(1), false);
    result->insert(std::make_pair(key, value));
  }
}

bool LBLocalStorageDatabaseAdapter::CommitChanges(
    bool clear_all_first,
    const dom_storage::ValuesMap& changes) {
  DCHECK(initialized_);

  sql::Connection *conn = LBSavegameSyncer::connection();

  if (clear_all_first) {
    sql::Statement clear_values(conn->GetCachedStatement(SQL_FROM_HERE,
        "DELETE FROM LocalStorageTable WHERE site_identifier = ?"));
    clear_values.BindString(0, id_);
    if (!clear_values.Run())
      return false;
  }

  sql::Statement delete_key(conn->GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM LocalStorageTable "
      "WHERE site_identifier = ? AND key = ?"));
  sql::Statement insert_key(conn->GetCachedStatement(SQL_FROM_HERE,
      "INSERT INTO LocalStorageTable (site_identifier, key, value) "
      "VALUES (?, ?, ?)"));

  dom_storage::ValuesMap::const_iterator i;
  for (i = changes.begin(); i != changes.end(); ++i) {
    string16 key = i->first;
    NullableString16 value = i->second;
    if (value.is_null()) {
      delete_key.Reset(true);
      delete_key.BindString(0, id_);
      delete_key.BindString16(1, key);
      if (!delete_key.Run())
        return false;
    } else {
      insert_key.Reset(true);
      insert_key.BindString(0, id_);
      insert_key.BindString16(1, key);
      insert_key.BindString16(2, value.string());
      if (!insert_key.Run())
        return false;
    }
  }

  return true;
}

void LBLocalStorageDatabaseAdapter::Reset() {
  DCHECK(initialized_);

  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement clear_site(conn->GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM LocalStorageTable WHERE site_identifier = ?"));
  clear_site.BindString(0, id_);
  clear_site.Run();
}

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
// static
void LBLocalStorageDatabaseAdapter::ClearAll() {
  DCHECK(initialized_);

  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement clear_all(conn->GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM LocalStorageTable"));
  clear_all.Run();
}

// static
void LBLocalStorageDatabaseAdapter::Dump(LBConsoleConnection *connection) {
  DCHECK(initialized_);

  connection->Output(StringPrintf("%20s %20s %20s\n",
                                  "database", "key", "value"));
  connection->Output(StringPrintf("%20s %20s %20s\n",
                                  "========", "===", "====="));

  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement get_all(conn->GetCachedStatement(SQL_FROM_HERE,
      "SELECT site_identifier, key, value FROM LocalStorageTable"));
  while (get_all.Step()) {
    std::string id = get_all.ColumnString(0);
    std::string key = get_all.ColumnString(1);
    std::string value = get_all.ColumnString(2);
    connection->Output(StringPrintf("%20s %20s  %s\n",
                                    id.c_str(), key.c_str(), value.c_str()));
  }
}
#endif

// Tell the DomStorageDatabaseAdapter to create LBLocalStorageDatabaseAdapters.
void LBLocalStorageDatabaseAdapter::Register() {
  dom_storage::DomStorageDatabaseAdapter::SetClassFactory(
        LBLocalStorageDatabaseAdapter::Create);
}

dom_storage::DomStorageDatabaseAdapter*
    LBLocalStorageDatabaseAdapter::Create(const std::string& id) {
        return new LBLocalStorageDatabaseAdapter(id);
}
