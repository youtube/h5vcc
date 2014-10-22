/*
 * Copyright 2013 Google Inc. All Rights Reserved.
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

#ifndef SRC_LB_SHELL_LAYOUT_TEST_RUNNER_H_
#define SRC_LB_SHELL_LAYOUT_TEST_RUNNER_H_

#include <vector>
#include <string>

#include "external/chromium/googleurl/src/gurl.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/simple_thread.h"

#if defined(__LB_LAYOUT_TESTS__)

#if defined(__LB_SHELL__FOR_RELEASE__)
#error You cannot build the layout test runner in gold builds
#endif

class LBShell;
namespace WebKit {
  class WebFrame;
}

class LBShellLayoutTestRunner
  : public base::RefCounted<LBShellLayoutTestRunner>
  , public base::SimpleThread {
 public:
  explicit LBShellLayoutTestRunner(LBShell* shell,
                                   const std::string& work_dir,
                                   const std::vector<std::string>& tests,
                                   MessageLoop* webkit_message_loop);

 private:
  friend class base::RefCounted<LBShellLayoutTestRunner>;
  virtual ~LBShellLayoutTestRunner();

  // Slots to be called when the given events occur
  void StartedProvisionalLoad(WebKit::WebFrame* frame);
  void OnLoadComplete(bool success, WebKit::WebFrame* frame);

  virtual void Run() OVERRIDE;

  // Used to assert that we are always in an expected state
  enum ThreadState {
    ThreadState_Running,
    ThreadState_WaitingForPageLoad,
  };

  // Helper functions used within Run()

  // Waits for the navigation to the given url to complete.  Returns whether
  // the load was successful or not.
  bool WaitForLoadComplete(const GURL& url);

  // Waits for the renderer to catch up with the current document, and
  // then takes a screenshot
  void TakeScreenshot(const std::string& filename);

  // Writes out a file to indicate to the outside world that this test is
  // complete
  void WriteTestCompleteSignalFile(const std::string& test_str);

  // Tasks to run on the test runner thread
  void TaskProcessLoadComplete(bool success, const GURL& url);

  std::string GetURLStrFromTest(const std::string& test_input_str);
  std::string GetFilePathFromTest(const std::string& test_input_str);

  LBShell* shell_;  // Reference to the shell we will be driving
  std::string work_dir_;  // The directory that we will do our work within
  std::vector<std::string> tests_;  // A list of URLs for tests we should run

  // Used as a flag to communicate between WaitForLoadComplete() and
  // TaskProcessLoadComplete()
  bool load_successful_;

  // If we're currently waiting for a URL to load, this indicates which one.
  GURL wait_for_url_;

  MessageLoop* message_loop_;
  base::WaitableEvent message_loop_setup_event_;  // Has the ML been setup yet?
  ThreadState thread_state_;

  MessageLoop* webkit_message_loop_;
  MessageLoop* composite_message_loop_;
};

#endif  // !defined(__LB_SHELL__FOR_RELEASE__)

#endif  // SRC_LB_SHELL_LAYOUT_TEST_RUNNER_H_
