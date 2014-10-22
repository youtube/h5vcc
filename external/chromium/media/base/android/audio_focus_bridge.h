#ifndef MEDIA_BASE_ANDROID_AUDIO_FOCUS_BRIDGE_H_
#define MEDIA_BASE_ANDROID_AUDIO_FOCUS_BRIDGE_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "media/base/media_export.h"

namespace media {

// This class serves as a bridge for native code to call focus related functions
// of android AudioManager class.
class MEDIA_EXPORT AudioFocusBridge {
 public:
  AudioFocusBridge();
  ~AudioFocusBridge();

  void RequestAudioFocus();
  void AbandonAudioFocus();

  static bool RegisterAudioFocusBridge(JNIEnv* env);

 private:
  base::android::ScopedJavaGlobalRef<jobject> j_audio_focus_;

  DISALLOW_COPY_AND_ASSIGN(AudioFocusBridge);
};

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_AUDIO_FOCUS_BRIDGE_H_

