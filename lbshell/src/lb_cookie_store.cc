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

#include "lb_cookie_store.h"

#include "base/bind.h"
#include "base/message_loop.h"
#include "base/time.h"
#include "googleurl/src/gurl.h"
#include "lb_savegame_syncer.h"
#include "net/cookies/canonical_cookie.h"
#include "sql/statement.h"

static const int kOriginalCookieSchemaVersion = 1;
static const int kLatestCookieSchemaVersion = 1;
static const base::TimeDelta kMaxCookieLifetime =
    base::TimeDelta::FromDays(365 * 2);

base::Lock LBCookieStore::init_lock_;

// static
bool LBCookieStore::initialized_ = false;

// static
void LBCookieStore::Init() {
  base::AutoLock init_lock(init_lock_);

  if (initialized_)
    return;

  Reinitialize();
  initialized_ = true;
}

// static
void LBCookieStore::Reinitialize() {
  LBSavegameSyncer::WaitForLoad();

  sql::Connection *conn = LBSavegameSyncer::connection();

  // Check the table's schema version.
  int version = LBSavegameSyncer::GetSchemaVersion("CookieTable");
  if (version == LBSavegameSyncer::kSchemaTableIsNew) {
    // This savegame predates the existence of the schema table.
    // Since the cookie table did not change between the initial release of
    // the app and the introduction of the schema table, assume that this
    // existing cookie table is schema version 1.  This avoids a loss of
    // cookies on upgrade.
    LBSavegameSyncer::UpdateSchemaVersion("CookieTable",
                                          kOriginalCookieSchemaVersion);
    version = kOriginalCookieSchemaVersion;
  } else if (version == LBSavegameSyncer::kSchemaVersionLost) {
    // Since there has only been one schema so far, treat this the same as
    // kSchemaTableIsNew.  When there are multiple schemas in the wild,
    // we may want to drop the table instead.
    LBSavegameSyncer::UpdateSchemaVersion("CookieTable",
                                          kOriginalCookieSchemaVersion);
    version = kOriginalCookieSchemaVersion;
  }

  if (version == LBSavegameSyncer::kNoSuchTable) {
    // The table does not exist, so create it in its latest form.
    sql::Statement create_table(conn->GetUniqueStatement(
        "CREATE TABLE CookieTable ("
        "url TEXT, "
        "name TEXT, "
        "value TEXT, "
        "domain TEXT, "
        "path TEXT, "
        "mac_key TEXT, "
        "mac_algorithm TEXT, "
        "creation INTEGER, "
        "expiration INTEGER, "
        "last_access INTEGER, "
        "secure INTEGER, "
        "http_only INTEGER, "
        "UNIQUE(name, domain, path) ON CONFLICT REPLACE)"));
    bool ok = create_table.Run();
    DCHECK(ok);
    LBSavegameSyncer::ReportCreatedTable();
    LBSavegameSyncer::UpdateSchemaVersion("CookieTable",
                                          kLatestCookieSchemaVersion);
  }
  // On schema change, update kLatestCookieSchemaVersion and write upgrade
  // logic for old tables.  For example:
  //
  // else if (version == 1) {
  //   sql::Statement update_table(conn->GetUniqueStatement(
  //       "ALTER TABLE CookieTable ADD COLUMN xyz INTEGER default 10"));
  //   bool ok = update_table.Run();
  //   DCHECK(ok);
  //   LBSavegameSyncer::UpdateSchemaVersion("CookieTable",
  //                                         kLatestCookieSchemaVersion);
  // }
}

LBCookieStore::LBCookieStore() {
}

LBCookieStore::~LBCookieStore() {
}

// static
std::vector<net::CanonicalCookie *> LBCookieStore::GetAllCookies() {
  DCHECK(initialized_);

  base::Time maximum_expiry = base::Time::Now() + kMaxCookieLifetime;

  std::vector<net::CanonicalCookie*> actual_cookies;
  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement get_all(conn->GetCachedStatement(SQL_FROM_HERE,
      "SELECT url, name, value, domain, path, mac_key, mac_algorithm, "
      "creation, expiration, last_access, secure, http_only "
      "FROM CookieTable"));
  while (get_all.Step()) {
    base::Time expiry = base::Time::FromInternalValue(get_all.ColumnInt64(8));
    if (expiry > maximum_expiry)
      expiry = maximum_expiry;

    net::CanonicalCookie *cookie =
        net::CanonicalCookie::Create(
            GURL(get_all.ColumnString(0)),
            get_all.ColumnString(1),
            get_all.ColumnString(2),
            get_all.ColumnString(3),
            get_all.ColumnString(4),
            get_all.ColumnString(5),
            get_all.ColumnString(6),
            base::Time::FromInternalValue(get_all.ColumnInt64(7)),
            expiry,
            get_all.ColumnBool(10),
            get_all.ColumnBool(11));
    cookie->SetLastAccessDate(
        base::Time::FromInternalValue(get_all.ColumnInt64(9)));
    actual_cookies.push_back(cookie);
  }
  return actual_cookies;
}

// static
void LBCookieStore::DeleteAllCookies() {
  DCHECK(initialized_);

  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement delete_all(conn->GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM CookieTable"));
  bool ok = delete_all.Run();
  DCHECK(ok);
}

// static
void LBCookieStore::QuickAddCookie(const net::CanonicalCookie &cc) {
  DCHECK(initialized_);

  // We expect that all cookies we are fed are meant to persist.
  DCHECK(cc.IsPersistent());

  base::Time maximum_expiry = base::Time::Now() + kMaxCookieLifetime;
  base::Time expiry = cc.ExpiryDate();
  if (expiry > maximum_expiry)
    expiry = maximum_expiry;

  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement insert_cookie(conn->GetCachedStatement(SQL_FROM_HERE,
      "INSERT INTO CookieTable ("
      "url, name, value, domain, path, mac_key, mac_algorithm, "
      "creation, expiration, last_access, secure, http_only"
      ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

  insert_cookie.BindString(0, cc.Source());
  insert_cookie.BindString(1, cc.Name());
  insert_cookie.BindString(2, cc.Value());
  insert_cookie.BindString(3, cc.Domain());
  insert_cookie.BindString(4, cc.Path());
  insert_cookie.BindString(5, cc.MACKey());
  insert_cookie.BindString(6, cc.MACAlgorithm());
  insert_cookie.BindInt64(7, cc.CreationDate().ToInternalValue());
  insert_cookie.BindInt64(8, expiry.ToInternalValue());
  insert_cookie.BindInt64(9, cc.LastAccessDate().ToInternalValue());
  insert_cookie.BindBool(10, cc.IsSecure());
  insert_cookie.BindBool(11, cc.IsHttpOnly());
  bool ok = insert_cookie.Run();
  DCHECK(ok);
}

// static
void LBCookieStore::QuickDeleteCookie(const net::CanonicalCookie &cc) {
  DCHECK(initialized_);

  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement delete_cookie(conn->GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM CookieTable WHERE name = ? AND domain = ? AND path = ?"));
  delete_cookie.BindString(0, cc.Name());
  delete_cookie.BindString(1, cc.Domain());
  delete_cookie.BindString(2, cc.Path());
  bool ok = delete_cookie.Run();
  DCHECK(ok);
}

void LBCookieStore::Load(const LoadedCallback& loaded_callback) {
  Init();

  MessageLoop::current()->PostNonNestableTask(FROM_HERE,
      base::Bind(loaded_callback, GetAllCookies()));
}

void LBCookieStore::LoadCookiesForKey(const std::string&,
                                      const LoadedCallback& loaded_callback) {
  DCHECK(initialized_);

  // This is always called after Load(), so there's nothing to do.
  // See comments in the header for more information.
  std::vector<net::CanonicalCookie*> empty_cookie_list;
  MessageLoop::current()->PostNonNestableTask(FROM_HERE,
      base::Bind(loaded_callback, empty_cookie_list));
}

void LBCookieStore::AddCookie(const net::CanonicalCookie &cc) {
  QuickAddCookie(cc);
}

void LBCookieStore::UpdateCookieAccessTime(const net::CanonicalCookie &cc) {
  DCHECK(initialized_);

  sql::Connection *conn = LBSavegameSyncer::connection();
  sql::Statement touch_cookie(conn->GetCachedStatement(SQL_FROM_HERE,
      "UPDATE CookieTable SET last_access = ? WHERE "
      "name = ? AND domain = ? AND path = ?"));

  base::Time maximum_expiry = base::Time::Now() + kMaxCookieLifetime;
  base::Time expiry = cc.ExpiryDate();
  if (expiry > maximum_expiry)
    expiry = maximum_expiry;

  touch_cookie.BindInt64(0, expiry.ToInternalValue());
  touch_cookie.BindString(1, cc.Name());
  touch_cookie.BindString(2, cc.Domain());
  touch_cookie.BindString(3, cc.Path());
  bool ok = touch_cookie.Run();
  DCHECK(ok);
}

void LBCookieStore::DeleteCookie(const net::CanonicalCookie &cc) {
  QuickDeleteCookie(cc);
}

void LBCookieStore::Flush(const base::Closure& callback) {
  MessageLoop::current()->PostNonNestableTask(FROM_HERE, callback);
}

