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
  ninja_path = which('ninja')
  if not ninja_path:
    print >> sys.stderr, \
      'ninja not found in PATH.'
    raise Exception('ninja not found')

  proc = subprocess.Popen([ninja_path, '--version'], stdout=subprocess.PIPE)
  out, err = proc.communicate()
  found_version = out.rstrip()
  expected_version = '1.1.0.steel'
  expected_path = os.path.join('lbshell', 'build', 'ninja', 'ninja.exe')
  if found_version != expected_version:
    print >> sys.stderr, \
      '\n%s version is %s.\nWin32 builds must use version %s (%s)\n' % \
      (ninja_path, found_version, expected_version, expected_path)
    raise Exception('ninja version error')


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

