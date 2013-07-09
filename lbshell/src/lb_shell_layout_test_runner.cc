#include "lb_shell_layout_test_runner.h"

#include <sstream>
#include <string>

#include "external/chromium/base/logging.h"
#include "external/chromium/base/synchronization/waitable_event.h"
#include "external/chromium/googleurl/src/gurl.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebDataSource.h"
#include "external/chromium/third_party/WebKit/Source/Platform/chromium/public/WebURLRequest.h"

#include "lb_graphics.h"
#include "lb_resource_loader_bridge.h"
#include "lb_shell.h"
#include "lb_shell_platform_delegate.h"
#include "lb_shell/lb_shell_constants.h"
#include "lb_web_graphics_context_3d.h"
#if defined(__LB_WIIU__)
#include "lb_shell/lb_graphics_wiiu.h"
#endif

#if defined(__LB_LAYOUT_TESTS__)

extern std::string* global_game_content_path;
extern const char *global_screenshot_output_path;

const int kTestRunnerThreadStackSize = 32 * 1024;
const int kTestRunnerThreadPriority = kWebKitThreadPriority;

LBShellLayoutTestRunner::LBShellLayoutTestRunner(
    const std::string& work_dir,
    const std::vector<std::string>& tests,
    MessageLoop* webkit_message_loop)
    : base::SimpleThread("LBShellLayoutTestRunner Thread",
          base::SimpleThread::Options(kTestRunnerThreadStackSize,
                                      kTestRunnerThreadPriority)),
    work_dir_(work_dir),
    tests_(tests),
    message_loop_(NULL),
    message_loop_setup_event_(true, false),
    thread_state_(ThreadState_Running) {
  // Ensure that nothing is prepended to screenshot filenames so that we
  // can send them to the contents directory
  global_screenshot_output_path = ".";

  webkit_message_loop_ = webkit_message_loop;

  // Disable white listing
  LBResourceLoaderBridge::SetPerimeterCheckEnabled(false);
  LBResourceLoaderBridge::SetPerimeterCheckLogging(false);

  shell_.reset(new LBShell("about:blank"));
  shell_->Show(WebKit::WebNavigationPolicyNewWindow);

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
  if (test_input_str.substr(0, 4) == "http") {
    return *global_game_content_path + "/local/Previous_OnlineLayoutTest";
  } else {
    return *global_game_content_path + "/local/" + work_dir_ + "/" +
           test_input_str;
  }
}

void LBShellLayoutTestRunner::Run() {
  // Create the message loop and signal that it is setup
  MessageLoop message_loop;
  message_loop_ = &message_loop;
  message_loop_setup_event_.Signal();

  // Wait for the initial LBShell startup URL navigation load to complete
  bool result = WaitForLoadComplete();
  DCHECK(result);

  for (int cur_test = 0; cur_test < tests_.size(); ++cur_test) {
    DLOG(INFO) << "LBShellLayoutTestRunner -- Test Start (" <<
        tests_[cur_test] << ").";

    std::string testURLStr = GetURLStrFromTest(tests_[cur_test]);

    // Navigate to the next test page
    shell_->webViewHost()->SendNavigateTask(GURL(testURLStr));
    if (WaitForLoadComplete()) {
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
  WaitForLoadComplete();

  shell_->ResetOnStartedProvisionalLoadCallback();
  shell_->ResetOnNetworkSuccessCallback();
  shell_->ResetOnNetworkFailureCallback();

  shell_->webViewHost()->RequestQuit();
}

void LBShellLayoutTestRunner::StartedProvisionalLoad(WebKit::WebFrame* frame) {
  if (frame == 0) return;

  GURL url(frame->provisionalDataSource()->originalRequest().url());

  DLOG(INFO) << "Started provisional load (URL: '" << url << "').";
}

void LBShellLayoutTestRunner::OnLoadComplete(bool success,
                                             WebKit::WebFrame* frame) {
  if (frame == 0) return;

  GURL url(frame->dataSource()->originalRequest().url());
  DLOG(INFO) << "Load complete (URL: '" << url << "').";

  // Relay the OnLoadComplete signal to the test runner thread
  message_loop_->PostTask(FROM_HERE,
    base::Bind(&LBShellLayoutTestRunner::TaskProcessLoadComplete,
                this,
                success));
}

bool LBShellLayoutTestRunner::WaitForLoadComplete() {
  DCHECK_EQ(thread_state_, ThreadState_Running);
  thread_state_ = ThreadState_WaitingForPageLoad;

  message_loop_->Run();

  DCHECK_EQ(thread_state_, ThreadState_WaitingForPageLoad);
  thread_state_ = ThreadState_Running;

  return load_successful_;
}
void LBShellLayoutTestRunner::TaskProcessLoadComplete(bool success) {
  DCHECK_EQ(thread_state_, ThreadState_WaitingForPageLoad);

  load_successful_ = success;

  message_loop_->QuitWhenIdle();
}

namespace {
void SyncWithMessageLoop(MessageLoop* message_loop) {
  base::WaitableEvent wait_event(true, false);
  message_loop->PostTask(FROM_HERE,
                         base::Bind(&base::WaitableEvent::Signal,
                                    base::Unretained(&wait_event)));
  wait_event.Wait();
}

void EnsureCompositeHasOccurred(MessageLoop* webkit_message_loop,
                                WebKit::WebLayerTreeView* layer_tree_view) {
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
}

void LBShellLayoutTestRunner::TakeScreenshot(const std::string& filename) {
  // Hide all on-screen information before taking the screenshot
  bool osd_was_visible = LBGraphics::GetPtr()->OSDVisible();
  if (LBGraphics::GetPtr()->OSDVisible()) {
    LBGraphics::GetPtr()->HideOSD();
  }
  bool console_was_visible = LBGraphics::GetPtr()->ConsoleVisible();
  if (LBGraphics::GetPtr()->ConsoleVisible()) {
    LBGraphics::GetPtr()->HideConsole();
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
    LBGraphics::GetPtr()->ShowConsole();
  }
  if (osd_was_visible) {
    LBGraphics::GetPtr()->ShowOSD();
  }
}

#endif  // __LB_LAYOUT_TESTS__
