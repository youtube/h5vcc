#ifndef MEDIA_FILTERS_SHELL_AUDIO_DECODER_LINUX_H_
#define MEDIA_FILTERS_SHELL_AUDIO_DECODER_LINUX_H_

#include <list>

#include "base/callback.h"
#include "base/message_loop.h"
#include "media/base/audio_decoder.h"
#include "media/base/data_buffer.h"
#include "media/base/demuxer_stream.h"
#include "media/filters/shell_audio_decoder.h"
#include "media/filters/shell_demuxer.h"

namespace media {

class MEDIA_EXPORT ShellAudioDecoderLinux : public ShellAudioDecoder {
 public:
  ShellAudioDecoderLinux(
      const scoped_refptr<base::MessageLoopProxy>& message_loop);
  virtual ~ShellAudioDecoderLinux();

    // AudioDecoder implementation.
  virtual void Initialize(const scoped_refptr<DemuxerStream>& stream,
                          const PipelineStatusCB& status_cb,
                          const StatisticsCB& statistics_cb) OVERRIDE;
  virtual void Read(const ReadCB& read_cb) OVERRIDE;
  virtual int bits_per_channel() OVERRIDE;
  virtual ChannelLayout channel_layout() OVERRIDE;
  virtual int samples_per_second() OVERRIDE;
  virtual void Reset(const base::Closure& closure) OVERRIDE;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ShellAudioDecoderLinux);
};


}  // namespace media

#endif  // MEDIA_FILTERS_SHELL_AUDIO_DECODER_LINUX_H_
