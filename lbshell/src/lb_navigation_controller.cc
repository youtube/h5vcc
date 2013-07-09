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
#include "lb_navigation_controller.h"

#include "external/chromium/base/logging.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/media/base/shell_filter_graph_log.h"
#include "lb_graphics.h"
#include "lb_shell.h"

// ----------------------------------------------------------------------------
// LBNavigationController
static const int kMaxHistoryItem = 16;

LBNavigationController::LBNavigationController(LBShell* shell)
    : current_entry_index_(-1)
    , pending_entry_index_(-1)
    , shell_(shell) {
}

LBNavigationController::~LBNavigationController() {
}

int LBNavigationController::GetCurrentEntryIndex() const {
  if (pending_entry_index_ != -1)
    return pending_entry_index_;
  return current_entry_index_;
}

WebKit::WebHistoryItem LBNavigationController::GetCurrentEntry() const {
  int index = GetCurrentEntryIndex();
  if (index <= -1)
    return WebKit::WebHistoryItem();
  return entries_[index];
}

WebKit::WebHistoryItem LBNavigationController::GetPendingEntry() const {
  return pending_entry_;
}

int LBNavigationController::GetEntryCount() const {
  return entries_.size();
}

void LBNavigationController::GoToOffset(int offset) {
  // Base the navigation on where we are now...
  int index = GetCurrentEntryIndex() + offset;
  if (index < 0 || index >= GetEntryCount())
    return;

  DiscardPendingEntry();

  pending_entry_index_ = index;
  NavigateToPendingEntry();
}

void LBNavigationController::Pending(const WebKit::WebHistoryItem& entry) {
  // When navigating to a new page, we don't know for sure if we will actually
  // end up leaving the current page.  The new page load could for example
  // result in a download or a 'no content' response (e.g., a mailto: URL).
  DiscardPendingEntry();
  pending_entry_ = entry;
}

void LBNavigationController::CommitPending(
    const WebKit::WebHistoryItem& entry) {
  if (pending_entry_index_ == -1) {
    if (pending_entry_.isNull() == false) {
      // A pending entry from a Pending() call is now done.
      // Commit it by adding it to the list.
      AddEntry(entry);
    } else {
      // This has never happened, but we will be safe and guess that the
      // correct behavior would be to update the current entry.
      UpdateCurrentEntry(entry);
    }
  } else {
    // An in-history navigation is now complete, so update the pending entry
    // pointed to by pending_entry_index_.
    UpdateCurrentEntry(entry);
    // Make the pending entry current.
    current_entry_index_ = pending_entry_index_;
  }
  DiscardPendingEntry();
}

void LBNavigationController::AddEntry(const WebKit::WebHistoryItem& entry) {
  // erase everything after the current entry:
  std::vector<WebKit::WebHistoryItem>::iterator next, end;
  next = entries_.begin() + current_entry_index_ + 1;
  end = entries_.end();
  entries_.erase(next, end);
  // add the new entry, and make it current:
  entries_.push_back(entry);
  current_entry_index_ = entries_.size() - 1;
  // We keep track of only a few history items.
  if (entries_.size() > kMaxHistoryItem)
    RemoveOldestEntry();
  if (!entry.isNull()) {
    std::string url = entry.urlString().utf8();
    DLOG(INFO) << "address now: " << url;
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    LBGraphics::GetPtr()->SetOSDURL(url);
#endif
    media::ShellFilterGraphLog::SetGraphLoggingPageURL(url.c_str());
  }
}

void LBNavigationController::UpdateCurrentEntry(const WebKit::WebHistoryItem& entry) {
  int index = GetCurrentEntryIndex();
  if (index == -1)
    return;
  entries_[index] = entry;
}

void LBNavigationController::DiscardPendingEntry() {
  pending_entry_ = WebKit::WebHistoryItem();
  pending_entry_index_ = -1;
}

void LBNavigationController::NavigateToPendingEntry() {
  bool ok;

  DCHECK(pending_entry_index_ != -1);

  if (pending_entry_index_ == current_entry_index_) {
    // reload
    ok = shell_->Reload();
  } else {
    // session history navigation, only the index is set
    ok = shell_->Navigate(entries_[pending_entry_index_]);
  }

  if (!ok) {
    DiscardPendingEntry();
  }
}

void LBNavigationController::RemoveOldestEntry() {
  // Remove an item from the beginning of entries.
  // TODO: a queue is a better choice for the entry list but
  // using a queue needs more changes but we are shipping now. We can
  // replace it later when we get some time.
  // The functionalities used for entries_ are:
  // moving n forwards/backwords
  // removing the first item
  // removing the last n items.
  if (entries_.size() > 1) {
    entries_.erase(entries_.begin());
    current_entry_index_--;
    if (pending_entry_index_ >= 0)
      pending_entry_index_--;
  }
}
