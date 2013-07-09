{
  'targets': [
    {
      'target_name': 'lb_shell_package',
      'type': 'none',
      'default_project': 1,
      'dependencies': [
        'lb_shell_exe.gyp:lb_shell',
        'lb_shell_contents',
      ],
    },

    {
      'target_name': 'lb_layout_tests_package',
      'type': 'none',
      'dependencies': [
        'lb_shell_exe.gyp:lb_layout_tests',
        'lb_shell_contents',
      ],
    },
    {
      'target_name': 'lb_unit_tests_package',
      'type': 'none',
      'dependencies': [
        'lb_shell_exe.gyp:unit_tests_base',
        'lb_shell_exe.gyp:unit_tests_crypto',
        'lb_shell_exe.gyp:unit_tests_media',
        'lb_shell_exe.gyp:unit_tests_net',
        'lb_shell_exe.gyp:unit_tests_sql',
        'lb_shell_exe.gyp:lb_unit_tests',
        'lb_shell_exe.gyp:unit_tests_image_decoder',
        'lb_shell_exe.gyp:unit_tests_xml_parser',
        'lb_shell_contents',
      ],
    },

    {
      'target_name': 'lb_shell_contents',
      'type': 'none',
      'dependencies': [
        '<@(platform_contents_lbshell)',
      ],
    },
  ],
}
