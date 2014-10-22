{
  'variables': {
    'libvpx_source': '<(DEPTH)/third_party/libvpx',
  },
  'targets': [
    {
      'target_name': 'libvpx',
      'type': 'static_library',
      'conditions': [
        ['target_arch == "ps4"', {
          'variables': {
            'use_system_yasm': '1',
            'yasm_output_path': '<(SHARED_INTERMEDIATE_DIR)/third_party/libvpx',
            'yasm_flags': [
              '-f', 'elf64',
              '-I', '<(libvpx_source)',
              '-I', '<(libvpx_source)/platforms/<(target_arch)',
            ],
          },
          'includes': [
            '../yasm/yasm_compile.gypi'
          ],
          'include_dirs': [
            '<(libvpx_source)',
            '<(libvpx_source)/platforms/<(target_arch)',
            '<(libvpx_source)/vpx_mem/memory_manager/include/',
            '<(libvpx_source)/third_party/libyuv/include',
          ],
        }],
      ],
      'sources': [
        '<!@(find <(libvpx_source) -type f -name \'md5_utils.c\')',
        '<!@(find <(libvpx_source) -type f -name \'vpxdec.c\')',
        '<!@(find <(libvpx_source) -type f -name \'y4menc.c\')',
        '<!@(find <(libvpx_source) -type f -name \'y4minput.c\')',

        # VP9 decoder sources
        '<!@(find <(libvpx_source)/vp9 -maxdepth 1 -type f -name \'*.c\')',
        '<!@(find <(libvpx_source)/vp9/common -maxdepth 1 -type f -name \'*.c\')',
        '<!@(find <(libvpx_source)/vp9/decoder -maxdepth 1 -type f -name \'*.c\')',

        # SIMD optimizations
        # TODO(wpwang): Split these up so they can be included by platform capability
        '<(libvpx_source)/vp9/common/x86/vp9_asm_stubs.c',
        '<!@(find <(libvpx_source)/vp9/common/x86 -maxdepth 1 -type f -name \'*mmx.asm\')',
        '<!@(find <(libvpx_source)/vp9/common/x86 -maxdepth 1 -type f -name \'*sse2.asm\')',
        '<!@(find <(libvpx_source)/vp9/common/x86 -maxdepth 1 -type f -name \'*sse2.c\')',
        '<!@(find <(libvpx_source)/vp9/common/x86 -maxdepth 1 -type f -name \'*ssse3.asm\')',
        '<!@(find <(libvpx_source)/vp9/common/x86 -maxdepth 1 -type f -name \'*ssse3.c\')',

        # VPX sources
        '<!@(find <(libvpx_source)/vpx/src -maxdepth 1 -type f -name \'*.c\')',
        '<!@(find <(libvpx_source)/vpx_mem -maxdepth 1 -type f -name \'*.c\')',
        '<!@(find <(libvpx_source)/vpx_mem/memory_manager -maxdepth 1 -type f -name \'*.c\')',
        '<!@(find <(libvpx_source)/vpx_ports -maxdepth 1 -type f -name \'*.asm\')',
        '<!@(find <(libvpx_source)/vpx_scale -maxdepth 1 -type f -name \'*.c\')',
        '<!@(find <(libvpx_source)/vpx_scale/generic -maxdepth 1 -type f -name \'*.c\')',
      ],
      'sources/': [
        ['exclude', 'svc_encodeframe'],
        ['exclude', 'vp9_cx_iface'],
      ]
    },
  ],
}
