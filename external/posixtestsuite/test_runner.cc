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
// This test runner requires that pthread_create, pthread_join, and
// pthread_setspecific all operate correctly in a basic fashion.
// It uses the basic functionality of these primitives to test the
// pthread implementation more deeply using a single process.

// Forward include all of the headers that the tests may want to use.
// This helps keep standard apis declared outside of any namespace,
// since the tests themselves will all be thrown into namespaces.
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

typedef int(*main_t)();

typedef struct {
  main_t fn;
  const char *name;
  int retval;
  int complete;
} test_t;

#if defined(__LB_XB1__) || defined(__LB_XB360__)
#undef FAILED
#endif

#ifdef __LB_ANDROID__
// The system's pthread_exit is missing the noreturn attribute, which causes
// the compiler to complain in strict mode (-Werror,-Wreturn-type).  Simply
// redeclare it with the attribute added so that the compiler understands.
extern void __noreturn pthread_exit(void * retval);
#endif

// Don't exit the app, exit the thread which contains this one test.
#define exit(x) pthread_exit((void *)(intptr_t)x)

// Disable alarms.  Let the test runner handle timeouts.
#define signal(signum, handler) do {} while (0)
#define alarm(timeout) do {} while (0)

// We don't need perror anywhere else, and may not have it on all platforms,
// so just fake it.
#undef perror
#define perror(msg) fprintf(stderr, msg": errno = %d\n", errno)

#ifdef __LB_LINUX__
// Undefine these macros which are defined on Linux and get redefined by tests.
#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE

// These macros don't exist on Linux, but we added them to our other platforms.
#define PTHREAD_STACK_MIN sysconf(_SC_THREAD_STACK_MIN)
#define PTHREAD_MAX_THREADS 64
#endif

#include "generated_pthread_test_list.cc"

// Undef exit, which we will be using directly in the test runner
// for exceptional circumstances.
#undef exit

#ifdef __LB_XB1__
// We have been mangling the mains of other files, so re-do the mangled main
// we should have gotten from posix_emulation.
# define main main_xb1
#endif

#define TEST_TIMEOUT_IN_SECONDS 30

static int g_passed = 0;
static int g_failed = 0;
static int g_unresolved = 0;
static int g_unsupported = 0;
static int g_untested = 0;
static int g_timed_out = 0;

// This is a key which stores the test_t object for each test thread.
// The destructor for this key will allow us to set the "complete" flag
// when the test thread exits.
static pthread_key_t g_test_object_key;

static void thread_cleanup(void *test_object_value) {
  test_t *test = (test_t *)test_object_value;
  // Mark the test as complete.  This allows the test runner to poll for
  // completion and timeout if a test hangs.
  test->complete = 1;
}

static void *test_thread(void *arg) {
  test_t *test = (test_t *)arg;
  // register for cleanup later.
  pthread_setspecific(g_test_object_key, arg);

  // for easier debugging when threads leak.
#if defined(__LB_LINUX__) || defined(__LB_ANDROID__)
  // there is a 16-character limit on thread names on these platforms.
  char name[16];
  strncpy(name, test->name, 16);
  name[15] = '\0';
  int name_error = pthread_setname_np(pthread_self(), name);
#else
  int name_error = pthread_setname_np(test->name);
#endif
  if (name_error) {
    printf("CAN'T SET THREAD NAME FOR DEBUGGING! (%d)\n", name_error);
    exit(2);
  }

  int retval = test->fn();
  return (void *)(intptr_t)retval;
}

static void run_test(test_t *test) {
  // Launch this test in its own thread.
  // This allows the test to use exit(), which has been mapped to
  // pthread_exit() instead.
  pthread_t t;
  int i;
  int create_error;

  test->complete = 0;
  printf("Running %s\n", test->name);
  create_error = pthread_create(&t, NULL, test_thread, test);
  if (create_error) {
    printf("FAILED TO LAUNCH TEST THREAD! (%d)\n", create_error);
    exit(2);
  }

  // Poll for test completion (up to the timeout).
  for (i = 0; i < TEST_TIMEOUT_IN_SECONDS * 2; ++i) {
    if (test->complete) break;
    usleep(500 * 1000);  // half a second
  }

  if (!test->complete) {
    printf("Test TIMED OUT, test thread left alive!\n");
    test->retval = PTS_TIMEOUT;
    pthread_detach(t);
  } else {
    void *retval;
    int join_error = pthread_join(t, &retval);
    if (join_error) {
      printf("FAILED TO JOIN TEST THREAD! (%d)\n", join_error);
      exit(2);
    }
    test->retval = (int)(intptr_t)retval;
  }

  switch (test->retval) {
    case PTS_PASS:
      ++g_passed;
      break;
    case PTS_FAIL:
      ++g_failed;
      break;
    case PTS_UNRESOLVED:
      ++g_unresolved;
      break;
    case PTS_UNSUPPORTED:
      ++g_unsupported;
      break;
    case PTS_UNTESTED:
      ++g_untested;
      break;
    case PTS_TIMEOUT:
      ++g_timed_out;
      break;
    default:
      printf("UNRECOGNIZED RETURN VALUE! (%d)\n", test->retval);
      exit(2);
      break;
  }
}

static const char *OUTCOME_STRING[] = {
  "passed",
  "FAILED",
  "is unresolved",
  "UNUSED CONSTANT",
  "is unsupported",
  "is untested",
  "timed out"
};

int main(int argc, char **argv) {
  test_t *test;
  int total = 0;
  const char *filter = NULL;

  // Optionally filter the list of tests to run.
  if (argc > 1) {
    filter = argv[1];
  }

  // Set up the global key which is used for thread cleanup.
  pthread_key_create(&g_test_object_key, thread_cleanup);

  for (test = all_tests; test->fn && test->name; ++test) {
    if (filter && !strstr(test->name, filter))
      continue;
    run_test(test);
    ++total;
  }

  printf("%d total, %d passed, %d failed, %d unresolved, %d unsupported, "
         "%d untested, %d timed out.\n",
         total, g_passed, g_failed, g_unresolved, g_unsupported,
         g_untested, g_timed_out);

  for (test = all_tests; test->fn && test->name; ++test) {
    if (filter && !strstr(test->name, filter))
      continue;
    if (test->retval != PTS_PASS) {
      printf("%s %s\n", test->name, OUTCOME_STRING[test->retval]);
    }
  }

  return g_passed == total ? 0 : 1;
}

