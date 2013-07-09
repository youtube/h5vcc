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

#include "lb_app_counters.h"

#include "base/bind.h"
#include "base/stringprintf.h"
#include "lb_memory_manager.h"
#include "lb_savegame_syncer.h"
#include "lb_shell/lb_shell_constants.h"
#include "sql/statement.h"

static const int kOriginalRunCountsSchemaVersion = 1;
static const int kLatestRunCountsSchemaVersion = 1;
static const int kRunCountsRowID = 1;

// static
LBAppCounters *LBAppCounters::instance_ = NULL;

LBAppCounters::LBAppCounters()
    : loaded_(true, false)  // initially false, signaled in InitTask, not reset
    , shutdown_(false)
    , startups_(0)
    , clean_exits_(0) {
  DCHECK(!instance_);
  if (instance_) return;

  instance_ = this;
  thread_.reset(LB_NEW base::Thread("AppCounterThread"));
  thread_->StartWithOptions(base::Thread::Options(
      MessageLoop::TYPE_DEFAULT, kAppCountersStackSize));
  // InitTask is the only purpose of this thread.
  thread_->message_loop()->PostTask(FROM_HERE,
                                    base::Bind(&LBAppCounters::InitTask,
                                               base::Unretained(this)));
}

LBAppCounters::~LBAppCounters() {
  // There should only be one instance, and it should be this.
  // But I will check just in case.
  if (instance_ == this) {
    // Check to see if we have loaded the data already.  We do this before
    // starting the shutdown because loaded_ is _always_ signaled by the time
    // the thread dies, as a means of avoiding deadlocks even in exceptional
    // circumstances.  However, we still want to know if the data was truly
    // loaded, because if not, we can avoid updating the counts.
    bool loaded_already = loaded_.IsSignaled();
    // Let the waiting process know it can give up if its still waiting.
    shutdown_ = true;
    thread_->Stop();

    if (loaded_already) {
      // Only update the counts if we actually loaded the data before shutdown.
      ++clean_exits_;
      StoreCounts();
      // Don't ForceSync!  That will happen on its own if/when the savegame
      // syncer shuts down cleanly.
    }

    instance_ = NULL;
  }
}

// static
int LBAppCounters::Startups() {
  DCHECK(instance_);
  if (instance_) {
    instance_->loaded_.Wait();
    DCHECK(!instance_->shutdown_) << "We shouldn't be able to shut down "
                                     "while waiting for counts.";
    return instance_->startups_;
  }
  return 0;
}

// static
int LBAppCounters::CleanExits() {
  DCHECK(instance_);
  if (instance_) {
    instance_->loaded_.Wait();
    DCHECK(!instance_->shutdown_) << "We shouldn't be able to shut down "
                                     "while waiting for counts.";
    return instance_->clean_exits_;
  }
  return 0;
}

void LBAppCounters::InitTask() {
  bool ready = false;
  base::TimeDelta max_wait = base::TimeDelta::FromSeconds(1);
  while (!ready && !shutdown_) {
    ready = LBSavegameSyncer::TimedWaitForLoad(max_wait);
  }
  if (!ready) {
    loaded_.Signal();
    return;
  }

  sql::Connection *conn = LBSavegameSyncer::connection();

  // Check the table's schema version.
  int version = LBSavegameSyncer::GetSchemaVersion("RunCountsTable");
  if (version == LBSavegameSyncer::kSchemaTableIsNew) {
    // This savegame predates the existence of the schema table.
    // Since the run-counts table did not change between its debut and the
    // introduction of the schema table, assume that this existing run-counts
    // table is schema version 1.  This avoids a loss of counts on upgrade.
    LBSavegameSyncer::UpdateSchemaVersion("RunCountsTable",
                                          kOriginalRunCountsSchemaVersion);
    version = kOriginalRunCountsSchemaVersion;
  } else if (version == LBSavegameSyncer::kSchemaVersionLost) {
    // Since there has only been one schema so far, treat this the same as
    // kSchemaTableIsNew.  When there are multiple schemas in the wild,
    // we may want to drop the table instead.
    LBSavegameSyncer::UpdateSchemaVersion("RunCountsTable",
                                          kOriginalRunCountsSchemaVersion);
    version = kOriginalRunCountsSchemaVersion;
  }

  if (version == LBSavegameSyncer::kNoSuchTable) {
    // The table does not exist, so create it in it's latest form.
    sql::Statement create_table(conn->GetUniqueStatement(
        "CREATE TABLE RunCountsTable ("
        "id INTEGER, "
        "startup INTEGER, "
        "clean_exit INTEGER, "
        "UNIQUE(id) ON CONFLICT REPLACE)"));
    bool ok = create_table.Run();
    DCHECK(ok);
    LBSavegameSyncer::ReportCreatedTable();
    LBSavegameSyncer::UpdateSchemaVersion("RunCountsTable",
                                          kLatestRunCountsSchemaVersion);
  }
  // On schema change, update kLatestRunCountsSchemaVersion and write upgrade
  // logic for old tables.  For example:
  //
  //else if (version == 1) {
  //  sql::Statement update_table(conn->GetUniqueStatement(
  //      "ALTER TABLE RunCountsTable ADD COLUMN xyz INTEGER default 10"));
  //  bool ok = update_table.Run();
  //  DCHECK(ok);
  //  LBSavegameSyncer::UpdateSchemaVersion("RunCountsTable",
  //                                        kLatestRunCountsSchemaVersion);
  //}

  LoadCounts();
  ++startups_;
  loaded_.Signal();

  StoreCounts();
  LBSavegameSyncer::ForceSync();
}

void LBAppCounters::LoadCounts() {
  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement load_counts(conn->GetCachedStatement(SQL_FROM_HERE,
      "SELECT startup, clean_exit FROM RunCountsTable WHERE id = ?"));
  load_counts.BindInt(0, kRunCountsRowID);
  if (load_counts.Step()) {
    startups_ = load_counts.ColumnInt(0);
    clean_exits_ = load_counts.ColumnInt(1);
    DLOG(INFO) << base::StringPrintf(
      "Loaded startup count: %d, clean exit count: %d from DB.",
      startups_, clean_exits_);
  } else {
    DLOG(WARNING) << "Unable to read run-counts line from table.";
  }
}

void LBAppCounters::StoreCounts() {
  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement store_counts(conn->GetCachedStatement(SQL_FROM_HERE,
      "INSERT INTO RunCountsTable (id, startup, clean_exit) "
      "VALUES (?, ?, ?)"));
  store_counts.BindInt(0, kRunCountsRowID);
  store_counts.BindInt(1, startups_);
  store_counts.BindInt(2, clean_exits_);
  store_counts.Run();
}

