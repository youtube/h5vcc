{
  'variables': {
    'posix_emulation_target_type%' : 'static_library',
  },
  'target_defaults' : {
    'defines' : [
      '__LB_SHELL_NO_CHROME__',
      '__LB_SHELL__FOR_QA__',
      '__LB_SHELL__',
    ],
  },
}
