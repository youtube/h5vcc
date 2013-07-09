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

/*
For plugins the inheritance chain is:

WebCore::Widget <-- WebCore::PluginViewBase <-- WebCore::PluginView.
                                ^
                                |
WebKit::WebPluginContainer <-- WebKit:: WebPluginContainerImpl

Chromium uses PluginViewBase and handles V8 bindings through that interface
completely outside of WebKit. JavaScriptCore assumes that its plugin bindings
happen within PluginView. A lot of WebKit/chromium code assumes that the plugin
object is a WebPluginContainerImpl.  By adding one function to PluginViewBase,
NPObject * WebPluginContainerImpl::scriptableObject(), we are able to add some
glue code here to the ScriptController that can cast the supplied Widget to
PluginViewBase and extract an NPObject from the WebPluginContainerImpl, which
extracts it from the WebPlugin object, allowing clients to create simple
scriptable plugins by overriding only that method in WebPlugin and returning the
appropriate NPObject, on which methods can be called via JavaScript.
*/
#include "config.h"
#include "ScriptController.h"

#include "BridgeJSC.h"
#include "c_instance.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "npruntime_impl.h"
#include "PluginViewBase.h"
#include "runtime_root.h"

#include <runtime/JSLock.h>
#include <runtime/JSValue.h>

using namespace JSC::Bindings;

namespace WebCore {

PassRefPtr<JSC::Bindings::Instance> ScriptController::createScriptInstanceForWidget(Widget* widget)
{
  PluginViewBase * plugin = reinterpret_cast<PluginViewBase*>(widget);
  if (!plugin)
    return 0;

  // FIXME: Do we still need the PluginViewBase hacks, or can we use scriptObject instead?
  NPObject * npObject = plugin->scriptableObject();
  if (!npObject)
    return 0;

  // this portion copied/modified from PluginView.cpp PluginView:
  RefPtr<JSC::Bindings::RootObject> root = createRootObject(widget);
  RefPtr<JSC::Bindings::Instance> instance = 
      JSC::Bindings::CInstance::create(npObject, root.release());

  _NPN_ReleaseObject(npObject);

  return instance.release();
}


} // namespace WebCore
