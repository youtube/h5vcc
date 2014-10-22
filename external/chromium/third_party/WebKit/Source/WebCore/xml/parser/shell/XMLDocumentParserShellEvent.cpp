/*
 * Copyright (C) 2000 Peter Kelly <pmk@post.com>
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

// Note: this file is a refactored version of ../XMLDocumentParserLibxml2.cpp.
// Many functions contain the same code as the original version, but the
// function names might be very different.

#include "config.h"
#include "XMLDocumentParserShellEvent.h"

#include "CachedScript.h"
#include "CDATASection.h"
#include "Document.h"
#include "DocumentType.h"
#include "dom/Comment.h"
#include "Frame.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "ProcessingInstruction.h"
#include "TransformSource.h"
#include "XMLDocumentParser.h"
#include "XMLDocumentParserScope.h"

namespace WebCore {

using namespace std;

/* static */
ContainerNode* XMLEvent::currentNode(const XMLDocumentParser* owner)
{
    return owner->m_currentNode;
}

/* static */
void XMLEvent::enterText(XMLDocumentParser* owner)
{
    owner->enterText();
}

/* static */
void XMLEvent::exitText(XMLDocumentParser* owner)
{
    owner->exitText();
}

/* static */
bool XMLEvent::sawFirstElement(const XMLDocumentParser* owner)
{
    return owner->m_sawFirstElement;
}

/* static */
void XMLEvent::setSawFirstElement(XMLDocumentParser* owner)
{
    owner->m_sawFirstElement = true;
}

/* static */
void XMLEvent::setBufferedText(XMLDocumentParser* owner, String& str)
{
    owner->m_bufferedText.swap(str);
}

/* static */
void XMLEvent::stopParsing(XMLDocumentParser* owner)
{
    owner->stopParsing();
}

/* static */
FragmentScriptingPermission XMLEvent::scriptingPermission(const XMLDocumentParser* owner)
{
    return owner->m_scriptingPermission;
}

/* static */
void XMLEvent::scriptingPermission(XMLDocumentParser* owner, FragmentScriptingPermission permission)
{
    owner->m_scriptingPermission = permission;
}

/* static */
void XMLEvent::pushCurrentNode(XMLDocumentParser* owner, ContainerNode* node)
{
    owner->pushCurrentNode(node);
}

/* static */
void XMLEvent::popCurrentNode(XMLDocumentParser* owner)
{
    owner->popCurrentNode();
}

/* static */bool XMLEvent::isDetached(const XMLDocumentParser* owner)
{
    return owner->isDetached();
}

/* static */
bool XMLEvent::isStopped(const XMLDocumentParser* owner)
{
    return owner->isStopped();
}

/* static */
FrameView* XMLEvent::getFrameView(const XMLDocumentParser* owner)
{
    return owner->m_view;
}

/* static */
TextPosition XMLEvent::textPosition(const XMLDocumentParser* owner)
{
    return owner->textPosition();
}

/* static */
TextPosition XMLEvent::scriptStartPosition(const XMLDocumentParser* owner)
{
    return owner->m_scriptStartPosition;
}

/* static */
void XMLEvent::setScriptStartPosition(XMLDocumentParser* owner, const TextPosition& position)
{
    owner->m_scriptStartPosition = position;
}

/* static */
Text* XMLEvent::leafTextNode(const XMLDocumentParser* owner)
{
    return owner->m_leafTextNode.get();
}

/* static */
void XMLEvent::setSawCSS(XMLDocumentParser* owner)
{
    owner->m_sawCSS = true;
}

ErrorXMLEvent::ErrorXMLEvent(XMLErrors::ErrorType type, const String& message, const TextPosition& position)
    : m_type(type)
    , m_message(message)
    , m_position(position)
{
}

/* virtual */
XMLEventRunResult ErrorXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    owner->handleError(m_type, m_message.utf8().data(), m_position);
    return ResultContinue;
}


enum StandaloneInfo {
    StandaloneUnspecified = -2,
    NoXMlDeclaration,
    StandaloneNo,
    StandaloneYes
};

StartDocumentXMLEvent::StartDocumentXMLEvent(const String& version, const String& encoding, int standalone)
    : m_version(version)
    , m_encoding(encoding)
    , m_standalone(standalone)

{
}

/* virtual */
XMLEventRunResult StartDocumentXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    StandaloneInfo standaloneInfo = (StandaloneInfo)m_standalone;
    if (standaloneInfo == NoXMlDeclaration) {
        document->setHasXMLDeclaration(false);
        return ResultContinue;
    }

    document->setXMLVersion(m_version, ASSERT_NO_EXCEPTION);
    if (m_standalone != StandaloneUnspecified)
        document->setXMLStandalone(standaloneInfo == StandaloneYes, ASSERT_NO_EXCEPTION);
    document->setXMLEncoding(m_encoding);
    document->setHasXMLDeclaration(true);
    return ResultContinue;
}

/* virtual */
XMLEventRunResult EndDocumentXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    return ResultContinue;
}

StartElementXMLEvent::StartElementXMLEvent(
    const AtomicString& xmlLocalName, const AtomicString& xmlPrefix,
    const AtomicString& xmlURI, Vector<Attribute, 8>& prefixedAttributes,
    int nb_defaulted)
    : m_localName(xmlLocalName)
    , m_prefix(xmlPrefix)
    , m_URI(xmlURI)
    , m_nb_defaulted(nb_defaulted)
{
    m_prefixedAttributes.swap(prefixedAttributes);
}

/* virtual */
XMLEventRunResult StartElementXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    exitText(owner);

    bool isFirstElement = !sawFirstElement(owner);
    setSawFirstElement(owner);

    QualifiedName qName(m_prefix, m_localName, m_URI);
    RefPtr<Element> newElement = document->createElement(qName, true);
    if (!newElement) {
        stopParsing(owner);
        return ResultError;
    }

    newElement->parserSetAttributes(m_prefixedAttributes, scriptingPermission(owner));
    newElement->beginParsingChildren();

    ScriptElement* scriptElement = toScriptElement(newElement.get());
    if (scriptElement)
        setScriptStartPosition(owner, textPosition(owner));

    currentNode(owner)->parserAppendChild(newElement.get());

    pushCurrentNode(owner, newElement.get());
    if (getFrameView(owner) && !newElement->attached())
        newElement->attach();

    if (newElement->hasTagName(HTMLNames::htmlTag))
        static_cast<HTMLHtmlElement*>(newElement.get())->insertedByParser();

    if (isFirstElement && document->frame())
        document->frame()->loader()->dispatchDocumentElementAvailable();
    return ResultContinue;
}

/* virtual */
XMLEventRunResult EndElementXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    if (isStopped(owner))
        return ResultContinue;

    // JavaScript can detach the parser.  Make sure this is not released
    // before the end of this method.
    RefPtr<XMLDocumentParser> protect(owner);

    exitText(owner);

    RefPtr<ContainerNode> n = currentNode(owner);
    n->finishParsingChildren();

    if (scriptingPermission(owner) == DisallowScriptingContent && n->isElementNode() && toScriptElement(static_cast<Element*>(n.get()))) {
        popCurrentNode(owner);
        ExceptionCode ec;
        n->remove(ec);
        return ResultError;
    }

    if (!n->isElementNode() || !getFrameView(owner)) {
        popCurrentNode(owner);
        return ResultError;
    }

    Element* element = static_cast<Element*>(n.get());

    // The element's parent may have already been removed from document.
    // Parsing continues in this case, but scripts aren't executed.
    if (!element->inDocument()) {
        popCurrentNode(owner);
        return ResultError;
    }

    ScriptElement* scriptElement = toScriptElement(element);
    if (!scriptElement) {
        popCurrentNode(owner);
        return ResultError;
    }

    // Don't load external scripts for standalone documents (for now).
    CachedResourceHandle<CachedScript> pendingScript;
    ASSERT(!pendingScript);

    if (scriptElement->prepareScript(scriptStartPosition(owner), ScriptElement::AllowLegacyTypeInTypeAttribute)) {
        // FIXME: Script execution should be shared between
        // the libxml2 and Qt XMLDocumentParser implementations.

        if (scriptElement->readyToBeParserExecuted())
            scriptElement->executeScript(ScriptSourceCode(scriptElement->scriptContent(), document->url(), scriptStartPosition(owner)));
        else if (scriptElement->willBeParserExecuted()) {
            pendingScript = scriptElement->cachedScript();
            pendingScript->addClient(owner);
        }

        // JavaScript may have detached the parser
        if (isDetached(owner))
            return ResultContinue;
    }
    popCurrentNode(owner);
    return ResultContinue;
}

CDataXMLEvent::CDataXMLEvent(const String& cdata)
    : m_data(cdata)
{
}

/* virtual */
XMLEventRunResult CDataXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    exitText(owner);

    RefPtr<CDATASection> newNode = CDATASection::create(document, m_data);
    currentNode(owner)->parserAppendChild(newNode.get());
    if (getFrameView(owner) && !newNode->attached())
        newNode->attach();
   return ResultContinue;
}

CommentXMLEvent::CommentXMLEvent(const String& comment)
    : m_comment(comment)

{
}

/* virtual */
XMLEventRunResult CommentXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    exitText(owner);

    RefPtr<Comment> newNode = Comment::create(document, m_comment);
    currentNode(owner)->parserAppendChild(newNode.get());
    if (getFrameView(owner) && !newNode->attached())
        newNode->attach();
   return ResultContinue;
}

CharactersXMLEvent::CharactersXMLEvent(const String& data)
    : m_data(data)
{
}

/* virtual */
XMLEventRunResult CharactersXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    if (!leafTextNode(owner))
        enterText(owner);
    setBufferedText(owner, m_data);
    return ResultContinue;
}

ProcessInstructionXMLEvent::ProcessInstructionXMLEvent(const String& target, const String& data)
    : m_target(target)
    , m_data(data)
{
}

/* virtual */
XMLEventRunResult ProcessInstructionXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
   exitText(owner);

    // ### handle exceptions
    ExceptionCode ec = 0;
    RefPtr<ProcessingInstruction> pi =
        document->createProcessingInstruction(m_target, m_data, ec);
    if (ec)
        return ResultError;

    pi->setCreatedByParser(true);

    currentNode(owner)->parserAppendChild(pi.get());
    if (getFrameView(owner) && !pi->attached())
        pi->attach();

    pi->finishParsingChildren();

    if (pi->isCSS())
        setSawCSS(owner);
    return ResultContinue;
}

InternalSubsetXMLEvent::InternalSubsetXMLEvent(const String& name, const String& externalID, const String& systemID)
    : m_name(name)
    , m_externalID(externalID)
    , m_systemID(systemID)
{
}

/* virtual */
XMLEventRunResult InternalSubsetXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    if (document)
        document->parserAppendChild(DocumentType::create(document, m_name, m_externalID, m_systemID));
    return ResultContinue;
}

ExternalSubsetXMLEvent::ExternalSubsetXMLEvent(const String& externalID)
    : m_externalID(externalID)
{
}

/* virtual */
XMLEventRunResult ExternalSubsetXMLEvent::run(XMLDocumentParser* owner, Document* document)
{
    if ((m_externalID == "-//W3C//DTD XHTML 1.0 Transitional//EN")
        || (m_externalID == "-//W3C//DTD XHTML 1.1//EN")
        || (m_externalID == "-//W3C//DTD XHTML 1.0 Strict//EN")
        || (m_externalID == "-//W3C//DTD XHTML 1.0 Frameset//EN")
        || (m_externalID == "-//W3C//DTD XHTML Basic 1.0//EN")
        || (m_externalID == "-//W3C//DTD XHTML 1.1 plus MathML 2.0//EN")
        || (m_externalID == "-//W3C//DTD XHTML 1.1 plus MathML 2.0 plus SVG 1.1//EN")
        || (m_externalID == "-//WAPFORUM//DTD XHTML Mobile 1.0//EN"))
        owner->setIsXHTMLDocument(true); // controls if we replace entities or not.
    return ResultContinue;
}

}  // namespace WebCore
