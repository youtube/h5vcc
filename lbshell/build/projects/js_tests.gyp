{
  'targets': [
    {
      'target_name': 'js_tests',
      'type': 'none',
      'conditions': [
        ['js_engine=="jsc"', {
          'dependencies': [
            'jsc_tests.gyp:jsc_tests_package',
          ],
        }, {
          'dependencies': [
            'v8_tests.gyp:v8_tests_package',
          ],
        }],
      ],
    }
  ],
}
