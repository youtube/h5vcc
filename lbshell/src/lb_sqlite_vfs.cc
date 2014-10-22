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

#include "lb_sqlite_vfs.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/synchronization/lock.h"
#include "third_party/sqlite/sqlite3.h"

namespace {

const char* kVfsName = "lb_vfs";

// ------------- File methods ------------

struct virtual_file {
  sqlite3_file base;
  LBVirtualFile *file;

  base::Lock *lock;
  int current_lock;
  int shared;
};

int VfsClose(sqlite3_file *file) {
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  delete vfile->lock;
  return SQLITE_OK;
}

int VfsRead(sqlite3_file *file, void *out, int bytes, sqlite_int64 offset) {
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  vfile->file->Read(out, bytes, offset);
  return SQLITE_OK;
}

int VfsWrite(sqlite3_file *file, const void* data, int bytes,
          sqlite3_int64 offset) {
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  vfile->file->Write(data, bytes, offset);
  return SQLITE_OK;
}

int VfsSync(sqlite3_file*, int flags) {
  return SQLITE_OK;
}

int VfsFileControl(sqlite3_file *pFile, int op, void *pArg) {
  return SQLITE_OK;
}

int VfsSectorSize(sqlite3_file * /*file*/) {
  // The number of bytes that can be read without disturbing other bytes in the
  // file.
  return 1;
}

int VfsLock(sqlite3_file *file, const int mode) {
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  base::AutoLock lock(*vfile->lock);

  // If there is already a lock of this type or more restrictive, do nothing
  if (vfile->current_lock >= mode) {
    return SQLITE_OK;
  }

  if (mode == SQLITE_LOCK_SHARED) {
    DCHECK_EQ(vfile->current_lock, SQLITE_LOCK_NONE);
    vfile->shared++;
    vfile->current_lock = SQLITE_LOCK_SHARED;
  }

  if (mode == SQLITE_LOCK_RESERVED) {
    DCHECK_EQ(vfile->current_lock, SQLITE_LOCK_SHARED);
    vfile->current_lock = SQLITE_LOCK_RESERVED;
  }

  if (mode == SQLITE_LOCK_EXCLUSIVE) {
    if (vfile->current_lock >= SQLITE_LOCK_PENDING) {
      return SQLITE_BUSY;
    }
    vfile->current_lock = SQLITE_LOCK_PENDING;
    if (vfile->shared > 1) {
      // There are some outstanding shared locks (greater than one because the
      // pending lock is an "upgraded" shared lock)
      return SQLITE_BUSY;
    }
    // Acquire the exclusive lock
    vfile->current_lock = SQLITE_LOCK_EXCLUSIVE;
  }

  return SQLITE_OK;
}

int VfsUnlock(sqlite3_file *file, int mode) {
  DCHECK_LE(mode, SQLITE_LOCK_SHARED);
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  base::AutoLock lock(*vfile->lock);

  COMPILE_ASSERT(SQLITE_LOCK_NONE < SQLITE_LOCK_SHARED,
                 sqlite_lock_constants_order_has_changed);
  COMPILE_ASSERT(SQLITE_LOCK_SHARED < SQLITE_LOCK_RESERVED,
                 sqlite_lock_constants_order_has_changed);
  COMPILE_ASSERT(SQLITE_LOCK_RESERVED < SQLITE_LOCK_PENDING,
                 sqlite_lock_constants_order_has_changed);
  COMPILE_ASSERT(SQLITE_LOCK_PENDING < SQLITE_LOCK_EXCLUSIVE,
                 sqlite_lock_constants_order_has_changed);

  if (mode == SQLITE_LOCK_NONE && vfile->current_lock >= SQLITE_LOCK_SHARED) {
    vfile->shared--;
  }

  vfile->current_lock = mode;
  return SQLITE_OK;
}

int VfsCheckReservedLock(sqlite3_file *file, int *result) {
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  base::AutoLock lock(*vfile->lock);

  // The function expects a result is 1 if the lock is reserved, pending, or
  // exclusive; 0 otherwise.
  *result = vfile->current_lock >= SQLITE_LOCK_RESERVED ? 1 : 0;
  return SQLITE_OK;
}

int VfsFileSize(sqlite3_file *file, sqlite3_int64 *out_size) {
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  *out_size = vfile->file->Size();
  return SQLITE_OK;
}

int VfsTruncate(sqlite3_file *file, sqlite3_int64 size) {
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  vfile->file->Truncate(size);
  return SQLITE_OK;
}

int VfsDeviceCharacteristics(sqlite3_file *file) {
  // TODO(wpwang): Implement this if needed.
  return 0;
}

// ------------- Database methods ------------

const sqlite3_io_methods s_lb_vfs_io = {
    1,  // Structure version number
    VfsClose,
    VfsRead,
    VfsWrite,
    VfsTruncate,
    VfsSync,
    VfsFileSize,
    VfsLock,
    VfsUnlock,
    VfsCheckReservedLock,
    VfsFileControl,
    VfsSectorSize,
    VfsDeviceCharacteristics
};

int VfsOpen(sqlite3_vfs *vfs, const char *path, sqlite3_file* file,
         int flags, int *out_flags) {
  virtual_file *vfile = reinterpret_cast<virtual_file*>(file);
  vfile->lock = new base::Lock;
  file->pMethods = &s_lb_vfs_io;

  LBVirtualFileSystem *lb_vfs = reinterpret_cast<LBVirtualFileSystem *>(
      vfs->pAppData);
  vfile->file = lb_vfs->Open(path);
  return SQLITE_OK;
}

int VfsDelete(sqlite3_vfs *vfs, const char *path, int sync_dir) {
  LBVirtualFileSystem *lb_vfs = reinterpret_cast<LBVirtualFileSystem *>(
      vfs->pAppData);
  lb_vfs->Delete(path);
  return SQLITE_OK;
}

int VfsFullPathname(sqlite3_vfs * /*vfs*/, const char *path, int out_size,
                 char *out_path) {
  memcpy(out_path, path, out_size);
  return SQLITE_OK;
}

int VfsAccess(sqlite3_vfs *vfs, const char *name, int flags, int *result) {
  // We should always have a valid, readable/writable file.
  *result |= SQLITE_ACCESS_EXISTS | SQLITE_ACCESS_READWRITE;
  return SQLITE_OK;
}

int VfsRandomness(sqlite3_vfs *, int bytes, char *out) {
  base::RandBytes(out, bytes);
  return SQLITE_OK;
}

sqlite3_vfs s_lb_vfs = {
    1,  // Structure version number
    sizeof(virtual_file),
    MAX_VFS_PATHNAME,
    0,
    kVfsName,
    NULL,  // pAppData; this is set in RegisterVFS
    VfsOpen,
    VfsDelete,
    VfsAccess,
    VfsFullPathname,
    NULL,  // xDlOpen
    NULL,  // xDlError
    NULL,  // xDlSym
    NULL,  // xDlClose
    VfsRandomness,
    NULL,  // xCurrentTime
    NULL,  // xGetLastError
};

}  // namespace

namespace LB {

void RegisterVfs(LBVirtualFileSystem *vfs) {
  int make_default = 1;

  // We only support registering one unique vfs for now. Registering a
  // different LBVirtualFileSystem instance may be harmless if intentional, but
  // that's not a currently expected use-case.
  if (s_lb_vfs.pAppData != NULL && s_lb_vfs.pAppData != vfs) {
    DCHECK(false) << "Attempted to register a different vfs.";
    return;
  }

  s_lb_vfs.pAppData = vfs;

#if defined(__LB_PS4__) || defined(__LB_XB360__)
  sqlite3_vfs_register(&s_lb_vfs, make_default);
#else  // defined(__LB_PS4__)
  // TODO(wpwang): This should be enabled on a per-platform basis.
  NOTREACHED();
#endif  // defined(__LB_PS4__)
}

void UnregisterVfs(LBVirtualFileSystem *vfs) {
  s_lb_vfs.pAppData = vfs;

#if defined(__LB_PS4__) || defined(__LB_XB360__)
  sqlite3_vfs_unregister(&s_lb_vfs);
#else  // defined(__LB_PS4__)
  // TODO(wpwang): This should be enabled on a per-platform basis.
  NOTREACHED();
#endif  // defined(__LB_PS4__)
}

}  // namespace LB
