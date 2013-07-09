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

#include "config.h"
#include "DebuggerShell.h"

#include <stdio.h>

#include <string>
#include <list>

#include "CallFrame.h"
#include "DebuggerCallFrame.h"
#include "Interpreter.h"
#include "JSGlobalData.h"
#include "JSGlobalObject.h"
#include "JSValue.h"
#include "SourceProvider.h"
#include "WebCore/platform/Timer.h"
#include "WebCore/platform/EventLoop.h"
#include "wtf/text/WTFString.h"
#include "wtf/ThreadingPrimitives.h"

namespace JSC {

DebuggerShell::Source::Source(const std::string& sourceURL) : url(sourceURL) {
  // It seems that eval'd code gets a source ID with a blank URL.
  if (url.size() == 0) {
    url = "<eval>";
    filename = "<eval>";
    return;
  }

  size_t last = url.rfind('/');
  if (last != std::string::npos) {
    filename = std::string(url.c_str() + last + 1);
  } else {
    filename = url;
  }
}

DebuggerShell::PendingEval::PendingEval(const std::string& code)
    : code(code), done(new WTF::ThreadCondition) {
}

DebuggerShell::PendingEval::~PendingEval() {
  delete done;
}

DebuggerShell::DebuggerShell(DebuggerTTYInterface* tty) : Debugger(), tty_(tty),
    tracing_(false), dumping_source_(false), nextParseID_(0),
    pause_ASAP_(false), paused_(false), eval_in_progress_(false), paused_frame_(NULL),
    pause_on_next_statement_(false), pause_at_frame_(NULL) {
  ASSERT(tty_);
}

DebuggerShell::~DebuggerShell() {
}

static inline std::string fromWTFString(const WTF::String& wtf_string) {
  if (wtf_string.isNull()) return std::string();
  WTF::CString cstring = wtf_string.ascii();
  return std::string(cstring.data(), cstring.length());
}

#define dumpToTTY(f) { if (!eval_in_progress_) tty_->output(std::string(f.data(), f.length())); }
#define dumpToTTYPopup(f) { if (!eval_in_progress_) tty_->output_popup(std::string(f.data(), f.length())); }

#define WHEREAMI_FMT "%s[%d]:%d"
#define WHEREAMI_DATA \
  source_map_[backtrace_.back().sourceID].url.c_str(), \
  source_map_[backtrace_.back().sourceID].parseID, \
  backtrace_.back().lineNumber

// because the debugger can be attached at any time, and also because other
// methods will be called on this debugger for sources that have already been
// detached, we make fake entries for unknown source entries as they are
// encountered, and we retain old sourceID entries until they are replaced
// by new data.  fake entries are handled by the Source constructor.
void DebuggerShell::sourceParsed(ExecState* execState,
                                 SourceProvider* sourceProvider,
                                 int errorLineNumber,
                                 const WTF::String& errorMessage) {
  intptr_t sourceID = sourceProvider->asID();
  Source source = Source(fromWTFString(sourceProvider->url()));
  source.parseID = nextParseID_++;
  source_map_[sourceID] = source;

  // The URL map is used in setting breakpoints.
  // Therefore, it's okay if we overwrite one entry with another.
  // This might happen if we parse both http://a.tld/foo.js and http://b.tld/foo.js ,
  // both of which would clobber the spot occupied by the filename foo.js .
  // This means if you want to break on one or the other, you'll have to
  // specify the full URL.  The convenient mapping for filename won't be useful.
  // But for the common case, filename is very very useful.
  url_map_[source.url] = sourceID;
  url_map_[source.filename] = sourceID;
  parse_id_map_[source.parseID] = sourceID;

  if (errorLineNumber >= 0) {
    WTF::CString error = errorMessage.ascii();
    WTF::CString f = WTF::String::format("Parse failed in %s on line %d: %s",
        source.url.c_str(), errorLineNumber, error.data()).ascii();
    dumpToTTY(f);
  } else {
    if (tracing_ || dumping_source_) {
      WTF::CString f = WTF::String::format("Parsed %s", source.url.c_str()).ascii();
      dumpToTTY(f);
    }
    if (dumping_source_) {
      // NOTE: We only dump first 100 bytes of source.  The point is to know
      // more or less what's in it, not to see all 2MB of JavaScript.
      // It's really most useful for seeing the source of evals full of JSON.
      WTF::CString code = sourceProvider->source().ascii();
      WTF::CString code_clipped(code.data(), code.length() > 100 ? 100 : code.length());
      WTF::CString f = WTF::String::format("Source of %s[%d]: %s", source.url.c_str(), source.parseID, code_clipped.data()).ascii();
      dumpToTTY(f);
    }
  }
}

void DebuggerShell::exception(const DebuggerCallFrame& callFrame, intptr_t sourceID, int lineNumber, int column, bool hasHandler) {
  ExecState *execState = callFrame.dynamicGlobalObject()->globalExec();
  WTF::CString exceptString = callFrame.exception().toString(execState)->getString(execState).ascii();

  if (tracing_ || !hasHandler) {
    WTF::CString f = WTF::String::format("Exception (%shandled) at "WHEREAMI_FMT": %s",
        hasHandler ? "" : "un", WHEREAMI_DATA, exceptString.data()).ascii();
    std::string& url = source_map_[sourceID].url;
    if (hasHandler) {
      dumpToTTY(f);
    } else {
      dumpToTTYPopup(f);
      pause_ASAP_ = true;
    }
  }

  maybeBlockExecution(callFrame);
}

void DebuggerShell::atStatement(const DebuggerCallFrame& callFrame, intptr_t sourceID, int lineNumber, int column) {
  // track progress
  ASSERT(backtrace_.size() > 0);
  backtrace_.back().lineNumber = lineNumber;

  if (tracing_) {
    WTF::CString f = WTF::String::format("Trace at "WHEREAMI_FMT, WHEREAMI_DATA).ascii();
    dumpToTTY(f);
  }
  maybeBlockExecution(callFrame);
}

void DebuggerShell::callEvent(const DebuggerCallFrame& callFrame, intptr_t sourceID, int lineNumber, int column) {
  // track call frames
  backtrace_.push_back(BTFrame(sourceID, lineNumber));
  if (tracing_) {
    WTF::CString f = WTF::String::format("Calling at "WHEREAMI_FMT", stack size %d", WHEREAMI_DATA, (int)backtrace_.size()).ascii();
    dumpToTTY(f);
  }
  maybeBlockExecution(callFrame);
}

void DebuggerShell::returnEvent(const DebuggerCallFrame& callFrame, intptr_t sourceID, int lineNumber, int column) {
  if (tracing_) {
    WTF::CString f = WTF::String::format("Returning at "WHEREAMI_FMT", stack size %d", WHEREAMI_DATA, (int)backtrace_.size()).ascii();
    dumpToTTY(f);
  }
  // NOTE: don't pause here, we've already seen atStatement on these return lines
  // track call frames

  // Go one up.
  unwrapFrameByOne();
}

void DebuggerShell::willExecuteProgram(const DebuggerCallFrame& callFrame, intptr_t sourceID, int lineNumber, int column) {
  backtrace_.push_back(BTFrame(sourceID, lineNumber));
  if (tracing_) {
    WTF::CString f = WTF::String::format("Started executing at "WHEREAMI_FMT", stack size %d", WHEREAMI_DATA, (int)backtrace_.size()).ascii();
    dumpToTTY(f);
  }
  maybeBlockExecution(callFrame);
}

void DebuggerShell::didExecuteProgram(const DebuggerCallFrame& callFrame, intptr_t sourceID, int lineNumber, int column) {
  if (tracing_) {
    WTF::CString f = WTF::String::format("Done executing at "WHEREAMI_FMT", stack size %d", WHEREAMI_DATA, (int)backtrace_.size()).ascii();
    dumpToTTY(f);
  }

  // Treat stepping over the end of a program like stepping out.
  unwrapFrameByOne();
}

void DebuggerShell::didReachBreakpoint(const DebuggerCallFrame& callFrame, intptr_t sourceID, int lineNumber, int column) {
  // NOTE: didReachBreakpoint is called in lieu of atStatement,
  // which only gets called for real statements.
  // Therefore, we have to track progress here, too.
  ASSERT(backtrace_.size() > 0);
  backtrace_.back().lineNumber = lineNumber;

  WTF::CString f = WTF::String::format("Breakpoint hard-coded at "WHEREAMI_FMT, WHEREAMI_DATA).ascii();
  dumpToTTYPopup(f);
  maybeBlockExecution(callFrame);
}

void DebuggerShell::setTrace(bool on) {
  tracing_ = on;
}

void DebuggerShell::setDumpSource(bool on) {
  dumping_source_ = on;
}

bool DebuggerShell::pause() {
  if (paused_) {
    return false;
  }

  pause_ASAP_ = true;
  return true;
}

// because "continue" is a reserved word.  :)
bool DebuggerShell::unpause() {
  // we could be waiting on a pause, in which case this
  // will cancel the request.
  pause_ASAP_ = false;

  if (!paused_) {
    return false;
  }

  paused_ = false;
  return true;
}

bool DebuggerShell::stepInto() {
  if (!paused_) return false; // do nothing

  pause_on_next_statement_ = true;
  unpause();
  return true;
}

bool DebuggerShell::stepOver() {
  if (!paused_) return false; // do nothing

  pause_at_frame_ = currentFrame();
  unpause();
  return true;
}

bool DebuggerShell::stepOut() {
  if (!paused_) return false; // do nothing

  pause_at_frame_ = backtrace_.size() > 1 ? &(backtrace_.at(backtrace_.size() - 2)) : NULL;
  unpause();
  return true;
}

bool DebuggerShell::resolveFileName(const std::string& file, intptr_t *sourceID) {
  // try by name
  {
    URLMap::iterator it = url_map_.find(file);
    if (it != url_map_.end()) {
      *sourceID = it->second;
      return true;
    }
  }

  // try by parse ID
  {
    WTF::String wtf_string(file.c_str());
    bool ok = false;
    int parseID = wtf_string.toIntStrict(&ok, 10);
    if (ok) {
      ParseIDMap::iterator it = parse_id_map_.find(parseID);
      if (it != parse_id_map_.end()) {
        *sourceID = it->second;
        return true;
      }
    }
  }

  return false;
}

bool DebuggerShell::setBreakpoint(const std::string& file, int line) {
  intptr_t sourceID;
  if (!resolveFileName(file, &sourceID)) {
    return false;
  }
  breakpoints_.insert(SourceLine(sourceID, line));
  return true;
}

bool DebuggerShell::clearBreakpoint(const std::string& file, int line) {
  intptr_t sourceID;
  if (!resolveFileName(file, &sourceID)) {
    return false;
  }
  return breakpoints_.erase(SourceLine(sourceID, line)) != 0;
}

bool DebuggerShell::evaluate(const std::string& code, std::string& output) {
  if (!paused_frame_) {
    return false;
  }

  PendingEval eval(code);

  WTF::MutexLocker lock(pending_evals_mutex_);
  pending_evals_.push(&eval);
  eval.done->wait(pending_evals_mutex_);

  output = eval.output;
  return true;
}

void DebuggerShell::evaluate_(const std::string& code, std::string& output) {
  // eval_in_progress_ inhibits trace and other noise during
  // code execution associated with a user-requested eval.
  eval_in_progress_ = true;
  JSValue exception_value;
  JSValue output_value = paused_frame_->evaluate(WTF::String(code.c_str(), code.length()), exception_value);
  eval_in_progress_ = false;
  // NOTE: I have never seen exception_value filled in.
  // Uncaught exceptions seem to go to output_value.

  ExecState *execState = paused_frame_->dynamicGlobalObject()->globalExec();
  WTF::CString output_string = output_value.toString(execState)->getString(execState).ascii();
  output = std::string(output_string.data(), output_string.length());
}

void DebuggerShell::evalWatchExpressions() {
  if (!paused_frame_) return;

  std::string entry_output;
  for(int i = 0; i <  watched_expressions_.size(); ++i) {
    evaluate_(watched_expressions_[i], entry_output);
    tty_->output(std::string(WTF::String::format("[%d] %s := %s", i + 1,
        watched_expressions_[i].c_str(), entry_output.c_str()).ascii().data()));
  }
}

bool DebuggerShell::backtrace(std::string& output) {
  if (!paused_frame_) {
    return false;
  }

  output.clear();

  const char *header = "At";
  int i;
  for (i = ((int)backtrace_.size() - 1); i >= 0; i--) {
    WTF::CString f = WTF::String::format("%s %s:%d", header,
        source_map_[backtrace_[i].sourceID].url.c_str(), backtrace_[i].lineNumber).ascii();
    output.append(f.data());
    output.append("\n"); // CString seems to eat my newlines if this is in the format.
    header = "Called from";
  }
  return true;
}

DebuggerShell::ListOfSources DebuggerShell::getSources() {
  ListOfSources list;
  SourceMap::iterator it;
  for (it = source_map_.begin(); it != source_map_.end(); ++it) {
    const Source& src = it->second;

    // skip anonymous or unknown sources:
    if (src.filename.length() == 0) continue;
    if (src.filename[0] == '<') continue;
    // HACK, but incredibly useful.  skip files that look like JSON data:
    if (src.filename.find("alt=json") != std::string::npos) continue;

    list.push_back(ParseIDAndFilename(src.parseID, src.filename));
  }
  return list;
}

// TODO: implement locals

bool DebuggerShell::currentlyOnBreakpoint() {
  // check to see if this line has a breakpoint set.
  BTFrame& current_frame = backtrace_.back();
  BreakpointSet::iterator it = breakpoints_.find(SourceLine(current_frame.sourceID, current_frame.lineNumber));
  if (it != breakpoints_.end()) {
    return true;
  }

  return false;
}

// only returns when it's time to continue executing JS
void DebuggerShell::maybeBlockExecution(const DebuggerCallFrame& frame) {
  if (paused_) {
    // we can't pause while we are already paused, that causes a dead-lock.
    // this function can be called from inside itself if you run an eval
    // while paused.
    return;
  }

  bool is_stepped_pause = pause_on_next_statement_ | (currentFrame() == pause_at_frame_);
  bool should_pause = pause_ASAP_ | is_stepped_pause | currentlyOnBreakpoint();

  if (should_pause == false) return;

  pause_ASAP_ = false;
  pause_on_next_statement_ = false;
  pause_at_frame_ = NULL;
  paused_ = true;
  paused_frame_ = &frame;

  WTF::CString f = WTF::String::format("Paused at "WHEREAMI_FMT, WHEREAMI_DATA).ascii();
  dumpToTTYPopup(f);

  evalWatchExpressions();

  // process events & evals while blocking JSC.
  WebCore::TimerBase::fireTimersInNestedEventLoop();
  WebCore::EventLoop loop;
  while (paused_ && !loop.ended()) {
    loop.cycle();
    WTF::MutexLocker lock(pending_evals_mutex_);
    while (pending_evals_.size()) {
      PendingEval* eval = pending_evals_.front();
      pending_evals_.pop();
      evaluate_(eval->code, eval->output);
      eval->done->signal();
    }
  }

  paused_frame_ = NULL;

  if (!is_stepped_pause) {
    f = WTF::String::format("Continuing from "WHEREAMI_FMT, WHEREAMI_DATA).ascii();
    dumpToTTY(f);
  }
}

// Unwraps the call frame by one when stepping out of a function or codeblock.
void DebuggerShell::unwrapFrameByOne() {
  bool update_pause_at_frame = (currentFrame() != NULL && pause_at_frame_ == currentFrame());
  backtrace_.pop_back();
  if (update_pause_at_frame) {
    pause_at_frame_ = currentFrame();
  }
}

} // namespace JSC
