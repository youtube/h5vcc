{
  'variables': {
    'lb_shell_android_src': '../../src/platform/android',
    'lb_shell_lib': 'lib/<(SHARED_LIB_PREFIX)lb_shell<(SHARED_LIB_SUFFIX)',
    # The following trick allows us to construct a complete path for the
    # content folder since <(PRODUCT_DIR) returns a relative path and this
    # doesn't work with the 'aapt' rule for some reason
    'lb_shell_assets': '<!(pwd -P)/<(PRODUCT_DIR)/content',
  },
  'targets': [
    {
      'target_name': 'lb_shell_apk',
      'type': 'none',
      'dependencies': [
        'lb_shell_exe.gyp:lb_shell',
      ],
      'variables': {
        'additional_input_paths': ['<(PRODUCT_DIR)/content/lb_shell.pak'],
        'apk_name': 'YouTubeTV',
        'asset_location': '<(lb_shell_assets)',
        'java_in_dir': '<(lb_shell_android_src)/lb_shell/java',
        'native_libs_paths': ['<(PRODUCT_DIR)/<(lb_shell_lib)'],
        'package_name': 'lb_shell',
        'resource_dir': 'res',
      },
      'includes': [ '../../../external/chromium/build/java_apk.gypi' ],
    },
  ], # end of targets
}
