#ifndef _LB_WEB_AUDIO_DEVICE_LINUX_H_
#define _LB_WEB_AUDIO_DEVICE_LINUX_H_

#include "external/chromium/base/compiler_specific.h"
#include "lb_web_audio_device.h"

class LBWebAudioDeviceLinux
    : public LBWebAudioDevice {
 public:
  LBWebAudioDeviceLinux(size_t buffer,
                        unsigned numberOfChannels,
                        double sampleRate,
                        WebKit::WebAudioDevice::RenderCallback* callback);
  virtual ~LBWebAudioDeviceLinux();

  // WebAudioDevice implementation
  virtual void start() OVERRIDE;
  virtual void stop() OVERRIDE;
  virtual double sampleRate() OVERRIDE;
};


#endif  // _LB_WEB_AUDIO_DEVICE_LINUX_H_

