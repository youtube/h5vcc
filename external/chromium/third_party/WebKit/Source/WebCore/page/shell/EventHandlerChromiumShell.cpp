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
#include "EventHandler.h"

#include "PlatformKeyboardEvent.h"
#include "MouseEventWithHitTestResults.h"
#include "PlatformWheelEvent.h"
#include "NotImplemented.h"

namespace WebCore {

unsigned EventHandler::accessKeyModifiers()
{
  notImplemented();
  return 0;
}

void EventHandler::focusDocumentView()
{
  notImplemented();
}

bool EventHandler::shouldTurnVerticalTicksIntoHorizontal(const HitTestResult&, const PlatformWheelEvent&) const
{
  notImplemented();
  return false;
}

bool EventHandler::passWheelEventToWidget(const PlatformWheelEvent& wheelEvent, Widget* widget)
{
  notImplemented();
  return false;
}


bool EventHandler::passMouseMoveEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe, HitTestResult* hoveredNode)
{
  notImplemented();
  return false;
}


bool EventHandler::passMouseReleaseEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
  notImplemented();
  return false;
}

bool EventHandler::passMousePressEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
  notImplemented();
  return false;
}

bool EventHandler::passWidgetMouseDownEventToWidget(const MouseEventWithHitTestResults& event)
{
  notImplemented();
  return false;
}

bool EventHandler::passMouseDownEventToWidget(Widget* widget)
{
  notImplemented();
  return false;
}

bool EventHandler::eventActivatedView(const PlatformMouseEvent& event) const
{
  notImplemented();
  return false;
}

bool EventHandler::tabsToAllFormControls(KeyboardEvent*) const
{
  notImplemented();
  return false;
}

} // namespace WebCore
