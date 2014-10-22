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
"""Utilites for use by gyp_driver and other build tools."""

import hashlib
import os
import re
import shutil
import subprocess
import sys


if sys.platform in ['win32', 'cygwin']:
  try:
    import cygpath  # pylint: disable=g-import-not-at-top,unused-import
  except ImportError:
    sys.stderr.write('cygpath module not found.\n'
                     'Please see lbshell/build/pylib/README.\n')
    sys.exit(1)


def Which(filename):
  for path in os.environ['PATH'].split(os.pathsep):
    full_name = os.path.join(path, filename)
    if os.path.exists(full_name) and os.path.isfile(full_name):
      return full_name
  return None


def CheckNinja():
  """Verify our custom ninja is installed."""

  if sys.platform in ['win32', 'cygwin']:
    exe_name = 'ninja.exe'
    source_exe = 'ninja-win.exe'
  else:
    exe_name = 'ninja'
    source_exe = 'ninja-linux'

  ninja_path = Which(exe_name)
  if not ninja_path:
    sys.stderr.write('ninja not found. '
                     'Make sure depot_tools is in your PATH\n')
    raise Exception('ninja not found')

  # Install our custom ninja into wherever ninja was found.
  source_path = os.path.join(os.path.dirname(__file__), 'ninja', source_exe)
  dest_path = os.path.join(os.path.dirname(ninja_path), exe_name)
  source_hash = hashlib.md5(open(source_path, 'rb').read()).digest()
  dest_hash = hashlib.md5(open(dest_path, 'rb').read()).digest()

  if source_hash != dest_hash:
    sys.stdout.write('Installing %s -> %s\n' % (source_path, dest_path))
    shutil.copy2(source_path, dest_path)


def GitHash():
  """Compute latest lbshell git hash."""

  # This will be part of the version info.
  cmd_line = [
      'repo forall lbshell -c \' git log --pretty=format:%h -n 1\' 2>/dev/null'
  ]
  try:
    sp = subprocess.check_output(cmd_line, shell=True)
    gh = sp.split()[0].rstrip()
  except subprocess.CalledProcessError:
    # Repo not installed?
    gh = '0'
  return gh


def _EnsureGomaRunning():
  """Ensure goma is running."""

  cmd_line = ['goma_ctl.py', 'ensure_start']

  try:
    subprocess.check_output(cmd_line, stderr=subprocess.STDOUT)
  except subprocess.CalledProcessError as e:
    sys.stderr.write('Error: Goma failed to start.\n%s:\n%s\n' %
                     (e.cmd, e.output))
    sys.exit(1)


def ShouldUseGoma(use_goma_default):
  """Query if we should use goma for the build."""

  if sys.platform in ['win32', 'cygwin']:
    return False

  # See if we can find gomacc somewhere the path.
  gomacc_path = Which('gomacc')

  if 'USE_GOMA' in os.environ:
    use_goma = int(os.environ['USE_GOMA'])
    if use_goma == 1 and not gomacc_path:
      sys.stderr.write('goma was requested but gomacc not found in PATH.\n')
      sys.exit(1)
  else:
    use_goma = (gomacc_path and use_goma_default)

  if use_goma:
    _EnsureGomaRunning()
  return use_goma


def GetConstantValue(file_path, constant_name):
  """Return an rvalue from a C++ header file.

  Search a C/C++ header file at |file_path| for an assignment to the
  constant named |constant_name|. The rvalue for the assignment must be a
  literal constant or an expression of literal constants.

  Args:
    file_path: Header file to search.
    constant_name: Name of C++ rvalue to evaluate.

  Returns:
    constant_name as an evaluated expression.
  """

  search_re = re.compile(r'%s\s*=\s*([\d\+\-*/\s\(\)]*);' % constant_name)
  with open(file_path, 'r') as f:
    match = search_re.search(f.read())

  if not match:
    sys.stderr.write('Could not query constant value.  The expression '
                     'should only have numbers, operators, spaces, and '
                     'parens.  Please check "%s" in %s.\n\n'
                     % (constant_name, file_path))
    sys.exit(1)

  expression = match.group(1)
  value = eval(expression)
  return value


def GetLBShellStackSizeConstant(platform, constant_name):
  """Gets a constant value from lb_shell_constants.h."""

  value = GetConstantValue(
      '../../src/platform/'+platform+'/lb_shell/lb_shell_constants.h',
      constant_name)

  # Make sure value is reasonable
  if not(value >= 1024 and value < 1024*1024*256):
    print >> sys.stderr, '%d is an unreasonable stack size.' % value
    sys.exit(1)

  return value
