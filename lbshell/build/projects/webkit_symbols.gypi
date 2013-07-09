{
  'targets': [
    {
      'target_name': 'webkit_symbols',
      'type': 'static_library',
      'include_dirs': [
        '<(DEPTH)/third_party/skia/include/config',
        '<(DEPTH)/third_party/skia/include/core',
        '<(DEPTH)/third_party/WebKit/Source/WebCore',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/platform',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/platform/graphics',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/platform/graphics/skia',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/platform/image-decoders',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/platform/image-decoders/shell',
        '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore', # for config.h
        '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/runtime', # to satisfy config.h deps
        '<(DEPTH)/third_party/WebKit/Source/WTF', # for Platform.h
        '<(DEPTH)/third_party/WebKit/Source/WTF/icu',
      ],
      'conditions': [
        # if any native image decoder is enabled, file in ../../src/platform/<(target_arch)/webkit/image-decoders/ should be included.
        ['"__LB_ENABLE_SHELL_NATIVE_JPEG_DECODER__=1" in image_decoder_defines', {
          'sources': [
            '../../src/platform/<(target_arch)/webkit/image-decoders/JPEGImageReaderNative.cpp',
          ]
        }],
        ['"__LB_ENABLE_SHELL_NATIVE_PNG_DECODER__=1" in image_decoder_defines', {
          'sources': [
            '../../src/platform/<(target_arch)/webkit/image-decoders/PNGImageReaderNative.cpp',
          ],
        }],
      ],
    }, # end of target 'webkit_symbols'
  ] # end of targets
}

