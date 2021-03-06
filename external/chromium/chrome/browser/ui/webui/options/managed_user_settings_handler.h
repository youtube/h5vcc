// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OPTIONS_MANAGED_USER_SETTINGS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_OPTIONS_MANAGED_USER_SETTINGS_HANDLER_H_

#include "chrome/browser/ui/webui/options/options_ui.h"

namespace options {

class ManagedUserSettingsHandler : public OptionsPageUIHandler {
 public:
  ManagedUserSettingsHandler();
  virtual ~ManagedUserSettingsHandler();

  // OptionsPageUIHandler implementation.
  virtual void GetLocalizedValues(
      base::DictionaryValue* localized_strings) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(ManagedUserSettingsHandler);
};

}  // namespace options

#endif  // CHROME_BROWSER_UI_WEBUI_OPTIONS_MANAGED_USER_SETTINGS_HANDLER_H_
