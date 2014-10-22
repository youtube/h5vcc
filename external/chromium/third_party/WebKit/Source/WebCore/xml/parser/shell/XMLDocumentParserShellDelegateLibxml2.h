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

using namespace std;

#ifndef Shell_XMLDocumentParserShellDelegateLibxml2_h
#define Shell_XMLDocumentParserShellDelegateLibxml2_h

#include "XMLDocumentParserShellDelegate.h"

#include <libxml/tree.h>
#include <libxml/xmlstring.h>

namespace WebCore {

class XMLDocumentParserShellDelegateLibxml2
    : public XMLDocumentParserShellDelegate {
public:
    XMLDocumentParserShellDelegateLibxml2(XMLDocumentParser* owner,
                                          XMLDocumentParserShellStream* stream);
    ~XMLDocumentParserShellDelegateLibxml2();

    // XMLDocumentParserShellDelegate functions
    virtual void init(XMLDocumentParserShellStream* stream) OVERRIDE;
    virtual void parse() OVERRIDE;
    virtual void stop() OVERRIDE;
    virtual void resume() OVERRIDE;
    virtual void deinit() OVERRIDE;
    virtual int column() const OVERRIDE;
    virtual int line() const OVERRIDE;

    static void switchToUTF16(xmlParserCtxtPtr ctxt);
    static void initLib();

private:
    static ThreadIdentifier m_loaderThread;
    xmlParserCtxtPtr m_parser;
    static bool m_initialized;
};

}

#endif  // Shell_XMLDocumentParserShellDelegateLibxml2_h