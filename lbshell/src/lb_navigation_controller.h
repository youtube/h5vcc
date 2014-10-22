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
#ifndef SRC_LB_NAVIGATION_CONTROLLER_H_
#define SRC_LB_NAVIGATION_CONTROLLER_H_

#include <string>
#include <vector>

#include "external/chromium/base/basictypes.h"
#include "external/chromium/googleurl/src/gurl.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebHistoryItem.h"

#include "lb_console_values.h"

class LBShell;

class LBNavigationController {
 public:
  explicit LBNavigationController(LBShell * shell);
  ~LBNavigationController();

  // Returns the index from which we would go back/forward or reload.
  int GetCurrentEntryIndex() const;

  // Returns the current entry.
  WebKit::WebHistoryItem GetCurrentEntry() const;

  // Returns the pending entry.
  WebKit::WebHistoryItem GetPendingEntry() const;

  // Returns the number of entries, excluding any pending entries.
  int GetEntryCount() const;

  // Causes the controller to go to the specified offset from current.  Does
  // nothing if out of bounds.
  void GoToOffset(int offset);

  // Used to inform us of a navigation starting.
  void Pending(const WebKit::WebHistoryItem& entry);

  // Used to inform us of a navigation being committed.
  void CommitPending(const WebKit::WebHistoryItem& entry);

  // Add a new entry that we didn't have to load.
  void AddEntry(const WebKit::WebHistoryItem& entry);

  // Update the address of the current entry.
  void UpdateCurrentEntry(const WebKit::WebHistoryItem& entry);

  // Wipe out the history.
  // Called from the WebKit thread to make sure that WebHistoryItems are
  // destroyed on the correct thread.
  void Clear();

 private:
  void DiscardPendingEntry();
  void NavigateToPendingEntry();

  // Remove the oldest entry in entry lits.
  void RemoveOldestEntry();

  std::vector<WebKit::WebHistoryItem> entries_;
  WebKit::WebHistoryItem pending_entry_;

  // currently visible entry
  int current_entry_index_;

  // index of pending entry if it is in entries_, or -1 otherwise.
  int pending_entry_index_;

  LBShell* shell_;

  LB::CVal<std::string> current_url_;

  DISALLOW_COPY_AND_ASSIGN(LBNavigationController);
};

#endif  // SRC_LB_NAVIGATION_CONTROLLER_H_
