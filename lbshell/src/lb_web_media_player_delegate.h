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

// Maintains a list of WebMediaPlayer instances and their state to support
// console-specific requirements like global pause of all media.

#ifndef SRC_LB_WEB_MEDIA_PLAYER_DELEGATE_H_
#define SRC_LB_WEB_MEDIA_PLAYER_DELEGATE_H_

#include "external/chromium/base/compiler_specific.h"
#include "external/chromium/base/memory/weak_ptr.h"
#include "external/chromium/webkit/media/webmediaplayer_delegate.h"

#include <map>
#include <list>

#include "lb_shell_export.h"

namespace webkit_media {

class LB_SHELL_EXPORT LBWebMediaPlayerDelegate
    : NON_EXPORTED_BASE(public WebMediaPlayerDelegate)
    , public base::SupportsWeakPtr<LBWebMediaPlayerDelegate> {
 public:
  LBWebMediaPlayerDelegate();
  // singleton instance
  static LBWebMediaPlayerDelegate* Instance();
  // returns a weak pointer to the singleton instance
  static base::WeakPtr<LBWebMediaPlayerDelegate> WeakInstance();
  // object setup and teardown
  static void Initialize();
  // delete the singleton on program exit
  static void Terminate();

  // WebMediaPlayerDelegate implementation, must be called on WebKit thread
  virtual void DidPlay(WebKit::WebMediaPlayer* player) OVERRIDE;
  virtual void DidPause(WebKit::WebMediaPlayer* player) OVERRIDE;
  virtual void PlayerGone(WebKit::WebMediaPlayer* player) OVERRIDE;

  // LB utility methods, need to be run on WebKit thread
  void PauseActivePlayers();
  void ResumeActivePlayers();

 private:
  virtual ~LBWebMediaPlayerDelegate();

  void SetDimmingState();

  // PlayerMap maps players to paused/play state (false/true)
  typedef std::map<WebKit::WebMediaPlayer*, bool> PlayerMap;
  PlayerMap player_map_;
  typedef std::list<WebKit::WebMediaPlayer*> PlayerList;
  PlayerList force_paused_list_;
  static LBWebMediaPlayerDelegate* instance_;
};

}  // namespace webkit_media

#endif  // SRC_LB_WEB_MEDIA_PLAYER_DELEGATE_H_
