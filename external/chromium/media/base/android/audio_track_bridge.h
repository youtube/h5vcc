#ifndef MEDIA_BASE_ANDROID_AUDIO_TRACK_BRIDGE_H_
#define MEDIA_BASE_ANDROID_AUDIO_TRACK_BRIDGE_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/time.h"
#include "base/threading/thread.h"
#include "media/base/media_export.h"

namespace media {

// This class serves as a bridge for native code to call java functions inside
// android AudioTrack class. For more information on android AudioTrack, check
// http://developer.android.com/reference/android/media/AudioTrack.html.
class MEDIA_EXPORT AudioTrackBridge {
 public:
  typedef base::Callback<int (const uint8*, uint32)> WriteFn;
  typedef base::Callback<void (const WriteFn&)> PeriodicNotificationCB;

  // Construct a AudioTrackBridge object with all the needed media player
  // callbacks. This object needs to call |manager|'s RequestMediaResources()
  // before decoding the media stream. This allows |manager| to track
  // unused resources and free them when needed. On the other hand, it needs
  // to call ReleaseMediaResources() when it is done with decoding.
  AudioTrackBridge(const PeriodicNotificationCB& periodic_notification_cb,
                   int sample_rate, int channel_count);
  ~AudioTrackBridge();

  void Play();
  void Pause();
  int GetPlaybackHeadPosition();
  void SetVolume(double volume);

  static int GetAudioBufferSizeInBytes();
  static bool RegisterAudioTrackBridge(JNIEnv* env);

 private:
  static const int kAudioCallbackPeriodInMilliseconds = 10;

  void PeriodicNotification();
  int Write(const uint8* buffer, uint32 size);

  // Java AudioTrack instance.
  base::android::ScopedJavaGlobalRef<jobject> j_audio_track_;
  PeriodicNotificationCB periodic_notification_cb_;
  base::Thread thread_;
  base::TimeDelta audio_callback_interval_;

  DISALLOW_COPY_AND_ASSIGN(AudioTrackBridge);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_AUDIO_TRACK_BRIDGE_H_
