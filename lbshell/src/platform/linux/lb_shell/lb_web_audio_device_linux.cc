#include "lb_web_audio_device_linux.h"

const int kLinuxOutputSampleRate = 48000;
const int kRenderBufferSizeFrames = 256;

// static
WebKit::WebAudioDevice* LBWebAudioDevice::Create(
    size_t buffer,
    unsigned numberOfChannels,
    double sampleRate,
    WebKit::WebAudioDevice::RenderCallback* callback) {
  return new LBWebAudioDeviceLinux(buffer,
                                   numberOfChannels,
                                   sampleRate,
                                   callback);
}

// static
double LBWebAudioDevice::GetAudioHardwareSampleRate() {
  return kLinuxOutputSampleRate;
}

// static
size_t LBWebAudioDevice::GetAudioHardwareBufferSize() {
  return kRenderBufferSizeFrames;
}

LBWebAudioDeviceLinux::LBWebAudioDeviceLinux(
    size_t buffer,
    unsigned numberOfChannels,
    double sampleRate,
    WebKit::WebAudioDevice::RenderCallback* callback) {
}

LBWebAudioDeviceLinux::~LBWebAudioDeviceLinux() {
}

void LBWebAudioDeviceLinux::start() {
}

void LBWebAudioDeviceLinux::stop() {
}

double LBWebAudioDeviceLinux::sampleRate() {
  return kLinuxOutputSampleRate;
}
