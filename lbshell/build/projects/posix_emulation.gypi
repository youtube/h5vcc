{
  'targets': [
    {
      'target_name': 'posix_emulation',
      'type': 'static_library',
      'include_dirs': [
        '../../src',
        '../../../external/chromium', # for general inclusion of Chromium headers
      ],
      'sources': [
        '../../src/platform/<(target_arch)/posix_emulation.h',
        '<!@(find ../../src/platform/<(target_arch)/posix_emulation/ -type f)',
      ],
    }, # end of target 'posix_emulation'
  ] # end of targets
}

