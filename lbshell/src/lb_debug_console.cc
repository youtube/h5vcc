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
#if defined(__LB_SHELL__ENABLE_CONSOLE__)

// This can be tweaked when you really need the debugger attached early.
#if !defined(ATTACH_DEBUGGER_ON_STARTUP)
#define ATTACH_DEBUGGER_ON_STARTUP 0
#endif

#include "lb_debug_console.h"

#include <sstream>

#include "external/chromium/base/logging.h"
#include "external/chromium/base/message_loop.h"
#include "external/chromium/base/message_pump_shell.h"
#include "external/chromium/base/string_util.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/base/utf_string_conversions.h"
#include "external/chromium/net/cookies/canonical_cookie.h"
#include "external/chromium/skia/ext/SkMemory_new_handler.h"
#include "external/chromium/sql/statement.h"
#include "external/chromium/third_party/WebKit/Source/JavaScriptCore/debugger/DebuggerShell.h"
#include "external/chromium/third_party/WebKit/Source/WebCore/platform/chromium/KeyboardCodes.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "external/chromium/third_party/WebKit/Source/WTF/wtf/OSAllocator.h"
#include "external/chromium/webkit/glue/webkit_glue.h"

#include "lb_cookie_store.h"
#include "lb_graphics.h"
#include "lb_local_storage_database_adapter.h"
#include "lb_memory_manager.h"
#include "lb_network_console.h"
#include "lb_resource_loader_bridge.h"
#include "lb_savegame_syncer.h"
#include "lb_shell.h"
#if defined(__LB_PS3__)
#include "lb_shell/lb_web_graphics_context_3d_ps3.h"
#endif

static const char * whitespace = " \t\n";

////////////////////////////////////////////////////////////////////////////////
// TASKS

static void DumpRenderTreeTask(WebKit::WebFrame * webframe,
                               JSC::DebuggerTTYInterface * tty) {
  std::string dump =
      UTF16ToUTF8(webkit_glue::DumpRenderer(webframe));
  tty->output(dump);
  DLOG(INFO) << dump;
}

static void NavigateTask(GURL url, LBShell * shell) {
  shell->LoadURL(url);
}

static void HistoryTask(int offset, LBShell * shell) {
  shell->navigation_controller()->GoToOffset(offset);
}

static void LocationTask(LBShell * shell, JSC::DebuggerTTYInterface * tty) {
  LBNavigationController * nav = shell->navigation_controller();
  WebKit::WebHistoryItem item = nav->GetCurrentEntry();
  std::string url = item.urlString().utf8();
  tty->output(url);
}

static void SetDebuggerTask(LBShell * shell, JSC::Debugger *debugger) {
  shell->webView()->mainFrame()->attachJSCDebugger(debugger);
}

#if LB_MEMORY_DUMP_GRAPH
static void ContinuousGraphTask(LBShell * shell) {
  if (shell->webViewHost() == NULL) return;  // this happened during shutdown.
  if (shell->webViewHost()->IsExiting()) return;

  char buf[128];
  snprintf(buf, sizeof(buf), "continuous/%d.png", (int)time(NULL));
  lb_memory_dump_fragmentation_graph(buf);

  LBResourceLoaderBridge::GetIoThread()->PostDelayedTask(FROM_HERE,
    base::Bind(ContinuousGraphTask, shell), base::TimeDelta::FromSeconds(1));
}
#endif

/** Get the remaining part of a command line expression concatenated into a string. */
std::string GetRemainingExpression(const std::vector<std::string>& tokens, int startpos) {
  std::string code;
  if (tokens.size() <= startpos) {
    return code;
  }

  for (int i = startpos; i < tokens.size(); ++i) {
    code.append(tokens[i]);
    code.append(" ");
  }
  return code;
}

// If respect_brackets is true, then the tokenizer will treat bracketed terms
// (e.g. [], <>) as a single word
static void TokenizeString(const std::string &statement, bool respect_brackets,
                           std::vector<std::string> *out_tokens) {
  DCHECK(out_tokens);

  std::size_t begin = statement.find_first_not_of(whitespace);
  std::size_t end = std::string::npos;
  while (begin != std::string::npos) {
    int token_size = 0;
    const char *delimiter = whitespace;
    if (respect_brackets) {
      if (statement.at(begin) == '<') delimiter = ">";
      if (statement.at(begin) == '[') delimiter = "]";
    }
    end = statement.find_first_of(delimiter, begin);

    // If we found a closing bracket, increment end so that the token we get
    // contains the bracket
    if (end != std::string::npos && strlen(delimiter) == 1) {
      ++end;
      if (end == statement.size())
        end = std::string::npos;
    }

    if (end != std::string::npos) {
      out_tokens->push_back(statement.substr(begin, end - begin));
      begin = statement.find_first_not_of(whitespace, end);
    } else {
      out_tokens->push_back(statement.substr(begin));
      break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// LBCommandBack
class LBCommandBack : public LBCommand {
 public:
  LBCommandBack(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "back";
    help_summary_ = "Navigate back.\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(HistoryTask, -1, shell()));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandBye

class LBCommandBye : public LBCommand {
 public:
  LBCommandBye(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "bye";
    help_summary_ = "Close telnet session.\n";
    help_details_ = "bye usage:\n"
                    "  bye\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if (shell()->webViewHost()->GetTelnetConnection()) {
      tty()->output("Bye.");
      shell()->webViewHost()->GetTelnetConnection()->Close();
    } else {
      // You can't close a console session.
      tty()->output("Hello.");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandDB

class LBCommandDB : public LBCommand {
 public:
  LBCommandDB(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "db ...";
    help_summary_ = "Database (cookie & local storage) commands.\n";
    help_details_ = "db usage:\n\n"

                    "  db cookies ...\n"
                    "manage stored browser cookies\n\n"

                    "  db exec\n"
                    "execute arbitrary sql on database\n\n"

                    "  db local\n"
                    "show local storage keys and values\n\n"

                    "  db wipe\n"
                    "clear all cookies and local storage\n\n"

                    "  db flush\n"
                    "flush the database to disk right away\n\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if (tokens[1] == "cookies") {
      if (tokens.size() < 3) {
        CookiesUsage(tty());
      } else {
        // view/edit browser cookies
        if (tokens[2] == "list") {
          CookiesList(tty());
        } else if (tokens[2] == "add") {
          if (tokens.size() < 6) {
            tty()->output(
              "db cookies add usage:\n\n"

              "  db cookies add DOMAIN PATH NAME [VALUE]\n"
              "Adds a cookie with the specified attributes.  VALUE can\n"
              "be empty or include spaces.  The specified domain should\n"
              "match the current URL (i.e. .youtube.com).  The cookie will\n"
              "be persistent and last 14 days.\n\n");
          } else {
            std::string value = "";
            for (int i = 6; i < tokens.size(); ++i) {
              value += tokens[i];
              value += (i == tokens.size()-1 ? "" : " ");
            }
            console_->shell()->webViewHost()->main_message_loop()->PostTask(
              FROM_HERE,
              base::Bind(CookiesAdd, tty(), console_->shell(),
                tokens[3], tokens[4], tokens[5], value));
          }
        } else if (tokens[2] == "delete") {
          if (tokens.size() != 6) {
            tty()->output(
              "db cookies delete usage:\n\n"

              "  db cookies delete DOMAIN PATH NAME\n"
              "Deletes all cookies with the specified attributes.\n"
              "As with 'db cookies search', specifying '*' for an\n"
              "attribute will ignore filtering on that attribute\n"
              "when determining which cookies to delete.\n\n"
              ".\n\n");
          } else {
            CookiesDelete(tty(),
              tokens[3], tokens[4], tokens[5]);
          }
        } else if (tokens[2] == "search") {
          if (tokens.size() != 6) {
            tty()->output(
              "db cookies search usage:\n\n"

              "  db cookies search DOMAIN PATH NAME\n"
              "Searches for and lists the cookies with the specified\n"
              "attributes.  '*' may be specified to ignore that column\n"
              "in the search.  i.e. 'db cookies search * * *' prints the\n"
              "same thing as 'db cookies list'.\n\n");
          } else {
            CookiesSearch(tty(),
              tokens[3], tokens[4], tokens[5]);
          }
        } else {
          CookiesUsage(tty());
        }
      }
    } else if (tokens[1] == "exec") {
      if (tokens.size() < 3 || tokens[2] == "help") {
        tty()->output(
            "db exec usage:\n"
            "  db exec <arbitrary sql command string>\n"
            "executes the provided command.\n\n");
      } else {
        // Reconstruct whitespace-separated SQL command.
        std::string sql_command = "";
        for (int i = 2; i < tokens.size(); ++i) {
          sql_command += tokens[i] + " ";
        }

        // Create a statement handle.
        sql::Connection *conn = LBSavegameSyncer::connection();
        sql::Statement stmt(conn->GetUniqueStatement(
            sql_command.c_str(),
            false // asks that errors in the statement not be considered fatal
        ));

        if (stmt.is_valid()) {
          // This output should be equivalent to the sqlite3 command-line app
          // with these directives applied: ".header ON"
          std::string buf;

          // First print column names.
          for (int i = 0; i < stmt.ColumnCount(); ++i) {
            if (i) buf.push_back('|');  // default separator for sqlite3
            buf.append(base::StringPrintf("%s", stmt.ColumnName(i).c_str()));
          }
          buf.push_back('\n');
          tty()->output(buf);
          buf.clear();

          while (stmt.Step()) {
            // Now print each row of data.
            for (int i = 0; i < stmt.ColumnCount(); ++i) {
              if (i) buf.push_back('|');  // default separator for sqlite3
              std::string output = stmt.ColumnString(i);
              buf.append(output);
            }
            buf.push_back('\n');
            tty()->output(buf);
            buf.clear();
          }
        }

        if (!stmt.is_valid() || !stmt.Succeeded()) {
          // Print an error message if the statement failed in some way.
          std::string err = conn->GetErrorMessage();
          if (err.length()) {
            err.push_back('\n');
            tty()->output(err);
          } else {
            tty()->output("Unknown sqlite3 error.\n");
          }
        }
      }
    } else if (tokens[1] == "local") {
      LBLocalStorageDatabaseAdapter::Dump(tty());
    } else if (tokens[1] == "wipe") {
      LBResourceLoaderBridge::ClearCookies();
      LBCookieStore::DeleteAllCookies();
      LBLocalStorageDatabaseAdapter::ClearAll();
      tty()->output("database wiped.\n");
    } else if (tokens[1] == "flush") {
      LBSavegameSyncer::ForceSync();
    } else {
      tty()->output("unknown db subcommand, see \"help db\".\n");
    }
  }

 private:
  static void CookiesUsage(JSC::DebuggerTTYInterface* tty) {
    tty->output(
      "db cookies usage:\n\n"

      "  db cookies list\n"
      "list all stored cookies\n\n"

      "  db cookies add\n"
      "add the specified cookie to the cookie store\n\n"

      "  db cookies delete\n"
      "delete the specified cookie from the cookie store\n\n"

      "  db cookies search\n"
      "search for the specified cookie in the cookie store\n\n");
  }

  // An internal function for filtering the full cookies list and returning
  // the filtered list
  static std::vector<net::CanonicalCookie*>
  FindCookies(const std::string& domain,
              const std::string& path,
              const std::string& name) {
    std::vector<net::CanonicalCookie*> ret;

    const std::vector<net::CanonicalCookie*> unfiltered(
      LBCookieStore::GetAllCookies());
    std::vector<net::CanonicalCookie*>::const_iterator it;
    for (it = unfiltered.begin(); it != unfiltered.end(); it++) {
      if (domain != "*" &&
          StringToUpperASCII(domain) != StringToUpperASCII((*it)->Domain())) {
        continue;
      }
      if (path != "*" &&
          StringToUpperASCII(path) != StringToUpperASCII((*it)->Path())) {
        continue;
      }
      if (name != "*" &&
          StringToUpperASCII(name) != StringToUpperASCII((*it)->Name())) {
        continue;
      }

      ret.push_back(*it);
    }

    return ret;
  }

  // Add the specified cookie to the cookie store
  static void CookiesAdd(JSC::DebuggerTTYInterface* tty,
                         LBShell* shell,
                         const std::string& domain,
                         const std::string& path,
                         const std::string& name,
                         const std::string& value) {
    LBNavigationController* nav = shell->navigation_controller();
    WebKit::WebHistoryItem item = nav->GetCurrentEntry();
    GURL url(item.urlString().utf8().data());

    // Make persistent cookies last 14 days
    base::Time creation_date = base::Time::Now();
    base::Time expiry_date = creation_date + base::TimeDelta::FromDays(14);

    std::string mac_key("");
    std::string mac_algorithm("");

    scoped_ptr<net::CanonicalCookie> cookie(net::CanonicalCookie::Create(
        url,
        name,
        value,
        domain,
        path,
        mac_key,
        mac_algorithm,
        creation_date,
        expiry_date,
        false,
        false));

    if (cookie.get() != NULL) {
      LBCookieStore::QuickAddCookie(*cookie);

      tty->output("Cookie added successfully.\n");
    } else {
      tty->output("There was an error adding the cookie.\n");
    }
  }

  // Delete all cookies that match this criteria
  static void CookiesDelete(JSC::DebuggerTTYInterface* tty,
                            const std::string& domain,
                            const std::string& path,
                            const std::string& name) {
    const std::vector<net::CanonicalCookie*> cookies(
        FindCookies(domain, path, name));
    std::vector<net::CanonicalCookie*>::const_iterator it;
    for (it = cookies.begin(); it != cookies.end(); it++) {
      LBCookieStore::QuickDeleteCookie(**it);
    }
  }

  // List all cookies that match the specified criteria
  static void CookiesSearch(JSC::DebuggerTTYInterface* tty,
                            const std::string& domain,
                            const std::string& path,
                            const std::string& name) {
    const std::vector<net::CanonicalCookie*> cookies(
        FindCookies(domain, path, name));
    std::vector<net::CanonicalCookie*>::const_iterator it;
    tty->output(StringPrintf("%20s %20s %20s %20s\n",
        "domain", "path", "name", "value"));
    tty->output(StringPrintf("%20s %20s %20s %20s\n",
        "======", "====", "====", "====="));
    for (it = cookies.begin(); it != cookies.end(); it++) {
      tty->output(StringPrintf("%20s %20s %20s  %s\n",
          (*it)->Domain().c_str(), (*it)->Path().c_str(),
          (*it)->Name().c_str(), (*it)->Value().c_str()));
    }
  }

  // List all cookies
  static void CookiesList(JSC::DebuggerTTYInterface* tty) {
    CookiesSearch(tty, "*", "*", "*");
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandDumpRenderTree
class LBCommandDumpRenderTree : public LBCommand {
 public:
  LBCommandDumpRenderTree(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "drt";
    help_summary_ = "Dump Render Tree.\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
      base::Bind(DumpRenderTreeTask, shell()->webView()->mainFrame(), tty()));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandExit
class LBCommandExit : public LBCommand {
 public:
  LBCommandExit(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "exit";
    help_summary_ = "Exit the application.\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    shell()->webViewHost()->RequestQuit();
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandForward
class LBCommandForward : public LBCommand {
 public:
  LBCommandForward(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "forward";
    help_summary_ = "Navigate forward.\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(HistoryTask, 1, shell()));
  }
};

// TODO: Make the Fuzzer command generic and not PS3 specific
#if defined(__LB_PS3__)
////////////////////////////////////////////////////////////////////////////////
// LBCommandFuzzer
class LBCommandFuzzer : public LBCommand {
 public:
  LBCommandFuzzer(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "fuzzer ...";
    help_summary_ = "Generate and save the random input to a file.\n"
                    "Input is generated at time intervals drawn from a "
                    "Gaussian distribution.\n";
    help_details_ = "fuzzer usage:\n"
                    "  fuzzer start <file-name> <time-mean> <time-std> [arrows]\n"
                    "    file-name: name of the output file\n"
                    "    time-mean: mean-value for the random time (secs)\n"
                    "    time-std: standard deviation for the random time (secs)\n"
                    "    arrows: add this if you want just arrow input (optional)\n"
                    "  fuzzer stop\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if ((tokens.size() == 5 || tokens.size() == 6)&& tokens[1] == "start") {
      // FUZZER START
      tty()->output("starting fuzzer..");
      const std::string& file_name = tokens[2];
      const float time_mean = atof(tokens[3].c_str());
      const float time_std = atof(tokens[4].c_str());
      if (tokens.size() == 5) {
        shell()->webViewHost()->FuzzerStart(file_name, time_mean, time_std, false);
      } else if (tokens[5] == "arrows") {
        shell()->webViewHost()->FuzzerStart(file_name, time_mean, time_std, true);
      }
    } else if (tokens.size() == 2 && tokens[1] == "stop") {
      // FUZZER STOP
      tty()->output("stopping fuzzer..");
      shell()->webViewHost()->FuzzerStop();
    } else {
      tty()->output(help_details_);
    }
  }
};
#endif

////////////////////////////////////////////////////////////////////////////////
// LBCommandHelp

class LBCommandHelp : public LBCommand {
 public:
  LBCommandHelp(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "help [<command>]";
    help_summary_ = "Help.  Prints this message or details about a command.\n";
    help_details_ = "help usage:\n"
                    "  help [<command>]";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    const std::map<std::string, LBCommand*> &commands = console_->GetRegisteredCommands();
    std::map<std::string, LBCommand*>::const_iterator itr;

    if (tokens.size() > 1) {
      // Display the 'man' page for the specified command
      std::string token = tokens[1];
      itr = commands.find(token);
      if (itr != commands.end()) {
        tty()->output(itr->second->GetHelpString(false));
      } else {
        tty()->output("Unrecognized command.\n");
      }
    } else {
      // Display the help summary
      // For formatting purposes, determine the max command length
      int max_length = 0;
      for (itr = commands.begin(); itr != commands.end(); itr++) {
        int size = static_cast<int>(itr->second->GetCommandSyntax().size());
        max_length = std::max(size, max_length);
      }

      tty()->output("Leanback Debug Console Commands:\n\n");

      // Output the summaries for each item
      for (itr = commands.begin(); itr != commands.end(); itr++) {
        std::string help_string = itr->second->GetCommandSyntax();
        // Insert proper number of spaces such that the summaries all line up
        for (int spaces = max_length - help_string.size(); spaces > 0; spaces--) {
          help_string.append(" ");
        }
        help_string.append(" - ");
        help_string.append(itr->second->GetHelpString(true));
        tty()->output(help_string);
      }

      tty()->output(
            "Help can also display help for a single command. For example,\n"
            "'help nav' will print more info about the nav command.\n");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandInput

class LBCommandInput : public LBCommand {
 public:
  LBCommandInput(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "input ...";
    help_summary_ = "User input recording and playback.\n";
    help_details_ = "input usage:\n\n"

                    "  input record [<filename>]\n"
                    "start recording controller input to filename\n"
                    "omit filename to stop\n\n"

                    "  input play <once/repeat> [<filename>]\n"
                    "playback recorded input from filename\n"
                    "omit filename to stop\n\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if (tokens[1] == "record") {
      // check for filename
      if (tokens.size() == 3) {
        shell()->webViewHost()->RecordKeyInput(tokens[2].c_str());
        tty()->output("recording input.\n");
      } else {
        // call with NULL to stop recording
        shell()->webViewHost()->RecordKeyInput(NULL);
        tty()->output("recording stopped.\n");
      }
    } else if (tokens[1] == "play") {
      if (tokens.size() == 4) {
        bool repeat = tokens[2] == "repeat";
        shell()->webViewHost()->PlaybackKeyInput(tokens[3].c_str(), repeat);
        tty()->output("input playback started.\n");
      } else {
        shell()->webViewHost()->PlaybackKeyInput(NULL, false);
        tty()->output("input playback stopped.\n");
      }
    } else {
      tty()->output("unknown input subcommand, see \"help input\".\n");
    }
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommandJS

class LBCommandJS : public LBCommand {
 public:
  LBCommandJS(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "js ...";
    help_summary_ = "JavaScript debugger commands.\n";
    help_details_ =
        "js usage:\n\n"

        // please keep this list alphabetized:
        "  js break <url or filename>:<line>\n"
        "set a breakpoint\n\n"

        "  js bt\n"
        "print a backtrace from the current frame\n\n"

        "  js clear <url or filename>:<line>\n"
        "clear a breakpoint\n\n"

        "  js continue\n"
        "continue execution\n\n"

        "  js dumpsource <on/off>\n"
        "tells the debugger whether to dump sources as they are parsed\n\n"

        "  js eval <code>\n"
        "evaluate code at the current frame\n\n"

        "  js pause\n"
        "pause execution at the next statement\n\n"

        "  js step [in]\n"
        "Step to next line of code. Will step into a function.\n\n"

        "  js next | js step over\n"
        "Execute next line of code. Will not enter functions.\n\n"

        "  js [watch|w] <code>\n"
        "Watch a variable. Print this everytime the debugger has paused.\n\n"

        "  js [unwatch|unw] <index>\n"
        "Stop watching a variable. Index is listed when the watch list gets displayed.\n\n"

        "  js finish | js step out\n"
        "Continue to end of function.\n\n"

        "  js sources\n"
        "list parsed source files\n\n"

        "  js trace <on/off>\n"
        "sets the debugger's trace setting\n\n";

    jscd_ = LB_NEW JSC::DebuggerShell(tty());
    debugger_attached_ = false;
  };
  ~LBCommandJS() {
    shell()->webView()->mainFrame()->attachJSCDebugger(NULL);
    debugger_attached_ = false;
    delete jscd_;
  }

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    // attach debugger before running any js commands.
    // it is not attached by default because of the performance impact
    // to the JavaScript interpretter.
    AttachDebugger();
    // TODO: add explicit commands for attach & detach

    if (tokens[1] == "trace") {
      if (tokens.size() == 3 && (tokens[2] == "on" || tokens[2] == "off")) {
        if (tokens[2] == "on") {
          tty()->output("trace enabled.\n");
          jscd_->setTrace(true);
        } else {
          jscd_->setTrace(false);
          tty()->output("trace disabled.\n");
        }
      } else {
        tty()->output("bad syntax, see \"help js\".\n");
      }
    } else if (tokens[1] == "dumpsource" || tokens[1] == "sourcedump") {
      if (tokens.size() == 3 && (tokens[2] == "on" || tokens[2] == "off")) {
        if (tokens[2] == "on") {
          tty()->output("source dumping enabled.\n");
          jscd_->setDumpSource(true);
        } else {
          jscd_->setDumpSource(false);
          tty()->output("source dumping disabled.\n");
        }
      } else {
        tty()->output("bad syntax, see \"help js\".\n");
      }
    } else if (tokens[1] == "pause") {
      if (tokens.size() == 2) {
        tty()->output("requesting a pause.\n");
        bool ok = jscd_->pause();
        if (!ok) {
          tty()->output("the interpreter is already paused.\n");
        }
      } else {
        tty()->output("bad syntax, see \"help js\".\n");
      }
    } else if (tokens[1] == "continue" || tokens[1] == "cont") {
      if (tokens.size() == 2) {
        tty()->output("continuing execution.\n");
        bool ok = jscd_->unpause();
        if (!ok) {
          tty()->output("the interpreter was not paused.\n");
        }
      } else {
        tty()->output("bad syntax, see \"help js\".\n");
      }
    } else if (tokens[1] == "sources") {
      JSC::DebuggerShell::ListOfSources sources = jscd_->getSources();
      std::sort(sources.begin(), sources.end());
      for (int i = 0; i < sources.size(); i++) {
        tty()->output(StringPrintf("[%d] = %s\n",
            sources[i].first, sources[i].second.c_str()));
      }
    } else if (tokens[1] == "break") {
      if (tokens.size() == 3) {
        std::size_t colon = tokens[2].find(':');
        if (colon == std::string::npos) {
          tty()->output("bad source line syntax, see \"help js\".\n");
        } else {
          std::string file(tokens[2], 0, colon);
          int line = atoi(tokens[2].c_str() + colon + 1);
          bool ok = jscd_->setBreakpoint(file, line);
          if (ok) {
            tty()->output("breakpoint set.\n");
          } else {
            tty()->output("source line not found.\n");
          }
        }
      } else {
        tty()->output("bad syntax, see \"help js\".\n");
      }
    } else if (tokens[1] == "clear") {
      if (tokens.size() == 3) {
        std::size_t colon = tokens[2].find(':');
        if (colon == std::string::npos) {
          tty()->output("bad source line syntax, see \"help js\".\n");
        } else {
          std::string file(tokens[2], 0, colon);
          int line = atoi(tokens[2].c_str() + colon + 1);
          bool ok = jscd_->clearBreakpoint(file, line);
          if (ok) {
            tty()->output("breakpoint cleared.\n");
          } else {
            tty()->output("breakpoint not found.\n");
          }
        }
      } else {
        tty()->output("bad syntax, see \"help js\".\n");
      }
    } else if (tokens[1] == "eval") {
      if (tokens.size() >= 3) {
        // build the code string.
        std::string code = GetRemainingExpression(tokens, 2);
        // evaluate it
        std::string output;
        bool ok = jscd_->evaluate(code, output);
        if (ok) {
          output.append("\n");
          tty()->output(output);
        } else {
          tty()->output("not paused, can't eval.\n");
        }
      } else {
        tty()->output("bad syntax, see \"help js\".\n");
      }
    } else if (tokens[1] == "watch" || tokens[1] == "w") {
      if (tokens.size() < 3) {
        tty()->output("bad syntax, see \"help js\".\n");
      } else {
        std::string code = GetRemainingExpression(tokens, 2);
        jscd_->addWatchExpression(code);
      }
    } else if (tokens[1] == "unwatch" || tokens[1] == "uw") {
      if (tokens.size() != 3) {
        tty()->output("bad syntax, see \"help js\".\n");
      } else if (!jscd_->removeWatchExpression(atoi(tokens[2].c_str()))) {
        tty()->output("Failed to remove watch index. Perhaps invalid index?\n");
      }
    } else if (tokens[1] == "backtrace" || tokens[1] == "bt") {
      if (tokens.size() == 2) {
        std::string output;
        bool ok = jscd_->backtrace(output);
        if (ok) {
          tty()->output(output);
        } else {
          tty()->output("not paused, can't backtrace.\n");
        }
      } else {
        tty()->output("bad syntax, see \"help js\".\n");
      }
    } else if ((tokens[1] == "step" || tokens[1] == "s") && (tokens.size() == 2 || tokens[2] == "in")) {
      if (!jscd_->stepInto()) {
        tty()->output("Failed to step in into the call. You probably need to pause first.");
      }
    } else if (tokens[1] == "next" || tokens[1] == "n" ||
        ((tokens[1] == "step" || tokens[1] == "s") && tokens.size() > 2 && tokens[2] == "over")) {
      if (!jscd_->stepOver()) {
        tty()->output("Failed to step over the statement. You probably need to pause first.");
      }
    } else if (tokens[1] == "finish" || tokens[1] == "f" ||
        ((tokens[1] == "step" || tokens[1] == "s") && tokens.size() > 2 && tokens[2] == "out")) {
      if (!jscd_->stepOut()) {
        tty()->output("Failed to step out of the function.  You probably need to pause first.");
      }
    } else {
      tty()->output("unknown js subcommand, see \"help js\".\n");
    }
  }

  void AttachDebugger() {
    if (debugger_attached_ == true) return;
    debugger_attached_ = true;

    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(SetDebuggerTask, shell(), jscd_));
  }

  void DetachDebugger() {
    if (debugger_attached_ == false) return;
    debugger_attached_ = false;

    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(SetDebuggerTask, shell(), (JSC::Debugger *)NULL));
  }

  JSC::DebuggerShell * jscd_;
  bool debugger_attached_;
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandKey

class LBCommandKey : public LBCommand {
 public:
  LBCommandKey(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "key <name>";
    help_summary_ = "Send a controller keypress to the browser.\n";
    help_details_ = "key usage:\n"
                    "  key <name>\n"
                    "where <name> is one of:\n"
                    "  up, down, left, right, enter, escape\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    int keyCode = -1;
    wchar_t charCode = 0;
    if (tokens[1] == "up") {
      keyCode = WebCore::VKEY_UP;
    } else if (tokens[1] == "down") {
      keyCode = WebCore::VKEY_DOWN;
    } else if (tokens[1] == "left") {
      keyCode = WebCore::VKEY_LEFT;
    } else if (tokens[1] == "right") {
      keyCode = WebCore::VKEY_RIGHT;
    } else if (tokens[1] == "enter") {
      keyCode = WebCore::VKEY_RETURN;
      charCode = WebCore::VKEY_RETURN;
    } else if (tokens[1] == "escape") {
      keyCode = WebCore::VKEY_ESCAPE;
      charCode = WebCore::VKEY_ESCAPE;
    } else {
      tty()->output(tokens[1] + ": unsupported key name."
          "  Type 'key help' for more info.\n");
    }
    if (keyCode != -1) {
      shell()->webViewHost()->InjectKeystroke(keyCode, charCode,
          static_cast<WebKit::WebKeyboardEvent::Modifiers>(0), false);
    }
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommandLabel

class LBCommandLabel : public LBCommand {
 public:
  LBCommandLabel(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "label <label id>";
    help_summary_ = "Navigate to a specific label.\n";
    help_details_ = "label usage:\n"
                    "  label <label id>\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    std::string url = shell()->GetStartupURL();
    size_t question_mark = url.find('?');
    if (question_mark != std::string::npos) {
      url.erase(question_mark);
    }
    url.append("?version=4&label=");
    url.append(tokens[1]);
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(NavigateTask, GURL(url), shell()));
  }
};



////////////////////////////////////////////////////////////////////////////////
// LBCommandLang

class LBCommandLang : public LBCommand {
 public:
  LBCommandLang(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "lang <lang code>";
    help_summary_ = "Override the system language.\n";
    help_details_ = "lang usage:\n"
                    "  lang <lang code>\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    shell()->SetPreferredLanguage(tokens[1]);
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandLocation

class LBCommandLocation : public LBCommand {
 public:
  LBCommandLocation(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "location";
    help_summary_ = "Dump the current URL.\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    // get current URL
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(LocationTask, shell(), tty()));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandLogToTelnet
class LBCommandLogToTelnet : public LBCommand {
 public:
  LBCommandLogToTelnet(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "log2telnet <on/off>";
    help_summary_ = "Log all output to the telnet console.\n";
    help_details_ = "log2telnet usage:\n"
                    "  log2telnet <on/off>\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if (tokens.size() > 1) {
      if (tokens[1] == "on")
        LBNetworkConsole::CaptureLogging(true);
      else if (tokens[1] == "off")
        LBNetworkConsole::CaptureLogging(false);
      else
        tty()->output(help_details_);
    } else {
      tty()->output(help_details_);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandMemDump

#if LB_MEMORY_DUMP_CALLERS
class LBCommandMemDump : public LBCommand {
 public:
  LBCommandMemDump(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "memdump [<file>]";
    help_summary_ = "Dump a raw memory allocation table to disk.\n";
    help_details_ = "memdump usage:\n"
                    "  memdump [<file>]\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    std::string filename = "memdump.txt";
    if (tokens.size() > 1) {
      filename = tokens[1];
    }
    lb_memory_dump_callers(filename.c_str());
  }
};
#endif


////////////////////////////////////////////////////////////////////////////////
// LBCommandMemGraph

#if LB_MEMORY_DUMP_GRAPH
class LBCommandMemGraph : public LBCommand {
 public:
  LBCommandMemGraph(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "memgraph [<file>]";
    help_summary_ = "Dump a fragmentation graph to disk.\n";
    help_details_ = "memgraph usage:\n"
                    "  memgraph [<file>|continuous]\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if (tokens.size() > 1 && tokens[1] == "continuous") {
      ContinuousGraphTask(shell());
    } else {
      std::string filename = "memgraph.png";
      if (tokens.size() > 1) {
        filename = tokens[1];
      }
      lb_memory_dump_fragmentation_graph(filename.c_str());
    }
  }
};
#endif


////////////////////////////////////////////////////////////////////////////////
// LBCommandMemInfo

class LBCommandMemInfo : public LBCommand {
 public:
  LBCommandMemInfo(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "meminfo";
    help_summary_ = "Print current memory statistics.\n";
    help_details_ = "meminfo usage:\n"
                    "  meminfo\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
#if LB_MEMORY_COUNT
    // Write some useful stats in JSON format for easier parsing.
#if defined (__LB_PS3__)
    int rsx_mem =
        LBGraphicsPS3::GetPtr()->GetGLResourceManager()->GetRSXUsedMemory();
#endif

    lb_memory_info_t info;
    lb_memory_stats(&info);

    int shell_allocator_bytes = OSAllocator::getCurrentBytesAllocated();
    int skia_bytes = sk_get_bytes_allocated();

    std::string memory_stats;
    memory_stats.append("{\"memory\": {");
    memory_stats.append(base::StringPrintf(
      "\"application\" : %d"
      ", \"free\" : %d"
      ", \"javascript\" : %d"
      ", \"skia\" : %d",
      info.application_memory,
      info.free_memory,
      shell_allocator_bytes,
      skia_bytes));
#if defined (__LB_PS3__)
    memory_stats.append(base::StringPrintf(
      ", \"rsx\" : %d", rsx_mem));
#endif
    memory_stats.append("}}\n");
    tty()->output(memory_stats);
#else
    tty()->output("Application was not compiled with LB_MEMORY_COUNT enabled.");
#endif
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandMouseClick
class LBCommandMouseClick : public LBCommand {
 public:
  LBCommandMouseClick(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "mouseclick <x> <y>";
    help_summary_ = "Sends a mouse click to WebKit at x, y.\n";
    help_details_ = "mouseclick usage:\n"
                    "  mouseclick <x> <y>\n";
  }

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if (tokens.size() != 3) {
      tty()->output(help_details_);
    } else {
      int x = atoi(tokens[1].c_str());
      int y = atoi(tokens[2].c_str());
      shell()->webViewHost()->SendMouseClick(WebKit::WebMouseEvent::ButtonLeft,
                                             x, y);
      tty()->output("mouse click sent\n");
    }
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommandNav

class LBCommandNav : public LBCommand {
 public:
  LBCommandNav(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "nav <url>";
    help_summary_ = "Point the browser at supplied URL.\n";
    help_details_ = "nav usage:\n"
                    "  nav <url>\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    GURL url(tokens[1]);
    if (url.is_valid()) {
      shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
          base::Bind(NavigateTask, url, shell()));
    } else {
      tty()->output(tokens[1] + ": bad url.\n");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandPerimeter

class LBCommandPerimeter : public LBCommand {
 public:
  LBCommandPerimeter(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "perimeter <mode>";
    help_summary_ = "Set network perimeter checking level.\n";
    help_details_ = "perimeter usage:\n"
                    "  perimeter <enable|warn|disable>\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if (tokens[1] == "enable") {
      // activate perimeter checking
      LBResourceLoaderBridge::SetPerimeterCheckEnabled(true);
      LBResourceLoaderBridge::SetPerimeterCheckLogging(true);
      tty()->output("Perimeter checking enabled\n");
    } else if (tokens[1] == "warn") {
      // disable perimeter checking, but log errors
      LBResourceLoaderBridge::SetPerimeterCheckEnabled(false);
      LBResourceLoaderBridge::SetPerimeterCheckLogging(true);
      tty()->output("Perimeter checking disabled with logging\n");
    } else if (tokens[1] == "disable") {
      // disable perimeter checking
      LBResourceLoaderBridge::SetPerimeterCheckEnabled(false);
      LBResourceLoaderBridge::SetPerimeterCheckLogging(false);
      tty()->output("Perimeter checking disabled silently\n");
    } else {
      tty()->output(help_details_);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandReload

class LBCommandReload : public LBCommand {
 public:
  LBCommandReload(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "reload";
    help_summary_ = "Reload the current page.\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(HistoryTask, 0, shell()));
  }
};

#if defined (__LB_PS3__)
////////////////////////////////////////////////////////////////////////////////
// LBCommandRequirePSN

class LBCommandRequirePSN : public LBCommand {
 public:
  LBCommandRequirePSN(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "requirepsn <mode>";
    help_summary_ = "Enables or disables PSN connection requirement.\n";
    help_details_ = "requirepsn usage: \n"
                    "  requirepsn <on/off>\n"
                    "sets whether a PSN connection is required to run the application\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    if (tokens[1] == "on" || tokens[1] == "true") {
      shell()->webViewHost()->RequirePSN(true);
    } else if (tokens[1] == "off" || tokens[1] == "false") {
      shell()->webViewHost()->RequirePSN(false);
    } else {
      tty()->output("unknown command, see \"help requirepsn\".\n");
    }
  }
};
#endif

#if defined (__LB_PS3__)
////////////////////////////////////////////////////////////////////////////////
// LBCommandRSX

class LBCommandRSX : public LBCommand {
 public:
  LBCommandRSX(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "rsx ...";
    help_summary_ = "RSX graphics debugging.\n";
    help_details_ = "rsx usage: \n"
                    "  rsx log\n"
                    "dump the most recent WebKit buffer to the console log\n\n"
                    "  rsx save <filename>\n"
                    "save the most recent WebKit buffer to the filename\n\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    LBWebGraphicsContext3DPS3* ps3_graphics_context =
        static_cast<LBWebGraphicsContext3DPS3*>(
            LBGraphicsPS3::GetPtr()->GetCompositorContext());

    if (tokens[1] == "log") {
      ps3_graphics_context->LogRSXBuffer();
    } else if (tokens[1] == "save") {
      if (tokens.size() == 3) {
        ps3_graphics_context->SaveRSXBuffer(
            tokens[2].c_str());
      } else {
        tty()->output("unknown command to rsx save, see \"help rsx\".\n");
      }
    } else {
      tty()->output("unknown rsx subcommand, see \"help rsx\".\n");
    }
  }
};
#endif

////////////////////////////////////////////////////////////////////////////////
// LBCommandScreenshot

class LBCommandScreenshot : public LBCommand {
 public:
  LBCommandScreenshot(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "ss [<filename>.png]";
    help_summary_ = "Takes a screen shot\n";
    help_details_ = "ss usage:\n"
                    "  ss [<filename>.png]\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    std::string filename = "";
    if (tokens.size() > 2) {
      tty()->output(help_details_);
    } else if (tokens.size() == 2) {
      filename = tokens[1];
    }
    LBGraphics::GetPtr()->TakeScreenshot(filename);
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandSearch

class LBCommandSearch : public LBCommand {
 public:
  LBCommandSearch(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "search [<terms>]";
    help_summary_ = "Search for the specified video.\n";
    help_details_ = "search usage:\n"
                    "  search [<terms>]\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    // go to search page
    std::string url = shell()->GetStartupURL();
    url.append("#/browse?mode=search&q=");
    for (int i = 1; i < tokens.size(); ++i) {
      url.append(tokens[i]);
      url.append("%20");
    }
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
      base::Bind(NavigateTask, GURL(url), shell()));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandTex

class LBCommandTex : public LBCommand {
 public:
  LBCommandTex(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "tex ...";
    help_summary_ = "WebKit texture debugging commands.\n";
    help_details_ = "tex usage:\n"
                    "NOTE: tex commands should only be used while WebKit is paused\n"
                    "with the \"wedge\" command.\n\n"

                    "  tex save <handle or \"all\"> <prefix>\n"
                    "saves one or all textures to <prefix><handle>.png\n\n"

                    "  tex status\n"
                    "print an overview of WebKit's texture usage\n\n"

                    "  tex watch <handle>\n"
                    "add texture to the onscreen watch, 0 to disable\n\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    // WebKit texture debugging
    if (tokens[1] == "save") {
      if (tokens.size() == 4) {
        uint32_t handle = 0;
        if (tokens[2] == "all") {
          handle = 0xffffffff;
        } else {
          handle = (uint32_t)atoi(tokens[2].c_str());
        }
        LBGraphics::GetPtr()->SaveTexture(handle, tokens[3].c_str());
      } else {
        tty()->output("bad syntax, see \"help tex\".\n");
      }
    } else if (tokens[1] == "status") {
      tty()->output(LBGraphics::GetPtr()->PrintTextureSummary());
    } else if (tokens[1] == "watch") {
      if (tokens.size() == 3) {
        uint32_t handle = (uint32_t)atoi(tokens[2].c_str());
        LBGraphics::GetPtr()->WatchTexture(handle);
      } else {
        tty()->output("bad syntax, see \"help tex\".\n");
      }
    } else {
      tty()->output("unknown tex subcommand, see \"help tex\".\n");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandToggleOSD
class LBCommandToggleOSD : public LBCommand {
 public:
  LBCommandToggleOSD(LBDebugConsole* console) : LBCommand(console) {
    command_syntax_ = "toggle_osd";
    help_summary_ = "Toggle the display of the onscreen display (FPS/Memory info, etc..)\n";
    help_details_ = "toggle_osd usage:\n"
                    "  toggle_osd\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    LBGraphics::GetPtr()->ToggleOSD();
  }
};


#if defined (__LB_PS3__) || defined (__LB_WIIU__)
////////////////////////////////////////////////////////////////////////////////
// LBCommandTweak
class LBCommandTweak : public LBCommand {
 public:
  LBCommandTweak(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "tweak ...";
    help_summary_ = "Tweak controller input settings.\n";
    help_details_ =
        "tweak usage:\n"
        "  tweak <name> <setting> <value>\n"
        "a non-exhaustive list of <name> values:\n"
        "  dpad_up, dpad_down, l1, r1, circle, cross, ...\n"
        "possible <setting> values:\n"
        "  delay (ms), rate (ms), min (0-128), max (128-255)\n";
    min_arguments_ = 3;
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    // FIXME: make controller input generic one day
    int value = (tokens.size() == 4) ? atoi(tokens[3].c_str()) : 0;
    if (value == 0 && tokens[3] != "0") {
      tty()->output(help_details_);
      return;
    } else {
      LBUserInputMapping::Tweakable fn = 0;
      if (tokens[2] == "delay") {
        fn = &LBUserInputMapping::SetDelay;
      } else if (tokens[2] == "rate") {
        fn = &LBUserInputMapping::SetRate;
      } else if (tokens[2] == "min") {
        fn = &LBUserInputMapping::SetMin;
      } else if (tokens[2] == "max") {
        fn = &LBUserInputMapping::SetMax;
      } else {
        tty()->output(StringPrintf("No such tweakable value '%s' found.\n",
            tokens[2].c_str()));
      }
      if (fn) {
        bool ret = shell()->webViewHost()->TweakControllers(
            fn, tokens[1].c_str(), value);
        if (!ret) {
          tty()->output(StringPrintf("No such mapping '%s' found.\n",
              tokens[1].c_str()));
        }
      }
    }
  }
};
#endif

////////////////////////////////////////////////////////////////////////////////
// LBCommandUnicode
class LBCommandUnicode : public LBCommand {
 public:
  LBCommandUnicode(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "unicode <hex>";
    help_summary_ = "Load a test page for the given hex codepoint.\n";
    help_details_ = "unicode usage:\n"
                    "  unicode <hex codepoint>\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    std::string url = "http://www.fileformat.info/info/unicode/char/" + tokens[1]
                      + "/browsertest.htm";
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(NavigateTask, GURL(url), shell()));
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommandVideo
class LBCommandVideo : public LBCommand {
 public:
  LBCommandVideo(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "video <base64 id>";
    help_summary_ = "Load a specific video in leanback.\n";
    help_details_ = "video usage:\n"
                    "  video <base64 id>\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    std::string url = shell()->GetStartupURL();
    url.append("#/watch?v=");
    url.append(tokens[1]);
    shell()->webViewHost()->main_message_loop()->PostTask(FROM_HERE,
        base::Bind(NavigateTask, GURL(url), shell()));
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommandWedge
class LBCommandWedge : public LBCommand {
 public:
  LBCommandWedge(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "wedge";
    help_summary_ = "Toggle blocking the WebKit thread.\n";
  };

 protected:
  virtual void DoCommand(const std::vector<std::string> &tokens) {
    // wedge - pause/unpause webkit thread
    if (shell()->webViewHost()->toggleWebKitWedged()) {
      tty()->output("webkit wedged.\n");
    } else {
      tty()->output("webkit unwedged.\n");
    }
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommand

// Calculates the minimum number of arguments of the command, for future
// validation of input.  Also checks the syntax pattern of a command to make
// sure it doesn't look malformed.
void LBCommand::ValidateSyntax() {
  if (!syntax_validated_) {
    std::vector<std::string> syntax_tokens;
    TokenizeString(command_syntax_, true, &syntax_tokens);

    // Store first token as command name
    command_name_ = syntax_tokens[0];

    // Make sure there are same number of opening and closing brackets
    int brackets = 0;
    int square_brackets = 0;
    for (int i = 0; i < command_syntax_.size(); i++) {
      if (command_syntax_.at(i) == '<')
        ++brackets;
      else if (command_syntax_.at(i) == '>')
        --brackets;
      if (command_syntax_.at(i) == '[')
        ++square_brackets;
      else if (command_syntax_.at(i) == ']')
        --square_brackets;
    }
    DCHECK(brackets == 0) << "Mismatch in <> brackets.";
    DCHECK(square_brackets == 0) << "Mismatch in [] brackets.";

    // Make sure each token appears valid
    for (int i = 1; i < syntax_tokens.size(); i++) {
      // Tokens should be ... or have surrounding <> or []
      char start = syntax_tokens[i].at(0);
      char end = syntax_tokens[i].at(syntax_tokens[i].size() - 1);

      if (start == '[') {
        if (end != ']') {
          DLOG(ERROR) << "Bad syntax: " << command_syntax_;
          DCHECK(0) << "Did you forget a closing bracket?";
        }
      } else if (start == '<') {
        if (end != '>') {
          DLOG(ERROR) << "Bad syntax: " << command_syntax_;
          DCHECK(0) << "Did you forget a closing bracket?";
        }
      } else if (syntax_tokens[i] != "...") {
        DLOG(ERROR) << "Bad syntax: " << command_syntax_;
        DCHECK(0) << "Arguments should be '...' or surrounded by <> or []";
      }
    }

    if (min_arguments_ == -1) {
      // Initialize min_arguments based on command_syntax_
      if (syntax_tokens.size() == 1) {
        min_arguments_ = 0;
      } else if (syntax_tokens.size() == 1 || syntax_tokens[1] == "...") {
        // If the first argument is "...", then the function is complex and
        // probably needs at least 1 argument.
        min_arguments_ = 1;
      } else {
        min_arguments_ = syntax_tokens.size() - 1;

        // For each argument, check whether or not it's optional (indicated
        // by square brackets), and update min_arguments_ accordingly
        for (int i = 1; i < syntax_tokens.size(); i++) {
          if (syntax_tokens[i].at(0) == '[' &&
              syntax_tokens[i].at(syntax_tokens[i].size()-1) == ']')
              --min_arguments_;
        }
      }
    }

    syntax_validated_ = true;
  }
}

void LBCommand::ExecuteCommand(std::vector<std::string> &tokens) {
  // Display help for the command if there aren't enough arguments
  if (tokens.size() - 1 < min_arguments_) {
    tty()->output(GetHelpString(false));
    return;
  }
  DoCommand(tokens);
}

const std::string& LBCommand::GetCommandName() const {
  return command_name_;
}

const std::string& LBCommand::GetCommandSyntax() const {
  return command_syntax_;
}


////////////////////////////////////////////////////////////////////////////////
// LBDebugConsole

inline JSC::DebuggerTTYInterface * LBDebugConsole::tty() const {
  return static_cast<JSC::DebuggerTTYInterface *>(shell_->webViewHost());
}

void LBDebugConsole::Init(LBShell * shell) {
  shell_ = shell;
#if ATTACH_DEBUGGER_ON_STARTUP
  AttachDebugger();
#endif
#if LB_MEMORY_CONTINUOUS_GRAPH
  ContinuousGraphTask(shell);
#endif

  // Register commands
  RegisterCommand(LB_NEW LBCommandBack(this));
  RegisterCommand(LB_NEW LBCommandBye(this));
  RegisterCommand(LB_NEW LBCommandDB(this));
  RegisterCommand(LB_NEW LBCommandDumpRenderTree(this));
  RegisterCommand(LB_NEW LBCommandExit(this));
  RegisterCommand(LB_NEW LBCommandForward(this));
  RegisterCommand(LB_NEW LBCommandHelp(this));
  RegisterCommand(LB_NEW LBCommandInput(this));
  RegisterCommand(LB_NEW LBCommandJS(this));
  RegisterCommand(LB_NEW LBCommandKey(this));
  RegisterCommand(LB_NEW LBCommandLabel(this));
  RegisterCommand(LB_NEW LBCommandLang(this));
  RegisterCommand(LB_NEW LBCommandLocation(this));
  RegisterCommand(LB_NEW LBCommandLogToTelnet(this));
  RegisterCommand(LB_NEW LBCommandMemInfo(this));
  RegisterCommand(LB_NEW LBCommandMouseClick(this));
  RegisterCommand(LB_NEW LBCommandNav(this));
  RegisterCommand(LB_NEW LBCommandPerimeter(this));
  RegisterCommand(LB_NEW LBCommandReload(this));
  RegisterCommand(LB_NEW LBCommandScreenshot(this));
  RegisterCommand(LB_NEW LBCommandSearch(this));
  RegisterCommand(LB_NEW LBCommandTex(this));
  RegisterCommand(LB_NEW LBCommandUnicode(this));
  RegisterCommand(LB_NEW LBCommandVideo(this));
  RegisterCommand(LB_NEW LBCommandWedge(this));

#if defined(__LB_PS3__)
  RegisterCommand(LB_NEW LBCommandFuzzer(this));
  RegisterCommand(LB_NEW LBCommandRequirePSN(this));
  RegisterCommand(LB_NEW LBCommandRSX(this));
  RegisterCommand(LB_NEW LBCommandTweak(this));
#endif

#if defined(__LB_WIIU__)
  RegisterCommand(LB_NEW LBCommandTweak(this));
#endif

  RegisterCommand(LB_NEW LBCommandToggleOSD(this));

#if LB_MEMORY_DUMP_CALLERS
  RegisterCommand(LB_NEW LBCommandMemDump(this));
#endif

#if LB_MEMORY_DUMP_GRAPH
  RegisterCommand(LB_NEW LBCommandMemGraph(this));
#endif
}

void LBDebugConsole::Shutdown() {
  std::map<std::string, LBCommand*>::iterator itr;
  for (itr = registered_commands_.begin(); itr != registered_commands_.end(); itr++) {
    delete itr->second;
  }
}


inline void LBDebugConsole::RegisterCommand(LBCommand *command) {
  command->ValidateSyntax();
  std::string command_string = command->GetCommandName();
  registered_commands_[command_string] = command;
}

void LBDebugConsole::ParseAndExecuteCommand(const std::string& command_string) {
  std::vector<std::string> tokens;
  TokenizeString(command_string, false, &tokens);

  // Do nothing on empty string
  if (tokens.size() == 0)
    return;

  // Find command object and execute commands
  std::map<std::string, LBCommand*>::iterator itr;
  itr = registered_commands_.find(tokens[0]);
  if (itr == registered_commands_.end()) {
    tty()->output(tokens[0] + ": unrecognized command. "
                  "Type 'help' to see all commands.");
    return;
  }

  itr->second->ExecuteCommand(tokens);
  return;
}


#endif // __LB_SHELL__ENABLE_CONSOLE__
