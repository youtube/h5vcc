# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'conditions': [
      ['OS == "android" or OS == "ios" or OS == "lb_shell"', {
        # Android and iOS don't use ffmpeg.
        'use_ffmpeg%': 0,
      }, {  # 'OS != "android" and OS != "ios" and OS != "lb_shell"'
        'use_ffmpeg%': 1,
      }],
    ],
    # Set |use_fake_video_decoder| to 1 to ignore input frames in |clearkeycdm|,
    # and produce video frames filled with a solid color instead.
    'use_fake_video_decoder%': 0,
    # Set |use_libvpx| to 1 to use libvpx for VP8 decoding in |clearkeycdm|.
    'use_libvpx%': 0,
  },
  'targets': [
    {
      'target_name': 'webkit_media',
      'type': 'static_library',
      'variables': { 'enable_wexit_time_destructors': 1, },
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',  # Needed by key_systems.cc.
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '<(DEPTH)/media/media.gyp:shared_memory_support',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/third_party/widevine/cdm/widevine_cdm.gyp:widevine_cdm_version_h',
      ],
      'sources': [
        'android/audio_decoder_android.cc',
        'android/media_player_bridge_manager_impl.cc',
        'android/media_player_bridge_manager_impl.h',
        'android/stream_texture_factory_android.h',
        'android/webmediaplayer_android.cc',
        'android/webmediaplayer_android.h',
        'android/webmediaplayer_impl_android.cc',
        'android/webmediaplayer_impl_android.h',
        'android/webmediaplayer_in_process_android.cc',
        'android/webmediaplayer_in_process_android.h',
        'android/webmediaplayer_manager_android.cc',
        'android/webmediaplayer_manager_android.h',
        'android/webmediaplayer_proxy_android.cc',
        'android/webmediaplayer_proxy_android.h',
        'active_loader.cc',
        'active_loader.h',
        'audio_decoder.cc',
        'audio_decoder.h',
        'audio_decoder_shell.cc',
        'buffered_data_source.cc',
        'buffered_data_source.h',
        'buffered_resource_loader.cc',
        'buffered_resource_loader.h',
        'cache_util.cc',
        'cache_util.h',
        'crypto/key_systems.cc',
        'crypto/key_systems.h',
        'crypto/key_systems_shell.cc',
        'crypto/ppapi_decryptor.cc',
        'crypto/ppapi_decryptor.h',
        'crypto/proxy_decryptor.cc',
        'crypto/proxy_decryptor.h',
        'filter_helpers.cc',
        'filter_helpers.h',
        'filter_helpers_shell.cc',
        'media_stream_audio_renderer.cc',
        'media_stream_audio_renderer.h',
        'media_stream_client.h',
        'preload.h',
        'shell_audio_file_reader.cc',
        'shell_audio_file_reader.h',
        'shell_audio_file_reader_aiff.cc',
        'shell_audio_file_reader_aiff.h',
        'shell_audio_file_reader_wav.cc',
        'shell_audio_file_reader_wav.h',
        'simple_video_frame_provider.cc',
        'simple_video_frame_provider.h',
        'video_frame_provider.cc',
        'video_frame_provider.h',
        'webmediaplayer_delegate.h',
        'webmediaplayer_impl.cc',
        'webmediaplayer_impl.h',
        'webmediaplayer_ms.cc',
        'webmediaplayer_ms.h',
        'webmediaplayer_proxy.cc',
        'webmediaplayer_proxy.h',
        'webmediaplayer_util.cc',
        'webmediaplayer_util.h',
        'webvideoframe_impl.cc',
        'webvideoframe_impl.h',
      ],
      'conditions': [
        ['inside_chromium_build == 0', {
          'dependencies': [
            '<(DEPTH)/webkit/support/setup_third_party.gyp:third_party_headers',
          ],
        }],
        ['OS == "lb_shell"', {
          'sources/': [
            # exclude everything to start
            ['exclude', '.*'],
            # exclude the contents of the crypto directory
            ['exclude', 'crypto/'],
            # but include the webmediaplayer stuff
            ['include', 'webmediaplayer'],
            # we use our own filter helpers
            ['include', 'filter_helpers_shell'],
            # probably going to want to use our own buffering logic but for now..
            ['include', 'buffered'],
            # and also deprecating this soon but again for now..
            ['include', 'skcanvas'],
            # can we do this one?
            ['include' , 'webvideoframe'],
            # or this one?
            ['include', 'active_loader'],
            # basically the whole kitchen sink now
            ['include', 'cache_util.cc'],
            # we use our own audio decoder
            ['include', 'audio_decoder_shell'],
            # we reimplement the key system checks from crypto
            ['include', 'crypto/key_systems.h'],
            ['include', 'crypto/key_systems_shell.cc'],
            # and the proxy decryptor which instantiates real decryptors
            ['include', 'crypto/proxy_decryptor'],
            # naturally include everything starting with the name shell
            ['include', 'shell_'],
          ],
        }, { # OS != lb_shell
          'dependencies': [
            '<(DEPTH)/media/media.gyp:yuv_convert',
          ],
          'sources!': [
            'audio_decoder_shell.cc',
            'filter_helpers_shell.cc',
            'filter_helpers_shell.h',
            'key_systems_shell.cc',
          ],
        }],
        ['OS == "android"', {
          'sources!': [
            'audio_decoder.cc',
            'audio_decoder.h',
            'filter_helpers.cc',
            'filter_helpers.h',
            'webmediaplayer_impl.cc',
            'webmediaplayer_impl.h',
            'webmediaplayer_proxy.cc',
            'webmediaplayer_proxy.h',
          ],
          'dependencies': [
            '<(DEPTH)/media/media.gyp:player_android',
          ],
        }, { # OS != "android"'
          'sources/': [
            ['exclude', '^android/'],
          ],
        }],
      ],
    },
    {
      'target_name': 'clearkeycdm',
      'type': 'none',
      # TODO(tomfinegan): Simplify this by unconditionally including all the
      # decoders, and changing clearkeycdm to select which decoder to use
      # based on environment variables.
      'conditions': [
        ['use_fake_video_decoder == 1' , {
          'defines': ['CLEAR_KEY_CDM_USE_FAKE_VIDEO_DECODER'],
          'sources': [
            'crypto/ppapi/fake_cdm_video_decoder.cc',
            'crypto/ppapi/fake_cdm_video_decoder.h',
          ],
        }],
        ['use_ffmpeg == 1'  , {
          'defines': ['CLEAR_KEY_CDM_USE_FFMPEG_DECODER'],
          'dependencies': [
            '<(DEPTH)/third_party/ffmpeg/ffmpeg.gyp:ffmpeg',
          ],
          'sources': [
            'crypto/ppapi/ffmpeg_cdm_audio_decoder.cc',
            'crypto/ppapi/ffmpeg_cdm_audio_decoder.h',
          ],
        }],
        ['use_ffmpeg == 1 and use_fake_video_decoder == 0'  , {
          'sources': [
            'crypto/ppapi/ffmpeg_cdm_video_decoder.cc',
            'crypto/ppapi/ffmpeg_cdm_video_decoder.h',
          ],
        }],
        ['use_libvpx == 1 and use_fake_video_decoder == 0' , {
          'defines': ['CLEAR_KEY_CDM_USE_LIBVPX_DECODER'],
          'dependencies': [
            '<(DEPTH)/third_party/libvpx/libvpx.gyp:libvpx',
          ],
          'sources': [
            'crypto/ppapi/libvpx_cdm_video_decoder.cc',
            'crypto/ppapi/libvpx_cdm_video_decoder.h',
          ],
        }],
        ['os_posix == 1 and OS != "mac"', {
          'type': 'loadable_module',  # Must be in PRODUCT_DIR for ASAN bots.
        }, {  # 'os_posix != 1 or OS == "mac"'
          'type': 'shared_library',
        }],
      ],
      'defines': ['CDM_IMPLEMENTATION'],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/media/media.gyp:media',
      ],
      'sources': [
        'crypto/ppapi/cdm_video_decoder.cc',
        'crypto/ppapi/cdm_video_decoder.h',
        'crypto/ppapi/clear_key_cdm.cc',
        'crypto/ppapi/clear_key_cdm.h',
      ],
    },
    {
      'target_name': 'clearkeycdmplugin',
      'type': 'none',
      'dependencies': [
        '<(DEPTH)/ppapi/ppapi.gyp:ppapi_cpp',
        'clearkeycdm',
      ],
      'sources': [
        'crypto/ppapi/cdm_wrapper.cc',
        'crypto/ppapi/cdm/content_decryption_module.h',
        'crypto/ppapi/linked_ptr.h',
      ],
      'conditions': [
        ['os_posix == 1 and OS != "mac"', {
          'cflags': ['-fvisibility=hidden'],
          'type': 'loadable_module',
          # Allow the plugin wrapper to find the CDM in the same directory.
          'ldflags': ['-Wl,-rpath=\$$ORIGIN'],
          'libraries': [
            # Built by clearkeycdm.
            '<(PRODUCT_DIR)/libclearkeycdm.so',
          ],
        }],
        ['OS == "win"', {
          'type': 'shared_library',
        }],
        ['OS == "mac"', {
          'type': 'loadable_module',
          'mac_bundle': 1,
          'product_extension': 'plugin',
          'xcode_settings': {
            'OTHER_LDFLAGS': [
              # Not to strip important symbols by -Wl,-dead_strip.
              '-Wl,-exported_symbol,_PPP_GetInterface',
              '-Wl,-exported_symbol,_PPP_InitializeModule',
              '-Wl,-exported_symbol,_PPP_ShutdownModule'
            ]},
        }],
      ],
    }
  ],
}
