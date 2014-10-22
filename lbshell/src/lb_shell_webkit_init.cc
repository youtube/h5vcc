/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// Provides accessor functions for webkit_glue::WebKitClientImpl
// to provide access to various lb_shell implementations of WebKit
// client functions.

#include "lb_shell_webkit_init.h"

#if defined(__LB_ANDROID__)
#include "base/android/build_info.h"
#endif
#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/stringprintf.h"
#include "chrome_version_info.h"
#if defined(__LB_XB1__)
#include "chromium/media/crypto/shell_playready_decryptor.h"
#endif
#if defined(__LB_ANDROID__)
#include "chromium/media/crypto/android_decryptor.h"
#endif
#include "lb_memory_manager.h"
#include "lb_resource_loader_bridge.h"
#include "lb_savegame_syncer.h"
#include "lb_shell.h"
#if defined(__LB_XB1__)
#include "lb_shell/lb_graphics_xb1.h"
#include "lb_shell/lb_web_view_host_impl.h"
#elif defined(__LB_XB360__)
#include "lb_shell/lb_graphics_xb360.h"
#endif
#include "lb_shell/lb_shell_constants.h"
#include "lb_shell_platform_delegate.h"
#include "lb_shell_switches.h"
#include "lb_web_audio_device.h"
#include "lb_web_graphics_context_proxy.h"
#if !defined(__LB_XB1__) && !defined(__LB_XB360__)
#include "media/audio/shell_audio_streamer.h"
#endif  // !defined(__LB_XB1__) && !defined(__LB_XB360__)
#include "media/crypto/shell_decryptor_factory.h"
#if __LB_SHELL_USE_WIDEVINE__
#include "media/crypto/shell_widevine_decryptor.h"
#endif
#include "steel_build_id.h"
#include "steel_version.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCompositorSupport.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/accountInfo/AccountInfo.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/audioConfig/AudioConfigArray.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/closedCaptionsSettings/ClosedCaptionsSettings.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/dvr/DvrManager.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/input/InputManager.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/remote/RemoteMediaInfo.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/search/SearchManager.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/sso/Sso.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/storage/H5vccStorage.h"
#include "third_party/WebKit/Source/WebCore/Modules/h5vcc/system/System.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebBlobRegistry.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDatabase.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebRuntimeFeatures.h"
#include "ui/base/resource/resource_bundle.h"
#include "webkit/glue/webthread_impl.h"
#include "webkit/user_agent/user_agent.h"
#include "webkit/user_agent/user_agent_util.h"
#include "webkit/tools/test_shell/simple_resource_loader_bridge.h"
#include "webkit/tools/test_shell/simple_socket_stream_bridge.h"

#if defined(__LB_ANDROID__)
#  define LB_VENDOR    "Google"
#  define LB_PLATFORM  "Android"
#elif defined(__LB_LINUX__)
#  define LB_VENDOR    "NoVendor"
#  define LB_PLATFORM  "Linux"
#elif defined(__LB_PS3__)
#  define LB_VENDOR    "Sony"
#  define LB_PLATFORM  "PS3"
#elif defined(__LB_PS4__)
#  define LB_VENDOR    "Sony"
#  define LB_PLATFORM  "PS4"
#elif defined(__LB_WIIU__)
#  define LB_VENDOR    "Nintendo"
#  define LB_PLATFORM  "WiiU"
#elif defined(__LB_XB1__)
#  define LB_VENDOR    "Microsoft"
#  define LB_PLATFORM  "XboxOne"
#elif defined(__LB_XB360__)
#  define LB_VENDOR    "Microsoft"
#  define LB_PLATFORM  "Xbox360"
#else
#  error Undefined platform
#endif

#if defined(LB_SHELL_BUILD_TYPE_DEBUG)
#  define LB_SHELL_BUILD_TYPE "Debug"
#elif defined(LB_SHELL_BUILD_TYPE_DEVEL)
#  define LB_SHELL_BUILD_TYPE "Devel"
#elif defined(LB_SHELL_BUILD_TYPE_QA)
#  define LB_SHELL_BUILD_TYPE "QA"
#elif defined(LB_SHELL_BUILD_TYPE_GOLD)
#  define LB_SHELL_BUILD_TYPE "Gold"
#else
#  error Unknown build type
#endif

#if __LB_SHELL_USE_WIDEVINE__
static media::Decryptor *CreateWidevine(media::DecryptorClient *client) {
  return new ShellWidevineDecryptor(client);
}
#endif

WTF::String H5vccSystemCommonImpl::GetLocalizedString(const WTF::String& key) {
  WTF::CString utf8_key = key.utf8();
  std::string utf8_data = LBShell::GetString(utf8_key.data(), "");
  if (!utf8_data.size()) {
    return WTF::String();  // becomes "undefined" in JS.
  }
  return WTF::String::fromUTF8(utf8_data.c_str(), utf8_data.length());
}

void H5vccSystemCommonImpl::RegisterExperiments(const WTF::String& ids) {
  WTF::CString utf8_ids = ids.utf8();
  DLOG(INFO) << base::StringPrintf("registered experiment ids: %s",
                                   utf8_ids.data());
  NOTIMPLEMENTED();
}

class H5vccStorageImpl : public H5vcc::Storage {
 public:
  virtual ~H5vccStorageImpl() {}

  virtual void ClearCookies() OVERRIDE {
    LBResourceLoaderBridge::ClearCookies();
  }

  virtual void Flush(bool sync) OVERRIDE {
    LBSavegameSyncer::ForceSync(sync);
  }

  virtual bool GetCookiesEnabled() OVERRIDE {
    return LBResourceLoaderBridge::GetCookiesEnabled();
  }

  virtual void SetCookiesEnabled(bool value) OVERRIDE {
    LBResourceLoaderBridge::SetCookiesEnabled(value);
  }
};

const size_t H5vccLogPingerImpl::kMaxPendingCallbackSize = 128;

H5vccLogPingerImpl::H5vccLogPingerImpl(
    const scoped_refptr<base::MessageLoopProxy>& webkit_message_loop)
    : webkit_inst_(NULL)
    , webkit_message_loop_(webkit_message_loop) {
}

void H5vccLogPingerImpl::Connect(WebCore::LogPinger* inst) {
  DCHECK(inst);
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());

  if (webkit_inst_) {
    Disconnect(webkit_inst_);
  }
  webkit_inst_ = inst;

  // If there are pending requests, fire them now.
  for (std::vector<std::string>::const_iterator it = pending_callbacks_.begin();
      it != pending_callbacks_.end(); ++it) {
    webkit_inst_->invokeStringCallback(
        WTF::String::fromUTF8(it->data(), it->size()));
  }
  pending_callbacks_.clear();
}

void H5vccLogPingerImpl::Disconnect(WebCore::LogPinger* inst) {
  DCHECK(inst);
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());

  if (webkit_inst_ == inst) {
    webkit_inst_->releaseReferences();
    webkit_inst_ = NULL;
  }
}

H5vccLogPingerImpl::~H5vccLogPingerImpl() {
  DCHECK(!webkit_inst_);
  if (webkit_inst_) {
    Disconnect(webkit_inst_);
  }
}

void H5vccLogPingerImpl::InvokeCallback(const std::string& value) {
  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(&H5vccLogPingerImpl::InvokeCallbackInternal,
                 base::Unretained(this), value));
}

void H5vccLogPingerImpl::InvokeCallbackInternal(const std::string& value) {
  DCHECK(webkit_message_loop_->BelongsToCurrentThread());

  if (webkit_inst_) {
    webkit_inst_->invokeStringCallback(
        WTF::String::fromUTF8(value.data(), value.size()));
  } else if (pending_callbacks_.size() < kMaxPendingCallbackSize) {
    pending_callbacks_.push_back(value);
  }
}

LBShellWebKitInit::LBShellWebKitInit() {
  WebKit::initialize(this);

  InitializeCompositor();

  WebKit::setLayoutTestMode(false);

  WebKit::WebRuntimeFeatures::enableMediaPlayer(true);

  // disable almost ALL features for right now
  WebKit::WebRuntimeFeatures::enableDatabase(false);
  // apparently nobody actually reads this local storage value, but it can't
  // hurt to set it...
  WebKit::WebRuntimeFeatures::enableLocalStorage(true);
  WebKit::WebRuntimeFeatures::enableSessionStorage(false);
  WebKit::WebRuntimeFeatures::enableSockets(false);
  WebKit::WebRuntimeFeatures::enableNotifications(false);
  WebKit::WebRuntimeFeatures::enableApplicationCache(false);
  WebKit::WebRuntimeFeatures::enableDataTransferItems(false);
  WebKit::WebRuntimeFeatures::enableGeolocation(false);
  WebKit::WebRuntimeFeatures::enableIndexedDatabase(false);
  WebKit::WebRuntimeFeatures::enableWebAudio(true);
  WebKit::WebRuntimeFeatures::enableTouch(false);
  WebKit::WebRuntimeFeatures::enableDeviceMotion(false);
  WebKit::WebRuntimeFeatures::enableDeviceOrientation(false);
  WebKit::WebRuntimeFeatures::enableSpeechInput(false);
  WebKit::WebRuntimeFeatures::enableXHRResponseBlob(false);
  WebKit::WebRuntimeFeatures::enableFileSystem(false);
  WebKit::WebRuntimeFeatures::enableJavaScriptI18NAPI(false);
  WebKit::WebRuntimeFeatures::enableQuota(false);

  // turn on MediaSource and Encrypted Media Extensions
  WebKit::WebRuntimeFeatures::enableMediaSource(true);
  DCHECK(WebKit::WebRuntimeFeatures::isMediaSourceEnabled());
  WebKit::WebRuntimeFeatures::enableEncryptedMedia(true);
  DCHECK(WebKit::WebRuntimeFeatures::isEncryptedMediaEnabled());

  // Put Chrome version and LeanbackShell version and build type in UA.
  std::string product = base::StringPrintf(
    "Chrome/%s LeanbackShell/%s %s build %s",
    CHROME_VERSION, STEEL_VERSION, LB_SHELL_BUILD_TYPE, STEEL_BUILD_ID);

  // Get language and country code.
  std::string full_language_code =
      LBShell::PreferredLanguage();
  size_t dash = full_language_code.find('-');
  std::string short_language_code;
  std::string country_code;
  if (dash != std::string::npos) {
    short_language_code = std::string(full_language_code.c_str(), dash);
    country_code = std::string(full_language_code.c_str() + dash + 1);
  } else {
    short_language_code = full_language_code;
    country_code = "XX";
  }

  std::string user_agent = webkit_glue::BuildUserAgentFromProduct(product);

  // Add to the UA to match Leanback UA spec:
  std::string vendor = LB_VENDOR;
  std::string device = LB_PLATFORM;
  std::string firmware_version = "";  // okay to leave blank
  std::string model = LB_PLATFORM;
  std::string sku = "";  // okay to leave blank

#if defined(__LB_ANDROID__)
  // These things need to be queried at runtime on Android.
  base::android::BuildInfo *info = base::android::BuildInfo::GetInstance();
  vendor = info->brand();
  model = info->model();
  device = info->device();
#endif

  user_agent.append(base::StringPrintf(" %s %s/%s (%s, %s, %s, %s)",
    vendor.c_str(), device.c_str(), firmware_version.c_str(),
    model.c_str(), sku.c_str(),
    short_language_code.c_str(), country_code.c_str()));

#if !defined(__LB_SHELL__FOR_RELEASE__)
  std::string cmd_line_user_agent =
      CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          LB::switches::kUserAgent);
  if (!cmd_line_user_agent.empty()) {
    user_agent = cmd_line_user_agent;
    DLOG(INFO) << "Overriding user agent with command line parameter";
  }
#endif

  DLOG(INFO) << base::StringPrintf("User-Agent: %s", user_agent.c_str());
  webkit_glue::SetUserAgent(user_agent, false);

  blob_registry_ = new LBWebBlobRegistryImpl;

  // mime registry says which MIME types we support
  mime_registry_.reset(new webkit_glue::SimpleWebMimeRegistryImpl);

  // external/chromium/webkit/glue/webfileutilities_impl.cc:84 asserts that this
  // is false
  file_utilities_.set_sandbox_enabled(false);

#if __LB_SHELL_USE_WIDEVINE__
  // Plug Widevine into chromium.
  media::ShellDecryptorFactory::RegisterDecryptor(
      "com.widevine", base::Bind(&CreateWidevine));
  media::ShellDecryptorFactory::RegisterDecryptor(
      "com.widevine.alpha", base::Bind(&CreateWidevine));
#endif
#if defined(__LB_ANDROID__)
  // This class will detect what encryption schemes the device supports,
  // and register the appropriate decryptors to ShellDecryptorFactory.
  media::AndroidDecryptorManager::RegisterDecryptors();
#endif
#if defined(__LB_XB1__)
  media::ShellDecryptorFactory::RegisterDecryptor(
      "com.youtube.playready",
      base::Bind(&ShellPlayReadyDecryptorFactory::Create));
#endif

  log_pinger_impl_.reset(new H5vccLogPingerImpl(
      base::MessageLoopProxy::current()));
  storage_impl_.reset(new H5vccStorageImpl);
}

WebCore::ObjectPositionReporter*
LBShellWebKitInit::createObjectPositionReporter() {
#if !defined(__LB_XB1__)
  return NULL;
#else
  return LBWebViewHostImpl::Get()->CreateObjectPositionReporter();
#endif
}

// Initialize the compositor in a different thread.
void LBShellWebKitInit::InitializeCompositor() {
  compositor_thread_.reset(new base::Thread("cc"));
  compositor_thread_->StartWithOptions(base::Thread::Options(
      MessageLoop::TYPE_DEFAULT, kCompositorThreadStackSize,
      kCompositorThreadPriority, kCompositorThreadAffinity));
  WebKit::Platform::current()->compositorSupport()->initialize(
      new webkit_glue::WebThreadImplForMessageLoop(
          compositor_thread_->message_loop_proxy()));
}

LBShellWebKitInit::~LBShellWebKitInit() {
  // This should mirror the constructor.
  mime_registry_.reset();
  // TODO: investigate WebCompositor post-rebase
  WebKit::Platform::current()->compositorSupport()->shutdown();
  WebKit::shutdown();
}

WebKit::WebBlobRegistry* LBShellWebKitInit::blobRegistry() {
  return blob_registry_.get();
}

WebKit::WebMimeRegistry* LBShellWebKitInit::mimeRegistry() {
  return mime_registry_.get();
}

WebKit::WebFileUtilities* LBShellWebKitInit::fileUtilities() {
  return &file_utilities_;
}

WebKit::WebCookieJar* LBShellWebKitInit::cookieJar() {
  return &cookie_jar_;
}

WebKit::WebString LBShellWebKitInit::defaultLocale() {
  return WebKit::WebString::fromUTF8(LBShell::PreferredLocale().c_str());
}

webkit_glue::ResourceLoaderBridge* LBShellWebKitInit::CreateResourceLoader(
    const webkit_glue::ResourceLoaderBridge::RequestInfo& request_info) {
  return LBResourceLoaderBridge::Create(request_info);
}

webkit_glue::WebSocketStreamHandleBridge*
LBShellWebKitInit::CreateWebSocketBridge(
    WebKit::WebSocketStreamHandle* handle,
    webkit_glue::WebSocketStreamHandleDelegate* delegate) {
  NOTIMPLEMENTED();
  return NULL;
}

WebKit::WebStorageNamespace* LBShellWebKitInit::createLocalStorageNamespace(
    const WebKit::WebString& path, unsigned quota) {
  return dom_storage_system_.CreateLocalStorageNamespace();
}


base::StringPiece LBShellWebKitInit::GetDataResource(
    int resource_id, ui::ScaleFactor scale_factor) {
  return ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

WebKit::WebGraphicsContext3D*
LBShellWebKitInit::createOffscreenGraphicsContext3D(
    const WebKit::WebGraphicsContext3D::Attributes& attributes) {
  return new LBWebGraphicsContextProxy();
}

bool LBShellWebKitInit::canAccelerate2dCanvas() {
#if !defined(__LB_DISABLE_SKIA_GPU__)
  return true;
#else
  return false;
#endif
}

double LBShellWebKitInit::audioHardwareSampleRate() {
  return LBWebAudioDevice::GetAudioHardwareSampleRate();
}

size_t LBShellWebKitInit::audioHardwareBufferSize() {
  return LBWebAudioDevice::GetAudioHardwareBufferSize();
}

int LBShellWebKitInit::audioHardwareMaxChannels() {
#if defined(__LB_XB1__) || defined(__LB_XB360__)
  // Platforms without ShellAudioStreamer support should override this function
  // in their platform specific LBShellWebkitInit so this function should never
  // be called.
  NOTIMPLEMENTED();
  return 2;
#else  // defined(__LB_XB1__) || defined(__LB_XB360__)
  return media::ShellAudioStreamer::Instance()->GetConfig().
      max_hardware_channels();
#endif  // defined(__LB_XB1__) || defined(__LB_XB360__)
}

WebKit::WebAudioDevice* LBShellWebKitInit::createAudioDevice(
    size_t buffer,
    unsigned numberOfChannels,
    double sampleRate,
    WebKit::WebAudioDevice::RenderCallback* callback) {
  return LBWebAudioDevice::Create(buffer,
                                  numberOfChannels,
                                  sampleRate,
                                  callback);
}
