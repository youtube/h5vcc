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

#ifndef SYMBOL_INFO_H_
#define SYMBOL_INFO_H_

#include <stdio.h>

#include "external/chromium/breakpad/src/common/memory.h"
#include "external/chromium/breakpad/src/common/linux/memory_mapped_file.h"
#include "external/chromium/breakpad/src/google_breakpad/common/minidump_format.h"

namespace crash_report {

class SymbolInfo {
 public:
  SymbolInfo(const char* symbol_path);

  ~SymbolInfo();

  bool Init();

  const char* GetSymbolName() {
    return symbol_name_;
  }

  MDGUID GetId() {
    return identifier_;
  }

 private:
  bool readIdentifier(const char* id);

  const char* symbol_path_;
  const char* symbol_name_;
  MDGUID identifier_;
};

} // namespace crash_report

#endif // SYMBOL_INFO_H_
