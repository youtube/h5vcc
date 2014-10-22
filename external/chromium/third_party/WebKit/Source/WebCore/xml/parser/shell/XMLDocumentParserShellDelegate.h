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

#ifndef Shell_XMLDocumentParserShellDelegate_h
#define Shell_XMLDocumentParserShellDelegate_h

#include "Attribute.h"
#include "FragmentScriptingPermission.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/text/TextPosition.h>
#include <wtf/text/WTFString.h>
#include "XMLErrors.h"

namespace WebCore {

class ContainerNode;
class Document;
class FrameView;
class Text;
class XMLDocumentParser;
class XMLDocumentParserShellDelegate;
class XMLDocumentParserShellStream;
// The function below is just a declaration and the implementation
// is not included in XMLDocumentParserShellDelegate.cpp.
// Different platform needs to select the right implementation of
// XMLDocumentParserShellDelegate to create (and implement if necessary)
// by including the right implementation of this creation function.
WTF::PassRefPtr<XMLDocumentParserShellDelegate>
    createXMLDocumentParserShellDelegate(XMLDocumentParser* owner,
                                         XMLDocumentParserShellStream* stream);

// XMLDocumentParserShellDelegate : this is the new XML parser API,
// including the virtual function needed to be implemented in subclasses
// and helper functions to be used by subclasses.
class XMLDocumentParserShellDelegate : public RefCounted<XMLDocumentParserShellDelegate> {
    WTF_MAKE_NONCOPYABLE(XMLDocumentParserShellDelegate);
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~XMLDocumentParserShellDelegate() {}

    // Interface functions. Subclasses need to implement these functions
    // Init parser
    virtual void init(XMLDocumentParserShellStream* stream) = 0;
    virtual void parse() = 0;
    virtual void stop() = 0;
    virtual void resume() = 0;
    virtual void deinit() = 0;
    virtual int column() const = 0;
    virtual int line() const = 0;

    // A function to parse attributes from a string directly without
    // creating an object. Subclass needs to implement it
    static bool parseAttributes(const XMLDocumentParserShellStream* stream, HashMap<String, String>* attributes);

    // Helper functions to be used by sub classes. These functions perform
    // DOM tasks according to incoming event
    void onError(XMLErrors::ErrorType type, const char* message, va_list args);
    void onStartDocument(const String& version, const String& encoding, int standalone);
    void onEndDocument();
    void onStartElement(const AtomicString& xmlLocalName, const AtomicString& xmlPrefix, const AtomicString& xmlURI,
                        Vector<Attribute, 8>& prefixedAttributes, int nb_defaulted);
    void onEndElement();
    void onComment(const String& comment);
    void onCharacters(const String& data);
    void onCData(const String& cdata);
    void onProcessingInstruction(const String& target, const String& data);
    void onInternalSubset(const String& name, const String& externalID, const String& systemID);
    void onExternalSubset(const String& name);
    void onStopParsing();

    bool isXHTMLDocument() const;

protected:
    XMLDocumentParserShellDelegate(XMLDocumentParser* owner, XMLDocumentParserShellStream* stream)
        : m_owner(owner), m_stream(stream) {}

    // Access document object in XMLDocumentParser
    Document* document() const;

    // Pointing back to XMLDocumentParser to send event back.
    XMLDocumentParser* m_owner;
    // Pointing to input stream
    XMLDocumentParserShellStream* m_stream;
};

}
#endif  // Shell_XMLDocumentParserShellDelegate_h