{
  'includes': [
    'posix_emulation.gypi',
    'wtf_symbols.gypi',
   ],
  'targets': [
    {
      'target_name': 'jsc_tests',
      'type': 'executable',
      'default_project': 1,
      'sources': [
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore/jsc.cpp',
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore/testRegExp.cpp',
        '../../src/javascript_test_runner/jsc_test_case.cc',
        '../../src/javascript_test_runner/jsc_test_case.h',
        '../../src/javascript_test_runner/test262_test_case.cc',
        '../../src/javascript_test_runner/test262_test_case.h',
        '../../src/javascript_test_runner/test_case.cc',
        '../../src/javascript_test_runner/test_case.h',
        '../../src/javascript_test_runner/test_runner_main.cc',
        '../../src/javascript_test_runner/util.cc',
        '../../src/javascript_test_runner/util.h',
        '../../src/javascript_test_runner/v8_test_case.cc',
        '../../src/javascript_test_runner/v8_test_case.h',
        '../../src/lb_memory_debug.c',
        '../../src/lb_memory_internal.h',
        '../../src/lb_memory_manager.h',
        '../../src/lb_memory_manager.cc',
        '../../src/oom_png.c',
        '../../src/oom_png.h',
        '../../src/platform/<(target_arch)/lb_shell/lb_memory_debug_<(target_arch).c',
        '../../src/platform/<(target_arch)/lb_shell/lb_memory_pages_<(target_arch).c',
        '../../src/platform/<(target_arch)/lb_shell/lb_shell_platform_delegate_<(target_arch).cc',
        '../../src/platform/<(target_arch)/lb_shell/lb_stack.cc',
        '../../src/platform/<(target_arch)/test_stub/stubs.cc',
      ],
      'include_dirs': [
        '../../..', # for our explicit external/ inclusion of headers
        '../../src',
        '../../src/javascript_test_runner',
        "<!@(find ../../../external/chromium/third_party/WebKit/Source/JavaScriptCore -mindepth 1 -maxdepth 1 -type d -and -not -name 'JavaScriptCore.*' -and -not -name 'os-win32')",
        '../../../external/chromium',
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore',
        '../../../external/chromium/third_party/WebKit/Source/WTF',
      ],
      'dependencies': [
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',  # needed to pick up some include paths at the beginning of the -I string
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/JavaScriptCore.gyp/JavaScriptCore.gyp:javascriptcore',
        'posix_emulation',
        'wtf_symbols',
        '<@(platform_contents_jsc_tests)',
        '<(DEPTH)/../dlmalloc/dlmalloc.gyp:dlmalloc',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
      'conditions': [
        ['target_arch == "ps3"', {
          'sources': [
            '../../src/platform/ps3/lb_shell/lb_mixer_ps3.cc',
            '../../src/platform/ps3/lb_shell/lb_spurs.cc',
          ],
        }],
      ]
    }, # end of target 'jsc_tests'
  ],
}
