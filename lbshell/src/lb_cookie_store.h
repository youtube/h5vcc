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
// Implements a cookie store on top of LBSavegameSyncer and sqlite.
// This is similar in concept to SQLitePersistentCookieStore, but much simpler.

#ifndef SRC_LB_COOKIE_STORE_H_
#define SRC_LB_COOKIE_STORE_H_

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "net/cookies/cookie_monster.h"

#include "lb_shell_export.h"

class LB_SHELL_EXPORT LBCookieStore
    : public net::CookieMonster::PersistentCookieStore {
 public:
  LBCookieStore();
  virtual ~LBCookieStore();

  // Loads all cookies from the db and returns them in a vector.
  static std::vector<net::CanonicalCookie *> GetAllCookies();

  // Deletes all cookies from the db.
  static void DeleteAllCookies();

  // Static version of AddCookie below, for easy access.
  static void QuickAddCookie(const net::CanonicalCookie& cc);

  // Static version of DeleteCookie below, for easy access.
  static void QuickDeleteCookie(const net::CanonicalCookie& cc);


  // net::CookieMonster::PersistentCookieStore methods:

  // Initializes the store and retrieves the existing cookies.  This will be
  // called only once at startup.
  virtual void Load(const LoadedCallback& loaded_callback) OVERRIDE;

  // Preempt Load() to get cookies for a specific domain first.  This is an
  // optimization for systems where getting cookies may be expensive, so that
  // the most important cookies can be loaded more quickly.  We don't get any
  // benefit from this optimization, so this is a stub for us.
  virtual void LoadCookiesForKey(const std::string& key,
    const LoadedCallback& loaded_callback) OVERRIDE;

  // Add a single cookie to the store.
  virtual void AddCookie(const net::CanonicalCookie& cc) OVERRIDE;

  // Update a single cookie's access time.
  virtual void UpdateCookieAccessTime(const net::CanonicalCookie& cc) OVERRIDE;

  // Delete a single cookie from the store.
  virtual void DeleteCookie(const net::CanonicalCookie& cc) OVERRIDE;

  // Instructs the store to not discard session-only cookies on shutdown.
  // We don't expect this to be called, and we don't implement these semantics.
  virtual void SetForceKeepSessionState() OVERRIDE { NOTREACHED(); }

  // Flushes the store and posts |callback| when complete.
  virtual void Flush(const base::Closure& callback) OVERRIDE;

  // Reinitialize the tables for cookies in the savegame database
  static void Reinitialize();

 private:
  static void Init();

  static base::Lock init_lock_;
  static bool initialized_;
};

#endif  // SRC_LB_COOKIE_STORE_H_
