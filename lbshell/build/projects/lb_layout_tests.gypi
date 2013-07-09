{
  'targets': [{
    'target_name': 'lb_layout_tests',
    'type': 'executable',
    'sources': [
      '../../src/lb_shell_main.cc',
      '../../src/lb_shell_layout_test_runner.cc',
      '../../src/lb_shell_layout_test_runner.h',
    ],
    'include_dirs': [
      '../../../', # for our explicit external/ inclusion of headers
      '../../../external/chromium', # for general inclusion of Chromium headers
      '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime', # to satisfy config.h deps
      '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore', # for including debugger
      '../../../external/chromium/third_party/WebKit/Source/WebCore/platform',
      '../../../external/chromium/third_party/WebKit/Source/WTF',
      '../../../external/chromium/third_party/WebKit/Source/WTF/wtf',
      '<(SHARED_INTERMEDIATE_DIR)',
    ],
    'defines' : [
      '__DISABLE_WTF_LOGGING__',
      '__LB_LAYOUT_TESTS__',
    ],
    'dependencies': [
      'lb_shell_lib',
      'steel_build_id',
      '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
    ],
    'libraries': [
      '<@(platform_libraries)',
    ],
  }],
}
