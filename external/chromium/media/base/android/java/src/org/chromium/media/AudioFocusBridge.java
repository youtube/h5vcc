package org.chromium.media;

import android.content.Context;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

@JNINamespace("media")
class AudioFocusBridge {
    private final Context mContext;
    private final OnAudioFocusChangeListener mAudioFocusChangeListener;

    AudioFocusBridge(Context context) {
        mContext = context;
        // We don't handle focus change event here as it is actually managed
        // with app lifetime. The instance serves as a token only.
        mAudioFocusChangeListener = new OnAudioFocusChangeListener() {
            @Override
            public void onAudioFocusChange(int focusChange) {}
        };
    }

    @CalledByNative
    private void release() {
      abandonAudioFocus();
    }

    @CalledByNative
    private static AudioFocusBridge create(Context context) {
        return new AudioFocusBridge(context);
    }

    @CalledByNative
    void requestAudioFocus() {
        AudioManager am = (AudioManager)mContext.getSystemService(mContext.AUDIO_SERVICE);
        am.requestAudioFocus(mAudioFocusChangeListener, AudioManager.STREAM_MUSIC,
            AudioManager.AUDIOFOCUS_GAIN);
    }

    @CalledByNative
    void abandonAudioFocus() {
        AudioManager am = (AudioManager)mContext.getSystemService(mContext.AUDIO_SERVICE);
        am.abandonAudioFocus(mAudioFocusChangeListener);
    }
}
