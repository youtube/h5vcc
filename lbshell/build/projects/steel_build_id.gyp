{
  'targets': [
    {
      'target_name': 'steel_build_id',
      'type': 'none',
      # Because we generate a header, we must set the hard_dependency flag.
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'steel_build_id',
          'variables': {
            'repo_path': '.',
            'build_id_py_path': '../build_id.py',
            'output_path': '<(SHARED_INTERMEDIATE_DIR)/steel_build_id.h',
          },
          'inputs': [
            '<(build_id_py_path)',
          ],
          'outputs': [
            '<(output_path)',
          ],
          'action': [
            '<(build_id_py_path)',
            '<(repo_path)',
            '<(output_path)',
            '<(lb_shell_sha1)',
          ],
          'message': 'Generating build information',
          'msvs_cygwin_shell': 1
        },
      ],
    }
  ] # end of targets
}
