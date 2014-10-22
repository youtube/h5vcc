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
// Many functions are the same as the original version.

#include "config.h"
#include "XMLDocumentParserShellDelegate.h"

#include "CachedScript.h"
#include "CDATASection.h"
#include "Document.h"
#include "DocumentType.h"
#include "dom/Comment.h"
#include "Frame.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "ProcessingInstruction.h"
#include "ResourceHandle.h"
#include "ScriptElement.h"
#include "ScriptSourceCode.h"
#include "SecurityOrigin.h"
#include "TextResourceDecoder.h"
#include "TransformSource.h"
#include "XMLDocumentParser.h"
#include "XMLDocumentParserScope.h"
#include "XMLDocumentParserShellEvent.h"

namespace WebCore {

using namespace std;

void XMLDocumentParserShellDelegate::onError(XMLErrors::ErrorType type, const char* message, va_list args)
{
    // Create event and set parameters
    String errorMessage = String::format(message, args);
    ErrorXMLEvent* event = new ErrorXMLEvent(type, errorMessage, m_owner->textPosition());
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onStartDocument(const String& version, const String& encoding, int standalone)
{
    StartDocumentXMLEvent* event = new StartDocumentXMLEvent(version, encoding, standalone);
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onEndDocument()
{
    EndDocumentXMLEvent* event = new EndDocumentXMLEvent();
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onStartElement(
    const AtomicString& xmlLocalName, const AtomicString& xmlPrefix,
    const AtomicString& xmlURI, Vector<Attribute, 8>& prefixedAttributes,
    int nb_defaulted)
{
    // Create event and set parameters
    StartElementXMLEvent* event = new StartElementXMLEvent(xmlLocalName, xmlPrefix, xmlURI, prefixedAttributes, nb_defaulted);
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onEndElement()
{
    // Create event
    EndElementXMLEvent* event = new EndElementXMLEvent();
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onComment(const String& comment)
{
    // Create event and set parameters
    CommentXMLEvent* event = new CommentXMLEvent(comment);
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onCharacters(const String& data)
{
    // Create event and set parameters
    CharactersXMLEvent* event = new CharactersXMLEvent(data);
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onCData(const String& cdata)
{
    // Create event and set parameters
    CDataXMLEvent* event = new CDataXMLEvent(cdata);
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onProcessingInstruction(const String& target, const String& data)
{
    // Create event and set parameters
    ProcessInstructionXMLEvent* event = new ProcessInstructionXMLEvent(target, data);
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onInternalSubset(const String& name, const String& externalID, const String& systemID)
{
    // Create event and set parameters
    InternalSubsetXMLEvent* event = new InternalSubsetXMLEvent(name, externalID, systemID);
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onExternalSubset(const String& extId)
{
    // Create event and set parameters
    ExternalSubsetXMLEvent* event = new ExternalSubsetXMLEvent(extId);
    // Send back to XMLDocumentParser
    m_owner->appendEvent(event);
}

void XMLDocumentParserShellDelegate::onStopParsing()
{
    m_owner->stopParsing();
}

bool XMLDocumentParserShellDelegate::isXHTMLDocument() const
{
    return m_owner->isXHTMLDocument();
}

Document* XMLDocumentParserShellDelegate::document() const
{
    return m_owner->document();
}

}  // namespace WebCore
