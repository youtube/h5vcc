package org.chromium.media;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

// This class implements the OnPlaybackPositionUpdateListener interface for android AudioTrack.
// Callbacks will be sent to the native class for processing.
@JNINamespace("media")
class AudioTrackBridge {
    private static final String TAG = "AudioTrackBridge";

    private static final int kAudioBufferSizeInBytes = 128 * 1024;
    private int mNativeAudioTrackBridge = 0;
    private AudioTrack mAudioTrack;

    private AudioTrackBridge(int nativeAudioTrackBridge, int sampleRate, int channelCount) {
        mNativeAudioTrackBridge = nativeAudioTrackBridge;
        int channelConfig = (channelCount == 1) ? AudioFormat.CHANNEL_OUT_MONO :
            AudioFormat.CHANNEL_OUT_STEREO;
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig,
            AudioFormat.ENCODING_PCM_16BIT, kAudioBufferSizeInBytes, AudioTrack.MODE_STREAM);
    }

    @CalledByNative
    private static AudioTrackBridge create(int nativeAudioTrackBridge,
                                           int sampleRate, int channelCount) {
        return new AudioTrackBridge(nativeAudioTrackBridge, sampleRate, channelCount);
    }

    @CalledByNative
    private void release() {
        mAudioTrack.release();
        mAudioTrack = null;
    }

    @CalledByNative
    private void play() {
        mAudioTrack.play();
    }

    @CalledByNative
    private void flush() {
        mAudioTrack.flush();
    }

    @CalledByNative
    private void pause() {
        mAudioTrack.pause();
    }

    @CalledByNative
    private int write(byte[] buf) {
        return mAudioTrack.write(buf, 0, buf.length);
    }

    @CalledByNative
    private int getPlaybackHeadPosition() {
        return mAudioTrack.getPlaybackHeadPosition();
    }

    @CalledByNative
    private void setVolume(double volume) {
        mAudioTrack.setStereoVolume((float)volume, (float)volume);
    }

    @CalledByNative
    private static int getAudioBufferSizeInBytes() {
        return kAudioBufferSizeInBytes;
    }
}
