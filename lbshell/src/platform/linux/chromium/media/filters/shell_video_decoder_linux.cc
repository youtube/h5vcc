#include "media/filters/shell_video_decoder_linux.h"

#include <limits.h>  // for ULLONG_MAX

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "lb_web_graphics_context_3d_linux.h"
#include "media/base/pipeline_status.h"
#include "media/base/shell_buffer_factory.h"
#include "media/base/video_frame.h"

namespace media {

//==============================================================================
// Static ShellVideoDecoder Methods
//

// static
ShellVideoDecoder* ShellVideoDecoder::Create(
    const scoped_refptr<base::MessageLoopProxy>& message_loop) {
  return new ShellVideoDecoderLinux(message_loop);
}

//==============================================================================
// ShellVideoDecoderLinux
//

ShellVideoDecoderLinux::ShellVideoDecoderLinux(
    const scoped_refptr<base::MessageLoopProxy>& message_loop) {
}

ShellVideoDecoderLinux::~ShellVideoDecoderLinux() {
  NOTIMPLEMENTED();
}

void ShellVideoDecoderLinux::Initialize(
    const scoped_refptr<DemuxerStream> &stream,
    const media::PipelineStatusCB &status_cb,
    const media::StatisticsCB &statistics_cb) {

}

void ShellVideoDecoderLinux::Read(const ReadCB& read_cb) {
}

void ShellVideoDecoderLinux::Reset(const base::Closure& closure) {
}


void ShellVideoDecoderLinux::Stop(const base::Closure& closure) {
}

}  // namespace media

