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
#include "XMLDocumentParser.h"

#include "CachedScript.h"
#include "CDATASection.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "HTMLElement.h"
#include "XMLDocumentParserScope.h"
#include "XMLDocumentParserShellDelegate.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/Vector.h>

namespace WebCore {

using namespace std;
using namespace WTF;

// --------------------------------

bool XMLDocumentParser::supportsXMLVersion(const String& version)
{
    return version == "1.0";
}

XMLDocumentParser::XMLDocumentParser(Document* document, FrameView* frameView)
    : ScriptableDocumentParser(document)
    , m_view(frameView)
    , m_currentNode(document)
    , m_sawError(false)
    , m_sawCSS(false)
    , m_sawXSLTransform(false)
    , m_sawFirstElement(false)
    , m_isXHTMLDocument(false)
    , m_parserPaused(false)
    , m_requestingScript(false)
    , m_finishCalled(false)
    , m_xmlErrors(document)
    , m_pendingScript(0)
    , m_scriptStartPosition(TextPosition::belowRangePosition())
    , m_parsingFragment(false)
    , m_scriptingPermission(AllowScriptingContent)
{
}

XMLDocumentParser::~XMLDocumentParser()
{
    // The XMLDocumentParser will always be detached before being destroyed.
    ASSERT(m_currentNodeStack.isEmpty());
    ASSERT(!m_currentNode);

    // FIXME: m_pendingScript handling should be moved into XMLDocumentParser.cpp!
    if (m_pendingScript)
        m_pendingScript->removeClient(this);
}

void XMLDocumentParser::doWrite(const String& parseString)
{
    ASSERT(!isDetached());
    // Protect the delegate object from deletion during a callback
    RefPtr<XMLDocumentParserShellDelegate> delegate = m_delegate;

    // Progressive parsing is not supported, hence stream shouldn't have
    // any data now.
    ASSERT(!m_stream.length());
    m_stream.open(parseString);
    initializeParserContext();
    if (parseString.length()) {
        // JavaScript may cause the parser to detach during xmlParseChunk
        // keep this alive until this function is done.
        RefPtr<XMLDocumentParser> protect(this);

        XMLDocumentParserScope scope(document()->cachedResourceLoader());
        m_delegate->parse();
        // JavaScript (which may be run under the xmlParseChunk callstack) may
        // cause the parser to be stopped or detached.
        if (isStopped()) {
            m_stream.close();
            return;
        }
    }
}

void XMLDocumentParser::initializeParserContext(const CString& chunk)
{
    // Progressive parsing is not supported.
    ASSERT(!m_parsingFragment);
    DocumentParser::startParsing();
    m_sawError = false;
    m_sawCSS = false;
    m_sawXSLTransform = false;
    m_sawFirstElement = false;
    XMLDocumentParserScope scope(document()->cachedResourceLoader());
    m_delegate = createXMLDocumentParserShellDelegate(this, &m_stream);
    m_delegate->init(&m_stream);
}

void XMLDocumentParser::doEnd()
{
    if (!isStopped()) {
        if (m_delegate) {
            // Tell delegate we're done.
            {
                XMLDocumentParserScope scope(document()->cachedResourceLoader());
                m_delegate->deinit();
            }
            m_delegate.clear();
        }
    }
    m_stream.close();
}

OrdinalNumber XMLDocumentParser::lineNumber() const
{
    return OrdinalNumber::fromOneBasedInt(m_delegate->line());
}

OrdinalNumber XMLDocumentParser::columnNumber() const
{
    return OrdinalNumber::fromOneBasedInt(m_delegate->column());
}

TextPosition XMLDocumentParser::textPosition() const
{
    return TextPosition(OrdinalNumber::fromOneBasedInt(m_delegate->line()),
                        OrdinalNumber::fromOneBasedInt(m_delegate->column()));
}

void XMLDocumentParser::stopParsing()
{
    DocumentParser::stopParsing();
    m_delegate->stop();
}

void XMLDocumentParser::resumeParsing()
{
    ASSERT(!isDetached());
    ASSERT(m_parserPaused);

    m_parserPaused = false;

    m_delegate->resume();
    executeParsing();
}

void XMLDocumentParser::executeParsing() {
    ASSERT(!m_parserPaused);
    // First, execute any pending callbacks
    XMLEventRunResult result = m_pendingEvents.run(this, document());
    if (result == ResultPause) {
        // A callback paused the parser
        m_parserPaused = true;
        return;
    }

    // Make sure there is no pending script
    ASSERT(!m_pendingSrc.length());

    // Finally, if finish() has been called and write() didn't result
    // in any further callbacks being queued, call end()
    if (m_finishCalled && m_pendingEvents.isEmpty())
        end();
}

void XMLDocumentParser::appendEvent(XMLEventBase* event)
{
    m_pendingEvents.appendEvent(event);
    if (!m_parserPaused)
        executeParsing();
}

// --------------------------------

HashMap<String, String> parseAttributes(const String& string, bool& attrsOK)
{
    static HashMap<String, String> attributes;
    XMLDocumentParserShellStream stream;
    stream.open(string);
    attributes.clear();
    attrsOK = XMLDocumentParserShellDelegate::parseAttributes(&stream, &attributes);
    stream.close();
    return attributes;
}

}  // namespace WebCore
