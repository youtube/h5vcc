{
  'targets': [
    {
      'target_name': 'chrome_symbols',
      'type': 'static_library',
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '../../src',
        '../../src/platform/<(target_arch)/chromium',
        '../../src/platform/<(target_arch)/lb_shell',
        '../../../external/chromium',
        '../../../external/chromium/third_party/WebKit/Source/WTF',
        '<(SHARED_INTERMEDIATE_DIR)/ui/gfx',
      ],
      'dependencies': [
        'lb_shell_resources',
        '<(DEPTH)/webkit/support/webkit_support.gyp:webkit_resources',
      ],
      'defines' : [
        '__DISABLE_WTF_LOGGING__',
      ],
      'sources': [
        '<!@(find ../../src/platform/<(target_arch)/chromium/ -type f)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../src/platform/<(target_arch)/chromium',
        ],
      },
    }, # end of target 'chrome_symbols'
  ] # end of targets
}
