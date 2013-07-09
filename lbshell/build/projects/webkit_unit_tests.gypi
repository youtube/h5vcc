# We made some changes (and will make more) in webcore/webkit and we'd like
# to add our own unittests to test these changes.
{
  'targets': [
    {
      'target_name': 'unit_tests_image_decoder',
      'type': 'executable',
      'sources': [
        '../../src/lb_run_all_unittests.cc',
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/tests/ImageDecoderShellTest.cpp',
      ],
      'defines' : [
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/base/base.gyp:test_support_base',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/skia/skia.gyp:skia',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/WebCore.gyp/WebCore.gyp:webcore'
      ],
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '<(DEPTH)/testing/gtest/include',
        '<(SHARED_INTERMEDIATE_DIR)',
        '<(SHARED_INTERMEDIATE_DIR)/inspector',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
    },
    {
      'target_name': 'unit_tests_xml_parser',
      'type': 'executable',
      'sources': [
        '../../src/lb_run_all_unittests.cc',
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/tests/XMLDocumentParserTest.cpp',
      ],
      'defines' : [
      ],
      'dependencies': [
        'lb_shell_lib',
        '<(DEPTH)/base/base.gyp:test_support_base',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/libxml/libxml.gyp:libxml',
        '<(DEPTH)/third_party/WebKit/Source/WebCore/WebCore.gyp/WebCore.gyp:webcore',
      ],
      'include_dirs': [
        '../../../', # for our explicit external/ inclusion of headers
        '<(DEPTH)/testing/gtest/include',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'libraries': [
        '<@(platform_libraries)',
      ],
    },
  ],
}
