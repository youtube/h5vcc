{
  'targets': [
    {
      'target_name': 'pthread_test_list',
      'type': 'none',
      'actions': [
        {
          'action_name': 'generate_pthread_test_list',
          'variables': {
            'generator_py': 'generate_test_list.py',
          },
          'inputs': [
            '<!@(find conformance/interfaces/ -type f)',
            '<(generator_py)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/generated_pthread_test_list.cc',
          ],
          'action': [
            'python',
            '<(generator_py)',
            '<@(_outputs)',
          ],
          'message': 'Generating pthread test list',
          'msvs_cygwin_shell': 1,
        },
      ],
    },
  ],
}
