{
  'includes' : [
    'generic_base.gypi',
  ],
  'variables' : {
    'target_arch' : 'linux',
    'use_native_http_stack' : 0,
    'compiler_flags_host' : [
      '-O2',
    ],
    'linker_flags_host' : [
    ],
    'image_decoder_defines': [
      # Choose native decoders to use
      '__LB_ENABLE_SHELL_NATIVE_JPEG_DECODER__=0',
      '__LB_ENABLE_SHELL_NATIVE_PNG_DECODER__=0',
    ],
    'compiler_flags' : [
      # The Steel build doesn't actually want Linux,
      # we will implement all our own wrappers.
      '-U__linux__',
      '-include', 'posix_emulation.h',
    ],
    'linker_flags' : [
      # In order to wrap and override the system allocator.
      '-Wl,--wrap=malloc',
      '-Wl,--wrap=calloc',
      '-Wl,--wrap=realloc',
      '-Wl,--wrap=memalign',
      '-Wl,--wrap=reallocalign',
      '-Wl,--wrap=free',
      '-Wl,--wrap=strdup',
      '-Wl,--wrap=malloc_usable_size',
      '-Wl,--wrap=malloc_stats_fast',
      # These symbols try to allocate memory under the hood:
      '-Wl,--wrap=backtrace_symbols',
      '-Wl,--wrap=__cxa_demangle',
      # To get symbols via backtrace_symbols:
      '-rdynamic',
    ],

    'compiler_flags_debug' : [
      '-O0', '-g',
    ],
    'linker_flags_debug' : [
    ],

    'compiler_flags_devel' : [
      '-O2', '-g',
    ],
    'linker_flags_devel' : [
    ],

    'compiler_flags_qa' : [
      '-O2',
    ],
    'linker_flags_qa' : [
    ],

    'compiler_flags_gold' : [
      '-O2',
    ],
    'linker_flags_gold' : [
    ],

    'platform_libraries': [
      '-ldl',
      '-lGL',
      '-lpthread',
      '-lrt',
      '-lX11',
      '-lXpm',
    ],
    'platform_dependencies': [
    ],
    'platform_contents_lbshell': [
      '../platforms/linux_contents.gyp:copy_contents_common',
      '../platforms/linux_contents.gyp:copy_contents_lbshell',
      '../platforms/linux_contents.gyp:copy_unit_test_data',
    ],
    'platform_contents_jsc_tests': [
      '../platforms/linux_contents.gyp:copy_contents_common',
      '../platforms/linux_contents.gyp:copy_jsc_test_scripts',
    ],
  },
  'conditions' : [
    ['clang==1', {
      'variables' : {
        'compiler_flags' : [
          '-Werror',
          '-fno-exceptions',
          '-fno-strict-aliasing',  # See http://crbug.com/32204
          # Clang spots more unused functions.
          '-Wno-unused-function',
          # Don't die on dtoa code that uses a char as an array index.
          '-Wno-char-subscripts',
          # Especially needed for gtest macros using enum values from Mac
          # system headers.
          # TODO(pkasting): In C++11 this is legal, so this should be
          # removed when we change to that.  (This is also why we don't
          # bother fixing all these cases today.)
          '-Wno-unnamed-type-template-args',
          # This (rightyfully) complains about 'override', which we use
          # heavily.
          '-Wno-c++11-extensions',
          '-Wno-c++11-compat',
          # Warns on switches on enums that cover all enum values but
          # also contain a default: branch. Chrome is full of that.
          '-Wno-covered-switch-default',
          # Don't warn about unused function params.  We use those everywhere.
          '-Wno-unused-parameter',
          # Disable warnings about C linkage of
          # uspoof_getSkeletonUnicodeString().
          '-Wno-return-type-c-linkage',

          # We use some statics that Chrome doesn't.
          '-Wno-exit-time-destructors',
        ],
      },
    }],
  ],
  'target_defaults' : {
    'defines' : [
      '__LB_LINUX__',
      '__STDC_FORMAT_MACROS', # so that we get PRI*
      # Disable macros that replace string functions.
      # This interferes with our wrapping of those symbols.
      '__NO_STRING_INLINES',
      # Enable GNU extensions to get prototypes like ffsl.
      '_GNU_SOURCE=1',
      # Enable useful macros from stdint.h
      '__STDC_LIMIT_MACROS',
      '__STDC_CONSTANT_MACROS',
      '__LB_ENABLE_NATIVE_HTTP_STACK__=<(use_native_http_stack)',
    ],

    'include_dirs' : [
    ],
    'include_dirs_target' : [
      '../../../external/chromium/third_party/mesa/MesaLib/include',
      '../../src',
      '../../src/platform/linux',
      # headers that we don't need, but should exist somewhere in the path:
      '../../src/platform/linux/posix_emulation/place_holders',
    ],
    'include_dirs_host' : [
    ],

    'default_configuration' : 'Linux_Debug',
    'configurations' : {
      'Linux_Debug' : {
        'inherit_from' : ['debug_base'],
      },
      'Linux_Devel' : {
        'inherit_from' : ['devel_base'],
      },
      'Linux_QA' : {
        'inherit_from' : ['qa_base'],
      },
      'Linux_Gold' : {
        'inherit_from' : ['gold_base'],
      },
    }, # end of configurations
  }, # end of target_defaults
}
