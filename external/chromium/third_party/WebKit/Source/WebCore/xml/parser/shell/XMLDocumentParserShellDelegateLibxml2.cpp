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
#include "XMLDocumentParserShellDelegateLibxml2.h"

#include "Document.h"
#include "Element.h"
#include "HTMLEntityParser.h"
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>
#include "QualifiedName.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>
#include <wtf/Threading.h>
#include <wtf/UnusedParam.h>
#include <wtf/Vector.h>
#include "XMLDocumentParser.h"
#include "XMLDocumentParserScope.h"
#include "XMLDocumentParserShellStream.h"
#include "XMLErrors.h"
#include "XMLNSNames.h"


using namespace std;

// Local helper functions
namespace {

inline WebCore::XMLDocumentParserShellDelegateLibxml2* getParser(void* closure)
{
    xmlParserCtxtPtr ctxt = static_cast<xmlParserCtxtPtr>(closure);
    return static_cast<WebCore::XMLDocumentParserShellDelegateLibxml2*>(ctxt->_private);
}

inline String toString(const xmlChar* string, size_t size)
{
    return String::fromUTF8(reinterpret_cast<const char*>(string), size);
}

inline String toString(const xmlChar* string)
{
    return String::fromUTF8(reinterpret_cast<const char*>(string));
}

inline AtomicString toAtomicString(const xmlChar* string, size_t size)
{
    return AtomicString::fromUTF8(reinterpret_cast<const char*>(string), size);
}

inline AtomicString toAtomicString(const xmlChar* string)
{
    return AtomicString::fromUTF8(reinterpret_cast<const char*>(string));
}

// Using a static entity and marking it XML_INTERNAL_PREDEFINED_ENTITY is
// a hack to avoid malloc/free. Using a global variable like this could cause trouble
// if libxml implementation details were to change
static xmlChar sharedXHTMLEntityResult[5] = {0, 0, 0, 0, 0};

static xmlEntityPtr sharedXHTMLEntity()
{
    static xmlEntity entity;
    if (!entity.type) {
        entity.type = XML_ENTITY_DECL;
        entity.orig = sharedXHTMLEntityResult;
        entity.content = sharedXHTMLEntityResult;
        entity.etype = XML_INTERNAL_PREDEFINED_ENTITY;
    }
    return &entity;
}

}  // namespace

namespace WebCore {

struct XMLNameSpace {
    const xmlChar* prefix;
    const xmlChar* uri;
};

struct XMLAttibute {
    const xmlChar* localname;
    const xmlChar* prefix;
    const xmlChar* uri;
    const xmlChar* value;
    const xmlChar* end;
};

/* static */
static void handleNamespaceAttributes(Vector<Attribute, 8>& prefixedAttributes, const xmlChar** libxmlNamespaces, int nb_namespaces, ExceptionCode& ec)
{
    XMLNameSpace* namespaces = reinterpret_cast<XMLNameSpace*>(libxmlNamespaces);
    for (int i = 0; i < nb_namespaces; i++) {
        AtomicString namespaceQName = xmlnsAtom;
        AtomicString namespaceURI = toAtomicString(namespaces[i].uri);
        if (namespaces[i].prefix)
            namespaceQName = "xmlns:" + toString(namespaces[i].prefix);

        QualifiedName parsedName = anyName;
        if (!Element::parseAttributeName(parsedName, XMLNSNames::xmlnsNamespaceURI, namespaceQName, ec))
            return;

        prefixedAttributes.append(Attribute(parsedName, namespaceURI));
    }
}

/* static */
static void handleElementAttributes(Vector<Attribute, 8>& prefixedAttributes, const xmlChar** libxmlAttributes, int nb_attributes, ExceptionCode& ec)
{
    XMLAttibute* attributes = reinterpret_cast<XMLAttibute*>(libxmlAttributes);
    for (int i = 0; i < nb_attributes; i++) {
        int valueLength = static_cast<int>(attributes[i].end - attributes[i].value);
        AtomicString attrValue = toAtomicString(attributes[i].value, valueLength);
        String attrPrefix = toString(attributes[i].prefix);
        AtomicString attrURI = attrPrefix.isEmpty() ? AtomicString() : toAtomicString(attributes[i].uri);
        AtomicString attrQName = attrPrefix.isEmpty() ? toAtomicString(attributes[i].localname) : attrPrefix + ":" + toString(attributes[i].localname);

        QualifiedName parsedName = anyName;
        if (!Element::parseAttributeName(parsedName, attrURI, attrQName, ec))
            return;

        prefixedAttributes.append(Attribute(parsedName, attrValue));
    }
}

/* static */
static xmlEntityPtr getXHTMLEntity(const xmlChar* name)
{
    UChar c = decodeNamedEntity(reinterpret_cast<const char*>(name));
    if (!c)
        return 0;

    CString value = String(&c, 1).utf8();
    ASSERT(value.length() < 5);
    xmlEntityPtr entity = sharedXHTMLEntity();
    entity->length = value.length();
    entity->name = name;
    memcpy(sharedXHTMLEntityResult, value.data(), entity->length + 1);

    return entity;
}

//  ======================  Libxml2 event handlers  ======================
static void startElementHandler(void* closure, const xmlChar* xmlLocalname, const xmlChar* xmlPrefix, const xmlChar* xmlURI, int nb_namespaces,
                                const xmlChar** libxmlNamespaces, int nb_attributes, int nb_defaulted, const xmlChar** libxmlAttributes)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);

    AtomicString localName = toAtomicString(xmlLocalname);
    AtomicString uri = toAtomicString(xmlURI);
    AtomicString prefix = toAtomicString(xmlPrefix);

    Vector<Attribute, 8> prefixedAttributes;
    ExceptionCode ec = 0;
    handleNamespaceAttributes(prefixedAttributes, libxmlNamespaces, nb_namespaces, ec);
    if (ec) {
        delegate->onStopParsing();
        return;
    }

    handleElementAttributes(prefixedAttributes, libxmlAttributes, nb_attributes, ec);
    if (ec) {
        delegate->onStopParsing();
        return;
    }

    delegate->onStartElement(localName, prefix, uri, prefixedAttributes, nb_defaulted);
}

static void endElementHandler(void* closure, const xmlChar*, const xmlChar*, const xmlChar*)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    delegate->onEndElement();
}

static void charactersHandler(void* closure, const xmlChar* s, int len)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    String characters = toString(s, len);
    delegate->onCharacters(characters);
}

static void processingInstructionHandler(void* closure, const xmlChar* xmlTarget, const xmlChar* xmlData)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    String target = toString(xmlTarget);
    String data = toString(xmlData);
    delegate->onProcessingInstruction(target, data);
}

static void cdataBlockHandler(void* closure, const xmlChar* s, int len)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    String cdata = toString(s, len);
    delegate->onCData(cdata);
}

static void commentHandler(void* closure, const xmlChar* comment)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    delegate->onComment(comment);
}

WTF_ATTRIBUTE_PRINTF(2, 3)
static void warningHandler(void* closure, const char* message, ...)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    va_list args;
    va_start(args, message);
    delegate->onError(XMLErrors::warning, message, args);
    va_end(args);
}

WTF_ATTRIBUTE_PRINTF(2, 3)
static void fatalErrorHandler(void* closure, const char* message, ...)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    va_list args;
    va_start(args, message);
    delegate->onError(XMLErrors::fatal, message, args);
    va_end(args);
}

WTF_ATTRIBUTE_PRINTF(2, 3)
static void normalErrorHandler(void* closure, const char* message, ...)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    va_list args;
    va_start(args, message);
    delegate->onError(XMLErrors::nonFatal, message, args);
    va_end(args);
}

static void entityDeclarationHandler(void* closure, const xmlChar* name, int type, const xmlChar* publicId, const xmlChar* systemId, xmlChar* content)
{
    xmlSAX2EntityDecl(closure, name, type, publicId, systemId, content);
}

static xmlEntityPtr getEntityHandler(void* closure, const xmlChar* name)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    xmlParserCtxtPtr ctxt = static_cast<xmlParserCtxtPtr>(closure);
    xmlEntityPtr ent = xmlGetPredefinedEntity(name);
    if (ent) {
        ent->etype = XML_INTERNAL_PREDEFINED_ENTITY;
        return ent;
    }

    ent = xmlGetDocEntity(ctxt->myDoc, name);
    if (!ent && delegate->isXHTMLDocument()) {
        ent = getXHTMLEntity(name);
        if (ent)
            ent->etype = XML_INTERNAL_GENERAL_ENTITY;
    }

    return ent;
}

static void startDocumentHandler(void* closure)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    xmlParserCtxt* ctxt = static_cast<xmlParserCtxt*>(closure);
    XMLDocumentParserShellDelegateLibxml2::switchToUTF16(ctxt);
    delegate->onStartDocument(toString(ctxt->version), toString(ctxt->encoding), ctxt->standalone);
    xmlSAX2StartDocument(closure);
}

static void endDocumentHandler(void* closure)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    delegate->onEndDocument();
    xmlSAX2EndDocument(closure);
}

static void internalSubsetHandler(void* closure, const xmlChar* name, const xmlChar* externalID, const xmlChar* systemID)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    delegate->onInternalSubset(name, externalID, systemID);
    xmlSAX2InternalSubset(closure, name, externalID, systemID);
}

static void externalSubsetHandler(void* closure, const xmlChar*, const xmlChar* externalId, const xmlChar*)
{
    XMLDocumentParserShellDelegateLibxml2 *delegate = getParser(closure);
    String extId = toString(externalId);
    delegate->onExternalSubset(extId);
}

static void ignorableWhitespaceHandler(void* closure, const xmlChar*, int)
{
    // nothing to do, but we need this to work around a crasher
    // http://bugzilla.gnome.org/show_bug.cgi?id=172255
    // http://bugs.webkit.org/show_bug.cgi?id=5792
}

static void attributesStartElementHandler(void* closure, const xmlChar* xmlLocalName, const xmlChar* /*xmlPrefix*/,
                                          const xmlChar* /*xmlURI*/, int /*nb_namespaces*/, const xmlChar** /*namespaces*/,
                                          int nb_attributes, int /*nb_defaulted*/, const xmlChar** libxmlAttributes)
{
    if (strcmp(reinterpret_cast<const char*>(xmlLocalName), "attrs") != 0)
        return;

    xmlParserCtxtPtr ctxt = static_cast<xmlParserCtxtPtr>(closure);
    HashMap<String, String>* attributes = static_cast<HashMap<String, String>*>(ctxt->_private);

    XMLAttibute* xmlAttributes = reinterpret_cast<XMLAttibute*>(libxmlAttributes);
    for (int i = 0; i < nb_attributes; i++) {
        String attrLocalName = toString(xmlAttributes[i].localname);
        int valueLength = (int) (xmlAttributes[i].end - xmlAttributes[i].value);
        String attrValue = toString(xmlAttributes[i].value, valueLength);
        String attrPrefix = toString(xmlAttributes[i].prefix);
        String attrQName = attrPrefix.isEmpty() ? attrLocalName : attrPrefix + ":" + attrLocalName;

        attributes->set(attrQName, attrValue);
    }
}

//  ===========  XMLDocumentParserShellDelegateLibxml2 methods  ===========

bool XMLDocumentParserShellDelegateLibxml2::m_initialized = false;

WTF::PassRefPtr<XMLDocumentParserShellDelegate>
    createXMLDocumentParserShellDelegate(XMLDocumentParser* owner, XMLDocumentParserShellStream* stream)
{
    return adoptRef(new XMLDocumentParserShellDelegateLibxml2(owner, stream));
}

/* static */
bool XMLDocumentParserShellDelegate::parseAttributes(const XMLDocumentParserShellStream* stream, HashMap<String, String>* attributes)
{
    XMLDocumentParserShellDelegateLibxml2::initLib();
    xmlSAXHandler sax;
    memset(&sax, 0, sizeof(sax));
    sax.startElementNs = attributesStartElementHandler;
    sax.initialized = XML_SAX2_MAGIC;

    xmlParserCtxtPtr parser = xmlCreatePushParserCtxt(&sax, 0, 0, 0, 0);
    parser->_private = attributes;
    parser->replaceEntities = true;
    XMLDocumentParserShellDelegateLibxml2::switchToUTF16(parser);

    String parseString = "<?xml version=\"1.0\"?><attrs " + stream->getString() + " />";
    xmlParseChunk(parser, reinterpret_cast<const char*>(parseString.characters()), parseString.length() * sizeof(UChar), 1);
    return !attributes->isEmpty();
}

ThreadIdentifier XMLDocumentParserShellDelegateLibxml2::m_loaderThread = 0;

XMLDocumentParserShellDelegateLibxml2::XMLDocumentParserShellDelegateLibxml2(XMLDocumentParser* owner, XMLDocumentParserShellStream* stream)
    : XMLDocumentParserShellDelegate(owner, stream)
    , m_parser(NULL)
{
}

XMLDocumentParserShellDelegateLibxml2::~XMLDocumentParserShellDelegateLibxml2()
{
    if (m_parser) {
        if (m_parser->myDoc)
            xmlFreeDoc(m_parser->myDoc);
        xmlFreeParserCtxt(m_parser);
        m_parser = NULL;
    }
}

void XMLDocumentParserShellDelegateLibxml2::init(XMLDocumentParserShellStream* stream)
{
    initLib();
    xmlSAXHandler sax;
    memset(&sax, 0, sizeof(sax));

    sax.error = WebCore::normalErrorHandler;
    sax.fatalError = WebCore::fatalErrorHandler;
    sax.characters = WebCore::charactersHandler;
    sax.processingInstruction = WebCore::processingInstructionHandler;
    sax.cdataBlock = WebCore::cdataBlockHandler;
    sax.comment = WebCore::commentHandler;
    sax.warning = WebCore::warningHandler;
    sax.startElementNs = WebCore::startElementHandler;
    sax.endElementNs = WebCore::endElementHandler;
    sax.getEntity = WebCore::getEntityHandler;
    sax.startDocument = WebCore::startDocumentHandler;
    sax.endDocument = WebCore::endDocumentHandler;
    sax.internalSubset = WebCore::internalSubsetHandler;
    sax.externalSubset = WebCore::externalSubsetHandler;
    sax.ignorableWhitespace = WebCore::ignorableWhitespaceHandler;
    sax.entityDecl = WebCore::entityDeclarationHandler;
    sax.initialized = XML_SAX2_MAGIC;

    m_parser = xmlCreatePushParserCtxt(&sax, 0, 0, 0, 0);
    m_parser->_private = this;
    m_parser->replaceEntities = true;
    // libXML throws an error if you try to switch the encoding for an empty string.
    if (stream->length())
        switchToUTF16(m_parser);
}

void XMLDocumentParserShellDelegateLibxml2::parse()
{
    xmlParseChunk(m_parser, reinterpret_cast<const char*>(m_stream->getString().characters()),
        sizeof(UChar) * m_stream->getString().length(), 0);
}

void XMLDocumentParserShellDelegateLibxml2::stop()
{
    if (m_parser)
        xmlStopParser(m_parser);
}

void XMLDocumentParserShellDelegateLibxml2::resume()
{
}

void XMLDocumentParserShellDelegateLibxml2::deinit()
{
    xmlParseChunk(m_parser, 0, 0, 1);
}

int XMLDocumentParserShellDelegateLibxml2::column() const
{
    if (m_parser)
        return m_parser->input->col;
    return 0;
}

int XMLDocumentParserShellDelegateLibxml2::line() const
{
    if (m_parser)
        return m_parser->input->line;
    return 0;
}

/* static */
void XMLDocumentParserShellDelegateLibxml2::switchToUTF16(xmlParserCtxtPtr ctxt)
{
    // Hack around libxml2's lack of encoding overide support by manually
    // resetting the encoding to UTF-16 before every chunk.  Otherwise libxml
    // will detect <?xml version="1.0" encoding="<encoding name>"?> blocks
    // and switch encodings, causing the parse to fail.
    const UChar BOM = 0xFEFF;
    const unsigned char BOMHighByte = *reinterpret_cast<const unsigned char*>(&BOM);
    xmlSwitchEncoding(ctxt, BOMHighByte == 0xFF ? XML_CHAR_ENCODING_UTF16LE : XML_CHAR_ENCODING_UTF16BE);
}

/* static */
void XMLDocumentParserShellDelegateLibxml2::initLib()
{
    if (!m_initialized) {
        xmlInitParser();
        m_loaderThread = currentThread();
        m_initialized = true;
    }
}

}  // namespace WebCore
