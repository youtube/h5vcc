# suffix for project files
project_suffix = 'gyp_jsc'
# special gypis for JSC tests
gyp_includes = [
  '../../../external/chromium/build/common.gypi',
  'jsc_tests_vars.gypi',
]
# special variables for JSC tests
gyp_vars = {
  'lb_shell_no_chrome': 1,
}
# the homedir for the application at runtime
home_dir = '../../..'
