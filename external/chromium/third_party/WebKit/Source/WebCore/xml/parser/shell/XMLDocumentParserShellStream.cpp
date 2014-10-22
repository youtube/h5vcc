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

#include "config.h"
#include "XMLDocumentParserShellStream.h"

#include "wtf/text/CString.h"

namespace WebCore {

void XMLDocumentParserShellStream::open(const String& parseString)
{
    m_buffer = parseString;
    m_currentOffset = 0;
}

int XMLDocumentParserShellStream::read(char* buffer, int len)
{
    int bytesLeft = m_buffer.length() - m_currentOffset;
    int lenToCopy = std::min(len, bytesLeft);
    if (lenToCopy) {
        memcpy(buffer, m_buffer.utf8().data() + m_currentOffset, lenToCopy);
        m_currentOffset += lenToCopy;
    }
    return lenToCopy;
}

void XMLDocumentParserShellStream::close()
{
    m_buffer = emptyString();
    m_currentOffset = 0;
}

}
