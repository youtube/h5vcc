#include "media/base/android/audio_track_bridge.h"

#include "base/android/jni_array.h"
#include "base/bind.h"
#include "base/logging.h"

// Auto generated jni class for AudioTrack.class.
// Check base/android/jni_generator/golden_sample_for_tests_jni.h for example.
#include "jni/AudioTrackBridge_jni.h"

using base::android::AttachCurrentThread;
using base::Bind;

namespace media {

AudioTrackBridge::AudioTrackBridge(
    const PeriodicNotificationCB& periodic_notification_cb, int sample_rate,
    int channel_count)
    : periodic_notification_cb_(periodic_notification_cb),
      thread_("AudioTrackBridge") {
  DCHECK(!periodic_notification_cb_.is_null());
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  j_audio_track_.Reset(
      Java_AudioTrackBridge_create(env, reinterpret_cast<intptr_t>(this),
                                   sample_rate, channel_count));

  // TODO(xiaomings) : We have too many constants for this scattering around.
  // Define it once in audio_decoder_config.h and use it every where.
  static const uint64 kSamplePerAACFrame = 1024;
  // Call audio callback at three times of the frequency of consuming audio
  // frames. Use a lower frequency to simulate an underflow.
  audio_callback_interval_ = base::TimeDelta::FromMilliseconds(
      kSamplePerAACFrame * 1000 / sample_rate / 3);
  thread_.Start();
  thread_.message_loop()->PostDelayedTask(FROM_HERE,
      Bind(&AudioTrackBridge::PeriodicNotification, base::Unretained(this)),
      audio_callback_interval_);
}

AudioTrackBridge::~AudioTrackBridge() {
  thread_.Stop();
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  Java_AudioTrackBridge_release(env, j_audio_track_.obj());
}

void AudioTrackBridge::Play() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  Java_AudioTrackBridge_play(env, j_audio_track_.obj());
}

void AudioTrackBridge::Pause() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  Java_AudioTrackBridge_pause(env, j_audio_track_.obj());
}

int AudioTrackBridge::GetPlaybackHeadPosition() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  return Java_AudioTrackBridge_getPlaybackHeadPosition(env, j_audio_track_.obj());
}

void AudioTrackBridge::SetVolume(double volume) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  Java_AudioTrackBridge_setVolume(env, j_audio_track_.obj(), volume);
}

int AudioTrackBridge::GetAudioBufferSizeInBytes() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  return Java_AudioTrackBridge_getAudioBufferSizeInBytes(env);
}

bool AudioTrackBridge::RegisterAudioTrackBridge(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void AudioTrackBridge::PeriodicNotification() {
  periodic_notification_cb_.Run(base::Bind(&AudioTrackBridge::Write,
                                           base::Unretained(this)));
  thread_.message_loop()->PostDelayedTask(FROM_HERE,
      Bind(&AudioTrackBridge::PeriodicNotification, base::Unretained(this)),
      audio_callback_interval_);
}

int AudioTrackBridge::Write(const uint8* buffer, uint32 size) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  ScopedJavaLocalRef<jbyteArray> byte_array =
      base::android::ToJavaByteArray(env, buffer, size);
  return Java_AudioTrackBridge_write(env, j_audio_track_.obj(),
                                     byte_array.obj());
}

}  // namespace media
