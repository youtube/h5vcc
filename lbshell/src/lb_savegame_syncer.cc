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

#include "lb_savegame_syncer.h"

#include "external/chromium/base/logging.h"

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/stringprintf.h"
#include "sql/statement.h"

// Database version "2" indicates that this was created by v1.x.
// Database version "0" indicates that this was created by v2.x beta,
//   patches 0-3.  Version "0" is a default from sqlite3 because these
//   versions of the application did not set this value at all.
// Database version "3" indicates that the schema versions of individual
//   tables should be tracked in SchemaTable.
static const int kDatabaseUserVersion = 3;

// static
int LBSavegameSyncer::loaded_database_version_ = 0;

// static
int LBSavegameSyncer::num_created_tables_ = 0;

// static
sql::Connection *LBSavegameSyncer::connection_ = NULL;

// static
bool LBSavegameSyncer::shutdown_ = false;

#if !defined(__LB_SHELL__FOR_RELEASE__)
// static
bool LBSavegameSyncer::save_disabled_ = false;

// static
FilePath LBSavegameSyncer::load_from_file_;
#endif

// static
base::WaitableEvent LBSavegameSyncer::loaded_(true, false);

// static
base::WaitableEvent LBSavegameSyncer::flushed_(true, false);

#if !defined(__LB_SHELL__FOR_RELEASE__) && !defined(__LB_SHELL__FOR_QA__)
class AutoTimer {
 public:
  explicit AutoTimer(const char *op)
      : start_(base::Time::Now())
      , op_(op) {
  }

  ~AutoTimer() {
    base::Time end = base::Time::Now();
    base::TimeDelta delta = end - start_;
    int delta_ms = delta.InMillisecondsRoundedUp();

    const char *name = "(no MessageLoop)";
    MessageLoop *message_loop = MessageLoop::current();
    if (message_loop) name = message_loop->thread_name().c_str();

    DLOG(INFO) << base::StringPrintf("%s took %d ms in thread \"%s\".",
        op_, delta_ms, name);
  }

 private:
  base::Time start_;
  const char *op_;
};

# define DEBUG_TIMING(op) AutoTimer timer(op)
#else
# define DEBUG_TIMING(op) {}
#endif

// static
void LBSavegameSyncer::Init() {
  DEBUG_TIMING("LBSavegameSyncer::Init");

  // To allow re-init after shutdown, such as we might do in testing:
  shutdown_ = false;
  flushed_.Reset();

  PlatformInit();

  CreateInMemoryDatabase();

  SavegameToFile(FinishInit);
}

// static
void LBSavegameSyncer::FinishInit() {
  if (shutdown_) {
    // Don't bother loading anything.  Abort & signal done.
  } else {
    FileToMemory();
  }

  // Get the DB version loaded from disk.
  sql::Statement get_db_version(connection_->GetUniqueStatement(
      "PRAGMA user_version"));
  bool ok = get_db_version.Step();
  DCHECK(ok);
  loaded_database_version_ = get_db_version.ColumnInt(0);

  // Create the schema table.
  sql::Statement create_table(connection_->GetUniqueStatement(
      "CREATE TABLE IF NOT EXISTS SchemaTable ("
      "name TEXT, "
      "version INTEGER, "
      "UNIQUE(name, version) ON CONFLICT REPLACE)"));
  ok = create_table.Run();
  DCHECK(ok);

  // Update the DB version which will be read in next time.
  // NOTE: Pragma statements cannot be bound, so we must construct the string
  // in full.
  std::string set_db_version_str = base::StringPrintf(
      "PRAGMA user_version = %d", kDatabaseUserVersion);
  sql::Statement set_db_version(connection_->GetUniqueStatement(
      set_db_version_str.c_str()));
  ok = get_db_version.Run();
  DCHECK(ok);

#if defined(__LB_SHELL__FORCE_LOGGING__)
  DLOG(INFO) << "Loaded these tables from disk:";
  PrintTables();
#endif

  loaded_.Signal();
}

// static
void LBSavegameSyncer::Shutdown() {
  DEBUG_TIMING("LBSavegameSyncer::Shutdown");

  if (shutdown_) return;

  shutdown_ = true;

  if (!loaded_.IsSignaled()) {
    // Nothing to sync to the savegame.
    // Wait for the load operation to complete/abort.
    loaded_.Wait();
    // Invoke the rest of the shutdown process right away.
    FinishShutdown();
  } else {
    MemoryToFile();

#if defined(__LB_SHELL__FOR_RELEASE__)
    const bool write_savegame = true;
#else
    bool write_savegame = !save_disabled_;
#endif
    if (write_savegame) {
     FileToSavegame(FinishShutdown);
    }
  }
}

// static
void LBSavegameSyncer::FinishShutdown() {
#if defined(__LB_SHELL__FORCE_LOGGING__)
  DLOG(INFO) << "Pushed these tables to disk:";
  PrintTables();
#endif

  DestroyInMemoryDatabase();

  // Reset loaded_ for a possible future Init.
  loaded_.Reset();

  // Signal that we are done flushing.
  flushed_.Signal();
}

// static
void LBSavegameSyncer::WaitForLoad() {
  DEBUG_TIMING("LBSavegameSyncer::WaitForLoad");

  loaded_.Wait();
}

// static
bool LBSavegameSyncer::TimedWaitForLoad(base::TimeDelta max_wait) {
  return loaded_.TimedWait(max_wait);
}

// static
void LBSavegameSyncer::WaitForFlush() {
  DEBUG_TIMING("LBSavegameSyncer::WaitForFlush");

  flushed_.Wait();

  PlatformShutdown();
}

// static
int LBSavegameSyncer::GetSchemaVersion(const char *table_name) {
  sql::Statement get_exists(connection_->GetUniqueStatement(
      "SELECT name FROM sqlite_master WHERE name = ? AND type = 'table'"));
  get_exists.BindString(0, table_name);
  bool exists = get_exists.Step();
  if (!exists)
    return kNoSuchTable;

  sql::Statement get_version(connection_->GetUniqueStatement(
      "SELECT version FROM SchemaTable WHERE name = ?"));
  get_version.BindString(0, table_name);
  bool row_found = get_version.Step();
  if (row_found)
    return get_version.ColumnInt(0);

  if (loaded_database_version_ != kDatabaseUserVersion) {
    // The schema table did not exist before this session, which is different
    // from the schema table being lost.
    return kSchemaTableIsNew;
  }
  return kSchemaVersionLost;
}

// static
void LBSavegameSyncer::UpdateSchemaVersion(const char *table_name,
                                           int version) {
  DCHECK_GT(version, 0) << "Schema version numbers must be positive.";

  sql::Statement update_version(connection_->GetUniqueStatement(
      "INSERT INTO SchemaTable (name, version)"
      "VALUES (?, ?)"));
  update_version.BindString(0, table_name);
  update_version.BindInt(1, version);
  bool ok = update_version.Run();
  DCHECK(ok);
}

// static
sql::Connection* LBSavegameSyncer::connection() {
  // We should be fully loaded before anyone accesses this connection.
  assert(loaded_.IsSignaled());
  return connection_;
}

// static
void LBSavegameSyncer::ForceSync() {
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (save_disabled_) {
    return;
  }
#endif

#if defined(__LB_WIIU__)
  base::AutoLock lock(background_lock_);
#endif

  MemoryToFile();
  FileToSavegame(FinishForceSync);
  flushed_.Wait();  // signalled by FinishForceSync
  flushed_.Reset();  // so that future WaitForFlush operations work as intended
}

// static
void LBSavegameSyncer::FinishForceSync() {
#if defined(__LB_SHELL__FORCE_LOGGING__)
  DLOG(INFO) << "Pushed these tables to disk:";
  PrintTables();
#endif

  flushed_.Signal();
}

// static
void LBSavegameSyncer::CreateInMemoryDatabase() {
  connection_ = new sql::Connection();
  bool ok = connection_->OpenInMemory();
  assert(ok);
}

// static
void LBSavegameSyncer::DestroyInMemoryDatabase() {
  delete connection_;
}

// static
void LBSavegameSyncer::FileToMemory() {
  FilePath path = GetFilePath();
#if !defined(__LB_SHELL__FOR_RELEASE__)
  if (!load_from_file_.empty()) {
    // override the filename we load from.
    path = load_from_file_;
  }
#endif
  if (file_util::PathExists(path)) {
    sql::Connection file_db;
    bool ok = file_db.Open(path);
    assert(ok);
    if (ok) {
      connection_->CloneFrom(&file_db);
    }
  }
}

// static
void LBSavegameSyncer::MemoryToFile() {
  sql::Connection file_db;
  file_util::Delete(GetFilePath(), false);
  bool ok = file_db.Open(GetFilePath());
  // If it fails, we will continue the app since this is not fatal error.
  DLOG(INFO) << "Memory to file failed.";
  if (ok) {
    file_db.CloneFrom(connection_);
  }
}

#if defined(__LB_SHELL__FORCE_LOGGING__)
// static
void LBSavegameSyncer::PrintTables() {
  sql::Statement get_tables(connection_->GetCachedStatement(SQL_FROM_HERE,
      "SELECT name FROM sqlite_master WHERE type='table'"));

  while (get_tables.Step()) {
    std::string tablename = get_tables.ColumnString(0);
    std::string query = base::StringPrintf(
        "SELECT * FROM %s", tablename.c_str());  // can't bind a table name
    // GetUniqueStatement is better than GetCachedStatement here, because
    // the statement is cached based on its ID (SQL_FROM_HERE) rather than
    // its content.  Since this statement changes per-table, don't cache it.
    sql::Statement get_data(connection_->GetUniqueStatement(query.c_str()));

    DLOG(INFO) << base::StringPrintf("Dumping table %s:", tablename.c_str());
    std::string buf;
    // print column names
    for (int i = 0; i < get_data.ColumnCount(); ++i) {
      buf.append(base::StringPrintf("%s, ", get_data.ColumnName(i).c_str()));
    }
    DLOG(INFO) << buf;
    DLOG(INFO) << "---";
    buf.clear();

    while (get_data.Step()) {
      for (int i = 0; i < get_data.ColumnCount(); ++i) {
        std::string output = get_data.ColumnString(i);
        buf.append(output).append(", ");
      }
      DLOG(INFO) << buf;
      buf.clear();
    }

    DLOG(INFO) << "--- Table dump done";
  }
}
#endif
