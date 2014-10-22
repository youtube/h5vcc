// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_MEDIA_DRM_BRIDGE_H_
#define MEDIA_BASE_ANDROID_MEDIA_DRM_BRIDGE_H_

#include <jni.h>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "media/base/decryptor.h"
#include "media/base/media_export.h"

namespace media {

// This file is from upstream Chromium.  In that version, Decryptor and
// DecryptorClient seem to have been replaced by MediaKeys and
// MediaPlayerManager.  We will adapt the newer MediaDrmBridge to our
// older Decryptor interface with as few modifications as possible.
// We will also define MediaPlayerManager abstractly here, and allow our
// Decryptor adaptor to implement this and proxy the events to its
// DecryptorClient instance.

// Defined elsewhere in upstream Chromium.  Minimal abstract definition
// to receive events from MediaDrmBridge.
class MEDIA_EXPORT MediaPlayerManager {
 public:
  // Called when MediaDrmBridge determines a SessionId.
  virtual void OnSessionCreated(int media_keys_id,
                                uint32 reference_id,
                                const std::string& session_id) = 0;

  // Called when MediaDrmBridge wants to send a Message event.
  virtual void OnSessionMessage(int media_keys_id,
                                uint32 reference_id,
                                const std::vector<uint8>& message,
                                const std::string& destination_url) = 0;

  // Called when MediaDrmBridge wants to send a Ready event.
  virtual void OnSessionReady(int media_keys_id,
                              uint32 reference_id) = 0;

  // Called when MediaDrmBridge wants to send an Error event.
  virtual void OnSessionError(int media_keys_id,
                              uint32 reference_id,
                              Decryptor::KeyError error_code,
                              int system_code) = 0;
};

// This class provides DRM services for the android EME implementation.
// We will apply an adaptor on top of this to connect with media::Decryptor.
class MEDIA_EXPORT MediaDrmBridge {
 public:
  enum SecurityLevel {
    SECURITY_LEVEL_NONE = 0,
    SECURITY_LEVEL_1 = 1,
    SECURITY_LEVEL_3 = 3,
  };

  typedef base::Callback<void(bool)> ResetCredentialsCB;

  virtual ~MediaDrmBridge();

  // Returns a MediaDrmBridge instance if |scheme_uuid| is supported, or a NULL
  // pointer otherwise.
  static scoped_ptr<MediaDrmBridge> Create(
      int media_keys_id,
      const std::vector<uint8>& scheme_uuid,
      const std::string& security_level,
      MediaPlayerManager* manager);

  // Checks whether MediaDRM is available.
  static bool IsAvailable();

  static bool IsSecurityLevelSupported(const std::vector<uint8>& scheme_uuid,
                                       const std::string& security_level);

  static bool IsCryptoSchemeSupported(const std::vector<uint8>& scheme_uuid,
                                      const std::string& container_mime_type);

  static bool IsSecureDecoderRequired(const std::string& security_level_str);

  static bool RegisterMediaDrmBridge(JNIEnv* env);

  // Modified MediaKeys implementation.
  bool CreateSession(uint32 reference_id,
                     const std::string& type,
                     const uint8* init_data,
                     int init_data_length);
  void UpdateSession(uint32 reference_id,
                     const uint8* response,
                     int response_length);
  void ReleaseSession(uint32 reference_id);

  // Returns a MediaCrypto object if it's already created. Returns a null object
  // otherwise.
  base::android::ScopedJavaLocalRef<jobject> GetMediaCrypto();

  // Sets callback which will be called when MediaCrypto is ready.
  // If |closure| is null, previously set callback will be cleared.
  void SetMediaCryptoReadyCB(const base::Closure& closure);

  // Called after a MediaCrypto object is created.
  void OnMediaCryptoReady(JNIEnv* env, jobject);

  // Called after we got the response for GenerateKeyRequest().
  void OnKeyMessage(JNIEnv* env, jobject, jstring j_session_id,
                    jbyteArray message, jstring destination_url);

  // Called when key is added.
  void OnKeyAdded(JNIEnv* env, jobject, jstring j_session_id);

  // Called when error happens.
  void OnKeyError(JNIEnv* env, jobject, jstring j_session_id);

  // Reset the device credentials.
  void ResetDeviceCredentials(const ResetCredentialsCB& callback);

  // Called by the java object when credential reset is completed.
  void OnResetDeviceCredentialsCompleted(JNIEnv* env, jobject, bool success);

  // Helper function to determine whether a protected surface is needed for the
  // video playback.
  bool IsProtectedSurfaceRequired();

  int media_keys_id() const { return media_keys_id_; }

 private:
  // Map between session_id and reference_id.
  typedef std::map<uint32_t, std::string> SessionMap;

  static bool IsSecureDecoderRequired(SecurityLevel security_level);

  MediaDrmBridge(int media_keys_id,
                 const std::vector<uint8>& scheme_uuid,
                 const std::string& security_level,
                 MediaPlayerManager* manager);

  // Get the security level of the media.
  SecurityLevel GetSecurityLevel();

  // Determine the corresponding reference_id for |session_id|.
  uint32_t DetermineReferenceId(const std::string& session_id);

  // Determine the corresponding session_id for |reference_id|. The returned
  // value is only valid on the main thread, and should be stored by copy.
  const std::string& LookupSessionId(uint32_t reference_id);

  // ID of the MediaKeys object.
  int media_keys_id_;

  // UUID of the key system.
  std::vector<uint8> scheme_uuid_;

  // Java MediaDrm instance.
  base::android::ScopedJavaGlobalRef<jobject> j_media_drm_;

  // Non-owned pointer.
  MediaPlayerManager* manager_;

  base::Closure media_crypto_ready_cb_;

  ResetCredentialsCB reset_credentials_cb_;

  SessionMap session_map_;

  // As the response from GenerateKeyRequest() will be asynchronous, add this
  // request to a queue and assume that the subsequent responses come back in
  // the order issued.
  // TODO(jrummell): Remove once the Java interface supports reference_id.
  std::queue<uint32_t> pending_key_request_reference_ids_;

  DISALLOW_COPY_AND_ASSIGN(MediaDrmBridge);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_MEDIA_DRM_BRIDGE_H_
