/*
 * Copyright (C) 2013 Google Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef Shell_XMLDocumentParserShellEventBase_h
#define Shell_XMLDocumentParserShellEventBase_h

namespace WebCore {

// XML event execution results
enum XMLEventRunResult {
    ResultContinue,         // The event runs successfully, continue to the next event
    ResultPause,            // The even needs a pause
    ResultError,            // An error is found
};

class Document;
class XMLDocumentParser;

// Base class of all events
class XMLEventBase {
public:
    XMLEventBase() {}
    virtual ~XMLEventBase() {}

    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) = 0;
};

}  // namespace WebCore
#endif  // Shell_XMLDocumentParserShellEventBase_h
