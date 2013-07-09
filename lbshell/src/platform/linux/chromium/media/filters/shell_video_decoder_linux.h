#ifndef MEDIA_FILTERS_SHELL_VIDEO_DECODER_LINUX_H_
#define MEDIA_FILTERS_SHELL_VIDEO_DECODER_LINUX_H_

#include <list>
#include <map>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop.h"
#include "media/base/demuxer_stream.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/filters/shell_demuxer.h"
#include "media/filters/shell_video_decoder.h"

namespace media {

class MEDIA_EXPORT ShellVideoDecoderLinux : public ShellVideoDecoder {
 public:
  ShellVideoDecoderLinux(
      const scoped_refptr<base::MessageLoopProxy>& message_loop);
  virtual ~ShellVideoDecoderLinux();

  // ShellVideoDecoder implementation.
  virtual void Initialize(const scoped_refptr<DemuxerStream>& stream,
                          const PipelineStatusCB& status_cb,
                          const StatisticsCB& statistics_cb) OVERRIDE;
  virtual void Read(const ReadCB& read_cb) OVERRIDE;
  virtual void Reset(const base::Closure& closure) OVERRIDE;
  virtual void Stop(const base::Closure& closure) OVERRIDE;

private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ShellVideoDecoderLinux);
};

}  // namespace media

#endif  // MEDIA_FILTERS_SHELL_PLATFORM_VIDEO_DECODER_PS3_H_
