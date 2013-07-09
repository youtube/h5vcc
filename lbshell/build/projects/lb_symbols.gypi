{
  'targets': [
    { 'target_name': 'lb_inspector_resources',
      'type': 'none',
      'variables': {
        'script_path': '../generate_data_header.py',
        'input_path': '<(PRODUCT_DIR)/resources/inspector',
        'output_path': '<(SHARED_INTERMEDIATE_DIR)/inspector/lb_inspector_resources.h',
      },
      'dependencies': [
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:inspector_resources',
      ],
      'actions': [
        {
          # Concatenate all of the inspector resources into a single file
          'action_name': 'lb_embed_inspector_resources',
          'inputs': [
            '<(script_path)',
          ],
          'outputs': [
            '<(output_path)',
          ],
          'action': ['python', '<(script_path)', 'LBInspectorResource', '<(input_path)', '<(output_path)'],
          'message': 'Concatenating resources from <(input_path) to <(output_path)',
          'msvs_cygwin_shell': 1,
        },
      ],
    },
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
    },
    {
      'target_name': 'lb_symbols',
      'type': 'static_library',
      # One of our dependencies is lb_inspector_resources, which generates a file
      # needed by lb_shell_lib.
      # Force lb_shell_lib to wait until this target is ready.
      'hard_dependency': 1,
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '../../src',
        '../../../external/chromium',
        '../../../external/chromium/third_party/icu/public/common', # to build JSC & WTF things
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime', # to satisfy config.h deps
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore', # for including debugger
        '../../../external/chromium/third_party/WebKit/Source/Platform/chromium', # to satisfy WebViewClient.h deps
        '../../../external/chromium/third_party/WebKit/Source/WebCore/platform', # for keyboard codes
        '../../../external/chromium/third_party/WebKit/Source/WTF',
        '../../../external/chromium/third_party/WebKit/Source/WTF/wtf',
        '<(SHARED_INTERMEDIATE_DIR)/lb_shell',
        '<(SHARED_INTERMEDIATE_DIR)/inspector',
        '<(SHARED_INTERMEDIATE_DIR)/shaders',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'dependencies': [
        'chrome_symbols',
        'lb_shell_resources',
        'lb_inspector_resources',
        'steel_build_id',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_resources',
        # for screenshot encoding
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(DEPTH)/third_party/freetype/freetype.gyp:ft2',
        '<@(platform_dependencies)',
      ],
      'conditions': [
        ['use_widevine==1', {
          'dependencies': [
            '<(DEPTH)/../cdm/ps3/cdm.gyp:wvcdm',
          ],
        }],
        ['target_arch=="wiiu"', {
          'defines': [
            '__LB_WIIU__CPU_PROFILER__=<(enable_wiiu_profiler)',
          ],
        }],
      ],
      'defines': [
        '__DISABLE_WTF_LOGGING__',
        '<@(image_decoder_defines)',
      ],
      'sources': [
        '../../src/platform/<(target_arch)/lb_platform.h',
        '<!@(find ../../src/platform/<(target_arch)/lb_shell/ -type f)',
      ],
    }, # end of target 'lb_symbols'
  ] # end of targets
}
