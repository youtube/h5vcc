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
#ifndef _LB_DEBUG_CONSOLE_H_
#define _LB_DEBUG_CONSOLE_H_

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

#include <map>
#include <string>
#include <vector>

#include "external/chromium/base/logging.h"

class LBCommand;
class LBShell;

namespace JSC {
class DebuggerTTYInterface;
}

class LBDebugConsole {
 public:
  void Init(LBShell * shell);
  void Shutdown();

  // parse a command string and execute results
  void ParseAndExecuteCommand(const std::string& command);
  void RegisterCommand(LBCommand *command);

  inline LBShell * shell() const { return shell_; }
  inline const std::map<std::string, LBCommand*> & GetRegisteredCommands() const {
    return registered_commands_;
  }
  JSC::DebuggerTTYInterface * tty() const;

 private:
  void AttachDebugger();
  void DetachDebugger();

  LBShell * shell_;

  std::map<std::string, LBCommand*> registered_commands_;
};


class LBCommand {
 public:
  // Argument description structure must be populated within constructor
  LBCommand(LBDebugConsole *console) : console_(console), min_arguments_(-1),
                                       syntax_validated_(false) {};
  virtual ~LBCommand() {};

  void ValidateSyntax();
  void ExecuteCommand(std::vector<std::string> &tokens);

  const std::string& GetHelpString(bool summary) const {
    if (summary || help_details_.empty())
      return help_summary_;
    else
      return help_details_;
  }

  const std::string& GetCommandName() const;
  const std::string& GetCommandSyntax() const;

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {};

  inline JSC::DebuggerTTYInterface * tty() const { return console_->tty(); }
  inline LBShell * shell() const { return console_->shell(); }

  LBDebugConsole *console_;
  std::string command_name_;
  std::string command_syntax_;
  std::string help_summary_;
  std::string help_details_;
  int min_arguments_;
  bool syntax_validated_;
};


#endif // __LB_SHELL__ENABLE_CONSOLE__
#endif // _LB_DEBUG_CONSOLE_H_
