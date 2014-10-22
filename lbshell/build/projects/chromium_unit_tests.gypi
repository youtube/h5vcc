{
  'targets': [
    {
      'target_name': 'unit_tests_base',
      'type': '<(final_executable_type)',
      'variables': {
        'main_thread_stack_size': '<(LB_UNIT_TEST_MAIN_THREAD_STACK_SIZE)',
      },
      'sources': [
        '<!@(find <(DEPTH)/base -type f -name \'*_unittest*.cc\')',
        '<!@(find <(lbshell_root)/src/platform/<(target_arch)/chromium/base -type f -name \'*_unittest*.cc\')',
        '<(DEPTH)/base/test/sequenced_worker_pool_owner.h',
        '<(DEPTH)/base/test/sequenced_worker_pool_owner.cc',
      ],
      'sources/': [
        # wildcard filters (don't compile or link)
        ['exclude', '/win/'],
        ['exclude', '/mac/'],
        ['exclude', '_win_'],
        ['exclude', '_ios'],
        # specific unit tests (don't compile or link)
        ['exclude', 'process_util_unittest'],
        ['exclude', 'dir_reader_posix_unittest'],
        ############### ALL TESTS BELOW ARE PERMANENTLY EXCLUDED ###############
        ['exclude', '/base/prefs/'], # prefs is a key/value store for application preferences
        ['exclude', 'environment_unittest'],  # code being tested is excluded
        ['exclude', 'file_descriptor_shuffle_unittest'],  # code being tested is excluded
        ['exclude', 'icu_string_conversions_unittest'],  # code being tested is marked as NOTREACHED because no data table is available.
        ['exclude', 'message_pump_glib_unittest'],
        ['exclude', 'message_pump_libevent_unittest'],
        ['exclude', 'shared_memory_unittest'],
        ['exclude', 'stats_table_unittest'],  # code being tested is excluded
        ['exclude', 'tcmalloc_unittest'],
        ['exclude', 'waitable_event_watcher_unittest'],
        ['exclude', 'xdg_util_unittest'],
      ],
      'include_dirs': [
        '<(DEPTH)/testing/gtest/include',
        '../../../', # for our explicit external/ inclusion of headers
        '../../src/',
        '<(DEPTH)/chromium', # for general inclusion of Chromium headers
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/base/base.gyp:base_static',
        '<(DEPTH)/base/base.gyp:test_support_base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        'posix_emulation.gyp:posix_emulation',
        '../platforms/<(target_arch)_contents.gyp:copy_contents_common',
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
          'sources!': [
            # # Broken on Android, and already disabled there. (see base_unittests)
            '<(DEPTH)/base/debug/stack_trace_unittest.cc',
          ],
        }, { # target_arch != "android"
          'sources/': [
            ['exclude', 'android'],
          ],
        }],
      ],
    },
    {
      'target_name': 'unit_tests_crypto',
      'type': '<(final_executable_type)',
      'variables': {
        'main_thread_stack_size': '<(LB_UNIT_TEST_MAIN_THREAD_STACK_SIZE)',
      },
      'sources': [
        '<!@(find <(DEPTH)/crypto -type f -name \'*_unittest*.cc\')',
      ],
      'sources/': [
        ############### ALL TESTS BELOW ARE PERMANENTLY EXCLUDED ###############
        ['exclude', 'openpgp_symmetric_encryption_unittest'],
        ['exclude', 'nss_util_unittest'],
        ['exclude', 'rsa_private_key_nss_unittest'],
      ],
      'include_dirs': [
        '<(DEPTH)/testing/gtest/include',
        '../../../', # for our explicit external/ inclusion of headers
        '../../src/',
        '<(DEPTH)/chromium', # for general inclusion of Chromium headers
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/crypto/crypto.gyp:crypto',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:test_support_base',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'posix_emulation.gyp:posix_emulation',
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
      ],
    },
    {
      'target_name': 'unit_tests_media',
      'type': '<(final_executable_type)',
      'variables': {
        'main_thread_stack_size': '<(LB_UNIT_TEST_MAIN_THREAD_STACK_SIZE)',
      },
      'sources': [
        '<!@(find <(DEPTH)/media -type f -name \'*_unittest*.cc\')',
      ],
      'sources/': [
        ['exclude', '/win/'],
        ['exclude', '/mac/'],
        ['exclude', '/linux/'],
        ['exclude', '/ios/'],
        # These tests don't compile or link
        ['exclude', 'chunk_demuxer_unittest'],
        ['exclude', 'async_socket_io_handler_unittest'],
        ['exclude', 'video_frame_unittest'],
        ['exclude', 'mp4_stream_parser_unittest'],
        ['exclude', 'decrypting_video_decoder_unittest'],
        ['exclude', 'decrypting_demuxer_stream_unittest'],
        ['exclude', 'decrypting_audio_decoder_unittest'],
        # Aes decryption is not implemented yet.
        ['exclude', 'aes_decryptor_unittest'],
        # These tests hang/crash/assert
        ['exclude', 'audio_renderer_impl_unittest'],
        ['exclude', 'file_data_source_unittest'],
        ############### ALL TESTS BELOW ARE PERMANENTLY EXCLUDED ###############
        # These tests reference ffmpeg
        ['exclude', 'ffmpeg'],
        ['exclude', 'decoder_buffer_unittest'],
        ['exclude', 'blocking_url_protocol_unittest'],
        # Deprecated due to new media stack
        ['exclude', 'data_buffer_unittest'],
        ['exclude', 'source_buffer_stream_unittest'],
        # These are too low level and steel has its own implementation
        ['exclude', 'audio_converter_unittest'],
        ['exclude', 'audio_decoder_selector_unittest'],
        ['exclude', 'audio_file_reader_unittest'],
        ['exclude', 'audio_input_volume_unittest'],
        ['exclude', 'audio_low_latency_input_output_unittest'],
        ['exclude', 'audio_output_device_unittest'],
        ['exclude', 'audio_renderer_algorithm_unittest'],
        ['exclude', 'audio_renderer_mixer_input_unittest'],
        ['exclude', 'channel_mixer_unittest'],
        ['exclude', 'convert_rgb_to_yuv_unittest'],
        ['exclude', 'cross_process_notification_unittest'],
        ['exclude', 'fake_audio_output_stream_unittest'],
        ['exclude', 'skcanvas_video_renderer_unittest'],
        ['exclude', 'video_capture_device_unittest'],
        ['exclude', 'virtual_audio_input_stream_unittest'],
        ['exclude', 'virtual_audio_output_stream_unittest'],
        ['exclude', 'webm_cluster_parser_unittest'],
        ['exclude', 'webm_parser_unittest'],
        ['exclude', 'yuv_convert_unittest'],
        # These tests reference AudioManager (not-implemented on PS3)
        ['exclude', 'simple_sources_unittest'],
        ['exclude', 'audio_input_controller_unittest'],
        ['exclude', 'audio_input_device_unittest'],
        ['exclude', 'audio_input_unittest'],
        ['exclude', 'audio_output_controller_unittest'],
        ['exclude', 'audio_output_proxy_unittest'],
        # These tests hang/crash/assert
        ['exclude', 'audio_renderer_mixer_unittest'],
      ],
      'include_dirs': [
        '<(DEPTH)/testing/gtest/include',
        '../../../', # for our explicit external/ inclusion of headers
        '../../src/',
        '<(DEPTH)/chromium', # for general inclusion of Chromium headers
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/media/media.gyp:media',
        '<(DEPTH)/media/media.gyp:media_test_support',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/base/base.gyp:test_support_base',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'posix_emulation.gyp:posix_emulation',
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
      ],
    },
    {
      'target_name': 'unit_tests_net',
      'type': '<(final_executable_type)',
      'variables': {
        'main_thread_stack_size': '<(LB_UNIT_TEST_MAIN_THREAD_STACK_SIZE)',
      },
      'sources': [
        '<!@(find <(DEPTH)/net -type f -name \'*_unittest*.cc\')',
        '<!@(find <(lbshell_root)/src/platform/<(target_arch)/chromium/net -type f -name \'*_unittest*.cc\')',
        '<(DEPTH)/net/base/mock_filter_context.cc',
        '<(DEPTH)/net/base/mock_filter_context.h',
        '<(DEPTH)/net/base/test_certificate_data.h',
        '<(DEPTH)/net/http/http_auth_handler_mock.cc',
        '<(DEPTH)/net/http/http_auth_handler_mock.h',
        '<(DEPTH)/net/http/http_pipelined_host_test_util.cc',
        '<(DEPTH)/net/http/http_pipelined_host_test_util.h',
        '<(DEPTH)/net/http/mock_allow_url_security_manager.cc',
        '<(DEPTH)/net/http/mock_allow_url_security_manager.h',
        '<(DEPTH)/net/http/mock_http_cache.cc',
        '<(DEPTH)/net/http/mock_http_cache.h',
        '<(DEPTH)/net/socket/mock_client_socket_pool_manager.cc',
        '<(DEPTH)/net/socket/mock_client_socket_pool_manager.h',
        '<(DEPTH)/net/spdy/spdy_frame_reader_test.cc',
        '<(DEPTH)/net/spdy/spdy_framer_test.cc',
        '<(DEPTH)/net/spdy/spdy_protocol_test.cc',
        '<(DEPTH)/net/spdy/spdy_stream_test_util.cc',
        '<(DEPTH)/net/spdy/spdy_stream_test_util.h',
        '<(DEPTH)/net/spdy/spdy_test_util_spdy3.cc',
        '<(DEPTH)/net/spdy/spdy_test_util_spdy3.h',
        '<(DEPTH)/net/spdy/spdy_test_util_spdy2.cc',
        '<(DEPTH)/net/spdy/spdy_test_util_spdy2.h',
        '<(DEPTH)/net/spdy/spdy_websocket_test_util_spdy2.cc',
        '<(DEPTH)/net/spdy/spdy_websocket_test_util_spdy2.h',
        '<(DEPTH)/net/spdy/spdy_websocket_test_util_spdy3.cc',
        '<(DEPTH)/net/spdy/spdy_websocket_test_util_spdy3.h',
        '<(DEPTH)/net/tools/dump_cache/url_to_filename_encoder.cc',
        '<(DEPTH)/net/tools/dump_cache/url_to_filename_encoder.h',
        '<(DEPTH)/net/tools/dump_cache/url_utilities.h',
        '<(DEPTH)/net/tools/dump_cache/url_utilities.cc',
        '<(DEPTH)/net/url_request/url_request_throttler_test_support.cc',
        '<(DEPTH)/net/url_request/url_request_throttler_test_support.h',
      ],
      'sources/': [
        ['exclude', '_android_'],
        ['exclude', '_linux_'],
        ['exclude', '_posix_'],
        ['exclude', '_win_'],
        # Causes duplicate symbol errors on android:
        ['exclude', 'proxy_config_service_common_unittest'],
        # These tests don't compile or link
        ['exclude', 'url_security_manager_unittest'],
        ['exclude', 'tcp_server_socket_unittest'],
        ['exclude', 'url_request_throttler_simulation_unittest'],
        ['exclude', 'url_request_unittest'],
        ['exclude', 'url_request_context_builder_unittest'],
        ['exclude', 'udp_socket_unittest'],
        ['exclude', 'dnssec_unittest'],
        ['exclude', 'tcp_client_socket_unittest'],
        ['exclude', 'dnsrr_resolver_unittest'],
        ['exclude', 'url_fetcher_impl_unittest'],
        ['exclude', 'ssl_client_socket_unittest'],
        ['exclude', 'nss_cert_database_unittest'],
        # These tests hang/crash/assert
        ['exclude', 'backend_unittest'],  # ref-counting problems
        ['exclude', 'block_files_unittest'],  # test data is little-endian
        ['exclude', 'entry_unittest'],
        ['exclude', 'tcp_listen_socket_unittest'],  #hang?
        ['exclude', 'cert_verify_proc_unittest'],
        ############### ALL TESTS BELOW ARE PERMANENTLY EXCLUDED ###############
        ['exclude', 'address_sorter_unittest'],   # at time of exclusion, address sorting only happens when IPv6 is enabled
        ['exclude', 'curvecp_transfer_unittest'],   # code being tested is excluded
        ['exclude', 'effective_tld_names_unittest1'],  # not standalone; included by registry_controlled_domain_unittest.cc
        ['exclude', 'effective_tld_names_unittest2'],  # not standalone; included by registry_controlled_domain_unittest.cc
        ['exclude', 'http_auth_handler_negotiate_unittest'],
        ['exclude', 'http_auth_gssapi_posix_unittest'],
        ['exclude', 'http_content_disposition_unittest'],
        ['exclude', 'cert_database_nss_unittest'],
        ['exclude', 'unix_domain_socket_posix_unittest'],
        ['exclude', 'sdch_filter_unittest'],
        ['exclude', 'proxy_resolver_v8_unittest'],
        ['exclude', 'python_utils_unittest'],
        ['exclude', 'proxy_script_fetcher_impl_unittest'],
        ['exclude', 'x509_cert_types_unittest'],  # ParseDistinguishedName() only exists for mac/win
        # FTP is not supported
        ['exclude', 'ftp_auth_cache_unittest'],
        ['exclude', 'ftp_ctrl_response_buffer_unittest'],
        ['exclude', 'ftp_directory_listing_parser_unittest'],
        ['exclude', 'ftp_directory_listing_parser_ls_unittest'],
        ['exclude', 'ftp_directory_listing_parser_netware_unittest'],
        ['exclude', 'ftp_directory_listing_parser_os2_unittest'],
        ['exclude', 'ftp_directory_listing_parser_vms_unittest'],
        ['exclude', 'ftp_directory_listing_parser_windows_unittest'],
        ['exclude', 'ftp_network_transaction_unittest'],
        ['exclude', 'ftp_util_unittest'],
        ['exclude', 'url_request_ftp_job_unittest'],
        ['exclude', 'disk_cache'],
        # WebSockets not supported
        ['exclude', 'socket_stream/'],
        ['exclude', 'websockets/'],
        ['exclude', 'spdy_websocket_stream_spdy2_unittest'],
        ['exclude', 'spdy_websocket_stream_spdy3_unittest'],
      ],
      'include_dirs': [
        '<(DEPTH)/testing/gtest/include',
        '../../../', # for our explicit external/ inclusion of headers
        '../../src/',
        '<(DEPTH)/chromium', # for general inclusion of Chromium headers
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/net/net.gyp:net',
        '<(DEPTH)/net/net.gyp:net_test_support',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/base.gyp:base_i18n',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/crypto/crypto.gyp:crypto',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/zlib/zlib.gyp:zlib',
        '../../../external/openssl/openssl.gyp:openssl',
        'posix_emulation.gyp:posix_emulation',
        '../platforms/<(target_arch)_contents.gyp:copy_contents_common',
        '../platforms/<(target_arch)_contents.gyp:copy_unit_test_data',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
      'conditions': [
        [ 'use_openssl==1', {
            # When building for OpenSSL, we need to exclude NSS specific tests.
            # TODO(bulach): Add equivalent tests when the underlying
            #               functionality is ported to OpenSSL.
            'sources/': [
              ['exclude', 'base/x509_util_nss_unittest.cc'],
              ['exclude', 'base/cert_database_nss_unittest.cc'],
              ['exclude', 'base/dnssec_unittest.cc'],
            ],
          }, {  # else !use_openssl: remove the unneeded files
            'sources/': [
              ['exclude', 'base/x509_util_openssl_unittest.cc'],
            ],
          },
        ],
        ['use_native_http_stack==1', {
          'sources!': [
            '<(DEPTH)/net/base/keygen_handler_unittest.cc',
            '<(DEPTH)/net/http/http_cache_unittest.cc',
            '<(DEPTH)/net/http/http_network_layer_unittest.cc',
            '<(DEPTH)/net/http/http_response_body_drainer_unittest.cc',
            '<(DEPTH)/net/http/http_pipelined_connection_impl_unittest.cc',
            '<(DEPTH)/net/http/http_pipelined_host_forced_unittest.cc',
            '<(DEPTH)/net/http/http_pipelined_host_impl_unittest.cc',
            '<(DEPTH)/net/http/http_pipelined_host_pool_unittest.cc',
            '<(DEPTH)/net/http/http_pipelined_network_transaction_unittest.cc',
            '<(DEPTH)/net/http/http_stream_factory_impl_unittest.cc',
            '<(DEPTH)/net/http/http_stream_parser_unittest.cc',
            '<(DEPTH)/net/proxy/dhcp_proxy_script_adapter_fetcher_win_unittest.cc',
            '<(DEPTH)/net/proxy/dhcp_proxy_script_fetcher_factory_unittest.cc',
            '<(DEPTH)/net/proxy/dhcp_proxy_script_fetcher_win_unittest.cc',
            '<(DEPTH)/net/proxy/multi_threaded_proxy_resolver_unittest.cc',
            '<(DEPTH)/net/proxy/network_delegate_error_observer_unittest.cc',
            '<(DEPTH)/net/proxy/proxy_resolver_js_bindings_unittest.cc',
            '<(DEPTH)/net/proxy/proxy_resolver_v8_unittest.cc',
            '<(DEPTH)/net/proxy/proxy_script_decider_unittest.cc',
            '<(DEPTH)/net/proxy/proxy_script_fetcher_impl_unittest.cc',
            '<(DEPTH)/net/proxy/proxy_server_unittest.cc',
            '<(DEPTH)/net/proxy/proxy_service_unittest.cc',
            '<(DEPTH)/net/proxy/sync_host_resolver_bridge_unittest.cc',
            '<(DEPTH)/net/url_request/view_cache_helper_unittest.cc',
            '<(DEPTH)/net/http/mock_http_cache.cc',
            '<(DEPTH)/net/http/mock_http_cache.h',
          ],
          'sources/': [
            ['exclude', 'socket'],
            ['exclude', 'spdy'],
            ['exclude', 'host_resolver'],
            ['exclude', 'dial'],
            ['exclude', 'dns'],
            ['exclude', 'auth'],
          ]
        }],
        ['target_arch=="android"', {
          'sources/': [
            # No res_ninit() et al on Android:
            ['exclude', 'dns/dns_config_service_posix_unittest.cc'],
            # No in-app DIAL on Android:
            ['exclude', 'dial'],
          ],
          'dependencies': [
            '<(DEPTH)/testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
      ],
    },
    {
      'target_name': 'unit_tests_sql',
      'type': '<(final_executable_type)',
      'variables': {
        'main_thread_stack_size': '<(LB_UNIT_TEST_MAIN_THREAD_STACK_SIZE)',
      },
      'sources': [
        '<!@(find <(DEPTH)/sql -type f -name \'*_unittest*.cc\')',
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/sql/sql.gyp:sql',
        '<(DEPTH)/base/base.gyp:test_support_base',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        'posix_emulation.gyp:posix_emulation',
      ],
      'include_dirs': [
        '<(DEPTH)',
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
      ],
    },
  ], # end of targets
  'conditions': [
    ['target_arch == "ps4"', {
      'targets': [
        {
          'target_name': 'unit_tests_libvpx',
          'type': '<(final_executable_type)',
          'variables': {
            'main_thread_stack_size': '<(LB_UNIT_TEST_MAIN_THREAD_STACK_SIZE)',
          },
          'sources': [
            '<(DEPTH)/third_party/libvpx/test/convolve_test.cc',
            '<(DEPTH)/third_party/libvpx/test/decode_test_driver.cc',
            '<(DEPTH)/third_party/libvpx/test/test_libvpx.cc',
            '<(DEPTH)/third_party/libvpx/test/test_vector_test.cc',
            '<(DEPTH)/third_party/libvpx/test/vp9_thread_test.cc',
          ],
          'dependencies': [
            'lb_shell_lib',
            '<(DEPTH)/third_party/libvpx/libvpx.gyp:libvpx',
            '<(DEPTH)/base/base.gyp:test_support_base',
            '<(DEPTH)/testing/gtest.gyp:gtest',
            'posix_emulation.gyp:posix_emulation',
          ],
          'include_dirs': [
            '<(DEPTH)',
            '<(DEPTH)/third_party/libvpx/',
            '<(DEPTH)/third_party/libvpx/platforms/<(target_arch)',
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
          ],
        },
      ],
    }],
    ['target_arch == "android"', {
      'targets': [
        {
          'target_name': 'unit_tests_base_apk',
          'type': 'none',
          'dependencies': [
            'unit_tests_base',
          ],
          'variables': {
            'test_suite_name': 'unit_tests_base',
            'input_shlib_path': '<(SHARED_LIB_DIR)/<(SHARED_LIB_PREFIX)unit_tests_base<(SHARED_LIB_SUFFIX)',
          },
          'includes': [ '../../../external/chromium/build/apk_test.gypi' ],
        },
        {
          'target_name': 'unit_tests_crypto_apk',
          'type': 'none',
          'dependencies': [
            'unit_tests_crypto',
          ],
          'variables': {
            'test_suite_name': 'unit_tests_crypto',
            'input_shlib_path': '<(SHARED_LIB_DIR)/<(SHARED_LIB_PREFIX)unit_tests_crypto<(SHARED_LIB_SUFFIX)',
          },
          'includes': [ '../../../external/chromium/build/apk_test.gypi' ],
        },
        {
          'target_name': 'unit_tests_media_apk',
          'type': 'none',
          'dependencies': [
            'unit_tests_media',
          ],
          'variables': {
            'test_suite_name': 'unit_tests_media',
            'input_shlib_path': '<(SHARED_LIB_DIR)/<(SHARED_LIB_PREFIX)unit_tests_media<(SHARED_LIB_SUFFIX)',
          },
          'includes': [ '../../../external/chromium/build/apk_test.gypi' ],
        },
        {
          'target_name': 'unit_tests_net_apk',
          'type': 'none',
          'dependencies': [
            'unit_tests_net',
          ],
          'variables': {
            'test_suite_name': 'unit_tests_net',
            'input_shlib_path': '<(SHARED_LIB_DIR)/<(SHARED_LIB_PREFIX)unit_tests_net<(SHARED_LIB_SUFFIX)',
          },
          'includes': [ '../../../external/chromium/build/apk_test.gypi' ],
        },
        {
          'target_name': 'unit_tests_sql_apk',
          'type': 'none',
          'dependencies': [
            'unit_tests_sql',
          ],
          'variables': {
            'test_suite_name': 'unit_tests_sql',
            'input_shlib_path': '<(SHARED_LIB_DIR)/<(SHARED_LIB_PREFIX)unit_tests_sql<(SHARED_LIB_SUFFIX)',
          },
          'includes': [ '../../../external/chromium/build/apk_test.gypi' ],
        },
      ],
    }],
  ], # end of conditions
}
