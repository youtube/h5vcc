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
// Provides a simple API to access and sync the savegame database.

#ifndef SRC_LB_SAVEGAME_SYNCER_H_
#define SRC_LB_SAVEGAME_SYNCER_H_

#include <string>

#include "base/file_path.h"
#include "base/synchronization/waitable_event.h"
#include "base/time.h"
#include "sql/connection.h"

class LBSavegameSyncer {
 public:
  // Initializes the syncer.  Loads data asynchronously from the savegame
  // into memory.  Is done loading when WaitForLoad() returns.
  static void Init();

  // Shuts down the syncer.  Writes data asynchronously from memory to the
  // savegame.  Is done shutting down when WaitForFlush() returns.  Should
  // only be called from the same thread as Init().
  static void Shutdown();

  // Blocks until savegame data has been brought into memory.  Once this
  // returns, it is safe to access connection().
  static void WaitForLoad();

  // Blocks until savegame data has been brought into memory or max_wait has
  // elapsed.  Once this returns true, it is safe to access connection().
  static bool TimedWaitForLoad(base::TimeDelta max_wait);

  // Blocks until any in-memory data has been fully flushed to the savegame.
  // Once this returns, it is safe to quit the application or to call Init()
  // again.  Should only be called from the same thread as Init().
  static void WaitForFlush();

  // Get the schema version for the given table.
  //
  // Returns kNoSuchTable if the table does not exist.
  //
  // Returns kSchemaTableIsNew if the table exists, but the schema table was
  // newly created in this session.
  //
  // Returns kSchemaVersionLost if the table exists, and the schema table
  // existed once before, but has since been lost.  In this case, the schema
  // version cannot be known or directly inferred.
  static int GetSchemaVersion(const char *table_name);
  static const int kNoSuchTable = -1;
  static const int kSchemaTableIsNew = -2;
  static const int kSchemaVersionLost = -3;

  // Updates the schema version for the given table.
  // The version number must be greater than 0 (enforced by DCHECK).
  static void UpdateSchemaVersion(const char *table_name, int version);

  // The savegame syncer keeps track of the number of created tables in a
  // session.  This is used to track DB corruption.
  static void ReportCreatedTable() { ++num_created_tables_; }
  static int GetCreatedTableCount() { return num_created_tables_; }

  // Returns an sqlite3 connection wrapper.
  // Use sql::Statement to execute queries, as documented in sql/connection.h.
  static sql::Connection* connection();

  // Force the in-memory database to flush to the savegame.  Block until
  // finished.  Resets the flushed_ flag before returning.
  static void ForceSync();

#if !defined (__LB_SHELL__FOR_RELEASE__)
  // Prevent the savegame file from being written.
  // If there is a crash during savegame write, the savegame will be
  // lost. If we know we won't need to save anything, we can set this.
  static void DisableSave() { save_disabled_ = true; }

  // Override the file from which the savegame is read.
  static void LoadFromFile(FilePath file) { load_from_file_ = file; }
#endif

  typedef void(*InternalCallback)();

#if defined(__LB_WIIU__)
  // A lock to prevent file IO when the app is in the background
  static base::Lock background_lock_;
#endif

 private:
  LBSavegameSyncer() {}

  // Platform-specific.  Initializes any platform-specific savegame mechanisms.
  // Should be fast.
  static void PlatformInit();

  // Platform-specific.  Shuts down any platform-specific savegame mechanisms.
  // Should tolerate being called multiple times.  Should be fast.
  static void PlatformShutdown();

  // Platform-specific.  Returns the file path for the on-disk database.
  // This may or may not be the final destination for this data.
  static FilePath GetFilePath();

  // Creates an empty in-memory database.
  static void CreateInMemoryDatabase();

  // Destroys the in-memory database.
  static void DestroyInMemoryDatabase();

  // Platform-specific.  Loads the savegame and writes the data to a file
  // at GetFilePath().  Asynchronous.
  static void SavegameToFile(InternalCallback cb);

  // Platform-specific.  Reads the file at GetFilePath() and writes the data
  // to the savegame.  Asynchronous.
  static void FileToSavegame(InternalCallback cb);

  // Clone the on-disk database at GetFilePath() into the in-memory database.
  static void FileToMemory();

  // Clone the in-memory database into the on-disk database at GetFilePath().
  static void MemoryToFile();


  // Callback to complete the work of Init() after the async stuff is done.
  static void FinishInit();

  // Callback to complete the work of Shutdown() after the async stuff is done.
  static void FinishShutdown();

  // Callback to unblock ForceSync().
  static void FinishForceSync();

#if defined(__LB_SHELL__FORCE_LOGGING__)
  // For debugging.  Prints the contents of all in-memory tables.
  static void PrintTables();
#endif

  // The version number from the database loaded from disk.
  static int loaded_database_version_;

  // The number of tables created this session.
  static int num_created_tables_;

  // The in-memory database connection.  Explicitly thread-safe.
  static sql::Connection *connection_;

  // True as soon as Shutdown() is called.  Used to abort a load.
  static bool shutdown_;

  // A signal that the savegame has been loaded into memory.
  static base::WaitableEvent loaded_;

  // A signal that the data in memory has been flushed to the savegame.
  static base::WaitableEvent flushed_;

#if !defined(__LB_SHELL__FOR_RELEASE__)
  // Prevent the savegame from being written.
  static bool save_disabled_;

  // Override the file from which the savegame is read.
  static FilePath load_from_file_;
#endif
};

#endif  // SRC_LB_SAVEGAME_SYNCER_H_
