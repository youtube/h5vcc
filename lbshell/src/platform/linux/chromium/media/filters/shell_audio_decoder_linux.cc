#include "media/filters/shell_audio_decoder_linux.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "base/time.h"
#include "lb_memory_manager.h"
#include "media/base/shell_buffer_factory.h"

namespace media {

//==============================================================================
// Static ShellAudioDecoder Methods
//

// static
ShellAudioDecoder* ShellAudioDecoder::Create(
    const scoped_refptr<base::MessageLoopProxy>& message_loop) {
  return new ShellAudioDecoderLinux(message_loop);
}

//==============================================================================
// ShellAudioDecoderLinux
//
ShellAudioDecoderLinux::ShellAudioDecoderLinux(
    const scoped_refptr<base::MessageLoopProxy>& message_loop) {
}

ShellAudioDecoderLinux::~ShellAudioDecoderLinux() {
}

void ShellAudioDecoderLinux::Initialize(
    const scoped_refptr<DemuxerStream> &stream,
    const media::PipelineStatusCB &status_cb,
    const media::StatisticsCB &statistics_cb) {
}


void ShellAudioDecoderLinux::Read(const media::AudioDecoder::ReadCB &read_cb) {
}



int ShellAudioDecoderLinux::bits_per_channel() {
  return 0;
}

ChannelLayout ShellAudioDecoderLinux::channel_layout() {
  return ChannelLayout();
}

int ShellAudioDecoderLinux::samples_per_second() {
  return 0;
}

void ShellAudioDecoderLinux::Reset(const base::Closure& closure) {
}

}  // namespace media
