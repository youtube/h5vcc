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

#include "chrome_version_info.h"
#if __LB_SHELL_USE_WIDEVINE__
#include "external/cdm/ps3/include/steel_widevine_proxy.h"
#endif
#include "external/chromium/base/command_line.h"
#include "external/chromium/base/i18n/rtl.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/media/crypto/shell_decryptor_factory.h"
#include "external/chromium/third_party/WebKit/Source/Platform/chromium/public/WebCompositorSupport.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebDatabase.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebRuntimeFeatures.h"
#include "external/chromium/webkit/glue/webthread_impl.h"
#include "external/chromium/ui/base/resource/resource_bundle.h"
#include "external/chromium/webkit/user_agent/user_agent.h"
#include "external/chromium/webkit/user_agent/user_agent_util.h"
#include "external/chromium/webkit/tools/test_shell/simple_resource_loader_bridge.h"
#include "external/chromium/webkit/tools/test_shell/simple_socket_stream_bridge.h"
#include "lb_memory_manager.h"
#include "lb_resource_loader_bridge.h"
#include "lb_shell.h"
#include "lb_shell/lb_shell_constants.h"
#include "lb_shell_platform_delegate.h"
#include "lb_shell_switches.h"
#include "lb_web_audio_device.h"
#include "lb_web_graphics_context_proxy.h"
#include "steel_build_id.h"
#include "steel_version.h"

#if defined(__LB_PS3__)
#  define LB_VENDOR    "Sony"
#  define LB_PLATFORM  "PS3"
#elif defined(__LB_WIIU__)
#  define LB_VENDOR    "Nintendo"
#  define LB_PLATFORM  "WiiU"
#elif defined(__LB_LINUX__)
#  define LB_VENDOR    "NoVendor"
#  define LB_PLATFORM  "Linux"
#endif

#if defined (LB_SHELL_BUILD_TYPE_DEBUG)
#define LB_SHELL_BUILD_TYPE "Debug"
#elif defined (LB_SHELL_BUILD_TYPE_DEVEL)
#define LB_SHELL_BUILD_TYPE "Devel"
#elif defined (LB_SHELL_BUILD_TYPE_QA)
#define LB_SHELL_BUILD_TYPE "QA"
#elif defined (LB_SHELL_BUILD_TYPE_GOLD)
#define LB_SHELL_BUILD_TYPE "Gold"
#else
#error "Unknown build type"
#endif

#if __LB_SHELL_USE_WIDEVINE__
static media::Decryptor *CreateWideVine(media::DecryptorClient *client) {
  return LB_NEW SteelWideVineProxy(client);
}
#endif

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

  // mime registry says which MIME types we support
  mime_registry_.reset(LB_NEW webkit_glue::SimpleWebMimeRegistryImpl);

  // external/chromium/webkit/glue/webfileutilities_impl.cc:84 asserts that this
  // is false
  file_utilities_.set_sandbox_enabled(false);

#if __LB_SHELL_USE_WIDEVINE__
  // Plug WideVine into chromium.
  media::ShellDecryptorFactory::RegisterDecryptor(
      "com.widevine", base::Bind(&CreateWideVine));
  media::ShellDecryptorFactory::RegisterDecryptor(
      "com.widevine.alpha", base::Bind(&CreateWideVine));
#endif
}

// Initialize the compositor in a different thread.
void LBShellWebKitInit::InitializeCompositor() {
  compositor_thread_.reset(LB_NEW base::Thread("cc"));
  compositor_thread_->StartWithOptions(base::Thread::Options(
      MessageLoop::TYPE_DEFAULT, kCompositorThreadStackSize,
      kCompositorThreadPriority, kCompositorThreadAffinity));
  WebKit::Platform::current()->compositorSupport()->initialize(
      LB_NEW webkit_glue::WebThreadImplForMessageLoop(
          compositor_thread_->message_loop_proxy()));
}

LBShellWebKitInit::~LBShellWebKitInit() {
  // This should mirror the constructor.
  mime_registry_.reset();
  // TODO: investigate WebCompositor post-rebase
  WebKit::Platform::current()->compositorSupport()->shutdown();
  WebKit::shutdown();
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

webkit_glue::WebSocketStreamHandleBridge* LBShellWebKitInit::CreateWebSocketBridge(
    WebKit::WebSocketStreamHandle* handle,
    webkit_glue::WebSocketStreamHandleDelegate* delegate) {
  return SimpleSocketStreamBridge::Create(handle, delegate);
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

WebKit::WebGraphicsContext3D* LBShellWebKitInit::createOffscreenGraphicsContext3D(
    const WebKit::WebGraphicsContext3D::Attributes& attributes) {
  return new LBWebGraphicsContextProxy();
}

double LBShellWebKitInit::audioHardwareSampleRate() {
  return LBWebAudioDevice::GetAudioHardwareSampleRate();
}

size_t LBShellWebKitInit::audioHardwareBufferSize() {
  return LBWebAudioDevice::GetAudioHardwareBufferSize();
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
