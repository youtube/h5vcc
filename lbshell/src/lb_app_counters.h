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
#ifndef _LB_APP_COUNTERS_H_
#define _LB_APP_COUNTERS_H_

#include "base/memory/scoped_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"

class LBAppCounters {
 public:
  // When constructed, starts an asynchronous task to increment the startup
  // counter once the savegame data becomes available.
  LBAppCounters();

  // When destroyed, increments the clean exit counter.
  ~LBAppCounters();

  static int Startups();
  static int CleanExits();

 private:
  static LBAppCounters *instance_;

  // Run asynchronously.  Waits for the savegame data to load, then increments
  // the startup count and forces a sync back to the savegame.
  void InitTask();

  void LoadCounts();
  void StoreCounts();

  scoped_ptr<base::Thread> thread_;

  // A signal that the counts have been loaded.
  base::WaitableEvent loaded_;

  bool shutdown_;

  int startups_;
  int clean_exits_;
};

#endif
