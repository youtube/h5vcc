{
  'targets': [
    {
      # This project is used to expose ShellMediaSource to the XB1 MediaEngine.
      # The XB1 MediaEngine requires to use IMFSchemeHandler to expose custom media source
      # and the object that expose IMFSchemeHandler has to be an activatable class in a dll.
      # We still want to keep the ShellMediaSource inside our main application. So this dll
      # project implements a IMFSchemeHandler that simply forward a ShellMediaSource pointer
      # passed to it to the MediaEngine.
      'target_name': 'shell_scheme_handler',
      'type': 'shared_library',
      'variables': {
        'use_posix_emulation': 0,
      },
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)/shell_scheme_handler',
      ],
      'defines' : [
        'SHELL_SCHEME_HANDLER_IMPLEMENTATION=1',
      ],
      'sources': [
        '<(lbshell_root)/src/platform/xb1/shell_scheme_handler/demuxer_source.def',
        '<(lbshell_root)/src/platform/xb1/shell_scheme_handler/demuxer_source.idl',
        '<(lbshell_root)/src/platform/xb1/shell_scheme_handler/dllmain.cc',
        '<(lbshell_root)/src/platform/xb1/shell_scheme_handler/shell_scheme_handler.cc',
        '<(lbshell_root)/src/platform/xb1/shell_scheme_handler/shell_scheme_handler.h',
        '<(lbshell_root)/src/platform/xb1/shell_scheme_handler/shell_scheme_handler_export.h',
      ],
      'msvs_settings': {
        'VCMIDLTool': {
          'OutputDirectory': '<(SHARED_INTERMEDIATE_DIR)/shell_scheme_handler',
        },
      },
    }, # end of target 'shell_scheme_handler'
  ] # end of targets
}
