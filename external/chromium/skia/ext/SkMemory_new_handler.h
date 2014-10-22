/*
**
** Copyright 2013, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef SkMemory_new_handler_DEFINED
#define SkMemory_new_handler_DEFINED

#include "SkPreConfig.h"
#include <stdlib.h>
#include <stdint.h>

// API for external code to query Skia memory stats.
// Must call sk_set_block_size_func so that
// sk_free can get the size of the blocks it frees.
#ifdef __cplusplus
extern "C" {
#endif
  typedef size_t (*BlockSizeFunc)(void*);

  SK_API size_t sk_get_bytes_allocated();
  SK_API size_t sk_get_max_bytes_allocated();
  SK_API void sk_set_block_size_func(BlockSizeFunc f);
#ifdef __cplusplus
}
#endif
#endif  // SkMemory_new_handler_DEFINED
