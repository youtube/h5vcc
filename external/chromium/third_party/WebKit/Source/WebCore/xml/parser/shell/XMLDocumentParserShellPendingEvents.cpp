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

#include "XMLDocumentParserShellPendingEvents.h"

#include <wtf/PassOwnPtr.h>

namespace WebCore {

void XMLPendingEvents::appendEvent(XMLEventBase* event)
{
    m_events.append(adoptPtr(event));
}

bool XMLPendingEvents::isEmpty() const
{
    return m_events.isEmpty();
}

XMLEventRunResult XMLPendingEvents::run(XMLDocumentParser* parser, Document* document)
{
    XMLEventRunResult result = ResultContinue;  // Default to succeed
    while (!isEmpty() && result == ResultContinue) {
        OwnPtr<XMLEventBase> event = m_events.takeFirst();
        result = event->run(parser, document);
    }

    return result;
}

}  // namespace WebCore
