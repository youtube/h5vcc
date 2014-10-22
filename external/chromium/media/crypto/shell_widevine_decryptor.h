/*
 * Copyright 2014 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MEDIA_CRYPTO_SHELL_WIDEVINE_DECRYPTOR_H_
#define MEDIA_CRYPTO_SHELL_WIDEVINE_DECRYPTOR_H_

#include <string>

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "media/base/decryptor.h"

namespace media {
class DecryptorClient;
}

class WidevineCdmHostFacade;

class ShellWidevineDecryptor : public media::Decryptor {
 public:
  explicit ShellWidevineDecryptor(media::DecryptorClient* decryptor_client);
  virtual ~ShellWidevineDecryptor();

  // media::Decryptor implementation.
  virtual bool GenerateKeyRequest(const std::string& key_system,
                                  const std::string& type,
                                  const uint8_t* init_data,
                                  int init_data_length) OVERRIDE;
  virtual void AddKey(const std::string& key_system,
                      const uint8_t* key,
                      int key_length,
                      const uint8_t* init_data,
                      int init_data_length,
                      const std::string& session_id) OVERRIDE;
  virtual void CancelKeyRequest(const std::string& key_system,
                                const std::string& session_id) OVERRIDE;
  virtual void RegisterKeyAddedCB(StreamType stream_type,
                                  const KeyAddedCB& key_added_cb) OVERRIDE;
  virtual void Decrypt(StreamType stream_type,
                       const scoped_refptr<media::ShellBuffer>& encrypted,
                       const DecryptCB& decrypt_cb) OVERRIDE;
  virtual void CancelDecrypt(StreamType stream_type) OVERRIDE;
  virtual void InitializeAudioDecoder(
      scoped_ptr<media::AudioDecoderConfig> config,
      const DecoderInitCB& init_cb) OVERRIDE;
  virtual void InitializeVideoDecoder(
      scoped_ptr<media::VideoDecoderConfig> config,
      const DecoderInitCB& init_cb) OVERRIDE;
  virtual void DecryptAndDecodeAudio(
      const scoped_refptr<media::ShellBuffer>& encrypted,
      const AudioDecodeCB& audio_decode_cb) OVERRIDE;
  virtual void DecryptAndDecodeVideo(
      const scoped_refptr<media::ShellBuffer>& encrypted,
      const VideoDecodeCB& video_decode_cb) OVERRIDE;
  virtual void ResetDecoder(StreamType stream_type) OVERRIDE;
  virtual void DeinitializeDecoder(StreamType stream_type) OVERRIDE;

 private:
  scoped_refptr<WidevineCdmHostFacade> host_;

  // DecryptorClient through which key events are fired.
  // Calls to the client are both cheap and thread-safe.
  media::DecryptorClient* client_;

  KeyAddedCB video_key_added_cb_;
  KeyAddedCB audio_key_added_cb_;

  DISALLOW_COPY_AND_ASSIGN(ShellWidevineDecryptor);
};

#endif  // MEDIA_CRYPTO_SHELL_WIDEVINE_DECRYPTOR_H_
