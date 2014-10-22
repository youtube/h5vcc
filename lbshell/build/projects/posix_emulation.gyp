{
  'targets': [
    {
      'target_name': 'posix_emulation',
      'toolsets': ['target'],
      'type': '<(posix_emulation_target_type)',
      # steel_build_id generates a header that lots of things might depend on.
      # Force posix_emulation to build first.
      'hard_dependency': 1,
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '../../src',
        '../../../external/chromium', # for general inclusion of Chromium headers
      ],
      'sources': [
        '../../src/platform/<(target_arch)/posix_emulation.h',
        '<!@(find ../../src/platform/<(target_arch)/posix_emulation/ -type f)',
        '../../src/lb_console_values.cc',
        '../../src/lb_console_values.h',
        '../../src/lb_globals.cc',
        '../../src/lb_globals.h',
        '../../src/lb_log_writer.cc',
        '../../src/lb_log_writer.h',
        '../../src/lb_memory_debug.cc',
        '../../src/lb_memory_internal.h',
        '../../src/lb_memory_manager.cc',
        '../../src/lb_memory_manager.h',
        '../../src/lb_new.cc',
        '../../src/oom_png.cc',
        '../../src/oom_png.h',
        '../../src/oom_fprintf.cc',
        '../../src/oom_fprintf.h',
      ],
      'defines': [
        'LB_BASE_IMPLEMENTATION=1',
      ],
      'dependencies': [
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(DEPTH)/../dlmalloc/dlmalloc.gyp:dlmalloc',
        'steel_build_id.gyp:steel_build_id',
      ],
      'conditions': [
        ['target_arch=="xb360"', {
          'sources': [
            '<(DEPTH)/../pthreads-win32/pthread.c',
          ]
        }],
        ['target_arch=="xb1"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ComponentExtensions': 'true'
            },
          },
        }],
      ],
    }, # end of target 'posix_emulation'
  ] # end of targets
}
