#!/usr/bin/python

# Copyright 2012 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import glob
import os
import re
import sys

class test_case(object):
  def __init__(self, path):
    self.path = path
    # The test name is based on the last two parts of the path.
    # For example, "conformance/interfaces/pthread_rwlock_rdlock/1-1.c"
    # has the name "pthread_rwlock_rdlock/1-1".
    self.name = os.path.splitext('/'.join(self.path.split('/')[-2:]))[0]
    # The contents would live in a namespace called "pthread_rwlock_rdlock_1_1".
    self.ns_name = self.name.replace('/', '_').replace('-', '_')

  def output_code(self, f):
    # Clean up these commonly used macros from one test to another.
    f.write('#undef CASE\n')
    f.write('#undef CASE_NEG\n')
    f.write('#undef CASE_POS\n')
    f.write('#undef CASE_UNK\n')
    f.write('#undef COUNT\n')
    f.write('#undef EXITING_THREAD\n')
    f.write('#undef FUNCTION\n')
    f.write('#undef INTERVAL\n')
    f.write('#undef KEY_VALUE\n')
    f.write('#undef LOOPS\n')
    f.write('#undef NSCENAR\n')
    f.write('#undef NTHREADS\n');
    f.write('#undef RETURN_CODE\n')
    f.write('#undef STD_MAIN\n')
    f.write('#undef TEST\n')
    f.write('#undef THREAD_NUM\n')
    f.write('#undef TIMEOUT\n')
    f.write('#undef UNRESOLVED\n')
    f.write('#undef UNRESOLVED_KILLALL\n')
    f.write('#undef UNTESTED\n')
    # Redefine the test's main to an argumentless version.
    f.write('#undef main\n')
    f.write('#define main(...) main()\n')
    # This namespace is so that static global variables in different tests
    # will not clash.
    f.write('namespace %s {\n' % self.ns_name)
    f.write('#include "%s"\n' % self.path)
    f.write('}\n\n')

  def output_entry(self, f):
    f.write('  { %s::main, "%s" },\n' % (self.ns_name, self.name))

  @staticmethod
  def output(f, all_tests):
    for t in all_tests:
      t.output_code(f)
    f.write('#undef main\n\n')
    f.write('static test_t all_tests[] = {\n')
    for t in all_tests:
      t.output_entry(f)
    f.write('  { NULL, NULL } // list terminator\n')
    f.write('};\n')


allowed_pthread_functions = set([
  'pthread_create',
  'pthread_detach',
  'pthread_equal',
  'pthread_exit',
  'pthread_join',
  'pthread_self',

  'pthread_attr_destroy',
  'pthread_attr_init',
  'pthread_attr_getdetachstate',
  'pthread_attr_getstacksize',
  'pthread_attr_setdetachstate',
  'pthread_attr_setstacksize',

  'pthread_mutex_destroy',
  'pthread_mutex_init',
  'pthread_mutex_lock',
  'pthread_mutex_trylock',
  'pthread_mutex_unlock',

  'pthread_mutexattr_destroy',
  'pthread_mutexattr_init',
  'pthread_mutexattr_gettype',
  'pthread_mutexattr_settype',

  'pthread_cond_broadcast',
  'pthread_cond_destroy',
  'pthread_cond_init',
  'pthread_cond_signal',
  'pthread_cond_timedwait',
  'pthread_cond_wait',

  'pthread_condattr_destroy',
  'pthread_condattr_init',

  'pthread_once',

  'pthread_key_create',
  'pthread_key_delete',
  'pthread_getspecific',
  'pthread_setaffinity_np',
  'pthread_setname_np',
  'pthread_setschedparam',
  'pthread_setspecific',

  'sched_yield',
])

# Match atexit, rlimit, fork, pthread_*, and sched_*.
function_calls = re.compile(r'((?:atexit|rlimit|fork|(?:pthread|sched)_[a-zA-Z_]*)) *\(')

def is_test_okay(path):
  contents = open(path, 'r').read()

  # Test that the primary function the test is meant for (2nd to last path
  # component).  This eliminates some tests for things like sched_setscheduler
  # which have no actual function calls in them and just return "unsupported".
  # If it's not supported by the test suite itself, we are not interested in
  # having those tests contribute to our numbers.
  function = path.split('/')[-2]
  if function not in allowed_pthread_functions:
    return False

  # Tests which use PTS_EXCLUDE should not be run.
  if "PTS_EXCLUDE" in contents:
    return False

  # Finally, filter out tests that rely on things we don't have.
  for function in function_calls.findall(contents):
    if function not in allowed_pthread_functions:
      #print "'%s' rejected due to '%s'" % (path, function)
      return False

  return True

def is_path_okay(path, exclusions):
  for substring in exclusions:
    if substring in path:
      return False
  return True

def find_tests(test_globs, exclusions):
  potential_tests = []

  for pattern in test_globs:
    potential_tests.extend(glob.glob(pattern))

  all_tests = []
  for test_path in potential_tests:
    if is_path_okay(test_path, exclusions) and is_test_okay(test_path):
      all_tests.append(test_case(test_path))

  return all_tests

def main(output_path):
  test_globs = [
    'conformance/interfaces/pthread_*/*.c',
    'conformance/interfaces/sched_*/*.c',
  ]
  exclusions = [
    'testfrmw.c',
  ]
  all_tests = find_tests(test_globs, exclusions)
  test_case.output(open(output_path, 'w'), all_tests)
  return 0

if __name__ == '__main__':
  output_path = sys.argv[1]
  sys.exit(main(output_path))

