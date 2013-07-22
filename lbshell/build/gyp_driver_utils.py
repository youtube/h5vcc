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

import os
import subprocess
import sys


if sys.platform == 'win32' or sys.platform == 'cygwin':
  try:
    import cygpath
  except ImportError as ie:
    sys.stderr.write('cygpath module not found.\n'
                     'Please see lbshell/build/pylib/README.\n')
    sys.exit(1)


def which(file):
  for path in os.environ['PATH'].split(os.pathsep):
    full_name = os.path.join(path, file)
    if os.path.exists(full_name):
      return full_name
  return None


def check_ninja():
  if sys.platform in ['win32', 'cygwin']:
    exe_name = 'ninja.exe'
    source_exe = 'ninja-win.exe'
  else:
    exe_name = 'ninja'
    source_exe = 'ninja-linux'

  ninja_path = which(exe_name)
  if not ninja_path:
    sys.stderr.write('ninja not found in PATH.\n')
    raise Exception('ninja not found')

  source_path = os.path.join(os.path.dirname(__file__), 'ninja', source_exe)
  (found_version, found_version_val) = compute_version(ninja_path)
  (expected_version, expected_version_val) = compute_version(source_path)

  if (found_version_val < expected_version_val or
      found_version.find('steel') == -1):
    sys.stderr.write(
        '\n%s version is %s. Expected %s.\n'
        'Please replace with %s\n' %
      (ninja_path, found_version, expected_version, source_path))
    sys.exit(1)


def git_hash():
  # Compute latest lbshell git hash.
  # This will be part of the version info.
  cmd_line = [
      'repo forall lbshell -c \' git log --pretty=format:%h -n 1\' 2>/dev/null'
  ]
  try:
    sp = subprocess.check_output(cmd_line, shell=True)
    git_hash = sp.split()[0].rstrip()
  except subprocess.CalledProcessError:
    # Repo not installed?
    git_hash = '0'
  return git_hash

