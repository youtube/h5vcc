import os
import subprocess
import sys
import platform
import cygpath
import threading
import find_executable
from layout_exceptions import *

sys.path.insert(0, os.path.join('..', '..', 'tools'))
import wiiu_launcher

exe_name = 'lb_layout_tests.rpx'
exe_base_directory = '../../out/lb_shell'
test_work_dir = 'test_work_dir'

default_build_order = [
  'WIIU_Devel',
  'WIIU_Debug'
]

def GetTestRunnerLauncher(tests, use_build):
  """ Initializes a platform specific test runner launcher and returns it. """

  test_exe = find_executable.FindTestExecutable(
      default_build_order,
      use_build,
      exe_name,
      exe_base_directory)
  content_dir = os.path.dirname(test_exe)

  print 'Found and using test executable "'+ test_exe + '".'

  test_args = [x.replace('\\', '/') for x in tests]

  content_dir = wiiu_launcher.GetCygwinAbsolutePath(
                    os.path.abspath(os.path.join(os.path.dirname(test_exe),
                                                 'content')))
  launcher = wiiu_launcher.WIIULauncher(test_exe)
  launcher.SetArgs([test_work_dir] + test_args)
  return launcher

def GetWorkDirectory(use_build):
  """ Return the directory that the platform will look for input files and
  place its output files within."""
  content_dir = os.path.dirname(find_executable.FindTestExecutable(
      default_build_order,
      use_build,
      exe_name,
      exe_base_directory))
  return os.path.join(content_dir, 'content/local', test_work_dir)
