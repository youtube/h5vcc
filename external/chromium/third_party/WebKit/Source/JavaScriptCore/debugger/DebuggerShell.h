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

#ifndef DebuggerShell_h
#define DebuggerShell_h

#include <config.h>
#include <debugger/Debugger.h>

#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

namespace WTF {
  class Mutex;
  class ThreadCondition;
  class String;
} // namespace WTF

namespace JSC {

// Abstract interface to output text from the JSC debugger.
// It is intended that objects outside of JSC would instantiate a DebuggerShell
// and provide it an object derived from DebuggerTTYInterface to receive output.
class DebuggerTTYInterface {
public:
  virtual void output(std::string data) = 0;
  virtual void output_popup(std::string data) = 0;
};

class DebuggerCallFrame;
class ExecState;
class SourceProvider;

class DebuggerShell : public Debugger {
public:
  DebuggerShell(DebuggerTTYInterface* tty);
  virtual ~DebuggerShell();

  // Debugger implementation, called by JSC from WebKit's thread.
  virtual void sourceParsed(ExecState*, SourceProvider*, int errorLineNumber, const WTF::String& errorMessage);
  virtual void exception(const DebuggerCallFrame&, intptr_t sourceID, int lineNumber, int column, bool hasHandler);
  virtual void atStatement(const DebuggerCallFrame&, intptr_t sourceID, int lineNumber, int column);
  virtual void callEvent(const DebuggerCallFrame&, intptr_t sourceID, int lineNumber, int column);
  virtual void returnEvent(const DebuggerCallFrame&, intptr_t sourceID, int lineNumber, int column);
  virtual void willExecuteProgram(const DebuggerCallFrame&, intptr_t sourceID, int lineNumber, int column);
  virtual void didExecuteProgram(const DebuggerCallFrame&, intptr_t sourceID, int lineNumber, int column);
  virtual void didReachBreakpoint(const DebuggerCallFrame&, intptr_t sourceID, int lineNumber, int column);

  // Commands to the debugger, called from outside from any thread.
  void setTrace(bool on);
  void setDumpSource(bool on);
  // returns false if already paused
  bool pause();
  // returns false if not paused
  bool unpause();
  // Stepping functions.
  bool stepInto();
  bool stepOver();
  bool stepOut();
  // returns false if the source file can't be found
  // source file can be filename or full URL or parse ID
  bool setBreakpoint(const std::string& file, int line);
  // returns false if the breakpoint isn't set
  // source file can be filename or full URL or parse ID
  bool clearBreakpoint(const std::string& file, int line);
  // returns false if not paused
  bool evaluate(const std::string& code, std::string& output);
  // returns false if not paused
  bool backtrace(std::string& output);
  // makes a list of sources that can be used for breakpoints
  typedef std::pair<int, std::string> ParseIDAndFilename;
  typedef std::vector<ParseIDAndFilename> ListOfSources;
  ListOfSources getSources();

  inline void addWatchExpression(const std::string& code) {
    watched_expressions_.push_back(code);
  }

  // returns false for incorrect index. Index is 1 to n
  inline bool removeWatchExpression(int index) {
    index--;
    if (index < 0 || index >= watched_expressions_.size()) {
      return false;
    }
    watched_expressions_.erase(watched_expressions_.begin() + index);
    return true;
  }

protected:
  // just to force the use of the tty ctor
  DebuggerShell() : Debugger() {}

  // NOTE: only safely callable from webkit's thread
  void evaluate_(const std::string& code, std::string& output);

  bool currentlyOnBreakpoint();
  void maybeBlockExecution(const DebuggerCallFrame& frame);
  void unwrapFrameByOne();
  void evalWatchExpressions();

  bool resolveFileName(const std::string& file, intptr_t *sourceID);

  struct Source {
    std::string url;
    std::string filename;
    int parseID;

    Source() : url("<Unknown>"), filename("<Unknown>") {}
    explicit Source(const std::string& sourceURL);
  };

  struct BTFrame {
    BTFrame(intptr_t sourceID, int lineNumber)
        : sourceID(sourceID), lineNumber(lineNumber) {
    }

    explicit BTFrame(intptr_t sourceID)
        : sourceID(sourceID), lineNumber(0) {
    }

    intptr_t sourceID;
    int lineNumber;
  };

  struct PendingEval {
    explicit PendingEval(const std::string& code);
    ~PendingEval();

    std::string code;
    std::string output;
    WTF::ThreadCondition* done;
  };

  const inline BTFrame* currentFrame() const {
    return backtrace_.size() > 0 ? &(backtrace_.back()) : NULL;
  }

  typedef std::map<intptr_t, Source> SourceMap;
  typedef std::map<std::string, intptr_t> URLMap;
  typedef std::map<int, intptr_t> ParseIDMap;
  typedef std::vector<BTFrame> BTStack;
  typedef std::pair<intptr_t, int> SourceLine;
  typedef std::set<SourceLine> BreakpointSet;
  typedef std::queue<PendingEval*> PendingEvalQueue;

  DebuggerTTYInterface* tty_;
  SourceMap source_map_;
  URLMap url_map_;
  ParseIDMap parse_id_map_;
  BTStack backtrace_;
  BreakpointSet breakpoints_;
  PendingEvalQueue pending_evals_;
  WTF::Mutex pending_evals_mutex_;

  bool tracing_;
  bool dumping_source_;
  int nextParseID_;

  bool pause_ASAP_;
  bool paused_;
  bool eval_in_progress_;
  const DebuggerCallFrame* paused_frame_;
  bool pause_on_next_statement_;
  const BTFrame* pause_at_frame_;
  std::vector<std::string> watched_expressions_;
};

} // namespace JSC

#endif
