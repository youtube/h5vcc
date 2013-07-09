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

#include "config.h"
#include "Pasteboard.h"

#include "DocumentFragment.h"
#include "Frame.h"
#include "Image.h"
#include "KURL.h"
#include "markup.h"
#include "NotImplemented.h"
#include "RenderImage.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

Pasteboard* Pasteboard::generalPasteboard() {
  static Pasteboard* pasteboard = new Pasteboard();
  return pasteboard;
}

Pasteboard::Pasteboard() {
  notImplemented();
}

bool Pasteboard::canSmartReplace() {
  notImplemented();
  return false;
}

void Pasteboard::clear() {
  notImplemented();
}

void Pasteboard::writeClipboard(Clipboard*) {
  notImplemented();
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame*,
                                                          PassRefPtr<Range>,
                                                          bool,
                                                          bool&) {
  notImplemented();
  return 0;
}

bool Pasteboard::isSelectionMode() const {
  return false;
}

String Pasteboard::plainText(Frame*) {
  notImplemented();
  return String();
}

void Pasteboard::setSelectionMode(bool) {
  notImplemented();
}

void Pasteboard::writeImage(Node*, const KURL&, const String&) {
  notImplemented();
}

void Pasteboard::writePlainText(const String&, SmartReplaceOption) {
  notImplemented();
}

void Pasteboard::writeSelection(Range*, bool, Frame*) {
  notImplemented();
}

void Pasteboard::writeURL(const KURL&, const String&, Frame*) {
  notImplemented();
}

}
