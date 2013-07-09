{
  'targets': [
    {
      'target_name': 'skia_symbols',
      'type': 'static_library',
      'include_dirs': [
        '../../../external/chromium/skia/config', # for SkUserConfig.h
        '../../../external/chromium/third_party/skia/include/core',
        '../../../external/chromium',
        '../../../external/chromium/third_party/WebKit/Source/WTF',
        '../../../external/chromium/third_party/WebKit/Source/WTF/wtf',
      ],
      'defines' : [
        '__DISABLE_WTF_LOGGING__',
      ],
      'sources': [
        '<!@(find ../../src/platform/<(target_arch)/skia/ -type f)',
      ],
    }, # end of target 'skia_symbols'
  ] # end of targets
}
