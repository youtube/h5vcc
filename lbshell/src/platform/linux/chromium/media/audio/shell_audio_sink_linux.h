#ifndef MEDIA_AUDIO_SHELL_AUDIO_SINK_LINUX_H_
#define MEDIA_AUDIO_SHELL_AUDIO_SINK_LINUX_H_

#include "base/threading/thread.h"
#include "media/audio/shell_audio_sink.h"
#include "media/base/audio_renderer_sink.h"

namespace media {

// platform-specific implementation of an audio endpoint.
class MEDIA_EXPORT ShellAudioSinkLinux
    : public ShellAudioSink {
 public:
  ShellAudioSinkLinux();
  virtual ~ShellAudioSinkLinux();

  // AudioRendererSink implementation
  virtual void Initialize(const AudioParameters& params,
                          RenderCallback* callback) OVERRIDE;
  virtual void Start() OVERRIDE;
  virtual void Stop() OVERRIDE;
  virtual void Pause(bool flush) OVERRIDE;
  virtual void Play() OVERRIDE;
  virtual void SetPlaybackRate(float rate) OVERRIDE;
  virtual bool SetVolume(double volume) OVERRIDE;

  virtual void ResumeAfterUnderflow(bool buffer_more_audio) OVERRIDE;
};

}  // namespace media

#endif  // MEDIA_AUDIO_SHELL_AUDIO_SINK_LINUX_H_
