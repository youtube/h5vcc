{
  'targets': [
    {
      'target_name': 'wtf_symbols',
      'type': 'static_library',
      'include_dirs': [
        '../../src',
        '../../../external/chromium/third_party/icu/public/common',
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore', # for config.h
        '../../../external/chromium/third_party/WebKit/Source/JavaScriptCore/runtime', # to satisfy config.h deps
        '../../../external/chromium/third_party/WebKit/Source/WebCore/platform',
        '../../../external/chromium/third_party/WebKit/Source/WTF', # for Platform.h
        '<(SHARED_INTERMEDIATE_DIR)/webkit',
      ],
      'sources': [
        '<!@(find ../../src/platform/<(target_arch)/wtf/ -type f)',
      ],
    }, # end of target 'wtf_symbols'
  ] # end of targets
}

