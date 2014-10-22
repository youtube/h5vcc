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
"""Linux-specific settings for gyp_driver."""

import os
import re
import subprocess
import sys

import gyp_driver_utils


def GetClangVersion(clang_path):
  """Parse clang version string into (major, minor)."""

  version_re = re.compile(r'clang version (\d+)\.(\d+)')
  output = subprocess.check_output([clang_path, '--version'])
  match = version_re.search(output)
  if match:
    major = int(match.group(1))
    minor = int(match.group(2))
  else:
    print >> sys.stderr, output
    raise Exception('unexpected version string')

  return major, minor


def CheckClangVersion():
  """Verify the expected version of clang is in the path."""

  if not gyp_driver_utils.Which('clang'):
    sys.stderr.write('clang not found in PATH.\n')
    raise Exception('clang not found')

  # Check that the right version of clang is in the path.
  chromium_path = os.path.join('..', '..', '..', 'external', 'chromium')
  update_script = os.path.join(chromium_path,
                               'tools', 'clang', 'scripts', 'update.sh')
  clang_bin_path = os.path.join(
      chromium_path,
      'third_party',
      'llvm-build',
      'Release+Asserts',
      'bin')
  clang_bin = os.path.join(clang_bin_path, 'clang')
  if not os.path.exists(clang_bin):
    print >> sys.stderr, 'clang not found, updating'
    subprocess.check_call(update_script)

  if not os.path.exists(clang_bin):
    raise Exception('clang not found')

  req_major, req_minor = GetClangVersion(clang_bin)
  major, minor = GetClangVersion('clang')

  if major < req_major or (major == req_major and minor < req_minor):
    msg = '\nclang version %d.%d or higher required' % (req_major, req_minor)
    msg += ' (%d.%d found).\n' % (major, minor)
    msg += 'Please ensure ' + clang_bin_path + ' is in the PATH.'
    print >> sys.stderr, msg
    raise Exception('clang version')

# environment settings for gyp for the linux platform
env_settings = {
    'CC': 'clang',
    'CXX': 'clang++',

    'CC_host': 'clang',
    'CXX_host': 'clang++',
    'LD_host': 'clang++',
    'ARFLAGS_host': 'rcs',
    'ARTHINFLAGS_host': 'rcsT',
}

# Do some goma checks. On Linux we will automatically use it
# if it's in the path.
gyp_driver_utils.ShouldUseGoma(True)
CheckClangVersion()

supported_formats = ['ninja']
# so that gyp can customize its output
flavor = 'linux'
# special gypis for linux builds
gyp_includes = [
    'linux_base.gypi',
]
# special variables for linux builds
gyp_vars = {
    'clang': 1,
    'use_widevine': 0,
    'LB_SHELL_MAIN_THREAD_STACK_SIZE':
        str(gyp_driver_utils.GetLBShellStackSizeConstant(
            'linux', 'kViewHostThreadStackSize')),
    'LB_UNIT_TEST_MAIN_THREAD_STACK_SIZE':
        str(gyp_driver_utils.GetLBShellStackSizeConstant(
            'linux', 'kUnitTestMainThreadStackSize')),
}
