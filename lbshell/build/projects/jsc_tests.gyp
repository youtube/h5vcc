{
  'targets': [
    {
      'target_name': 'jsc_tests',
      'product_name': 'js_tests',
      'type': 'executable',
      'variables': {
        'main_thread_stack_size': '<(LB_UNIT_TEST_MAIN_THREAD_STACK_SIZE)',
      },
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
        '../../src/platform/<(target_arch)/lb_shell/lb_shell_platform_delegate_<(target_arch).cc',
      ],
      'include_dirs': [
        '../../..', # for our explicit external/ inclusion of headers
        '../../src',
        '../../src/javascript_test_runner',
        "<!@(find ../../../external/chromium/third_party/WebKit/Source/JavaScriptCore -mindepth 1 -maxdepth 1 -type d -and -not -name 'JavaScriptCore.*' -and -not -name 'os-win32')",
        '../../../external/chromium',
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore',
        '../../../external/chromium/third_party/WebKit/Source/WebCore/platform',
        '../../../external/chromium/third_party/WebKit/Source/WTF',
        '../../../external/chromium/third_party/WebKit/Source/WTF/wtf',
        '../../../external/chromium/third_party/WebKit/Source/Platform/chromium',
      ],
      'dependencies': [
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',  # needed to pick up some include paths at the beginning of the -I string
        '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/JavaScriptCore.gyp/JavaScriptCore.gyp:javascriptcore',
        'posix_emulation.gyp:posix_emulation',
        '<@(platform_contents_js_tests)',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
      'conditions': [
        ['target_arch == "ps3"', {
          'sources': [
            '../../src/platform/ps3/lb_shell/lb_spurs.cc',
          ],
        }],
      ],
      'defines': [
        'LB_SHELL_IMPLEMENTATION=1',
        '__DISABLE_WTF_LOGGING__',
      ],
      # Don't complain about calling specific versions of templatized
      # functions (e.g. in RefPtrHashMap.h).
      'msvs_disabled_warnings': [4344],
    }, # end of target 'jsc_tests'
    {
      'target_name': 'jsc_tests_package',
      'type': 'none',
      'default_project': 1,
      'dependencies': [
        'jsc_tests',
      ],
      'conditions': [
        ['target_arch not in ["linux", "android", "wiiu"]', {
          'dependencies': [
            '../platforms/<(target_arch)_deploy.gyp:js_tests_deploy',
          ],
        }],
      ],
    },
  ],
}
