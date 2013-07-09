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
  // TODO: unimplemented!
}

// static
void LBSavegameSyncer::PlatformShutdown() {
  // TODO: unimplemented!
}

// static
void LBSavegameSyncer::SavegameToFile(InternalCallback cb) {
  // TODO: unimplemented!
  cb();
}

// static
void LBSavegameSyncer::FileToSavegame(InternalCallback cb) {
  // TODO: unimplemented!
  cb();
}

// static
FilePath LBSavegameSyncer::GetFilePath() {
  // TODO: unimplemented!
  return FilePath();
}

