{
  'targets': [
    {
      'target_name': 'lb_shell_package',
      'type': 'none',
      'default_project': 1,
      'dependencies': [
        'lb_shell_contents',
      ],
      'conditions': [
        ['target_arch=="android"', {
          'dependencies': [
            'lb_shell_android.gyp:lb_shell_apk',
          ],
        }, {
          'dependencies': [
            'lb_shell_exe.gyp:lb_shell',
          ],
        }],
        ['target_arch not in ["linux", "android", "wiiu"]', {
          'dependencies': [
            '../platforms/<(target_arch)_deploy.gyp:lb_shell_deploy',
          ],
        }],
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
        '<@(platform_contents_unit_tests)',
      ],
      'conditions': [
        ['target_arch not in ["linux", "android", "wiiu"]', {
          'dependencies': [
            '../platforms/<(target_arch)_deploy.gyp:unit_tests_base_deploy',
            '../platforms/<(target_arch)_deploy.gyp:unit_tests_media_deploy',
            '../platforms/<(target_arch)_deploy.gyp:unit_tests_net_deploy',
            '../platforms/<(target_arch)_deploy.gyp:unit_tests_sql_deploy',
            '../platforms/<(target_arch)_deploy.gyp:lb_unit_tests_deploy',
          ],
        }],
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
  'conditions': [
    ['target_arch == "android"', {
      'targets': [
        {
          'target_name': 'lb_unit_tests_package_apks',
          'type': 'none',
          'dependencies': [
            'lb_shell_exe.gyp:unit_tests_base_apk',
            'lb_shell_exe.gyp:unit_tests_crypto_apk',
            'lb_shell_exe.gyp:unit_tests_media_apk',
            'lb_shell_exe.gyp:unit_tests_net_apk',
            'lb_shell_exe.gyp:unit_tests_sql_apk',
            'lb_shell_exe.gyp:lb_unit_tests_apk',
            'lb_shell_exe.gyp:unit_tests_image_decoder_apk',
            'lb_shell_exe.gyp:unit_tests_xml_parser_apk',
            'lb_shell_contents',
            '<@(platform_contents_unit_tests)',
          ],
        },
      ],
    }],
  ],
}
