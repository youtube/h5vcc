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
#include "base/sys_info.h"

#include <sys/statvfs.h>
namespace base {

// TODO: Read these settings from a config file.

int64 SysInfo::AmountOfPhysicalMemory() {
  // emulate PS3 for now.
  return 256LL*1024*1024;
}

int SysInfo::NumberOfProcessors() {
  return 1;
}

// static
int64 SysInfo::AmountOfFreeDiskSpace(const FilePath& path) {
  struct statvfs stats;
  if (statvfs(path.value().c_str(), &stats) != 0) {
    return -1;
  }
  return static_cast<int64>(stats.f_bavail) * stats.f_frsize;
}

}  // namespace base
