#include "media/filters/shell_audio_renderer_linux.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/stringprintf.h"

namespace media {

// static
ShellAudioRenderer* ShellAudioRenderer::Create(
    media::AudioRendererSink* sink,
    const SetDecryptorReadyCB& set_decryptor_ready_cb,
    const scoped_refptr<base::MessageLoopProxy>& message_loop) {
  return new ShellAudioRendererLinux(sink, set_decryptor_ready_cb, message_loop);
}

ShellAudioRendererLinux::ShellAudioRendererLinux(
    media::AudioRendererSink* sink,
    const SetDecryptorReadyCB& set_decryptor_ready_cb,
    const scoped_refptr<base::MessageLoopProxy>& message_loop)
    : message_loop_(message_loop)
    , sink_(sink) {
  DCHECK(message_loop_ != NULL);
}

ShellAudioRendererLinux::~ShellAudioRendererLinux() {
}

void ShellAudioRendererLinux::Play(const base::Closure &callback) {
  NOTIMPLEMENTED();
}

void ShellAudioRendererLinux::Pause(const base::Closure &callback) {
  NOTIMPLEMENTED();
}

void ShellAudioRendererLinux::Flush(const base::Closure &callback) {
  NOTIMPLEMENTED();
}

void ShellAudioRendererLinux::Stop(const base::Closure &callback) {
  NOTIMPLEMENTED();
}

void ShellAudioRendererLinux::SetPlaybackRate(float rate) {
  NOTIMPLEMENTED();
}

void ShellAudioRendererLinux::Initialize(
    const scoped_refptr<DemuxerStream>& stream,
    const AudioDecoderList& decoders,
    const PipelineStatusCB& init_cb,
    const StatisticsCB& statistics_cb,
    const base::Closure& underflow_cb,
    const TimeCB& time_cb,
    const base::Closure& ended_cb,
    const base::Closure& disabled_cb,
    const PipelineStatusCB& error_cb) {
}

void ShellAudioRendererLinux::Preroll(
    base::TimeDelta time,
    const PipelineStatusCB& callback) {
  NOTIMPLEMENTED();
}

// called by the Pipline to indicate that it has processed the underflow
// we announced, and that we should begin rebuffering
void ShellAudioRendererLinux::ResumeAfterUnderflow(bool buffer_more_audio) {
  NOTIMPLEMENTED();
}

void ShellAudioRendererLinux::SetVolume(float volume) {
  NOTIMPLEMENTED();
}

// CAUTION: this method will not usually execute on the renderer thread!
int ShellAudioRendererLinux::Render(AudioBus* dest,
                                    int audio_delay_milliseconds) {
  NOTIMPLEMENTED();
  return 0;
}

void ShellAudioRendererLinux::OnRenderError() {
  NOTIMPLEMENTED();
}

void ShellAudioRendererLinux::SinkFull() {
  NOTIMPLEMENTED();
}

scoped_refptr<ShellFilterGraphLog> ShellAudioRendererLinux::filter_graph_log() {
  return filter_graph_log_;
}

}  // namespace media
