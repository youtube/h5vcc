# suffix for project files
project_suffix = 'gyp_lb'
# special gypis for lb_shell
gyp_includes = [
  '../../../external/chromium/build/common.gypi',
  'lb_shell_vars.gypi',
]
# special variables for lb_shell
gyp_vars = {
  'lb_shell_no_chrome': 0,
}
# the homedir for the application at runtime
home_dir = '../../../content'
