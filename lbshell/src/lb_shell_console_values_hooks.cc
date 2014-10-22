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
#include "lb_shell_console_values_hooks.h"

#include <stdio.h>

#include "base/debug/trace_event.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/string_tokenizer.h"

#include "lb_globals.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace LB {

ShellConsoleValueHooks* ShellConsoleValueHooks::instance_ = NULL;

ShellConsoleValueHooks::ShellConsoleValueHooks() {
  DCHECK_EQ(instance_, reinterpret_cast<ShellConsoleValueHooks*>(NULL));
  instance_ = this;
}

ShellConsoleValueHooks::~ShellConsoleValueHooks() {
  DCHECK_NE(instance_, reinterpret_cast<ShellConsoleValueHooks*>(NULL));
  instance_ = NULL;
}

ShellConsoleValueHooks::Whitelist::Whitelist()
    : lastused_whitelist_filename_(
          FilePath(GetGlobalsPtr()->screenshot_output_path).Append(
              "lastused_cval_whitelist.cvals")) {
  // Load the user's last used whitelist in to memory, but if there's a problem
  // doing that (i.e. the last used file does not exist) fallback to default.
  bool load_successful = false;

  base::AutoLock lock(monitor_lock_);  // To appease some AssertAcquired()s.
  load_successful = LoadWhitelistFromDisc(lastused_whitelist_filename_);

  if (!load_successful) {
    RestoreToDefaultInternal();
  }
}

void ShellConsoleValueHooks::Whitelist::RestoreToDefaultInternal() {
  monitor_lock_.AssertAcquired();

  // Setup a default whitelist.  Platform-specific values can go here as well,
  // if a value appears in the whitelist but the CVal does not actually exist,
  // it will be ignored.  Note that if the default whitelist becomes more
  // complicated/messy and more platform-specific values are specified,
  // it may make sense to specify the default whitelist in a different way,
  // perhaps by loading it from files on disc.
  values_.clear();

  values_.insert("gpu.UsedMemory");

  values_.insert("Memory.App");
  values_.insert("Memory.Exe");
  values_.insert("Memory.Free");
  values_.insert("Memory.MSApp");
  values_.insert("Memory.PS4.DirectBytesMapped");
  values_.insert("Memory.PS4.FlexibleBytesMapped");

  values_.insert("Skia.Memory");
  values_.insert("Telnet.Port");
  values_.insert("VersionInfo.LB.BuildId");
  values_.insert("WebInspector");

  values_.insert("WebKit.DOM.NumNodes");
  values_.insert("WebKit.JS.Memory");
}

std::set<std::string>
ShellConsoleValueHooks::Whitelist::GetOrderedValues() const {
  base::AutoLock lock(monitor_lock_);

  return std::set<std::string>(values_.begin(), values_.end());
}

bool ShellConsoleValueHooks::Whitelist::Contains(
    const std::string& cval_name) const {
  base::AutoLock lock(monitor_lock_);

  return values_.find(cval_name) != values_.end();
}

bool ShellConsoleValueHooks::Whitelist::Add(const std::string& cval_name) {
  base::AutoLock lock(monitor_lock_);

  std::pair<CValSetType<std::string>::iterator, bool> found =
      values_.insert(cval_name);

  SaveToLastUsedFile();

  return found.second;
}

bool ShellConsoleValueHooks::Whitelist::Remove(const std::string& cval_name) {
  base::AutoLock lock(monitor_lock_);

  CValSetType<std::string>::iterator found = values_.find(cval_name);
  if (found == values_.end()) {
    return false;
  } else {
    values_.erase(found);
    SaveToLastUsedFile();
    return true;
  }
}

void ShellConsoleValueHooks::Whitelist::RestoreToDefault() {
  base::AutoLock lock(monitor_lock_);

  RestoreToDefaultInternal();

  SaveToLastUsedFile();
}

bool ShellConsoleValueHooks::Whitelist::LoadNamedWhitelist(
    const std::string& filename) {
  base::AutoLock lock(monitor_lock_);
  bool result = LoadWhitelistFromDisc(GetFilePathForNamedWhitelist(filename));
  if (result) {
    SaveToLastUsedFile();
  }
  return result;
}

bool ShellConsoleValueHooks::Whitelist::SaveNamedWhitelist(
    const std::string& filename) {
  base::AutoLock lock(monitor_lock_);
  return SaveWhitelistToDisc(GetFilePathForNamedWhitelist(filename));
}

FilePath ShellConsoleValueHooks::Whitelist::GetFilePathForNamedWhitelist(
    const std::string& filename) {
  monitor_lock_.AssertAcquired();
  return FilePath(GetGlobalsPtr()->screenshot_output_path).Append(filename);
}

// Whitelist file formats are simply a list of CVal names separated by
// newlines.
bool ShellConsoleValueHooks::Whitelist::LoadWhitelistFromDisc(
    const FilePath& filepath) {
  monitor_lock_.AssertAcquired();

  // First read the entire file in to memory
  std::string contents;
  if (!file_util::ReadFileToString(filepath, &contents)) {
    return false;
  }

  // Now tokenize the results deliminating by newline characters, adding
  // each line as a separate CVal.
  values_.clear();
  StringTokenizer tokenizer(contents, "\n");
  while (tokenizer.GetNext()) {
    base::StringPiece token = tokenizer.token_piece();
    if (token.length() > 0) {
      values_.insert(token.as_string());
    }
  }

  return true;
}

bool ShellConsoleValueHooks::Whitelist::SaveWhitelistToDisc(
    const FilePath& filepath) {
  monitor_lock_.AssertAcquired();
  file_util::ScopedFILE fp(file_util::OpenFile(filepath, "w"));
  if (!fp.get()) {
    return false;
  }

  // Print out each CVal in our current whitelist line by line to a file.
  for (CValSetType<std::string>::const_iterator iter = values_.begin();
       iter != values_.end(); ++iter) {
    int bytes_written = fprintf(fp.get(), "%s\n", iter->c_str());
    if (bytes_written < 0) {
      return false;
    }
  }

  return true;
}

void ShellConsoleValueHooks::Whitelist::SaveToLastUsedFile() {
  if (!SaveWhitelistToDisc(lastused_whitelist_filename_)) {
    DLOG(WARNING) << "Could not save CVal whitelist changes to file "
                  << lastused_whitelist_filename_.value().c_str();
  }
}

// For memory logging, if we notice that a numerical value has been modified,
// track it as a memory logging counter value.
void MemoryLoggerConsoleValueHook::OnValueChanged(
    const std::string& name, const ConsoleGenericValue& value) {
  if (!LB::Memory::IsContinuousLogEnabled()) {
    return;
  }
  if (!ShellConsoleValueHooks::GetInstance()->GetWhitelist()->Contains(name)) {
    return;
  }

  if (value.CanConvert<int64_t>()) {
    LB::Memory::LogNamedCounter(name, value.Convert<int64_t>());
  }
}

// For TRACE_EVENT logging, signal a counter event if it is a numerical value,
// otherwise signal an instant event.
void TraceEventConsoleValueHook::OnValueChanged(
    const std::string& name, const ConsoleGenericValue& value) {
  if (!ShellConsoleValueHooks::GetInstance()->GetWhitelist()->Contains(name)) {
    return;
  }

  if (value.CanConvert<int64_t>()) {
    TRACE_COPY_COUNTER1("CVal", name.c_str(), value.Convert<int64_t>());
  } else {
    TRACE_EVENT_COPY_INSTANT1("CVal", name.c_str(),
                              "Value",
                              value.AsString().c_str());
  }
}

}  // namespace LB

#endif  // #if defined(__LB_SHELL__ENABLE_CONSOLE__)
