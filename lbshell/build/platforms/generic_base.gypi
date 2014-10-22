{
  # we want to make sure that any headers we include from WebKit/chromium
  # have the same USE flags as what they are compiled under.
  'includes' : [
    '../../../external/chromium/third_party/WebKit/Source/WebKit/chromium/features.gypi',
  ],
  'target_defaults' : {
    'variables' : {
      'main_thread_stack_size' : 0,
    },
    'configurations' : {
      'base'  : {
        'abstract' : 1,
        'cflags' : [ '<@(compiler_flags)' ],
        'ldflags' : [ '<@(linker_flags)' ],
        'cflags_host' : [
          '<@(compiler_flags_host)',
          '-D__LB_HOST__',  # since there is no defines_host
        ],
        'ldflags_host' : [ '<@(linker_flags_host)' ],
        'defines' : [
          '<@(feature_defines)',
          '__LB_ENABLE_NATIVE_HTTP_STACK__=<(use_native_http_stack)',
          '__LB_ENABLE_WEB_SPEECH_API__=<(use_web_speech_api)',
        ],
        'conditions': [
          ['skia_gpu == 0', {
            'defines': [
              '__LB_DISABLE_SKIA_GPU__=1',
            ],
          }],
          ['posix_emulation_target_type == "shared_library"', {
            'defines': [
              '__LB_BASE_SHARED__=1',
            ],
          }],
        ],
      },
      'debug_base' : {
        'abstract' : 1,
        'inherit_from' : [ 'base' ],
        # no optimization, include symbols:
        'cflags' : [ '<@(compiler_flags_debug)' ],
        'ldflags' : [ '<@(linker_flags_debug)' ],
        'defines': [
          '_DEBUG',
          'LB_SHELL_BUILD_TYPE_DEBUG',
        ],
      }, # end of debug_base
      'devel_base' : {
        'abstract' : 1,
        'inherit_from' : [ 'base' ],
        # optimize, include symbols:
        'cflags' : [ '<@(compiler_flags_devel)' ],
        'ldflags' : [ '<@(linker_flags_devel)' ],
        'defines': [
          'NDEBUG',
          'LB_SHELL_BUILD_TYPE_DEVEL',
        ],
      }, # end of devel_base
      'qa_base' : {
        'abstract' : 1,
        'inherit_from' : [ 'base' ],
        # optimize:
        'cflags' : [ '<@(compiler_flags_qa)' ],
        'ldflags' : [ '<@(linker_flags_qa)' ],
        'defines': [
          'NDEBUG',
          'LB_SHELL_BUILD_TYPE_QA',
        ],
      }, # end of devel_base
      'gold_base' : {
        'abstract' : 1,
        'inherit_from' : [ 'base' ],
        # optimize:
        'cflags' : [ '<@(compiler_flags_gold)' ],
        'ldflags' : [ '<@(linker_flags_gold)' ],
        'defines' : [
          'NDEBUG',
          'LB_SHELL_BUILD_TYPE_GOLD',
        ],
      }, # end of gold_base
    }, # end of configurations
  }, # end of target_defaults
}
