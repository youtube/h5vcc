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

#ifndef Shell_XMLDocumentParserShellEvent_h
#define Shell_XMLDocumentParserShellEvent_h

#include "XMLDocumentParserShellEventBase.h"

#include "FragmentScriptingPermission.h"
#include "HTMLHtmlElement.h"
#include "ResourceHandle.h"
#include "ScriptElement.h"
#include "ScriptSourceCode.h"
#include "SecurityOrigin.h"
#include "TextResourceDecoder.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/TextPosition.h>
#include <wtf/text/WTFString.h>
#include <wtf/Vector.h>
#include "XMLErrors.h"

// This file includes an XMLEvent class and a few subclasses for real DOM events.
// The XMLEvent class provides access to some functions/variables in XMLDocumentParser and Document needed by subclasses,
// and the subclasses handles events come from xml library.

namespace WebCore {

class ContainerNode;
class Document;
class FrameView;
class Text;
class XMLDocumentParser;
// Event class with access functions to XMLDocumentParser and Document objects
class XMLEvent : public XMLEventBase {
protected:
    // Helper functions to access XMLDocumentParser variables/functions.
    static ContainerNode* currentNode(const XMLDocumentParser* owner);
    static void enterText(XMLDocumentParser* owner);
    static void exitText(XMLDocumentParser* owner);
    static bool sawFirstElement(const XMLDocumentParser* owner);
    static void setSawFirstElement(XMLDocumentParser* owner);
    static void stopParsing(XMLDocumentParser* owner);
    static FragmentScriptingPermission scriptingPermission(const XMLDocumentParser* owner);
    static void scriptingPermission(XMLDocumentParser* owner, FragmentScriptingPermission permission);
    static void pushCurrentNode(XMLDocumentParser* owner, ContainerNode* node);
    static void popCurrentNode(XMLDocumentParser* owner);
    static bool isDetached(const XMLDocumentParser* owner);
    static bool isStopped(const XMLDocumentParser* owner);
    static FrameView* getFrameView(const XMLDocumentParser* owner);
    static TextPosition textPosition(const XMLDocumentParser* owner);
    static TextPosition scriptStartPosition(const XMLDocumentParser* owner);
    static void setScriptStartPosition(XMLDocumentParser* owner, const TextPosition& position);
    static Text* leafTextNode(const XMLDocumentParser* owner);
    static void setSawCSS(XMLDocumentParser* owner);
    static void setBufferedText(XMLDocumentParser* owner, String& str);
};

// Real DOM events
class ErrorXMLEvent : public XMLEvent{
public:
    ErrorXMLEvent(XMLErrors::ErrorType type, const String& message, const TextPosition& position);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    XMLErrors::ErrorType m_type;
    String m_message;
    TextPosition m_position;
};

class StartDocumentXMLEvent : public XMLEvent {
public:
    StartDocumentXMLEvent(const String& version, const String& encoding, int standalone);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    String m_version;
    String m_encoding;
    int m_standalone;
};

class EndDocumentXMLEvent : public XMLEvent {
public:
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;
};

class StartElementXMLEvent : public XMLEvent {
public:
    StartElementXMLEvent(const AtomicString& xmlLocalName, const AtomicString& xmlPrefix, const AtomicString& xmlURI,
                        Vector<Attribute, 8>& prefixedAttributes, int nb_defaulted);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    AtomicString m_localName;
    AtomicString m_prefix;
    AtomicString m_URI;
    Vector<Attribute, 8> m_prefixedAttributes;
    int m_nb_defaulted;
};

class EndElementXMLEvent : public XMLEvent {
public:
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;
};

class CDataXMLEvent : public XMLEvent {
public:
    CDataXMLEvent(const String& cdata);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    String m_data;
};

class CommentXMLEvent : public XMLEvent {
public:
    CommentXMLEvent(const String& comment);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    String m_comment;
};

class CharactersXMLEvent : public XMLEvent {
public:
    CharactersXMLEvent(const String& data);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    String m_data;
};

class ProcessInstructionXMLEvent : public XMLEvent {
public:
    ProcessInstructionXMLEvent(const String& target, const String& data);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    String m_target;
    String m_data;
};

class InternalSubsetXMLEvent : public XMLEvent {
public:
    InternalSubsetXMLEvent(const String& name, const String& externalID, const String& systemID);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    String m_name;
    String m_externalID;
    String m_systemID;
};

class ExternalSubsetXMLEvent : public XMLEvent {
public:
    ExternalSubsetXMLEvent(const String& externalID);
    virtual XMLEventRunResult run(XMLDocumentParser* owner, Document* document) OVERRIDE;

private:
    String m_externalID;
};

}  // namespace WebCore
#endif  // Shell_XMLDocumentParserShellEvent_h
