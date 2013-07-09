#ifndef MEDIA_FILTERS_SHELL_AUDIO_RENDERER_LINUX_H_
#define MEDIA_FILTERS_SHELL_AUDIO_RENDERER_LINUX_H_

#include "base/callback.h"
#include "base/message_loop.h"
#include "media/base/audio_decoder.h"
#include "media/base/shell_filter_graph_log.h"
#include "media/filters/shell_audio_renderer.h"

namespace media {

class MEDIA_EXPORT ShellAudioRendererLinux : public ShellAudioRenderer {
 public:
  explicit ShellAudioRendererLinux(
      media::AudioRendererSink* sink,
      const SetDecryptorReadyCB& set_decryptor_ready_cb,
      const scoped_refptr<base::MessageLoopProxy>& message_loop);

  // ======== AudioRenderer Implementation

  // Initialize a AudioRenderer with the given AudioDecoder, executing the
  // |init_cb| upon completion.
  //
  // |statistics_cb| is executed periodically with audio rendering stats.
  //
  // |underflow_cb| is executed when the renderer runs out of data to pass to
  // the audio card during playback. ResumeAfterUnderflow() must be called
  // to resume playback. Pause(), Preroll(), or Stop() cancels the underflow
  // condition.
  //
  // |time_cb| is executed whenever time has advanced by way of audio rendering.
  //
  // |ended_cb| is executed when audio rendering has reached the end of stream.
  //
  // |disabled_cb| is executed when audio rendering has been disabled due to
  // external factors (i.e., device was removed). |time_cb| will no longer be
  // executed.
  //
  // |error_cb| is executed if an error was encountered.
  virtual void Initialize(const scoped_refptr<DemuxerStream>& stream,
                          const AudioDecoderList& decoders,
                          const PipelineStatusCB& init_cb,
                          const StatisticsCB& statistics_cb,
                          const base::Closure& underflow_cb,
                          const TimeCB& time_cb,
                          const base::Closure& ended_cb,
                          const base::Closure& disabled_cb,
                          const PipelineStatusCB& error_cb) OVERRIDE;

  // Start prerolling audio data for samples starting at |time|, executing
  // |callback| when completed.
  //
  // Only valid to call after a successful Initialize() or Flush().
  virtual void Preroll(base::TimeDelta time,
                       const PipelineStatusCB& callback) OVERRIDE;

  // Sets the output volume.
  virtual void SetVolume(float volume) OVERRIDE;

  // Resumes playback after underflow occurs.
  //
  // |buffer_more_audio| is set to true if you want to increase the size of the
  // decoded audio buffer.
  virtual void ResumeAfterUnderflow(bool buffer_more_audio) OVERRIDE;

  // The pipeline has resumed playback.  Filters can continue requesting reads.
  // Filters may implement this method if they need to respond to this call.
  virtual void Play(const base::Closure& callback) OVERRIDE;

  // The pipeline has paused playback.  Filters should stop buffer exchange.
  // Filters may implement this method if they need to respond to this call.
  virtual void Pause(const base::Closure& callback) OVERRIDE;

  // The pipeline has been flushed.  Filters should return buffer to owners.
  // Filters may implement this method if they need to respond to this call.
  virtual void Flush(const base::Closure& callback) OVERRIDE;

  // The pipeline is being stopped either as a result of an error or because
  // the client called Stop().
  virtual void Stop(const base::Closure& callback) OVERRIDE;

  // The pipeline playback rate has been changed.  Filters may implement this
  // method if they need to respond to this call.
  virtual void SetPlaybackRate(float playback_rate) OVERRIDE;

  virtual void SinkFull() OVERRIDE;

  // Carry out any actions required to seek to the given time, executing the
  // callback upon completion.
  //virtual void Seek(base::TimeDelta time,
  //                  const PipelineStatusCB& callback) OVERRIDE;

  // ======== ShellAudioRenderer Implementation
  virtual scoped_refptr<ShellFilterGraphLog> filter_graph_log() OVERRIDE;

 protected:
  virtual ~ShellAudioRendererLinux();

  // media::AudioRendererSink::RenderCallback implementation.
  // Attempts to completely fill all channels of |dest|, returns actual
  // number of frames filled.
  // Render() is run on the sink's thread.
  virtual int Render(AudioBus* dest, int audio_delay_milliseconds) OVERRIDE;
  virtual void OnRenderError() OVERRIDE;

  void DecodedAudioReady(AudioDecoder::Status status,
                         const scoped_refptr<Buffer>& buffer);

  // objects communicating with the rest of the filter graph
  scoped_refptr<base::MessageLoopProxy> message_loop_;
  scoped_refptr<media::AudioRendererSink> sink_;
  scoped_refptr<ShellFilterGraphLog> filter_graph_log_;
};

}  // namespace media

#endif  // MEDIA_FILTERS_SHELL_AUDIO_RENDERER_LINUX_H_
