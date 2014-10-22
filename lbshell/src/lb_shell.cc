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
// The primary shell object.  Responsible primarily for creating the view
// object.

#include "lb_shell.h"

#include <memory.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include <map>
#include <string>
#include <vector>

#include "external/chromium/base/command_line.h"
#include "external/chromium/base/debug/trace_event.h"
#include "external/chromium/base/file_util.h"
#include "external/chromium/base/format_macros.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/base/message_loop.h"
#include "external/chromium/base/rand_util.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/base/utf_string_conversions.h"
#if __LB_ENABLE_NATIVE_HTTP_STACK__
#include "external/chromium/net/http/shell/http_stream_shell_loader.h"
#endif
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/platform/WebURLRequest.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebRuntimeFeatures.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "external/chromium/ui/base/resource/resource_bundle.h"
#include "external/chromium/webkit/glue/webkit_glue.h"
#include "external/chromium/webkit/glue/webpreferences.h"
#include "grit/lb_shell_resources.h"
#include "lb_debug_console.h"
#include "lb_globals.h"
#include "lb_memory_manager.h"
#include "lb_resource_loader_bridge.h"
#include "lb_shell_platform_delegate.h"
#include "lb_shell_switches.h"
#include "webkit/grit/webkit_resources.h"

using WebKit::WebFrame;
using WebKit::WebView;
using WebKit::WebURLRequest;

// initialize static member variables
webkit_glue::WebPreferences* LBShell::web_prefs_ = NULL;
std::map<std::string, std::string> LBShell::strings_;
std::string LBShell::preferred_language_;
std::string LBShell::preferred_locale_;

namespace {

std::string GenerateLogName() {
  // TODO: Get the name from "steel_build_id.h"
  const char* kAppName = "lb_shell";

  time_t t = time(NULL);
  struct tm *timeinfo = localtime(&t);

  std::string path;
  path = ::StringPrintf(
      "%s\\%s-%02d%02d-%02d%02d%02d.log",
      GetGlobalsPtr()->logging_output_path,
      kAppName, timeinfo->tm_mon + 1, timeinfo->tm_mday,
      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  return path;
}

}  // namespace

// static
void LBShell::InitializeLBShell() {
  web_prefs_ = new webkit_glue::WebPreferences();
  ResetWebPreferences();

#if __LB_ENABLE_NATIVE_HTTP_STACK__
  net::HttpStreamShellLoaderGlobalInit();
#endif

  // InitSharedInstanceWithPakFile is much simpler and dumber than the real
  // InitSharedInstance but it's exactly what we want in this case
  base::PlatformFileError err;
  base::PlatformFile file = base::CreatePlatformFile(
      FilePath(GetGlobalsPtr()->game_content_path).Append("lb_shell.pak"),
      base::PLATFORM_FILE_OPEN | base::PLATFORM_FILE_READ,
      NULL, &err);
  if (base::PLATFORM_FILE_OK == err) {
    ui::ResourceBundle::InitSharedInstanceWithPakFile(file, true);
    base::ClosePlatformFile(file);
  }
}

// static
void LBShell::ShutdownLBShell() {
#if __LB_ENABLE_NATIVE_HTTP_STACK__
  net::HttpStreamShellLoaderGlobalDeinit();
#endif

  ui::ResourceBundle::CleanupSharedInstance();
  delete web_prefs_;
}

// static
void LBShell::InitLogging() {
  bool log_to_file = false;

#if defined(__LB_SHELL__FORCE_LOGGING__)
#if defined(__LB_XB1__)
  log_to_file = true;
#endif
  logging::LoggingDestination destination = log_to_file ?
      logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG :
      logging::LOG_ONLY_TO_SYSTEM_DEBUG_LOG;
  logging::LogLockingState locking = logging::LOCK_LOG_FILE;
#else
  logging::LoggingDestination destination = logging::LOG_NONE;
  logging::LogLockingState locking = logging::DONT_LOCK_LOG_FILE;
#endif

  std::string log_filename;
  if (log_to_file) {
    log_filename = GenerateLogName();
  }

  logging::InitLogging(
      log_to_file ? log_filename.c_str() : NULL,
      destination,
      locking,
      logging::DELETE_OLD_LOG_FILE,
      logging::DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);

  webkit_glue::EnableWebCoreLogChannels(
      CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          LB::switches::kWebCoreLogChannels));

#if defined(__LB_SHELL__FORCE_LOGGING__)
  logging::SetLogItems(
      false,  // process_id is meaningless in a single-process
      true,   // thread_id might be handy
      true,   // timestamp useful
      true);  // tick count useful

  // please DO warn about notImplementeds
  webkit_glue::EnableWebCoreLogChannels(std::string("NotYetImplemented, ") +
                                        std::string("Loading, ") +
                                        std::string("Media, ") +
                                        std::string("Network, ") +
                                        std::string("PageCache, ") +
                                        std::string("SQLDatabase"));
#else
  logging::SetLogItems(false, false, false, false);
#endif
}

// static
void LBShell::CleanupLogging() {
  // This function is empty, yet entirely implemented. There's just nothing for
  // this function to do. InitLogging() doesn't do anything which requires
  // teardown.
}

LBShell::LBShell(const std::string& startup_url,
                 MessageLoop* webkit_message_loop)
    : num_retries_(0)
    , total_retry_delay_ms_(0)
    , should_display_network_error_(false) {
  delegate_.reset(new LBWebViewDelegate(this));
  navigation_controller_.reset(new LBNavigationController(this));

  startup_url_ = startup_url;

  webkit_message_loop_ = webkit_message_loop;

  web_view_host_.reset(LBWebViewHost::Create(this, delegate_.get(),
                                             *web_prefs_));
}

LBShell::~LBShell() {
  // Make sure the WebHistoryItems in the navigation controller are destroyed
  // from the correct (WebKit) thread.  After this, it is safe to destroy the
  // navigation controller itself.
  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBNavigationController::Clear,
                 base::Unretained(navigation_controller_.get())));
  SyncWithWebKit();

  // web_view_host_ needs to be destroyed after debug console
  if (web_view_host_.get()) {
    web_view_host_->Destroy();
    web_view_host_.reset();
  }
}

void LBShell::RunLoop() {
  web_view_host_->RunLoop();
}

void LBShell::SyncWithWebKit() {
  base::WaitableEvent wait_event(true, false);
  webkit_message_loop_->PostTask(FROM_HERE,
      base::Bind(&base::WaitableEvent::Signal, base::Unretained(&wait_event)));
  wait_event.Wait();
}

// does a DFS over the tree starting at |node| and ending at the first element
// whose name is |name|.
static xmlNode *XMLDFS(xmlNode *node, const char *name) {
  xmlNode *child = node->children;
  while (child) {
    if (child->type == XML_ELEMENT_NODE) {
      if (strcmp((const char *)child->name, name) == 0) {
        return child;
      }
      xmlNode *descendant = XMLDFS(child, name);
      if (descendant) {
        return descendant;
      }
    }
    child = child->next;
  }
  return NULL;
}

// static
void LBShell::LoadStrings(const std::string& lang) {
  // construct the XLB filename
  std::string filename(GetGlobalsPtr()->game_content_path);

  filename.append("/i18n/");
  filename.append(lang);
  filename.append(".xlb");
  FilePath filepath(filename);

  // Try to open the XLB file
  std::string content;
  if (!file_util::ReadFileToString(filepath, &content)) {
#if !defined(__LB_SHELL__FOR_RELEASE__)
    fprintf(stderr, "Unable to open XLB %s\n", filename.c_str());
#endif
    // Fall back to a generic version of the same language.
    size_t dash = lang.find('-');
    if (dash != std::string::npos) {
      // Chop off the country part of the language string and try again.
      std::string generic_lang(lang.c_str(), dash);
      LoadStrings(generic_lang);
    }
    return;
  }

  // parse the XML document
  xmlDoc *doc = xmlParseDoc(reinterpret_cast<const xmlChar *>(content.c_str()));
  if (!doc) {
#if !defined(__LB_SHELL__FOR_RELEASE__)
    fprintf(stderr, "Unable to parse XLB %s\n", filename.c_str());
#endif
    return;
  }

  // find the first <msg> node by DFS
  xmlNode *msg = XMLDFS(xmlDocGetRootElement(doc), "msg");
  // iterate through the sibling <msg> elements
  while (msg) {
    if (!strcmp((const char *)msg->name, "msg")) {
      // add the data from this element to the strings list
      char *name =
          reinterpret_cast<char *>(xmlGetProp(msg, (const xmlChar *)"name"));
      char *value =
          reinterpret_cast<char *>(xmlNodeListGetString(doc, msg->children, 1));
      strings_[name] = value;
      xmlFree(name);
      xmlFree(value);
    }
    msg = msg->next;
  }

  xmlFreeDoc(doc);
}

// static
void LBShell::InitStrings() {
  // First load the US English strings.
  LoadStrings("en-US");

  // Now patch over them with the system language.
  LoadStrings(LBShellPlatformDelegate::GetSystemLanguage());

  // Now patch over them with the "preferred" language, which may be overriden.
  LoadStrings(PreferredLanguage());

  // Any strings missing in the user's language(s) are provided in US English.
}

// static
std::string LBShell::GetString(const char *id, const std::string& fallback) {
  std::map<std::string, std::string>::iterator it;
  it = strings_.find(id);
  if (it == strings_.end()) {
    return fallback;
  }
  return it->second;
}

bool LBShell::Reload() {
  // Get the right target frame for the entry.
  WebFrame* frame = webView()->mainFrame();

  // If we are reloading, then WebKit will use the state of the current page.
  frame->reload(false);
  // Focus.
  webView()->setFocusedFrame(frame);
  return true;
}

bool LBShell::Retry() {
  // Get the pending entry from the nav controller.
  WebKit::WebHistoryItem item = navigation_controller_->GetPendingEntry();
  // If there isn't one, reload the current page instead.
  if (item.isNull()) return Reload();

  // Get the right target frame for the entry.
  WebFrame* frame = webView()->mainFrame();
  // Try this URL again.
  frame->loadRequest(WebURLRequest(GURL(item.urlString())));
  // Focus.
  webView()->setFocusedFrame(frame);
  return true;
}

bool LBShell::Navigate(const GURL& url) {
  TRACE_EVENT1("lb_shell", "LBShell::Navigate", "url",
               url.possibly_invalid_spec());

  // Get the right target frame for the entry.
  WebFrame* frame = webView()->mainFrame();
  // Load the new page.
  frame->loadRequest(WebURLRequest(url));
  // Focus.
  webView()->setFocusedFrame(frame);
  return true;
}

bool LBShell::Navigate(const WebKit::WebHistoryItem& item) {
  TRACE_EVENT0("lb_shell", "NavigateToItem");

  // Get the right target frame for the entry.
  WebFrame* frame = webView()->mainFrame();
  // Give webkit the state to navigate to.
  frame->loadHistoryItem(item);
  // Focus.
  webView()->setFocusedFrame(frame);
  return true;
}

bool LBShell::NavigateToAboutBlank(const base::Closure& closure) {
  delegate_->SetNavCompletedClosure(closure);
  return Navigate(GURL("about:blank"));
}

void LBShell::LoadURL(const GURL &url) {
  if (!url.is_valid())
    return;
  Navigate(url);
}

void LBShell::Show(WebKit::WebNavigationPolicy policy) {
  delegate_->show(policy);
}

static void RetryTask(LBShell *shell) {
  // Something was wrong with the main page, so try again.
  if (shell->webViewHost()->IsExiting()) return;
  shell->Retry();
}

static void NavigateTask(LBShell *shell, const GURL &url) {
  shell->Navigate(url);
}

// http://en.wikipedia.org/wiki/Exponential_backoff#An_example_of_an_exponential_backoff_algorithm
void LBShell::HandleNetworkFailure(WebKit::WebFrame* frame) {
  // In phase 1, we delay a random amount of time and then retry.  After each
  // retry, we double the maximum amount of time we might choose for the next
  // delay in phase 1.  When the total time we've delayed breaks a threshold,
  // we display an error dialog to the user.  When the user acknowledges it,
  // we move onto phase 2.

  // In phase 2, we delay by a large, fixed amount of time (equal to the
  // threshold from phase 1) and then retry.  Then we display an error dialog
  // again, and repeat phase 2.

  // The size of an interval.  Random delays are multiples of this interval.
  const int64_t interval_ms = 100;
  // The maximum amount of time we will retry without showing an error message.
  // Also the maximum length of any one delay.
  const int64_t max_retry_ms = 10 * 1000;
  // The maximum number of intervals used in a random delay is
  // 1<<max_interval_shift.
  const int max_interval_shift = 8;

  if (should_display_network_error_) {
    DLOG(ERROR) << base::StringPrintf("Failed to load the main frame for "
                                      "the last %" PRId64" ms.  Displaying "
                                      "error message to the user.",
                                      total_retry_delay_ms_);
    should_display_network_error_ = false;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
    if (!network_failure_cb_.is_null()) {
      network_failure_cb_.Run(frame);
    }
#endif

    web_view_host_->ShowNetworkFailure();
  } else {
    int64_t delay_ms;

    if (total_retry_delay_ms_ >= max_retry_ms) {
      // If we've been at it a while, stop doing the random interval and peg it
      // at the maximum.
      delay_ms = max_retry_ms;
    } else {
      // Pick a random delay.
      int max_intervals = 1 << std::min(num_retries_, max_interval_shift);
      int num_intervals_chosen = base::RandInt(0, max_intervals);
      delay_ms = num_intervals_chosen * interval_ms;
      delay_ms = std::min(delay_ms, max_retry_ms);
    }

    DLOG(WARNING) << base::StringPrintf("Failed to load the main frame, "
                                        "delaying %" PRId64" ms and trying "
                                        "again.", delay_ms);

    total_retry_delay_ms_ += delay_ms;
    ++num_retries_;

    if (total_retry_delay_ms_ >= max_retry_ms) {
      should_display_network_error_ = true;
    }

    webkit_message_loop()->PostDelayedTask(FROM_HERE,
        base::Bind(RetryTask, this),
        base::TimeDelta::FromMilliseconds(delay_ms));
  }
}

void LBShell::HandleNetworkSuccess(WebKit::WebFrame* frame) {
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  if (!network_success_cb_.is_null()) {
    network_success_cb_.Run(frame);
  }
#endif

  num_retries_ = 0;
  total_retry_delay_ms_ = 0;
}

void LBShell::StartedProvisionalLoad(WebKit::WebFrame* frame) {
#if defined(__LB_SHELL__ENABLE_CONSOLE__)
  if (!started_provisional_load_cb_.is_null()) {
    started_provisional_load_cb_.Run(frame);
  }
#endif
}

// static
void LBShell::ResetWebPreferences() {
  DCHECK(web_prefs_);

  // Match the settings used by Mac DumpRenderTree, with the exception of
  // fonts.
  if (web_prefs_) {
      *web_prefs_ = webkit_glue::WebPreferences();
      web_prefs_->default_encoding = "utf-8";
      web_prefs_->javascript_can_open_windows_automatically = false;
      web_prefs_->dom_paste_enabled = false;
      web_prefs_->developer_extras_enabled = false;
      web_prefs_->site_specific_quirks_enabled = false;
      web_prefs_->shrinks_standalone_images_to_fit = false;
      web_prefs_->uses_universal_detector = false;
      web_prefs_->text_areas_are_resizable = false;
      web_prefs_->java_enabled = false;
      web_prefs_->allow_scripts_to_close_windows = false;
      web_prefs_->javascript_can_access_clipboard = false;
      web_prefs_->xss_auditor_enabled = false;
      web_prefs_->remote_fonts_enabled = true;
      web_prefs_->local_storage_enabled = true;
      web_prefs_->application_cache_enabled = false;
      web_prefs_->databases_enabled = true;
      web_prefs_->allow_file_access_from_file_urls = false;
      web_prefs_->allow_universal_access_from_file_urls = false;
      web_prefs_->accelerated_compositing_enabled = true;
      web_prefs_->accelerated_animation_enabled = true;
      web_prefs_->accelerated_compositing_for_video_enabled = true;
      web_prefs_->accelerated_2d_canvas_enabled = true;
      web_prefs_->accelerated_painting_enabled = true;
      web_prefs_->accelerated_filters_enabled = true;
      web_prefs_->accelerated_compositing_for_plugins_enabled = true;

#if defined(__LB_SHELL__ENABLE_CONSOLE__)
      web_prefs_->developer_extras_enabled = true;
#endif
  }
}

// The locale is the same as the language, but locales use underscores instead
// of dashes.
static std::string LocaleFromLanguage(const std::string& lang) {
  std::string locale = lang;
  for (size_t i = 0; i < locale.size(); i++) {
    if (locale[i] == '-') locale[i] = '_';
  }
  return locale;
}

void LBShell::SetPreferredLanguage(const std::string& lang) {
  // for future requests for the preferred language:
  preferred_language_ = lang;
  // in case the resource loader has already asked and set its language:
  LBResourceLoaderBridge::ChangeLanguage(lang);
}

std::string LBShell::PreferredLanguage() {
  if (preferred_language_.empty()) {
    preferred_language_ = LBShellPlatformDelegate::GetSystemLanguage();
  }
  return preferred_language_;
}

std::string LBShell::PreferredLocale() {
  if (preferred_locale_.empty()) {
    preferred_locale_ = LocaleFromLanguage(PreferredLanguage());
  }
  return preferred_locale_;
}

namespace webkit_glue {

string16 GetLocalizedString(int message_id) {
  return ResourceBundle::GetSharedInstance().GetLocalizedString(message_id);
}

base::StringPiece GetDataResource(int resource_id) {
  switch (resource_id) {
    case IDR_BROKENIMAGE:
      resource_id = IDR_BROKENIMAGE_TESTSHELL;
      break;
    case IDR_TEXTAREA_RESIZER:
      resource_id = IDR_TEXTAREA_RESIZER_TESTSHELL;
      break;
  }
  return ResourceBundle::GetSharedInstance().GetRawDataResource(resource_id);
}

}  // namespace webkit_glue
