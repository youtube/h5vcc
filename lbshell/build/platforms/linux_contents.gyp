{
  'variables': {
    'static_contents_dir': '../../content',
    'output_contents_dir': '<(PRODUCT_DIR)/content',
    'shaders_source_dir': '../../src/platform/linux/shaders',
  },
  'targets': [
    {
      'target_name': 'copy_contents_lbshell',
      'type': 'none',
      'inputs': [
        '<(static_contents_dir)/spinner.png',
        '<(SHARED_INTERMEDIATE_DIR)/repack/lb_shell.pak',
      ],
      'inputs_local': [
        '<(SHARED_INTERMEDIATE_DIR)/licenses.html',
      ],
      'inputs_shaders': [
        '<(shaders_source_dir)/fragment_reftex.glsl',
        '<(shaders_source_dir)/fragment_solid.glsl',
        '<(shaders_source_dir)/fragment_text.glsl',
        '<(shaders_source_dir)/vertex_pass.glsl',
        '<(shaders_source_dir)/vertex_spinner.glsl',
        '<(shaders_source_dir)/vertex_text.glsl',
      ],
      'copies': [
        {
          'destination': '<(output_contents_dir)',
          'files': ['<@(_inputs)'],
        },
        {
          'destination': '<(output_contents_dir)/local',
          'files': ['<@(_inputs_local)'],
        },
        {
          'destination': '<(output_contents_dir)/shaders',
          'files': ['<@(_inputs_shaders)'],
        },
      ],
    },
    # content files used by both jsc_tests and lb_shell.
    {
      'target_name': 'copy_contents_common',
      'type': 'none',
      'inputs': [
        '<(static_contents_dir)/fonts/',
      ],
      'inputs_i18n': [
        '<!@(find <(static_contents_dir)/i18n/platform/linux/*.xlb)',
      ],
      'inputs_icu': [
        '<(static_contents_dir)/icu/icudt46l/',
      ],
      'copies': [
        {
          'destination': '<(output_contents_dir)',
          'files': ['<@(_inputs)'],
        },
        {
          'destination': '<(output_contents_dir)/icu',
          'files': ['<@(_inputs_icu)'],
        },
        {
          'destination': '<(output_contents_dir)/i18n',
          'files': ['<@(_inputs_i18n)'],
        },
      ],
    },
    {
      'target_name': 'copy_unit_test_data',
      'type': 'none',
      'inputs' : [
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/tests/data/imagedecoder',
        '<(DEPTH)/third_party/WebKit/Source/WebKit/chromium/tests/data/xml',
      ],
      'copies': [
        {
          'destination':  '<(output_contents_dir)/dir_source_root/base',
          'files': ['<(DEPTH)/base/data'],
        },
        {
          'destination':  '<(output_contents_dir)/dir_source_root/net',
          'files': ['<(DEPTH)/net/data'],
        },
        {
          'destination':  '<(output_contents_dir)/dir_source_root',
          'files': ['<@(_inputs)'],
        },
      ],
    },
    {
      'target_name': 'copy_jsc_test_scripts',
      'type': 'none',
      'inputs': [
        '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/tests/mozilla',
        '<(DEPTH)/third_party/WebKit/Source/JavaScriptCore/tests/regexp/RegExpTest.data',
      ],
      'copies': [
        {
          'destination':  '<(output_contents_dir)/test_scripts',
          'files': ['<@(_inputs)'],
        },
        # V8
        {
          'destination': '<(output_contents_dir)/test_scripts/v8',
          'files': ['<(DEPTH)/v8/tools'],
        },
        {
          'destination': '<(output_contents_dir)/test_scripts/v8/test',
          'files': ['<(DEPTH)/v8/test/mjsunit'],
        },
        {
          'destination': '<(output_contents_dir)/test_scripts/v8/test/test262/data',
          'files': ['<(DEPTH)/v8/test/test262/data/console'],
        },
        {
          'destination': '<(output_contents_dir)/test_scripts/v8/test/test262/data',
          'files': ['<(DEPTH)/v8/test/test262/data/test'],
        },
        {
          'destination': '<(output_contents_dir)/test_scripts/v8/test/test262/data',
          'files': ['<(DEPTH)/v8/test/test262/data/tools'],
        },
        {
          'destination': '<(output_contents_dir)/test_scripts/v8/test/test262',
          'files': ['<(DEPTH)/v8/test/test262/harness-adapt.js'],
        },
      ],
    },
  ],
}
