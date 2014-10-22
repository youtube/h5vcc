#include "media/base/android/audio_focus_bridge.h"

#include "base/logging.h"
#include "jni/AudioFocusBridge_jni.h"

using base::android::AttachCurrentThread;

namespace media {

AudioFocusBridge::AudioFocusBridge() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  jobject j_context = base::android::GetApplicationContext();
  j_audio_focus_.Reset(Java_AudioFocusBridge_create(env, j_context));
}

AudioFocusBridge::~AudioFocusBridge() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  Java_AudioFocusBridge_release(env, j_audio_focus_.obj());
}

void AudioFocusBridge::RequestAudioFocus() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  Java_AudioFocusBridge_requestAudioFocus(env, j_audio_focus_.obj());
}

void AudioFocusBridge::AbandonAudioFocus() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  Java_AudioFocusBridge_abandonAudioFocus(env, j_audio_focus_.obj());
}

bool AudioFocusBridge::RegisterAudioFocusBridge(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace media

