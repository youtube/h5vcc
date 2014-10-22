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

#ifndef Shell_XMLDocumentParserShellStream_h
#define Shell_XMLDocumentParserShellStream_h

#include "wtf/text/WTFString.h"

namespace WebCore {

class XMLDocumentParserShellStream {
    WTF_MAKE_FAST_ALLOCATED;
public:
    XMLDocumentParserShellStream() : m_currentOffset(0) {}

    void open(const String& parseString);
    int read(char* buffer, int len);
    void close();

    int length() const { return m_buffer.length(); }
    const String& getString() const { return m_buffer; }

private:
    String m_buffer;
    size_t m_currentOffset;
};

}
#endif