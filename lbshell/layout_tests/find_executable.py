import os
from layout_exceptions import *

build_target_suggestion_message = \
  'Did you forget to build the ninja target "lb_layout_tests" first?'

def FindTestExecutable(default_build_order, use_build, exe_name, exe_base_dir):
  def PathToExe(build):
    return os.path.join(exe_base_dir, build, exe_name)

  if use_build:
    path_to_exe = PathToExe(use_build)
    # If provided, search for and use the specified executable
    if os.path.exists(path_to_exe):
      return os.path.expanduser(path_to_exe)
    else:
      raise TestClientError('Unable to find layout test executable\n' \
        + path_to_exe + '\n' + build_target_suggestion_message)
  else:
    # Search for the layout test executable in the project 'out' directory
    for build in default_build_order:
      path_to_exe = PathToExe(build)
      if os.path.exists(path_to_exe):
        return os.path.expanduser(path_to_exe)

    raise TestClientError('Unable to find layout test executable in ' \
      + 'base directory\n' \
      + '"' + exe_base_dir + '"\n' \
      + 'after searching through sub-directories ' + str(default_build_order) \
      + '.\n' + build_target_suggestion_message)

