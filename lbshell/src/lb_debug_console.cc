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

#include "Platform.h"

#include "base/logging.h"
#include "base/message_loop.h"
#include "base/message_pump_shell.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/sys_string_conversions.h"
#include "base/utf_string_conversions.h"
#include "net/cookies/canonical_cookie.h"
#include "skia/ext/SkMemory_new_handler.h"
#include "sql/statement.h"
#include "third_party/WebKit/Source/WebCore/platform/chromium/KeyboardCodes.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/WebKit/Source/WTF/config.h"
#include "third_party/WebKit/Source/WTF/wtf/OSAllocator.h"
#include "webkit/glue/webkit_glue.h"

#include "lb_cookie_store.h"
#include "lb_graphics.h"
#include "lb_local_storage_database_adapter.h"
#include "lb_memory_manager.h"
#include "lb_network_console.h"
#include "lb_on_screen_display.h"
#include "lb_resource_loader_bridge.h"
#include "lb_savegame_syncer.h"
#include "lb_shell.h"
#include "lb_shell_console_values_hooks.h"
#if defined(__LB_PS3__)
#include "lb_shell/lb_web_graphics_context_3d_ps3.h"
#endif

static const char * whitespace = " \t\n";

////////////////////////////////////////////////////////////////////////////////
// TASKS

static void DumpRenderTreeTask(WebKit::WebFrame *webframe,
                               LBConsoleConnection *connection) {
  std::string dump =
      UTF16ToUTF8(webkit_glue::DumpRenderer(webframe));
  connection->Output(dump);
  DLOG(INFO) << dump;
}

static void NavigateTask(GURL url, LBShell *shell) {
  shell->LoadURL(url);
}

static void HistoryTask(int offset, LBShell *shell) {
  shell->navigation_controller()->GoToOffset(offset);
}

static void LocationTask(LBShell *shell, LBConsoleConnection *connection) {
  LBNavigationController * nav = shell->navigation_controller();
  WebKit::WebHistoryItem item = nav->GetCurrentEntry();
  std::string url = item.urlString().utf8();
  connection->Output(url + "\n");
}

static void ContinuousGraphTask(LBShell *shell) {
  if (shell->webViewHost() == NULL) return;  // this happened during shutdown.
  if (shell->webViewHost()->IsExiting()) return;

  char buf[128];
  snprintf(buf, sizeof(buf), "continuous/%d.png", static_cast<int>(time(NULL)));
  LB::Memory::DumpFragmentationGraph(buf);

  LBResourceLoaderBridge::GetIoThread()->PostDelayedTask(FROM_HERE,
    base::Bind(ContinuousGraphTask, shell), base::TimeDelta::FromSeconds(1));
}

// Get the remaining part of a command line expression concatenated into
// a string.
std::string GetRemainingExpression(const std::vector<std::string>& tokens,
                                   int startpos) {
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
  explicit LBCommandBack(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "back";
    help_summary_ = "Navigate back.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(HistoryTask, -1, shell()));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandBye

class LBCommandBye : public LBCommand {
 public:
  explicit LBCommandBye(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "bye";
    help_summary_ = "Close telnet session.\n";
    help_details_ = "bye usage:\n"
                    "  bye\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    connection->Output("Bye.\n");
    connection->Close();
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandCompositorLogging

class LBCommandCompositorLogging : public LBCommand {
 public:
  explicit LBCommandCompositorLogging(LBDebugConsole *console)
      : LBCommand(console) {
    command_syntax_ = "comp_log";
    help_summary_ = "Enable compositor logging to stderr.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection* connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(&webkit_glue::EnableWebCoreLogChannels, "Compositing"));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandCVal
class LBCommandCVal : public LBCommand {
 public:
  explicit LBCommandCVal(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "cval ...";
    help_summary_ = "Interact with system console values.\n";
    help_details_ = "cval usage:\n\n"

                    "  cval add CVAL_PATTERN\n"
                    "Add CVals matching CVAL_PATTERN to the CVal whitelist.\n"
                    "CVAL_PATTERN can contain wildcards like * and ?.\n\n"

                    "  cval remove CVAL_PATTERN\n"
                    "Remove CVals matching CVAL_PATTERN from the CVal\n"
                    "whitelist\n"
                    "CVAL_PATTERN can contain wildcards like * and ?\n\n"

                    "  cval default\n"
                    "Restores the CVal whitelist to the default\n\n"

                    "  cval save FILENAME\n"
                    "Saves the current CVal whitelist to the specified file\n\n"

                    "  cval load FILENAME\n"
                    "Loads a CVal whitelist from the specified file\n\n"

                    "  cval listwl\n"
                    "List all CVals currently in the whitelist\n\n"

                    "  cval list [CVAL_PATTERN]\n"
                    "List all CVals whose name matches CVAL_PATTERN.\n"
                    "If CVAL_PATTERN is not given, list all CVals.\n\n"

                    "  cval print CVAL_NAME\n"
                    "Prints the value of the given CVal.\n\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    LB::ShellConsoleValueHooks* shell_hooks =
        LB::ShellConsoleValueHooks::GetInstance();
    LB::ShellConsoleValueHooks::Whitelist* whitelist =
        shell_hooks->GetWhitelist();
    LB::ConsoleValueManager* cvm = LB::ConsoleValueManager::GetInstance();

    if (tokens[1] == "add") {
      if (tokens.size() < 3) {
        connection->Output("Error: No CVAL_NAME specified.");
      } else {
        bool cval_matched = false;
        std::set<std::string> cval_set = cvm->GetOrderedCValNames();
        for (std::set<std::string>::const_iterator iter = cval_set.begin();
             iter != cval_set.end(); ++iter) {
          if (MatchPattern(*iter, tokens[2])) {
            cval_matched = true;
            if (!whitelist->Add(*iter)) {
              connection->Output("CVal " + *iter + " is already in whitelist.");
            }
          }
        }
        if (!cval_matched) {
          connection->Output("No CVals matched the given pattern.");
        }
      }
    } else if (tokens[1] == "remove") {
      if (tokens.size() < 3) {
        connection->Output("Error: No CVAL_NAME specified.");
      } else {
        std::set<std::string> cval_set = whitelist->GetOrderedValues();
        bool cval_matched = false;
        for (std::set<std::string>::const_iterator iter = cval_set.begin();
             iter != cval_set.end(); ++iter) {
          if (MatchPattern(*iter, tokens[2])) {
            cval_matched = true;
            bool result = whitelist->Remove(*iter);
            DCHECK(result);
          }
        }
        if (!cval_matched) {
          connection->Output("Pattern not found in whitelist.");
        }
      }
    } else if (tokens[1] == "default") {
      whitelist->RestoreToDefault();
    } else if (tokens[1] == "save") {
      if (tokens.size() < 3) {
        connection->Output("Error: No filename specified.");
      } else {
        if (!whitelist->SaveNamedWhitelist(tokens[2])) {
          connection->Output("Error saving whitelist to the specified file.");
        }
      }
    } else if (tokens[1] == "load") {
      if (tokens.size() < 3) {
        connection->Output("Error: No filename specified.");
      } else {
        if (!whitelist->LoadNamedWhitelist(tokens[2])) {
          connection->Output(
              "Error loading whitelist from the specified file.");
        }
      }
    } else if (tokens[1] == "listwl") {
      std::set<std::string> wl_cvals = whitelist->GetOrderedValues();
      for (std::set<std::string>::const_iterator iter = wl_cvals.begin();
           iter != wl_cvals.end(); ++iter) {
        connection->Output(*iter);
      }
    } else if (tokens[1] == "list") {
      std::set<std::string> cval_set = cvm->GetOrderedCValNames();
      for (std::set<std::string>::const_iterator iter = cval_set.begin();
           iter != cval_set.end(); ++iter) {
        if (MatchPattern(*iter, (tokens.size() >= 3 ? tokens[2] : "*"))) {
          connection->Output(*iter);
        }
      }
    } else if (tokens[1] == "print") {
      if (tokens.size() < 3) {
        connection->Output("Error: No CVAL_NAME specified.");
      } else {
        LB::ConsoleValueManager::ValueQueryResults value_info =
            cvm->GetValueAsString(tokens[2]);
        if (value_info.valid) {
          connection->Output(value_info.value);
        } else {
          connection->Output("Error: Could not find specified CVal.");
        }
      }
    } else {
      connection->Output("Error: Unknown cval command.");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandDB

class LBCommandDB : public LBCommand {
 public:
  explicit LBCommandDB(LBDebugConsole *console) : LBCommand(console) {
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
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    if (tokens[1] == "cookies") {
      if (tokens.size() < 3) {
        CookiesUsage(connection);
      } else {
        // view/edit browser cookies
        if (tokens[2] == "list") {
          CookiesList(connection);
        } else if (tokens[2] == "add") {
          if (tokens.size() < 6) {
            connection->Output(
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
            console_->shell()->webViewHost()->webkit_message_loop()->PostTask(
              FROM_HERE,
              base::Bind(CookiesAdd, connection, console_->shell(),
                tokens[3], tokens[4], tokens[5], value));
          }
        } else if (tokens[2] == "delete") {
          if (tokens.size() != 6) {
            connection->Output(
              "db cookies delete usage:\n\n"

              "  db cookies delete DOMAIN PATH NAME\n"
              "Deletes all cookies with the specified attributes.\n"
              "As with 'db cookies search', specifying '*' for an\n"
              "attribute will ignore filtering on that attribute\n"
              "when determining which cookies to delete.\n\n"
              ".\n\n");
          } else {
            CookiesDelete(connection,
              tokens[3], tokens[4], tokens[5]);
          }
        } else if (tokens[2] == "search") {
          if (tokens.size() != 6) {
            connection->Output(
              "db cookies search usage:\n\n"

              "  db cookies search DOMAIN PATH NAME\n"
              "Searches for and lists the cookies with the specified\n"
              "attributes.  '*' may be specified to ignore that column\n"
              "in the search.  i.e. 'db cookies search * * *' prints the\n"
              "same thing as 'db cookies list'.\n\n");
          } else {
            CookiesSearch(connection,
              tokens[3], tokens[4], tokens[5]);
          }
        } else {
          CookiesUsage(connection);
        }
      }
    } else if (tokens[1] == "exec") {
      if (tokens.size() < 3 || tokens[2] == "help") {
        connection->Output(
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
            // asks that errors in the statement not be considered fatal
            false));

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
          connection->Output(buf);
          buf.clear();

          while (stmt.Step()) {
            // Now print each row of data.
            for (int i = 0; i < stmt.ColumnCount(); ++i) {
              if (i) buf.push_back('|');  // default separator for sqlite3
              std::string output = stmt.ColumnString(i);
              buf.append(output);
            }
            buf.push_back('\n');
            connection->Output(buf);
            buf.clear();
          }
        }

        if (!stmt.is_valid() || !stmt.Succeeded()) {
          // Print an error message if the statement failed in some way.
          std::string err = conn->GetErrorMessage();
          if (err.length()) {
            err.push_back('\n');
            connection->Output(err);
          } else {
            connection->Output("Unknown sqlite3 error.\n");
          }
        }
      }
    } else if (tokens[1] == "local") {
      LBLocalStorageDatabaseAdapter::Dump(connection);
    } else if (tokens[1] == "wipe") {
      LBResourceLoaderBridge::ClearCookies();
      LBCookieStore::DeleteAllCookies();
      LBLocalStorageDatabaseAdapter::ClearAll();
      connection->Output("database wiped.\n");
    } else if (tokens[1] == "flush") {
      LBSavegameSyncer::ForceSync(true);
    } else {
      connection->Output("unknown db subcommand, see \"help db\".\n");
    }
  }

 private:
  static void CookiesUsage(LBConsoleConnection *connection) {
    connection->Output(
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
  static void CookiesAdd(LBConsoleConnection* connection,
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

      connection->Output("Cookie added successfully.\n");
    } else {
      connection->Output("There was an error adding the cookie.\n");
    }
  }

  // Delete all cookies that match this criteria
  static void CookiesDelete(LBConsoleConnection* connection,
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
  static void CookiesSearch(LBConsoleConnection* connection,
                            const std::string& domain,
                            const std::string& path,
                            const std::string& name) {
    const std::vector<net::CanonicalCookie*> cookies(
        FindCookies(domain, path, name));
    std::vector<net::CanonicalCookie*>::const_iterator it;
    connection->Output(StringPrintf("%20s %20s %20s %20s\n",
        "domain", "path", "name", "value"));
    connection->Output(StringPrintf("%20s %20s %20s %20s\n",
        "======", "====", "====", "====="));
    for (it = cookies.begin(); it != cookies.end(); it++) {
      connection->Output(StringPrintf("%20s %20s %20s  %s\n",
          (*it)->Domain().c_str(), (*it)->Path().c_str(),
          (*it)->Name().c_str(), (*it)->Value().c_str()));
    }
  }

  // List all cookies
  static void CookiesList(LBConsoleConnection* connection) {
    CookiesSearch(connection, "*", "*", "*");
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandDumpRenderTree
class LBCommandDumpRenderTree : public LBCommand {
 public:
  explicit LBCommandDumpRenderTree(LBDebugConsole *console)
      : LBCommand(console) {
    command_syntax_ = "drt";
    help_summary_ = "Dump Render Tree.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
      base::Bind(DumpRenderTreeTask, shell()->webView()->mainFrame(),
      connection));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandExit
class LBCommandExit : public LBCommand {
 public:
  explicit LBCommandExit(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "exit";
    help_summary_ = "Exit the application.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    shell()->webViewHost()->RequestQuit();
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandForward
class LBCommandForward : public LBCommand {
 public:
  explicit LBCommandForward(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "forward";
    help_summary_ = "Navigate forward.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(HistoryTask, 1, shell()));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandFuzzer
class LBCommandFuzzer : public LBCommand {
 public:
  explicit LBCommandFuzzer(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "fuzzer ...";
    help_summary_ = "Generate and save the random input to a file.\n"
                    "Input is generated at time intervals drawn from a "
                    "Gaussian distribution.\n";
    help_details_ =
        "fuzzer usage:\n"
        "  fuzzer start <file-name> <time-mean> <time-std> [arrows|all]\n"
        "    file-name: name of the output file\n"
        "    time-mean: mean-value for the random time (secs)\n"
        "    time-std: standard deviation for the random time (secs)\n"
        "    arrows: add this if you want just arrow input (optional)\n"
        "    all: add this if you want to fuzz keys that are not mapped"
            " to the controller (optional)\n"
        "    default behaviour is to fuzz only keys that are mapped to a"
            " controller"
        "  fuzzer stop\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    if ((tokens.size() == 5 || tokens.size() == 6) && tokens[1] == "start") {
      // FUZZER START
      connection->Output("starting fuzzer...\n");
      const std::string& file_name = tokens[2];
      const float time_mean = atof(tokens[3].c_str());
      const float time_std = atof(tokens[4].c_str());
      if (tokens.size() == 5) {
        shell()->webViewHost()->FuzzerStart(
            file_name, time_mean, time_std, LBInputFuzzer::kMappedKeys);
      } else if (tokens[5] == "arrows") {
        shell()->webViewHost()->FuzzerStart(
            file_name, time_mean, time_std, LBInputFuzzer::kArrowKeys);
      } else if (tokens[5] == "all") {
        shell()->webViewHost()->FuzzerStart(
            file_name, time_mean, time_std, LBInputFuzzer::kAllKeys);
      }
    } else if (tokens.size() == 2 && tokens[1] == "stop") {
      // FUZZER STOP
      connection->Output("stopping fuzzer...\n");
      shell()->webViewHost()->FuzzerStop();
    } else {
      connection->Output(help_details_);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandHelp

class LBCommandHelp : public LBCommand {
 public:
  explicit LBCommandHelp(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "help [<command>]";
    help_summary_ = "Help.  Prints this message or details about a command.\n";
    help_details_ = "help usage:\n"
                    "  help [<command>]";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    const std::map<std::string, LBCommand*> &commands =
        console_->GetRegisteredCommands();
    std::map<std::string, LBCommand*>::const_iterator itr;

    if (tokens.size() > 1) {
      // Display the 'man' page for the specified command
      std::string token = tokens[1];
      itr = commands.find(token);
      if (itr != commands.end()) {
        connection->Output(itr->second->GetHelpString(false));
      } else {
        connection->Output("Unrecognized command.\n");
      }
    } else {
      // Display the help summary
      // For formatting purposes, determine the max command length
      int max_length = 0;
      for (itr = commands.begin(); itr != commands.end(); itr++) {
        int size = static_cast<int>(itr->second->GetCommandSyntax().size());
        max_length = std::max(size, max_length);
      }

      connection->Output("Debug Console Commands:\n\n");

      // Output the summaries for each item
      for (itr = commands.begin(); itr != commands.end(); itr++) {
        std::string help_string = itr->second->GetCommandSyntax();
        // Insert proper number of spaces such that the summaries all line up
        for (int spaces = max_length - help_string.size(); spaces > 0;
             spaces--) {
          help_string.append(" ");
        }
        help_string.append(" - ");
        help_string.append(itr->second->GetHelpString(true));
        connection->Output(help_string);
      }

      connection->Output(
            "Help can also display help for a single command. For example,\n"
            "'help nav' will print more info about the nav command.\n");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandInput

class LBCommandInput : public LBCommand {
 public:
  explicit LBCommandInput(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "input ...";
    help_summary_ = "User input recording and playback.\n";
    help_details_ = "input usage:\n\n"

                    "  input record [<filename>]\n"
                    "start recording controller input to filename\n"
                    "omit filename to stop\n\n"

                    "  input play <once/repeat> [<filename>]\n"
                    "playback recorded input from filename\n"
                    "omit filename to stop\n\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    if (tokens[1] == "record") {
      // check for filename
      if (tokens.size() == 3) {
        shell()->webViewHost()->RecordKeyInput(tokens[2].c_str());
        connection->Output("recording input.\n");
      } else {
        // call with NULL to stop recording
        shell()->webViewHost()->RecordKeyInput(NULL);
        connection->Output("recording stopped.\n");
      }
    } else if (tokens[1] == "play") {
      if (tokens.size() == 4) {
        bool repeat = tokens[2] == "repeat";
        shell()->webViewHost()->PlaybackKeyInput(tokens[3].c_str(), repeat);
        connection->Output("input playback started.\n");
      } else {
        shell()->webViewHost()->PlaybackKeyInput(NULL, false);
        connection->Output("input playback stopped.\n");
      }
    } else {
      connection->Output("unknown input subcommand, see \"help input\".\n");
    }
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommandKey

class LBCommandKey : public LBCommand {
 public:
  explicit LBCommandKey(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "key <name>";
    help_summary_ = "Send a controller keypress to the browser.\n";
    help_details_ = "key usage:\n"
                    "  key <name>\n"
                    "where <name> is one of:\n"
                    "  up, down, left, right, enter, escape\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
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
      connection->Output(tokens[1] + ": unsupported key name.\n");
      connection->Output("Type 'key help' for more info.\n");
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
  explicit LBCommandLabel(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "label <label id>";
    help_summary_ = "Navigate to a specific label.\n";
    help_details_ = "label usage:\n"
                    "  label <label id>\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    std::string url = shell()->GetStartupURL();
    size_t question_mark = url.find('?');
    if (question_mark != std::string::npos) {
      url.erase(question_mark);
    }
    url.append("?version=4&label=");
    url.append(tokens[1]);
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(NavigateTask, GURL(url), shell()));
  }
};



////////////////////////////////////////////////////////////////////////////////
// LBCommandLang

class LBCommandLang : public LBCommand {
 public:
  explicit LBCommandLang(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "lang <lang code>";
    help_summary_ = "Override the system language.\n";
    help_details_ = "lang usage:\n"
                    "  lang <lang code>\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    shell()->SetPreferredLanguage(tokens[1]);
    // Reinitialize the xlb string database
    LBShell::InitStrings();
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandLayerBackingsInfo

class LBCommandLayerBackingsInfo : public LBCommand {
 public:
  explicit LBCommandLayerBackingsInfo(LBDebugConsole *console)
      : LBCommand(console) {
    command_syntax_ = "lbi";
    help_summary_ = "Dump info about current backed render layers to the "
                    "console.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection* connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(DumpCompositeInfo, shell()->webView()->mainFrame(),
                   connection));
  }

 private:
  static void DumpCompositeInfo(WebKit::WebFrame *webframe,
                                LBConsoleConnection *connection) {
    std::string dump =
        UTF16ToUTF8(webframe->layerBackingsInfo());
    connection->Output(dump);
    DLOG(INFO) << dump;
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandLocation

class LBCommandLocation : public LBCommand {
 public:
  explicit LBCommandLocation(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "location";
    help_summary_ = "Dump the current URL.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    // get current URL
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(LocationTask, shell(), connection));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandLogToTelnet
class LBCommandLogToTelnet : public LBCommand {
 public:
  explicit LBCommandLogToTelnet(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "log2telnet <on/off>";
    help_summary_ = "Log all output to the telnet console.\n";
    help_details_ = "log2telnet usage:\n"
                    "  log2telnet <on/off>\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    if (tokens.size() > 1) {
      if (tokens[1] == "on") {
        LBNetworkConsole::CaptureLogging(true);
      } else if (tokens[1] == "off") {
        LBNetworkConsole::CaptureLogging(false);
      } else {
        connection->Output(help_details_);
      }
    } else {
      connection->Output(help_details_);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandMemDump
class LBCommandMemDump : public LBCommand {
 public:
  explicit LBCommandMemDump(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "memdump [<file>]";
    help_summary_ = "Dump a raw memory allocation table to disk.\n";
    help_details_ = "memdump usage:\n"
                    "  memdump [<file>]\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    std::string filename = "memdump.txt";
    if (tokens.size() > 1) {
      filename = tokens[1];
    }
    LB::Memory::DumpCallers(filename.c_str());
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandMemGraph
class LBCommandMemGraph : public LBCommand {
 public:
  explicit LBCommandMemGraph(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "memgraph [<file>]";
    help_summary_ = "Dump a fragmentation graph to disk.\n";
    help_details_ = "memgraph usage:\n"
                    "  memgraph [<file>|continuous]\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    if (tokens.size() > 1 && tokens[1] == "continuous") {
      ContinuousGraphTask(shell());
    } else {
      std::string filename = "memgraph.png";
      if (tokens.size() > 1) {
        filename = tokens[1];
      }
      LB::Memory::DumpFragmentationGraph(filename.c_str());
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandMemInfo

class LBCommandMemInfo : public LBCommand {
 public:
  explicit LBCommandMemInfo(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "meminfo";
    help_summary_ = "Print current memory statistics.\n";
    help_details_ = "meminfo usage:\n"
                    "  meminfo\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    if (LB::Memory::IsCountEnabled()) {
    // Write some useful stats in JSON format for easier parsing.
#if defined (__LB_PS3__)
      int rsx_mem =
          LBGraphicsPS3::GetPtr()->GetGLResourceManager()->GetRSXUsedMemory();
#endif

      LB::Memory::Info info;
      LB::Memory::GetInfo(&info);

      int shell_allocator_bytes = OSAllocator::getCurrentBytesAllocated();
      int skia_bytes = sk_get_bytes_allocated();

      std::string memory_stats;
      memory_stats.append("{\"memory\": {");
      memory_stats.append(base::StringPrintf(
        "\"application\" : %d"
        ", \"free\" : %d"
        ", \"javascript\" : %d"
        ", \"skia\" : %d",
        static_cast<int>(info.application_memory),
        static_cast<int>(info.free_memory),
        shell_allocator_bytes,
        skia_bytes));
#if defined (__LB_PS3__)
      memory_stats.append(base::StringPrintf(
        ", \"rsx\" : %d", rsx_mem));
#endif
      memory_stats.append("}}\n");
      connection->Output(memory_stats);
    } else {
      connection->Output("Not compiled with kLbMemoryCount enabled.\n");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandNav

class LBCommandNav : public LBCommand {
 public:
  explicit LBCommandNav(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "nav <url>";
    help_summary_ = "Point the browser at supplied URL.\n";
    help_details_ = "nav usage:\n"
                    "  nav <url>\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    GURL url(tokens[1]);
    if (url.is_valid()) {
      shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
          base::Bind(NavigateTask, url, shell()));
    } else {
      connection->Output(tokens[1] + ": bad url.\n");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandPerimeter

#if !defined(__LB_SHELL__FOR_RELEASE__)
class LBCommandPerimeter : public LBCommand {
 public:
  explicit LBCommandPerimeter(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "perimeter <mode>";
    help_summary_ = "Set network perimeter checking level.\n";
    help_details_ = "perimeter usage:\n"
                    "  perimeter <enable|warn|disable>\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    if (tokens[1] == "enable") {
      // activate perimeter checking
      LBResourceLoaderBridge::SetPerimeterCheckEnabled(true);
      LBResourceLoaderBridge::SetPerimeterCheckLogging(true);
      connection->Output("Perimeter checking enabled\n");
    } else if (tokens[1] == "warn") {
      // disable perimeter checking, but log errors
      LBResourceLoaderBridge::SetPerimeterCheckEnabled(false);
      LBResourceLoaderBridge::SetPerimeterCheckLogging(true);
      connection->Output("Perimeter checking disabled with logging\n");
    } else if (tokens[1] == "disable") {
      // disable perimeter checking
      LBResourceLoaderBridge::SetPerimeterCheckEnabled(false);
      LBResourceLoaderBridge::SetPerimeterCheckLogging(false);
      connection->Output("Perimeter checking disabled silently\n");
    } else {
      connection->Output(help_details_);
    }
  }
};
#endif

////////////////////////////////////////////////////////////////////////////////
// LBCommandReload

class LBCommandReload : public LBCommand {
 public:
  explicit LBCommandReload(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "reload";
    help_summary_ = "Reload the current page.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(HistoryTask, 0, shell()));
  }
};


#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
////////////////////////////////////////////////////////////////////////////////
// LBCommandScreenshot

class LBCommandScreenshot : public LBCommand {
 public:
  explicit LBCommandScreenshot(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "ss [<filename>.png]";
    help_summary_ = "Takes a screen shot\n";
    help_details_ = "ss usage:\n"
                    "  ss [<filename>.png]\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    std::string filename = "";
    if (tokens.size() > 2) {
      connection->Output(help_details_);
    } else if (tokens.size() == 2) {
      filename = tokens[1];
    }
    LBGraphics::GetPtr()->TakeScreenshot(filename);
  }
};
#endif

////////////////////////////////////////////////////////////////////////////////
// LBCommandSearch

class LBCommandSearch : public LBCommand {
 public:
  explicit LBCommandSearch(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "search [<terms>]";
    help_summary_ = "Search for the specified video.\n";
    help_details_ = "search usage:\n"
                    "  search [<terms>]\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    // go to search page
    std::string url = shell()->GetStartupURL();
    url.append("#/browse?mode=search&q=");
    for (int i = 1; i < tokens.size(); ++i) {
      url.append(tokens[i]);
      url.append("%20");
    }
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
      base::Bind(NavigateTask, GURL(url), shell()));
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandToggleOSD
class LBCommandToggleOSD : public LBCommand {
 public:
  explicit LBCommandToggleOSD(LBDebugConsole* console) : LBCommand(console) {
    command_syntax_ = "toggle_osd";
    help_summary_ = "Toggle the display of the onscreen display\n"
                    "(FPS/Memory info, etc..)\n";
    help_details_ = "toggle_osd usage:\n"
                    "  toggle_osd\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    LB::OnScreenDisplay::GetPtr()->ToggleStats();
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandTracing

class LBCommandTracing : public LBCommand {
 public:
  explicit LBCommandTracing(LBDebugConsole* console) : LBCommand(console) {
    command_syntax_ = "tracing <start|end>";
    help_summary_ = "Enables or disables Chrome tracing.\n";
    help_details_ = "Records Chrome runtime trace events and will\n"
                    "save them to a file when turned off.\n"
                    "\n"
                    "Results can be viewed by browsing to\n"
                    "about:tracing on your desktop chrome and\n"
                    "loading the produced JSON file.\n\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    if (tokens[1] == "start") {
      if (!shell()->tracing_manager()->IsEnabled()) {
        shell()->tracing_manager()->EnableTracing(true);
      } else {
        connection->Output("Tracing is already enabled!\n");
      }
    } else if (tokens[1] == "end") {
      if (shell()->tracing_manager()->IsEnabled()) {
        shell()->tracing_manager()->EnableTracing(false);
      } else {
        connection->Output("Tracing is not currently enabled!\n");
      }
    } else {
      connection->Output("Unexpected parameter passed to tracing command.\n");
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// LBCommandUnicode
class LBCommandUnicode : public LBCommand {
 public:
  explicit LBCommandUnicode(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "unicode <hex>";
    help_summary_ = "Load a test page for the given hex codepoint.\n";
    help_details_ = "unicode usage:\n"
                    "  unicode <hex codepoint>\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    std::string url = "http://www.fileformat.info/info/unicode/char/"
                      + tokens[1]
                      + "/browsertest.htm";
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(NavigateTask, GURL(url), shell()));
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommandVideo
class LBCommandVideo : public LBCommand {
 public:
  explicit LBCommandVideo(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "video <base64 id>";
    help_summary_ = "Load a specific video in leanback.\n";
    help_details_ = "video usage:\n"
                    "  video <base64 id>\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    std::string url = shell()->GetStartupURL();
    url.append("#/watch?v=");
    url.append(tokens[1]);
    shell()->webViewHost()->webkit_message_loop()->PostTask(FROM_HERE,
        base::Bind(NavigateTask, GURL(url), shell()));
  }
};


////////////////////////////////////////////////////////////////////////////////
// LBCommandWedge
class LBCommandWedge : public LBCommand {
 public:
  explicit LBCommandWedge(LBDebugConsole *console) : LBCommand(console) {
    command_syntax_ = "wedge";
    help_summary_ = "Toggle blocking the WebKit thread.\n";
  }

 protected:
  virtual void DoCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) OVERRIDE {
    // wedge - pause/unpause webkit thread
    if (shell()->webViewHost()->toggleWebKitWedged()) {
      connection->Output("webkit wedged.\n");
    } else {
      connection->Output("webkit unwedged.\n");
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
    DCHECK_EQ(brackets, 0) << "Mismatch in <> brackets.";
    DCHECK_EQ(square_brackets, 0) << "Mismatch in [] brackets.";

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

void LBCommand::ExecuteCommand(
      LBConsoleConnection *connection,
      const std::vector<std::string> &tokens) {
  // Display help for the command if there aren't enough arguments
  if (tokens.size() - 1 < min_arguments_) {
    connection->Output(GetHelpString(false));
    return;
  }
  DoCommand(connection, tokens);
}

const std::string& LBCommand::GetCommandName() const {
  return command_name_;
}

const std::string& LBCommand::GetCommandSyntax() const {
  return command_syntax_;
}


////////////////////////////////////////////////////////////////////////////////
// LBDebugConsole

void LBDebugConsole::Init(LBShell * shell) {
  shell_ = shell;
#if ATTACH_DEBUGGER_ON_STARTUP
  AttachDebugger();
#endif
  if (LB::Memory::IsContinuousGraphEnabled()) {
    ContinuousGraphTask(shell);
  }

  // Register commands
  RegisterCommand(new LBCommandBack(this));
  RegisterCommand(new LBCommandBye(this));
  RegisterCommand(new LBCommandCompositorLogging(this));
  RegisterCommand(new LBCommandCVal(this));
  RegisterCommand(new LBCommandDB(this));
  RegisterCommand(new LBCommandDumpRenderTree(this));
  RegisterCommand(new LBCommandExit(this));
  RegisterCommand(new LBCommandForward(this));
  RegisterCommand(new LBCommandHelp(this));
  RegisterCommand(new LBCommandInput(this));
  RegisterCommand(new LBCommandKey(this));
  RegisterCommand(new LBCommandLabel(this));
  RegisterCommand(new LBCommandLang(this));
  RegisterCommand(new LBCommandLayerBackingsInfo(this));
  RegisterCommand(new LBCommandLocation(this));
  RegisterCommand(new LBCommandLogToTelnet(this));
  RegisterCommand(new LBCommandMemInfo(this));
  RegisterCommand(new LBCommandNav(this));
#if !defined(__LB_SHELL__FOR_RELEASE__)
  RegisterCommand(new LBCommandPerimeter(this));
#endif
  RegisterCommand(new LBCommandReload(this));
#if defined(__LB_SHELL__ENABLE_SCREENSHOT__)
  RegisterCommand(new LBCommandScreenshot(this));
#endif
  RegisterCommand(new LBCommandSearch(this));
  RegisterCommand(new LBCommandUnicode(this));
  RegisterCommand(new LBCommandVideo(this));
  RegisterCommand(new LBCommandWedge(this));
  RegisterCommand(new LBCommandFuzzer(this));
  RegisterCommand(new LBCommandToggleOSD(this));
  RegisterCommand(new LBCommandTracing(this));

  if (LB::Memory::IsDumpCallersEnabled()) {
    RegisterCommand(new LBCommandMemDump(this));
  }

  if (LB::Memory::IsDumpGraphEnabled()) {
    RegisterCommand(new LBCommandMemGraph(this));
  }

  // Register any platform-specific commands
  RegisterPlatformConsoleCommands(this);
}

void LBDebugConsole::Shutdown() {
  std::map<std::string, LBCommand*>::iterator itr;
  for (itr = registered_commands_.begin(); itr != registered_commands_.end();
       itr++) {
    delete itr->second;
  }
}

void LBDebugConsole::RegisterCommand(LBCommand *command) {
  command->ValidateSyntax();
  std::string command_string = command->GetCommandName();
  registered_commands_[command_string] = command;
}

void LBDebugConsole::ParseAndExecuteCommand(
    LBConsoleConnection *connection,
    const std::string& command_string) {
  std::vector<std::string> tokens;
  TokenizeString(command_string, false, &tokens);

  // Do nothing on empty string
  if (tokens.size() == 0)
    return;

  // Find command object and execute commands
  std::map<std::string, LBCommand*>::iterator itr;
  itr = registered_commands_.find(tokens[0]);
  if (itr == registered_commands_.end()) {
    connection->Output(tokens[0] + ": unrecognized command.\n");
    connection->Output("Type 'help' to see all commands.\n");
    return;
  }

  LBCommand *command = itr->second;
  command->ExecuteCommand(connection, tokens);
}

#endif  // __LB_SHELL__ENABLE_CONSOLE__
