/*
 * Copyright 2013 Google Inc. All Rights Reserved.
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

#include "symbol_info.h"

#include <assert.h>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "external/chromium/breakpad/src/common/memory_range.h"

namespace crash_report {

SymbolInfo::SymbolInfo(const char* symbol_path) : symbol_path_(symbol_path) {
  assert(symbol_path_);
}

SymbolInfo::~SymbolInfo() {
}

// Read identifier from the symbol file and store in |identifier_|
// |id| is hex string read from symbol file.
// |identifier_| is of type MDGUID. MDGUID is defined in minidump_format.h.
bool SymbolInfo::readIdentifier(const char* id) {
  for (int i = 0; i < strlen(id); ++i) {
    if (!isxdigit(id[i])) return false;
  }
  std::string id_string(id);
  std::string data1 = id_string.substr(0, 8);
  identifier_.data1 = strtol(data1.c_str(), NULL, 16);
  std::string data2 = id_string.substr(8, 4);
  identifier_.data2 = strtol(data2.c_str(), NULL, 16);
  std::string data3 = id_string.substr(12, 4);
  identifier_.data3 = strtol(data3.c_str(), NULL, 16);
  for (int i = 0; i < 8; ++i) {
    std::string data = id_string.substr(16 + 2 * i, 2);
    identifier_.data4[i] = strtol(data.c_str(), NULL, 16);
  }
  return true;
}

bool SymbolInfo::Init() {
  std::ifstream symbol_file;
  symbol_file.open(symbol_path_, std::ifstream::in);
  std::string module, os, architecture, id, name;
  symbol_file >> module >> os >> architecture >> id >> name;
  if (!readIdentifier(id.c_str())) {
    return false;
  }
  symbol_name_ = name.c_str();
  return true;
}

} // namespace crash_report
