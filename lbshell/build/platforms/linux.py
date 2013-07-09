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

import os
import os.path
import re
import subprocess
import sys

def GetClangVersion(clang_path):
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
  # Check that the right version of clang is in the path.
  chromium_path = os.path.join('..', '..', '..', 'external', 'chromium')
  update_script = os.path.join(chromium_path,
    'tools', 'clang', 'scripts', 'update.sh')
  clang_bin_path = os.path.join(chromium_path,
    'third_party', 'llvm-build', 'Release+Asserts', 'bin')
  clang_bin = os.path.join(clang_bin_path, 'clang')
  if not os.path.exists(clang_bin):
    print >> sys.stderr, 'clang not found, updating'
    subprocess.check_call(update_script)

  if not os.path.exists(clang_bin):
    raise Exception('clang not found')

  req_major, req_minor = GetClangVersion(clang_bin)
  major, minor = GetClangVersion('clang')

  if major < req_major or (major == req_major and minor < req_minor):
    str = '\nclang version %d.%d or higher required' % (req_major, req_minor)
    str += ' (%d.%d found).\n' % (major, minor)
    str += 'Please ensure ' + clang_bin_path + ' is in the PATH.'
    print >> sys.stderr, str
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

CheckClangVersion()

supported_formats = [ 'ninja' ]
# so that gyp can customize its output
flavor = 'linux'
# special gypis for linux builds
gyp_includes = [
  'linux_base.gypi',
]
# special variables for linux builds
gyp_vars = {
  'clang' : 1,
  'use_widevine' : 0,
}
