/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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
#ifndef SRC_LB_SHELL_CONSOLE_VALUES_HOOKS_H_
#define SRC_LB_SHELL_CONSOLE_VALUES_HOOKS_H_

#include <set>

#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/synchronization/lock.h"

#include "lb_console_values.h"
#include "lb_memory_manager.h"
#include "lb_shell_export.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace LB {

// Returns a whitelist of CVal names that we are interested in and will
// receive hook callbacks for.
LB_SHELL_EXPORT CValSetType<std::string> GetLBCValNotifyWhitelist();

// Hook up the memory logging system to CVals
class LB_SHELL_EXPORT MemoryLoggerConsoleValueHook
    : public ConsoleValueManager::OnChangedHook {
 public:
  virtual void OnValueChanged(const std::string& name,
                              const ConsoleGenericValue& value) OVERRIDE;
};

// Hook up the Chrome TRACE_EVENT system to CVals.
class LB_SHELL_EXPORT TraceEventConsoleValueHook
    : public ConsoleValueManager::OnChangedHook {
 public:
  virtual void OnValueChanged(const std::string& name,
                              const ConsoleGenericValue& value) OVERRIDE;
};

// A class to aggregate all the hooks in one place.  This is useful to provide
// a quick way of attaching many useful LB value tracking hooks in one call.
class LB_SHELL_EXPORT ShellConsoleValueHooks {
 public:
  ShellConsoleValueHooks();
  ~ShellConsoleValueHooks();

  static ShellConsoleValueHooks* GetInstance() { return instance_; }

  class Whitelist {
   public:
    Whitelist();

    // Sets this whitelist to the default whitelist
    void RestoreToDefault();

    // Loads a whitelist from the specified filename
    bool LoadNamedWhitelist(const std::string& filename);

    // Saves the current whitelist to the specified file
    bool SaveNamedWhitelist(const std::string& filename);

    // Returns a set of ordered CVal names that are in the whitelist
    std::set<std::string> GetOrderedValues() const;

    // Returns true if the whitelist contains the specified CVal, and false
    // otherwise.
    bool Contains(const std::string& cval_name) const;

    // Adds the specified CVal to the whitelist.  Returns false if it is
    // already in the whitelist and true otherwise.
    bool Add(const std::string& cval_name);

    // Removes the specified CVal from the whitelist.  Returns false if it is
    // not in the whitelist, and true otherwise.
    bool Remove(const std::string& cval_name);

   private:
    void SaveToLastUsedFile();  // Writes current whitelist to "last used" file
    void RestoreToDefaultInternal();  // Does not save to last used list

    // Loads whitelist values from the specified file
    bool LoadWhitelistFromDisc(const FilePath& filename);

    // Saves current whitelist values to the specified file
    bool SaveWhitelistToDisc(const FilePath& filename);

    // Returns a full FilePath for a named whitelist (i.e. what the user
    // types in at the console)
    FilePath GetFilePathForNamedWhitelist(const std::string& filename);

    // The actual whitelist values
    CValSetType<std::string> values_;

    // Filenames where the last used whitelist is kept
    const FilePath lastused_whitelist_filename_;

    mutable base::Lock monitor_lock_;
  };

  Whitelist* GetWhitelist() { return &whitelist_; }

 private:
  static ShellConsoleValueHooks* instance_;

  MemoryLoggerConsoleValueHook memory_logger_hook_;
  TraceEventConsoleValueHook trace_event_hook_;

  Whitelist whitelist_;
};

}  // namespace LB

#endif  // #if defined(__LB_SHELL__ENABLE_CONSOLE__)

#endif  // SRC_LB_SHELL_CONSOLE_VALUES_HOOKS_H_
