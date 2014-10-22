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

#include "lb_tracing_manager.h"
#include "external/chromium/base/debug/trace_event.h"
#include "external/chromium/base/memory/ref_counted.h"
#include "external/chromium/base/stringprintf.h"

#include "lb_globals.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace LB {

namespace {
const char* kTraceFileName = "TraceOutput.json";

std::string GetTraceFilePath() {
  return base::StringPrintf("%s/%s",
      GetGlobalsPtr()->logging_output_path, kTraceFileName);
}

class TraceOutputter : public base::RefCountedThreadSafe<TraceOutputter> {
 public:
  explicit TraceOutputter(const std::string& output_trace_filename);
  bool error() const { return !fp_ || ferror(fp_); }
  void OnTraceDataCollected(
      const scoped_refptr<base::RefCountedString>& events_str_ptr);

 protected:
  friend class base::RefCountedThreadSafe<TraceOutputter>;

  virtual ~TraceOutputter();

  std::string output_trace_filename_;
  int data_collected_calls_;
  FILE* fp_;
};

TraceOutputter::TraceOutputter(const std::string& output_trace_filename) {
  output_trace_filename_ = output_trace_filename;
  data_collected_calls_ = 0;

  // On tracing disable, save the results to an output tracing file
  fp_ = fopen(output_trace_filename_.c_str(), "w");

  if (!fp_) {
    DLOG(ERROR) << "Unable to open file: " << output_trace_filename_.c_str();
  } else {
    // Tracing data should be prepended with this header
    std::string header_text("{\"traceEvents\": [\n");
    fwrite(header_text.c_str(), sizeof(header_text[0]),
           header_text.size(), fp_);
  }
}

TraceOutputter::~TraceOutputter() {
  if (fp_) {
    DCHECK_EQ(ferror(fp_), 0);
    // Finally append to the output this concluding text.
    fprintf(fp_, "\n]}");
    fclose(fp_);
  }
}

void TraceOutputter::OnTraceDataCollected(
    const scoped_refptr<base::RefCountedString>& events_str_ptr) {
  DCHECK(fp_);
  DCHECK_EQ(ferror(fp_), 0);

  // Called by base::debug::TraceLog::Flush() which is called within
  // TracingManager::EnableTracing(false).  It may get called by Flush()
  // multiple times.
  if (data_collected_calls_ != 0) {
    fprintf(fp_, ",\n");
  }
  const std::string& event_str = events_str_ptr->data();
  fwrite(event_str.c_str(), sizeof(event_str[0]), event_str.size(), fp_);

  ++data_collected_calls_;
}

}  // namespace

TracingManager::TracingManager() {
}

TracingManager::~TracingManager() {
  if (base::debug::TraceLog::GetInstance()->IsEnabled()) {
    // If tracing was enabled, disable it (and write out the results) on exit.
    EnableTracing(false);
  }
}

void TracingManager::EnableTracing(bool enable) {
  base::debug::TraceLog* trace_log = base::debug::TraceLog::GetInstance();
  DCHECK(trace_log->IsEnabled() != enable);

  trace_log->SetEnabled(enable);

  if (!enable) {
    // The TraceOutputter will write trace output to a file.  Due to a callback
    // to itself submitted in its constructor, it has the potential to live
    // longer than this immediate scope here.
    scoped_refptr<TraceOutputter> trace_outputter(new TraceOutputter(
        GetTraceFilePath()));
    if (!trace_outputter->error()) {
      // Write out the actual data by calling Flush().  Within Flush(), this
      // will call OnTraceDataCollected(), possibly multiple times.
      trace_log->Flush(base::Bind(&TraceOutputter::OnTraceDataCollected,
                                  trace_outputter));
    }
  }
}

bool TracingManager::IsEnabled() const {
  return base::debug::TraceLog::GetInstance()->IsEnabled();
}
}  // namespace LB

#endif  // __LB_SHELL__ENABLE_CONSOLE__
