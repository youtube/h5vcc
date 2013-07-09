#include "media/audio/shell_audio_sink_linux.h"

#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/stringprintf.h"

namespace media {

// static
ShellAudioSink* ShellAudioSink::Create() {
  return new ShellAudioSinkLinux();
}

ShellAudioSinkLinux::ShellAudioSinkLinux() {
}

ShellAudioSinkLinux::~ShellAudioSinkLinux() {
}

void ShellAudioSinkLinux::Initialize(const AudioParameters& params,
                                   RenderCallback* callback) {
}

void ShellAudioSinkLinux::Start() {
}

void ShellAudioSinkLinux::Stop() {
}

void ShellAudioSinkLinux::Pause(bool flush) {
}

void ShellAudioSinkLinux::Play() {
}

void ShellAudioSinkLinux::SetPlaybackRate(float rate) {
  // if playback rate is 0.0 pause, if it's 1.0 go, if it's other
  // numbers issue a warning
  if (rate == 0.0f) {
    Pause(false);
  } else if (rate == 1.0f) {
    Play();
  } else {
    DLOG(WARNING) << base::StringPrintf(
        "audio sink got unsupported playback rate: %f", rate);
    Play();
  }
}

bool ShellAudioSinkLinux::SetVolume(double volume) {
  // TODO: set the surround mixer volume?
  if (volume != 1.0) {
    NOTIMPLEMENTED();
  }
  return volume == 1.0;
}

void ShellAudioSinkLinux::ResumeAfterUnderflow(bool buffer_more_audio) {
}

}  // namespace media
