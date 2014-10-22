"""Linux-specific settings for layout test runner."""

import os
import sys

import find_executable

sys.path.insert(0, os.path.join('..', '..', 'tools'))
import linux_launcher  # pylint: disable=g-import-not-at-top,g-bad-import-order

exe_name = 'lb_layout_tests'
exe_base_directory = '../../out/lb_shell'
test_work_dir = 'test_work_dir'
exe_working_directory = '../..'

default_build_order = [
    'Linux_Devel',
    'Linux_Debug'
]


def GetTestRunnerLauncher(tests, use_build):
  """Initializes a platform specific test runner launcher and returns it."""

  test_exe = find_executable.FindTestExecutable(
      default_build_order,
      use_build,
      exe_name,
      exe_base_directory)

  print 'Found and using test executable "' + test_exe + '".'

  launcher = linux_launcher.LinuxLauncher(test_exe)
  launcher.SetArgs([test_work_dir] + tests)
  return launcher


def GetWorkDirectory(use_build):
  """Return the working directory.

  Return the directory hat the platform will look for input files and
  place its output files within.

  Args:
    use_build: Use the executable from the build directory.

  Returns:
    Path to the working directory for the layout test runner.
  """
  content_dir = os.path.dirname(find_executable.FindTestExecutable(
      default_build_order,
      use_build,
      exe_name,
      exe_base_directory))
  return os.path.join(content_dir, 'content/local', test_work_dir)
