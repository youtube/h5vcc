{
  'includes': [
    'posix_emulation.gypi',
    'wtf_symbols.gypi',
    'chrome_symbols.gypi',
    'skia_symbols.gypi',
    'lb_symbols.gypi',
    'chromium_unit_tests.gypi',
    'webkit_unit_tests.gypi',
    'lb_layout_tests.gypi',
    'crash_report.gypi',
    'webkit_symbols.gypi',
  ],
  'targets': [
    {
      'target_name': 'lb_shell_lib',
      'type': 'static_library',
      'sources': [
        # sources from lb_shell, please keep alphabetized
        '../../src/data_pack_shell.cc',
        '../../src/lb_app_counters.cc',
        '../../src/lb_app_counters.h',
        '../../src/lb_cookie_store.cc',
        '../../src/lb_cookie_store.h',
        '../../src/lb_data_buffer_pool.h',
        '../../src/lb_debug_console.cc',
        '../../src/lb_debug_console.h',
        '../../src/lb_device_plugin.cc',
        '../../src/lb_device_plugin.h',
        '../../src/lb_devtools_agent.cc',
        '../../src/lb_devtools_agent.h',
        '../../src/lb_framerate_tracker.cc',
        '../../src/lb_framerate_tracker.h',
        '../../src/lb_http_handler.cc',
        '../../src/lb_http_handler.h',
        '../../src/lb_local_storage_database_adapter.cc',
        '../../src/lb_local_storage_database_adapter.h',
        '../../src/lb_memory_debug.c',
        '../../src/lb_memory_internal.h',
        '../../src/lb_memory_manager.cc',
        '../../src/lb_memory_manager.h',
        '../../src/lb_native_javascript_object.h',
        '../../src/lb_navigation_controller.cc',
        '../../src/lb_navigation_controller.h',
        '../../src/lb_network_console.cc',
        '../../src/lb_network_console.h',
        '../../src/lb_request_context.cc',
        '../../src/lb_request_context.h',
        '../../src/lb_resource_loader_bridge.cc',
        '../../src/lb_resource_loader_bridge.h',
        '../../src/lb_resource_loader_check.h',
        '../../src/lb_savegame_syncer.cc',
        '../../src/lb_savegame_syncer.h',
        '../../src/lb_scriptable_np_object.cc',
        '../../src/lb_scriptable_np_object.h',
        '../../src/lb_session_storage_database.cc',
        '../../src/lb_shell.cc',
        '../../src/lb_shell.h',
        '../../src/lb_shell_constants.h',
        '../../src/lb_shell_platform_delegate.cc',
        '../../src/lb_shell_platform_delegate.h',
        '../../src/lb_shell_switches.cc',
        '../../src/lb_shell_switches.h',
        '../../src/lb_shell_thread.h',
        '../../src/lb_shell_webkit_init.cc',
        '../../src/lb_shell_webkit_init.h',
        '../../src/lb_stack.h',
        '../../src/lb_webcookiejar_impl.cc',
        '../../src/lb_webcookiejar_impl.h',
        '../../src/lb_web_audio_device.h',
        '../../src/lb_web_graphics_context_3d.h',
        '../../src/lb_web_graphics_context_proxy.h',
        '../../src/lb_web_media_player_delegate.cc',
        '../../src/lb_web_media_player_delegate.h',
        '../../src/lb_web_view_delegate.cc',
        '../../src/lb_web_view_delegate.h',
        '../../src/object_watcher_shell.cc',
        '../../src/object_watcher_shell.h',
        '../../src/oom_png.c',
        '../../src/oom_png.h',
        '../../src/steel_version.h',
        '../../src/tcp_client_socket_shell.cc',
        '../../src/tcp_client_socket_shell.h',
        '<!@(if [ -d ../../src/private ] ; then find ../../src/private -type f ; else find ../../src/public -type f ; fi)',
      ],
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '../../src/',
        '../../../external/chromium', # for general inclusion of Chromium headers
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime', # to satisfy config.h deps
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore', # for including debugger
        '../../../external/chromium/third_party/WebKit/Source/WebCore/platform',
        '../../../external/chromium/third_party/WebKit/Source/WTF',
        '../../../external/chromium/third_party/WebKit/Source/WTF/wtf',
        '<(SHARED_INTERMEDIATE_DIR)',
        '<(SHARED_INTERMEDIATE_DIR)/inspector',
      ],
      'defines' : [
        '__DISABLE_WTF_LOGGING__',
      ],
      'dependencies': [
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '<(DEPTH)/webkit/support/webkit_support.gyp:glue',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_media',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_support_common',
        # actually include the JavaScriptCore interpreter
        '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/JavaScriptCore.gyp/JavaScriptCore.gyp:javascriptcore',
        '<(DEPTH)/third_party/harfbuzz/harfbuzz.gyp:harfbuzz',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(DEPTH)/third_party/libxml/libxml.gyp:libxml',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/ui/ui.gyp:ui',
        '<(DEPTH)/ui/ui.gyp:ui_resources',
        '<(DEPTH)/net/net.gyp:http_server',
        'chrome_version_info',
        'steel_build_id',
        'posix_emulation',
        'wtf_symbols',
        'chrome_symbols',
        'skia_symbols',
        'lb_symbols',
        'lb_shell_resources',
        '<(DEPTH)/../dlmalloc/dlmalloc.gyp:dlmalloc',
        'webkit_symbols',
      ],
    }, # end of target 'lb_shell_lib'
    {
      'target_name': 'chrome_version_info',
      'type': 'none',
      # Because we generate a header, we must set the hard_dependency flag.
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'chrome_version_info',
          'variables': {
            'version_py_path': '../../../external/chromium/chrome/tools/build/version.py',
            'version_path': '../../../external/chromium/chrome/VERSION',
            'template_input_path': '../../src/chrome_version_info.h.version',
          },
          'inputs': [
            '<(template_input_path)',
            '<(version_path)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/chrome_version_info.h',
          ],
          'action': [
            'python',
            '<(version_py_path)',
            '-f', '<(version_path)',
            '<(template_input_path)',
            '<@(_outputs)',
          ],
          'message': 'Generating version information',
          'msvs_cygwin_shell': 1
        },
      ],
    },
    {
      'target_name': 'lb_shell_resources',
      'type': 'none',
      'variables': {
        'grit_grd_file': '../../src/lb_shell_resources.grd',
        'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/lb_shell',
      },
      'actions': [
        {
          'action_name': 'lb_shell_resources',
          'includes': ['../../../external/chromium/build/grit_action.gypi'],
        },
      ],
      'includes': ['../../../external/chromium/build/grit_target.gypi'],
    },
    {
      'target_name': 'lb_shell_pak',
      'type': 'none',
      'variables': {
        'repack_path': '../../../external/chromium/tools/grit/grit/format/repack.py',
        'pak_path': '<(SHARED_INTERMEDIATE_DIR)/repack/lb_shell.pak',
      },
      'dependencies': [
        '<(DEPTH)/ui/ui.gyp:ui_resources',
        '<(DEPTH)/net/net.gyp:net_resources',
        'lb_shell_resources',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_resources',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_strings',
      ],
      'actions': [
        {
          # Generate resources database in a .pak file. Then, copy it to a place
          # where the console can read it at runtime.
          'action_name': 'lb_shell_repack',
          'variables': {
            'pak_inputs': [
              '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources/gfx_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/net/net_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/lb_shell/lb_shell_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_strings_en-US.pak',
            ],
          },
          'inputs': [
            '<(repack_path)',
            '<@(pak_inputs)',
          ],
          'outputs': [
            '<(pak_path)',
          ],
          'action': ['python', '<(repack_path)', '<@(_outputs)', '<@(pak_inputs)'],
          'message': 'Repacking resources from <@(pak_inputs) to <@(_outputs)',
          'msvs_cygwin_shell': 1,
        },
      ],
    },
    {
      'target_name': 'license_info',
      'type': 'none',
      'variables' : {
        'steel_license_path': '../steel_licenses.py',
        'license_output_path' : '<(SHARED_INTERMEDIATE_DIR)/licenses.html',
      },
      'dependencies': [
        'lb_shell',
      ],
      'actions': [
        {
          # Generate an HTML file containing all the licensing information
          # for steel.
          'action_name': 'generate_licenses',
          'inputs' : [
            '<(steel_license_path)',
          ],
          'outputs' : [
            '<(license_output_path)',
          ],
          'action': ['python', '<(steel_license_path)', 'license', '<(PRODUCT_DIR)', '<(license_output_path)'],
          'message': 'Generating license file.',
          'msvs_cygwin_shell' : 1,
        },
      ],
    },
    {
      'target_name': 'lb_shell',
      'type': 'executable',
      'sources': [
        '../../src/lb_shell_main.cc',
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
      ],
      'dependencies': [
        # the icu target is already in the dependency tree,
        # but it has to be included in our own target to make the order of the -I line correct
        # (it has a defaults section which gets applied)
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        'lb_shell_lib',
        'steel_build_id',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
    },
    {
      'target_name': 'lb_unit_tests',
      'type': 'executable',
      'sources': [
        '<!@(find ../../src/ -type f -maxdepth 1 -name *_unittest*.cc)',
      ],
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '../../src/',
        '<(DEPTH)/testing/gtest/include',
        '<(SHARED_INTERMEDIATE_DIR)',
        '<(SHARED_INTERMEDIATE_DIR)/inspector',
        '../../../external/chromium',
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/net/net.gyp:net_test_support',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
      'conditions': [
        ['target_arch == "wiiu"', {
          'sources': [
            '<!@(find ../../src/platform/<(target_arch)/unittests/ -type f -name *_unittest*.cc)',
          ],
        }],
      ],
    },
  ], # end of targets
  'conditions': [
    ['use_widevine == 1', {
      # begin widevine targets
      'targets': [
        {
          'target_name': 'wvcdm_test',
          'type': 'executable',
          'dependencies': [
            'lb_shell_lib',
            '<(DEPTH)/base/base.gyp:base',
            '<(DEPTH)/testing/gmock.gyp:gmock',
            '<(DEPTH)/testing/gtest.gyp:gtest',
            '<(DEPTH)/net/net.gyp:net',
            '<(DEPTH)/net/net.gyp:net_test_support',
            '<(DEPTH)/../cdm/ps3/cdm.gyp:wvcdm',
          ],
          'include_dirs': [
            '<(DEPTH)/../cdm/core/include',
            '<(DEPTH)/../cdm/ps3/include',
          ],
          'libraries': [
            '<@(platform_libraries)',
          ],
          'sources': [
            '<(DEPTH)/../cdm/ps3/test/request_license_test.cpp',
          ],
        },
      ],
    }],
  ],
}
