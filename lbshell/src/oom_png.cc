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
// OOM-safe libpng wrappers.

#include "lb_memory_manager.h"
#if !defined(__LB_SHELL__FOR_RELEASE__)

#include "lb_platform.h"
#include "oom_png.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// OOM-safe memory allocation of up to 1MB for libpng.
static char png_oom_buffer[1024 * 1024];
static char *png_oom_next_addr;
static char *png_oom_last_addr = png_oom_buffer + sizeof(png_oom_buffer);

static void png_oom_allocator_reset() {
  png_oom_next_addr = png_oom_buffer;
}

static png_voidp png_oom_malloc(png_structp png, png_size_t size) {
  png_voidp ptr = (png_voidp)png_oom_next_addr;
  png_oom_next_addr += size;
  if (png_oom_next_addr > png_oom_last_addr) {
#if defined(_DEBUG)
    LB::Platform::DEBUG_BREAK();
#endif
    ptr = NULL;
  }
  return ptr;
}

static void png_oom_free(png_structp png, png_voidp ptr) {
  // nop
}


// OOM-safe IO for libpng.
static void png_oom_write(png_structp png, png_bytep data, png_size_t length) {
  int fd = static_cast<int>(reinterpret_cast<intptr_t>(png_get_io_ptr(png)));
  write(fd, data, length);
}

static void png_oom_flush(png_structp png) {
  // nop
}


// OOM-safe wrappers around libpng's writing functions.
png_structp oom_png_create(const char *file_name, int width, int height,
                           int bit_depth, int color_type) {
  // open the output file.
  int fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
  if (fd < 0) {
    oom_fprintf(2, "Unable to open %s for writing.\n", file_name);
    return NULL;
  }

  // reset the custom allocator.
  png_oom_allocator_reset();

  // allocate a structure for writing, using our custom allocator.
  png_structp png = png_create_write_struct_2(
      PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL,
      reinterpret_cast<void *>((intptr_t)fd),
      png_oom_malloc, png_oom_free);
  if (!png) {
    oom_fprintf(2, "Unable to create png structure.\n");
    return NULL;
  }

  // tell libpng to use our custom writing functions.
  png_set_write_fn(
      png, reinterpret_cast<void *>((intptr_t)fd),
      png_oom_write, png_oom_flush);

  // allocate an info structure.
  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_write_struct(&png, NULL);
    oom_fprintf(2, "Unable to create png info structure.\n");
    return NULL;
  }

  // set the error-handler.
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    oom_fprintf(2, "Unable to write png headers.\n");
    return NULL;
  }

  // write the headers.
  png_set_IHDR(png, info, width, height, bit_depth, color_type,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);

  // free the info structure.
  png_destroy_info_struct(png, &info);

  return png;
}

void oom_png_destroy(png_structp png) {
  int fd = static_cast<int>(reinterpret_cast<intptr_t>(png_get_mem_ptr(png)));

  // finish the png and free its memory.
  png_write_end(png, NULL);
  png_destroy_write_struct(&png, NULL);

  // close the output file.
  close(fd);
}

#endif  // #if !defined(__LB_SHELL__FOR_RELEASE__)
