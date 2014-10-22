/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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
#include "lb_shell_layout_test_runner.h"

#include <sstream>
#include <string>

#include "external/chromium/base/debug/trace_event.h"
#include "external/chromium/base/logging.h"
#include "external/chromium/base/synchronization/waitable_event.h"
#include "external/chromium/googleurl/src/gurl.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebDataSource.h"
#include "external/chromium/third_party/WebKit/Source/Platform/chromium/public/WebURLRequest.h"

#include "lb_globals.h"
#include "lb_graphics.h"
#include "lb_on_screen_display.h"
#include "lb_resource_loader_bridge.h"
#include "lb_shell.h"
#include "lb_shell_platform_delegate.h"
#include "lb_shell/lb_shell_constants.h"
#include "lb_web_graphics_context_3d.h"
#if defined(__LB_WIIU__)
#include "lb_shell/lb_graphics_wiiu.h"
#endif

#if defined(__LB_LAYOUT_TESTS__)

const int kTestRunnerThreadStackSize = 32 * 1024;
const int kTestRunnerThreadPriority = kWebKitThreadPriority;

LBShellLayoutTestRunner::LBShellLayoutTestRunner(
    LBShell* shell,
    const std::string& work_dir,
    const std::vector<std::string>& tests,
    MessageLoop* webkit_message_loop)
    : base::SimpleThread("LBShellLayoutTestRunner",
          base::SimpleThread::Options(kTestRunnerThreadStackSize,
                                      kTestRunnerThreadPriority))
    , message_loop_setup_event_(true, false) {
  // Enable tracing during layout tests as they can be very helpful
  // to debug flaky test results (and performance is not currently an
  // issue for layout tests).
  shell->tracing_manager()->EnableTracing(true);

  work_dir_ = work_dir;
  tests_ = tests;
  shell_ = shell;
  message_loop_ = NULL;

  thread_state_ = ThreadState_Running;

  // Ensure that nothing is prepended to screenshot filenames so that we
  // can send them to the contents directory
  GetGlobalsPtr()->screenshot_output_path = strdup(".");

  webkit_message_loop_ = webkit_message_loop;

  // Disable white listing
  LBResourceLoaderBridge::SetPerimeterCheckEnabled(false);
  LBResourceLoaderBridge::SetPerimeterCheckLogging(false);

  // Setup all the callback bindings that we're interested in
  shell_->SetOnStartedProvisionalLoadCallback(
    base::Bind(&LBShellLayoutTestRunner::StartedProvisionalLoad, this));
  shell_->SetOnNetworkSuccessCallback(
    base::Bind(&LBShellLayoutTestRunner::OnLoadComplete, this, true));
  shell_->SetOnNetworkFailureCallback(
    base::Bind(&LBShellLayoutTestRunner::OnLoadComplete, this, false));
  Start();

  // Wait for the message loop to be initialized
  message_loop_setup_event_.Wait();
  DCHECK(message_loop_);
}

LBShellLayoutTestRunner::~LBShellLayoutTestRunner() {
  Join();
}

std::string LBShellLayoutTestRunner::GetURLStrFromTest(
    const std::string& test_input_str) {
  if (test_input_str.substr(0, 4) == "http") {
    return test_input_str;
  } else {
    return "local:///" + work_dir_ + "/" + test_input_str + ".html";
  }
}

std::string LBShellLayoutTestRunner::GetFilePathFromTest(
    const std::string& test_input_str) {
  const std::string game_content_path(GetGlobalsPtr()->game_content_path);
  if (test_input_str.substr(0, 4) == "http") {
    return game_content_path + "/local/Previous_OnlineLayoutTest";
  } else {
    return game_content_path + "/local/" + work_dir_ + "/" +
           test_input_str;
  }
}

void LBShellLayoutTestRunner::Run() {
  // Create the message loop and signal that it is setup
  MessageLoop message_loop;
  message_loop_ = &message_loop;
  message_loop_setup_event_.Signal();

  bool result = WaitForLoadComplete(GURL("about:blank"));
  DCHECK(result);

  for (int cur_test = 0; cur_test < tests_.size(); ++cur_test) {
    TRACE_EVENT1("lb_layout_tests", "Layout Test Iteration",
                 "Test", tests_[cur_test]);
    DLOG(INFO) << "LBShellLayoutTestRunner -- Test Start (" <<
        tests_[cur_test] << ").";

    GURL testURL = GURL(GetURLStrFromTest(tests_[cur_test]));

    // Navigate to the next test page
    TRACE_EVENT_INSTANT0("lb_layout_tests", "Sending navigate task");
    shell_->webViewHost()->SendNavigateTask(testURL);
    if (WaitForLoadComplete(testURL)) {
      // Take a screenshot
      std::ostringstream oss;
      oss << GetFilePathFromTest(tests_[cur_test]) << "-actual";
      TakeScreenshot(oss.str());
    } else {
      // There was an error loading this test
      DLOG(INFO) << "Error loading test '" << tests_[cur_test] << "'.";
    }

    // Signal the outside world that the test is complete
    DLOG(INFO) << "LBShellLayoutTestRunner -- Test Done (" <<
        tests_[cur_test] << ").";
  }

  DLOG(INFO) << "LBShellLayoutTestRunner -- All tests complete.";

  shell_->webViewHost()->SendNavigateTask(GURL("about:blank"));
  WaitForLoadComplete(GURL("about:blank"));

  shell_->ResetOnStartedProvisionalLoadCallback();
  shell_->ResetOnNetworkSuccessCallback();
  shell_->ResetOnNetworkFailureCallback();

  shell_->webViewHost()->RequestQuit();
}

void LBShellLayoutTestRunner::StartedProvisionalLoad(WebKit::WebFrame* frame) {
  if (frame == 0) return;

  GURL url(frame->provisionalDataSource()->originalRequest().url());
  TRACE_EVENT_INSTANT1("lb_layout_tests",
                       "LBShellLayoutTestRunner::StartedProvisionalLoad",
                       "url", url.possibly_invalid_spec());
  DLOG(INFO) << "Started provisional load (URL: '" << url << "').";
}

void LBShellLayoutTestRunner::OnLoadComplete(bool success,
                                             WebKit::WebFrame* frame) {
  if (frame == 0) return;

  GURL url(frame->dataSource()->originalRequest().url());
  TRACE_EVENT2("lb_layout_tests",
               "LBShellLayoutTestRunner::OnLoadComplete",
               "success", success,
               "url", url.possibly_invalid_spec());
  DLOG(INFO) << "Load complete (URL: '" << url << "').";

  // Relay the OnLoadComplete signal to the test runner thread
  message_loop_->PostTask(FROM_HERE,
      base::Bind(&LBShellLayoutTestRunner::TaskProcessLoadComplete,
                 this,
                 success,
                 url));
}

bool LBShellLayoutTestRunner::WaitForLoadComplete(const GURL& url) {
  TRACE_EVENT1("lb_layout_tests",
               "LBShellLayoutTestRunner::WaitForLoadComplete",
               "url", url.possibly_invalid_spec());
  DCHECK_EQ(thread_state_, ThreadState_Running);
  thread_state_ = ThreadState_WaitingForPageLoad;
  wait_for_url_ = url;

  message_loop_->Run();

  DCHECK_EQ(thread_state_, ThreadState_WaitingForPageLoad);
  thread_state_ = ThreadState_Running;

  return load_successful_;
}
void LBShellLayoutTestRunner::TaskProcessLoadComplete(bool success,
                                                      const GURL& url) {
  TRACE_EVENT2("lb_layout_tests",
               "LBShellLayoutTestRunner::TaskProcessLoadComplete",
               "success", success,
               "url", url.possibly_invalid_spec());
  DCHECK_EQ(thread_state_, ThreadState_WaitingForPageLoad);

  if (url == wait_for_url_) {
    // The URL we were waiting to load on has finished loading.
    load_successful_ = success;

    message_loop_->QuitWhenIdle();
  }
}

namespace {
void SyncWithMessageLoop(MessageLoop* message_loop) {
  TRACE_EVENT0("lb_layout_tests",
               "LBShellLayoutTestRunner::SyncWithMessageLoop");

  base::WaitableEvent wait_event(true, false);
  message_loop->PostTask(FROM_HERE,
                         base::Bind(&base::WaitableEvent::Signal,
                                    base::Unretained(&wait_event)));
  wait_event.Wait();
}

void EnsureCompositeHasOccurred(MessageLoop* webkit_message_loop,
                                WebKit::WebLayerTreeView* layer_tree_view) {
  TRACE_EVENT0("lb_layout_tests",
               "LBShellLayoutTestRunner::EnsureCompositeHasOccurred");

  // Make sure all webkit events are flushed before taking the screenshot
  webkit_message_loop->PostTask(
      FROM_HERE,
      base::Bind(&WebKit::WebLayerTreeView::composite,
                 base::Unretained(layer_tree_view)));
  webkit_message_loop->PostTask(
      FROM_HERE,
      base::Bind(&WebKit::WebLayerTreeView::setNeedsRedraw,
                 base::Unretained(layer_tree_view)));

  // Wait for the above commands to be sent
  SyncWithMessageLoop(webkit_message_loop);

  // There are no public facing commands that I know of to force the chrome
  // compositor scheduler to do a composite immediately, we must wait for the
  // scheduler to do the composite.  The scheduler awakens at least every
  // VSync, so wait for at least 2 of them to pass before we return.
  usleep(40000);

  // Make sure the render has all wrapped up.
  webkit_message_loop->PostTask(
      FROM_HERE,
      base::Bind(&WebKit::WebLayerTreeView::finishAllRendering,
                 base::Unretained(layer_tree_view)));
  SyncWithMessageLoop(webkit_message_loop);
}
}  // namespace

void LBShellLayoutTestRunner::TakeScreenshot(const std::string& filename) {
  TRACE_EVENT0("lb_layout_tests",
               "LBShellLayoutTestRunner::TakeScreenshot");

  // Hide all on-screen information before taking the screenshot
  bool osd_was_visible = LB::OnScreenDisplay::GetPtr()->StatsVisible();
  if (LB::OnScreenDisplay::GetPtr()->StatsVisible()) {
    LB::OnScreenDisplay::GetPtr()->HideStats();
  }
  bool console_was_visible = LB::OnScreenDisplay::GetPtr()->ConsoleVisible();
  if (LB::OnScreenDisplay::GetPtr()->ConsoleVisible()) {
    LB::OnScreenDisplay::GetPtr()->HideConsole();
  }

#if defined(__LB_WIIU__)
  bool error_viewer_was_visible =
      LBGraphicsWiiU::GetPtr()->ErrorViewerVisible();
  LBGraphicsWiiU::GetPtr()->SetErrorViewerVisible(false);
#endif

  EnsureCompositeHasOccurred(webkit_message_loop_,
                             shell_->webViewHost()->webview()->layerTreeView());

  LBGraphics::GetPtr()->TakeScreenshot(filename);

#if defined(__LB_WIIU__)
  LBGraphicsWiiU::GetPtr()->SetErrorViewerVisible(error_viewer_was_visible);
#endif

  if (console_was_visible) {
    LB::OnScreenDisplay::GetPtr()->ShowConsole();
  }
  if (osd_was_visible) {
    LB::OnScreenDisplay::GetPtr()->ShowStats();
  }
}

#endif  // __LB_LAYOUT_TESTS__
