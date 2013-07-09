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

#include "lb_shell_switches.h"

namespace LB {
namespace switches {

// The page to load on startup.
const char kUrl[] = "url";

// The part of the url after the '?', since certain platforms don't support
// that character on the command line
const char kQueryString[] = "query";

// Additional WebKit channels to open up for logging. Separate multiple
// channels by a ',' character.
const char kWebCoreLogChannels[] = "webcore-log-channels";

// Prevent LBSavegameSyncer from saving the game.
const char kDisableSave[] = "disable-save";

// Force LBSavegameSyncer to load a different file.
const char kLoadSavegame[] = "load-savegame";

// Override the User Agent string
const char kUserAgent[] = "user-agent";

// Turn on shell filter graph logging
const char kFilterGraphLog[] = "filter-graph-log";

#if defined(__LB_WIIU__)
// Test the error viewer by continuously reloading and displaying an error
// message.  Pass the error message ID as a value.
const char kErrorTest[] = "error-test";

#endif

// Override the system language.  This is a two-letter language code with an
// optional country code, such as "de" or "pt-BR".
const char kLang[] = "lang";

// Ignore platform specific authentication (PSN, Miiverse, etc).
const char kIgnorePlatformAuthentication[] = "ignore-platform-authentication";

#if defined(__LB_LINUX__)
// Print the version number and exit.
const char kVersion[] = "version";
#endif

}  // namespace switches
}  // namespace LB

