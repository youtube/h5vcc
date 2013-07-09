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

#include "lb_web_media_player_delegate.h"

#include "external/chromium/third_party/WebKit/Source/WebKit/chromium/public/WebMediaPlayer.h"
#include "lb_graphics.h"

namespace webkit_media {

LBWebMediaPlayerDelegate* LBWebMediaPlayerDelegate::instance_ = NULL;

LBWebMediaPlayerDelegate::LBWebMediaPlayerDelegate()
    : force_pausing_(false) {
}

// static
LBWebMediaPlayerDelegate* LBWebMediaPlayerDelegate::Instance() {
  return instance_;
}

// static
base::WeakPtr<LBWebMediaPlayerDelegate>
     LBWebMediaPlayerDelegate::WeakInstance() {
  return instance_->AsWeakPtr();
}

// static
void LBWebMediaPlayerDelegate::Initialize() {
  instance_ = new LBWebMediaPlayerDelegate();
}

// static
void LBWebMediaPlayerDelegate::Terminate() {
  delete instance_;
  instance_ = NULL;
}

void LBWebMediaPlayerDelegate::DidPlay(WebKit::WebMediaPlayer* player) {
  // add or update player map with this instance of the player
  player_map_[player] = true;
  SetDimmingState();
}

void LBWebMediaPlayerDelegate::DidPause(WebKit::WebMediaPlayer* player) {
  // this will be called when we force pause players, skip if so
  if (!force_pausing_) {
    player_map_[player] = false;
  }
  SetDimmingState();
}

void LBWebMediaPlayerDelegate::PlayerGone(WebKit::WebMediaPlayer* player) {
  // look for our player in the player map
  PlayerMap::iterator it = player_map_.find(player);
  if (it != player_map_.end()) {
    player_map_.erase(it);
  }
  SetDimmingState();
}

void LBWebMediaPlayerDelegate::PauseActivePlayers() {
  force_pausing_ = true;
  // pause all currently playing players in the map
  for (PlayerMap::iterator it = player_map_.begin();
       it != player_map_.end(); ++it) {
    if (it->second) {
      it->first->pause();
      force_paused_list_.push_back(it->first);
    }
  }
  force_pausing_ = false;
}

void LBWebMediaPlayerDelegate::ResumeActivePlayers() {
  // resume all force-paused players in the list
  for (PlayerList::iterator it = force_paused_list_.begin();
      it != force_paused_list_.end(); ++it) {
    // make sure object still exists in our map
    PlayerMap::iterator m_it = player_map_.find(*it);
    if (m_it != player_map_.end()) {
      (*it)->play();
    }
  }
  force_paused_list_.clear();
}

LBWebMediaPlayerDelegate::~LBWebMediaPlayerDelegate() {
}

void LBWebMediaPlayerDelegate::SetDimmingState() {
  int num_paused = 0;
  for (PlayerMap::iterator it = player_map_.begin();
       it != player_map_.end(); ++it) {
    if (it->second == false) {
      ++num_paused;
    }
  }

  // If there are no players, or if all players are paused,
  // enable screen-dimming.
  if (num_paused == player_map_.size()) {
    LBGraphics::GetPtr()->EnableDimming();
  } else {
    LBGraphics::GetPtr()->DisableDimming();
  }
}

}  // namespace webkit_media

