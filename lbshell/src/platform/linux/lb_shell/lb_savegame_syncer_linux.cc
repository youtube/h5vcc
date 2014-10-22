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

#include "lb_savegame_syncer.h"

#include "base/file_path.h"
#include "lb_shell_platform_delegate.h"

// static
void LBSavegameSyncer::PlatformInit() {
  // no-op.
}

// static
void LBSavegameSyncer::PlatformShutdown() {
  // no-op.
}

// static
bool LBSavegameSyncer::PlatformMountStart(bool readonly) {
  return true;
}

// static
bool LBSavegameSyncer::PlatformMountEnd() {
  return true;
}

// static
void LBSavegameSyncer::SavegameToFile(const base::Closure& cb) {
  // no-op.
  cb.Run();
}

// static
void LBSavegameSyncer::FileToSavegame(const base::Closure& cb) {
  // no-op.
  cb.Run();
}

// static
FilePath LBSavegameSyncer::GetFilePath() {
  const char *home = getenv("HOME");
  if (!home) home = "/";
  return FilePath(home).Append(".steel.db");
}
