import os
import subprocess
import sys
import find_executable
from layout_exceptions import *

sys.path.insert(0, os.path.join('..', '..', 'tools'))
import linux_launcher

exe_name = 'lb_layout_tests'
exe_base_directory = '../../out/lb_shell'
test_work_dir = 'test_work_dir'
exe_working_directory = '../..'

default_build_order = [
  'Linux_Devel',
  'Linux_Debug'
]

def GetTestRunnerLauncher(tests, use_build):
  """ Initializes a platform specific test runner launcher and returns it. """

  test_exe = find_executable.FindTestExecutable(
      default_build_order,
      use_build,
      exe_name,
      exe_base_directory)

  print 'Found and using test executable "'+ test_exe + '".'

  launcher = linux_launcher.LinuxLauncher(test_exe)
  launcher.SetArgs([test_work_dir] + tests)
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

