{
  'includes': [
    'chromium_unit_tests.gypi',
    'webkit_unit_tests.gypi',
    'lb_layout_tests.gypi',
    'pthread_tests.gypi',
    'crash_report.gypi',
  ],
  'targets': [
    {
      'target_name': 'lb_shell_lib',
      'type': '<(component)',
      'sources': [
        # sources from lb_shell, please keep alphabetized
        '../../src/lb_cookie_store.cc',
        '../../src/lb_cookie_store.h',
        '../../src/lb_data_buffer_pool.h',
        '../../src/lb_debug_console.cc',
        '../../src/lb_debug_console.h',
        '../../src/lb_devtools_agent.cc',
        '../../src/lb_devtools_agent.h',
        '../../src/lb_download_manager.cc',
        '../../src/lb_download_manager.h',
        '../../src/lb_framerate_tracker.cc',
        '../../src/lb_framerate_tracker.h',
        '../../src/lb_gl_image_utils.cc',
        '../../src/lb_gl_image_utils.h',
        '../../src/lb_heads_up_display.cc',
        '../../src/lb_heads_up_display.h',
        '../../src/lb_http_handler.cc',
        '../../src/lb_http_handler.h',
        '../../src/lb_input_fuzzer.cc',
        '../../src/lb_input_fuzzer.h',
        '../../src/lb_local_storage_database_adapter.cc',
        '../../src/lb_local_storage_database_adapter.h',
        '../../src/lb_navigation_controller.cc',
        '../../src/lb_navigation_controller.h',
        '../../src/lb_network_console.cc',
        '../../src/lb_network_console.h',
        '../../src/lb_on_screen_console.cc',
        '../../src/lb_on_screen_console.h',
        '../../src/lb_on_screen_display.cc',
        '../../src/lb_on_screen_display.h',
        '../../src/lb_output_surface.cc',
        '../../src/lb_output_surface.h',
        '../../src/lb_platform_mic.h',
        '../../src/lb_request_context.cc',
        '../../src/lb_request_context.h',
        '../../src/lb_resource_loader_bridge.cc',
        '../../src/lb_resource_loader_bridge.h',
        '../../src/lb_resource_loader_check.h',
        '../../src/lb_savegame_syncer.cc',
        '../../src/lb_savegame_syncer.h',
        '../../src/lb_shell.cc',
        '../../src/lb_shell.h',
        '../../src/lb_shell_console_values_hooks.cc',
        '../../src/lb_shell_console_values_hooks.h',
        '../../src/lb_shell_constants.h',
        '../../src/lb_shell_platform_delegate.cc',
        '../../src/lb_shell_platform_delegate.h',
        '../../src/lb_shell_switches.cc',
        '../../src/lb_shell_switches.h',
        '../../src/lb_shell_thread.h',
        '../../src/lb_shell_webkit_init.cc',
        '../../src/lb_shell_webkit_init.h',
        '../../src/lb_speech_grammar.cc',
        '../../src/lb_speech_grammar.h',
        '../../src/lb_speech_recognition_client.cc',
        '../../src/lb_speech_recognition_client.h',
        '../../src/lb_speech_recognizer.h',
        '../../src/lb_spinner_overlay.cc',
        '../../src/lb_spinner_overlay.h',
        '../../src/lb_splash_screen.cc',
        '../../src/lb_splash_screen.h',
        '../../src/lb_sqlite_vfs.cc',
        '../../src/lb_sqlite_vfs.h',
        '../../src/lb_storage_cleanup.cc',
        '../../src/lb_storage_cleanup.h',
        '../../src/lb_system_stats_tracker.cc',
        '../../src/lb_system_stats_tracker.h',
        '../../src/lb_text_printer.cc',
        '../../src/lb_text_printer.h',
        '../../src/lb_tracing_manager.cc',
        '../../src/lb_tracing_manager.h',
        '../../src/lb_user_input_device.h',
        '../../src/lb_user_playback_input_device.cc',
        '../../src/lb_video_overlay.cc',
        '../../src/lb_video_overlay.h',
        '../../src/lb_virtual_file_system.cc',
        '../../src/lb_virtual_file_system.h',
        '../../src/lb_web_audio_device.h',
        '../../src/lb_web_audio_device_impl.cc',
        '../../src/lb_web_graphics_context_3d.h',
        '../../src/lb_web_graphics_context_proxy.h',
        '../../src/lb_webblobregistry_impl.cc',
        '../../src/lb_webblobregistry_impl.h',
        '../../src/lb_webcookiejar_impl.cc',
        '../../src/lb_webcookiejar_impl.h',
        '../../src/lb_web_graphics_context_3d_command_buffer.cc',
        '../../src/lb_web_graphics_context_3d_command_buffer.h',
        '../../src/lb_web_media_player_delegate.cc',
        '../../src/lb_web_media_player_delegate.h',
        '../../src/lb_web_view_delegate.cc',
        '../../src/lb_web_view_delegate.h',
        '../../src/lb_web_view_host_common.cc',
        '../../src/lb_web_view_host.h',
        '../../src/steel_version.h',
        '<!@(if [ -d ../../src/private ] ; then find ../../src/private -type f ; else find ../../src/public -type f ; fi)',
        '../../src/platform/<(target_arch)/lb_platform.h',
        '<!@(find ../../src/platform/<(target_arch)/lb_shell/ -type f)',
      ],
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '<(DEPTH)', # for general inclusion of Chromium headers
        '<(DEPTH)/third_party/icu/public/common', # to build JSC & WTF things
        '<(DEPTH)/third_party/WebKit/Source/Platform/chromium', # to satisfy WebViewClient.h deps
        '<(DEPTH)/third_party/WebKit/Source/WebCore/dom',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/inspector',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/page',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/platform',
        '<(DEPTH)/third_party/WebKit/Source/WTF',
        '<(DEPTH)/third_party/WebKit/Source/WTF/wtf',
        '../../src/',
        '<(SHARED_INTERMEDIATE_DIR)',
        '<(SHARED_INTERMEDIATE_DIR)/inspector',
        '<(SHARED_INTERMEDIATE_DIR)/lb_shell',
        '<(SHARED_INTERMEDIATE_DIR)/shaders',
        '<(SHARED_INTERMEDIATE_DIR)/ui/gl',
        '<(DEPTH)/../angle/include',
      ],
      'defines' : [
        '__DISABLE_WTF_LOGGING__',
        '<@(image_decoder_defines)',
        'LB_SHELL_IMPLEMENTATION=1',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/net/net.gyp:http_server',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/sql/sql.gyp:sql',
        '<(DEPTH)/third_party/freetype/freetype.gyp:ft2',
        '<(DEPTH)/third_party/harfbuzz/harfbuzz.gyp:harfbuzz',
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(DEPTH)/third_party/libxml/libxml.gyp:libxml',
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit_wtf_support',
        '<(DEPTH)/ui/ui.gyp:ui',
        '<(DEPTH)/ui/ui.gyp:ui_resources',
        '<(DEPTH)/gpu/command_buffer/command_buffer.gyp:gles2_utils',
        '<(DEPTH)/gpu/gpu.gyp:command_buffer_service',
        '<(DEPTH)/gpu/gpu.gyp:command_buffer_client',
        '<(DEPTH)/gpu/gpu.gyp:gles2_c_lib',
        '<(DEPTH)/gpu/gpu.gyp:gles2_implementation',
        '<(DEPTH)/ui/gl/gl.gyp:gl',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_base',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_media',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_resources',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_storage',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_support_common',
        '<@(platform_dependencies)',
        'chrome_version_info',
        'lb_inspector_resources',
        'lb_shell_resources',
        'posix_emulation.gyp:posix_emulation',
      ],
      'conditions': [
        ['use_gpu_command_buffer==0', {
          'sources!': [
            '../../src/lb_web_graphics_context_3d_command_buffer.cc',
            '../../src/lb_web_graphics_context_3d_command_buffer.h',
          ],
          'dependencies!': [
            '<(DEPTH)/gpu/command_buffer/command_buffer.gyp:gles2_utils',
            '<(DEPTH)/gpu/gpu.gyp:command_buffer_service',
            '<(DEPTH)/gpu/gpu.gyp:command_buffer_client',
            '<(DEPTH)/gpu/gpu.gyp:gles2_c_lib',
            '<(DEPTH)/gpu/gpu.gyp:gles2_implementation',
            '<(DEPTH)/ui/gl/gl.gyp:gl',
            '<(DEPTH)/ui/ui.gyp:ui',
          ]
        }],
        ['target_arch=="wiiu"', {
          'defines': [
            '__LB_WIIU__CPU_PROFILER__=<(enable_wiiu_profiler)',
          ],
        }],
        ['target_arch=="xb1"', {
          'dependencies': [
            '<(DEPTH)/../angle/src/build_angle.gyp:libEGL',
            '<(DEPTH)/../angle/src/build_angle.gyp:libGLESv2',
            'xb1_events.gyp:xbox_common_events',
            'xb1_oneguide_task.gyp:lb_oneguide_update_task',
            '<(DEPTH)/../xbox_services_api/xbox_services_api.gyp:XboxServicesApi',
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ComponentExtensions': 'true'
            },
          },
        }],
        ['target_arch=="xb360"', {
          'dependencies': [
            '<(DEPTH)/../angle/src/build_angle.gyp:libEGL',
            '<(DEPTH)/../angle/src/build_angle.gyp:libGLESv2',
          ],
        }],
        ['target_arch=="android"', {
          'dependencies': [
            '<(DEPTH)/base/base.gyp:base_java',
            '<(DEPTH)/breakpad/breakpad.gyp:breakpad_client',
            '<(DEPTH)/media/media.gyp:media_java',
            '<(DEPTH)/media/media.gyp:player_android',
            '<(DEPTH)/net/net.gyp:net_java',
            '<(DEPTH)/ui/ui.gyp:ui_java',
            'lb_shell_jni_headers',
          ],
          'include_dirs': [
            '../../../external/chromium/breakpad/src',
            '../../../external/chromium/breakpad/src/common/linux',
          ],
        }],
        ['target_arch=="ps4"', {
          'dependencies': [
            '<(DEPTH)/third_party/libvpx/libvpx.gyp:libvpx',
          ],
        }],
        ['skia_gpu!=0', {
          'dependencies': [
            '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_gpu',
          ],
        }],
        ['js_engine == "jsc"', {
          'include_dirs+': [
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore', # for including debugger
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/API',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/assembler',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/bytecode',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/disassembler',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/heap',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/interpreter',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/jit',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/llint',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/profiler',
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/runtime', # to satisfy config.h deps
            '<(DEPTH)/third_party/WebKit/Source/WebCore/bindings/js',
          ],
          'dependencies': [
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/JavaScriptCore.gyp/JavaScriptCore.gyp:javascriptcore',
          ],
        }],
        ['js_engine == "v8"', {
          'include_dirs': [
            '<(DEPTH)/third_party/WebKit/Source/WebCore/bindings/v8',
            '<(DEPTH)/v8/include',
          ],
          'dependencies': [
            '<(DEPTH)/v8/tools/gyp/v8.gyp:v8',
          ],
          # All targets that depend on lb_shell_lib get this include.
          'direct_dependent_settings': {
            'include_dirs': [
              '<(DEPTH)/v8/include',
            ],
          },
        }],
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
      'type': '<(final_executable_type)',
      'variables': {
        'main_thread_stack_size': '<(LB_SHELL_MAIN_THREAD_STACK_SIZE)',
      },
      'sources': [
        '../../src/lb_shell_main.cc',
      ],
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '../../../external/chromium', # for general inclusion of Chromium headers
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
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit_wtf_support',
        '<(DEPTH)/webkit/support/webkit_support.gyp:glue',
        'posix_emulation.gyp:posix_emulation',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
      'conditions': [
        ['target_arch=="android"', {
          'sources': [
            '../../src/platform/android/lb_shell_launcher_android.cc',
          ],
        }],
        ['js_engine == "jsc"', {
          'include_dirs': [
            '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime', # to satisfy config.h deps
            '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore', # for including debugger
          ],
          'dependencies': [
            '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/JavaScriptCore.gyp/JavaScriptCore.gyp:javascriptcore',
          ],
        }],
      ],
    },
    {
      'target_name': 'lb_unit_tests',
      'type': '<(final_executable_type)',
      'sources': [
        '<!@(find ../../src/ -type f -maxdepth 1 -name *_unittest*.cc)',
        '<!@(find ../../src/platform/<(target_arch)/unittests/ -type f -name *_unittest*.cc)',
      ],
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '../../src/',
        '<(DEPTH)/testing/gtest/include',
        '<(SHARED_INTERMEDIATE_DIR)',
        '<(SHARED_INTERMEDIATE_DIR)/inspector',
        '../../../external/chromium',
        '../../../external/chromium/third_party/WebKit/Source/WebCore/platform',
        '../../../external/chromium/third_party/WebKit/Source/WTF',
        '../../../external/chromium/third_party/WebKit/Source/WTF/wtf',
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/net/net.gyp:net_test_support',
        '<(DEPTH)/webkit/support/webkit_support.gyp:glue',
        'posix_emulation.gyp:posix_emulation',
        '../platforms/<(target_arch)_contents.gyp:copy_unit_test_data',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
       'conditions': [
        ['target_arch == "android"', {
          'dependencies': [
            '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
        ['target_arch == "xb1"', {
          'dependencies': [
            'xb1_oneguide_task.gyp:lb_oneguide_update_task',
          ],
        }],
      ],
   },
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
  ], # end of targets
  'conditions': [
    ['target_arch == "android"', {
      'targets' : [
        {
          'target_name': 'lb_unit_tests_apk',
          'type': 'none',
          'dependencies': [
            'lb_unit_tests',
          ],
          'variables': {
            'test_suite_name': 'lb_unit_tests',
            'input_shlib_path': '<(SHARED_LIB_DIR)/<(SHARED_LIB_PREFIX)lb_unit_tests<(SHARED_LIB_SUFFIX)',
          },
          'includes': [ '../../../external/chromium/build/apk_test.gypi' ],
        },
        {
          'target_name': 'lb_shell_jni_headers',
          'type': 'none',
          'sources': [
            '<(lb_shell_java_src)/LbShellAccountManager.java',
            '<(lb_shell_java_src)/LbShellSearch.java',
            '<(lb_shell_java_src)/LbShellView.java',
          ],
          'variables': {
            'jni_gen_dir': 'lb_shell',
            'lb_shell_java_src': '<(lbshell_root)/src/platform/android/lb_shell/java/src/com/google/android/apps/youtube/pano',
          },
          'includes': [ '../../../external/chromium/build/jni_generator.gypi' ],
        },
      ],
    }],
  ],
}
