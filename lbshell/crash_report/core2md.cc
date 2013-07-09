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

#include <stdio.h>

#include "core_dumper.h"
#include "minidump_writer.h"
#include "symbol_info.h"

using crash_report::CoreDumper;
using crash_report::SymbolInfo;

bool WriteMinidumpFromCore(const char* filename,
                           const char* core_path,
                           const char* symbol_file) {
  CoreDumper dumper(core_path);
  SymbolInfo symbol_info(symbol_file);
  return crash_report::WriteMinidump(filename, &dumper, &symbol_info);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <core file> <symbol path> <output>\n", argv[0]);
    return 1;
  }

  const char* core_file = argv[1];
  const char* symbol_file = argv[2];
  const char* minidump_file = argv[3];

  if (!WriteMinidumpFromCore(minidump_file, core_file, symbol_file)) {
    fprintf(stderr, "Unable to generate minidump.\n");
    return 1;
  }
  return 0;
}
