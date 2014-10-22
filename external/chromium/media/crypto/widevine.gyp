# Copyright 2014 Google Inc. All Rights Reserved.
{
  'variables': {
    'oemcrypto_target': 'oec_steel/oec_steel.gyp:oec_steel',

    # Use the chromium target for protobuf.
    'protobuf_lib_type': 'target',
    'protobuf_lib': '<(DEPTH)/third_party/protobuf/protobuf.gyp:protobuf_lite',
    'protoc_dir': '<(PRODUCT_DIR)',
  },
  'target_defaults': {
    'include_dirs': [
      # Get protobuf headers from the chromium tree.
      '<(DEPTH)/third_party/protobuf/src',
    ],
    'dependencies': [
      # Depend on the locally-built protoc.
      '<(DEPTH)/../openssl/openssl.gyp:openssl',
      # Depend on the locally-built OpenSSL, used in privacy crypto.
      '<(DEPTH)/third_party/protobuf/protobuf.gyp:protoc#host',
    ],
  },
  'includes': [
    # global_config.gypi sets defaults for variables expected by cdm.gyp.
    '../../../cdm/platforms/global_config.gypi',
    # platform_settings.gypi overrides defaults per-platform.
    'oec_steel/platform_settings.gypi',
    # cdm.gyp defines wvcdm_static, referenced above by vendor_cma_backend
    '../../../cdm/cdm/cdm.gyp',
  ],  # end of includes
}
