// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(__LB_XB1__) && !defined(__LB_SHELL__FOR_RELEASE__)

// Stack tracing is using DbgHelp which doesn't support the TV_APP partition
// at the moment. To make it work, we have to trick the headers into thinking
// that they are compiled for a desktop application. This is the reason
// why the functionality will only be enabled in internal builds.

#pragma push_macro("WINAPI_PARTITION_DESKTOP")
#undef WINAPI_PARTITION_DESKTOP
#define WINAPI_PARTITION_DESKTOP 1
#include "stack_trace_win.cc"
#pragma pop_macro("WINAPI_PARTITION_DESKTOP")

// EnableInProcessStackDumping is implemented in stack_trace_xb1.cc

#elif !defined(__LB_ANDROID__) && !defined(__LB_XB360__) \
    && !defined(__LB_PS4__)
// stack_trace_android.cc is already compiled via base.gypi
// stack_trace_ps4.cc is already compiled via base.gypi
// stack_trace_xb360.cc is already compiled via base.gypi

#include "stack_trace_posix.cc"

namespace base {
namespace debug {

// But re-implement this:
bool EnableInProcessStackDumping() {
  // We need this to return true to run unit tests
  return true;
}

} // namespace debug
} // namespace base

#endif

#if defined(__LB_XB360__) || defined(__LB_PS4__)
namespace base {
namespace debug {
namespace internal {

// NOTE: code from sandbox/linux/seccomp-bpf/demo.cc.
char *itoa_r(intptr_t i, char *buf, size_t sz, int base) {
  // Make sure we can write at least one NUL byte.
  size_t n = 1;
  if (n > sz)
    return NULL;

  if (base < 2 || base > 16) {
    buf[0] = '\000';
    return NULL;
  }

  char *start = buf;

  uintptr_t j = i;

  // Handle negative numbers (only for base 10).
  if (i < 0 && base == 10) {
    j = -i;

    // Make sure we can write the '-' character.
    if (++n > sz) {
      buf[0] = '\000';
      return NULL;
    }
    *start++ = '-';
  }

  // Loop until we have converted the entire number. Output at least one
  // character (i.e. '0').
  char *ptr = start;
  do {
    // Make sure there is still enough space left in our output buffer.
    if (++n > sz) {
      buf[0] = '\000';
      return NULL;
    }

    // Output the next digit.
    *ptr++ = "0123456789abcdef"[j % base];
    j /= base;
  } while (j);

  // Terminate the output with a NUL character.
  *ptr = '\000';

  // Conversion to ASCII actually resulted in the digits being in reverse
  // order. We can't easily generate them in forward order, as we can't tell
  // the number of characters needed until we are done converting.
  // So, now, we reverse the string (except for the possible "-" sign).
  while (--ptr > start) {
    char ch = *ptr;
    *ptr = *start;
    *start++ = ch;
  }
  return buf;
}

}  // namespace internal
}  // namespace debug
}  // namespace base

#endif