{
  'targets': [
    {
      'target_name': 'pthread_tests',
      'type': 'executable',
      'sources': [
        '<(DEPTH)/../posixtestsuite/test_runner.cc',
      ],
      'include_dirs': [
        '<(DEPTH)/../posixtestsuite',
        '<(DEPTH)/../posixtestsuite/include',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'defines' : [
      ],
      'dependencies': [
        'lb_shell_lib', # memory debugging
        'posix_emulation.gyp:posix_emulation',
        '<(DEPTH)/../posixtestsuite/posixtestsuite.gypi:pthread_test_list',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
      'conditions': [
        ['target_arch=="xb1"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ComponentExtensions': 'true'
            },
          },
        }],
      ],
    },
  ] # end of targets
}
