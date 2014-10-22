{
  'variables': {
    # Enable Widevine by default for any build which has the source.
    'use_widevine%' : '<!(test -d ../../../external/cdm && echo 1 || echo 0)',
    'posix_emulation_target_type%' : 'static_library',
  },
  'target_defaults': {
    'defines' : [
      '__LB_SHELL__',
      '__LB_SHELL_USE_WIDEVINE__=<(use_widevine)',
    ],
    'conditions' : [
      ['js_engine == "jsc"', {
        'defines' : [
          '__LB_SHELL_USE_JSC__',
        ],
      }],
      ['js_engine == "v8"', {
        'defines' : [
          '__LB_SHELL_USE_V8__',
        ],
      }],
    ],
    # These are mix-in defines for various configurations.
    'configurations' : {
      'debug_base' : {
        'defines': [
          '__LB_SHELL__FORCE_LOGGING__',
          '__LB_SHELL__ENABLE_CONSOLE__',
          '__LB_SHELL__ENABLE_SCREENSHOT__',
        ],
      },
      'devel_base' : {
        'defines': [
          '__LB_SHELL__FORCE_LOGGING__',
          '__LB_SHELL__ENABLE_CONSOLE__',
          '__LB_SHELL__ENABLE_SCREENSHOT__',
        ],
      },
      'qa_base' : {
        'defines': [
          '__LB_SHELL__FOR_QA__',
          '__LB_SHELL__ENABLE_CONSOLE__',
          '__LB_SHELL__ENABLE_SCREENSHOT__',
        ],
      },
      'gold_base' : {
        'defines' : [
          '__LB_SHELL__FOR_RELEASE__',
        ],
      },
    }, # end of configurations
  }, # end of target_defaults
}
