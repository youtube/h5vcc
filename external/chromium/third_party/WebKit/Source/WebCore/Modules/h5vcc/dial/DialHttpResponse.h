/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DialHttpResponse_h
#define DialHttpResponse_h

#include "config.h"
#if ENABLE(DIAL_SERVER)

#include "Dictionary.h"
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DialServer;

class DialHttpResponse: public ThreadSafeRefCounted<DialHttpResponse> {
 public:
  ~DialHttpResponse() {}

  // getters
  const String& path() const { return m_path; }
  const String& method() const { return m_method; }
  const String body() { return m_body; }
  const int responseCode() { return m_responseCode; }
  const String mimeType() { return m_mimeType; }
  const Vector<String>& headers() const { return m_headers; }

  // setters
  void setBody(const String& body) { m_body = body; }
  void setResponseCode(int responseCode) { m_responseCode = responseCode; }
  void setMimeType(const String& mimeType) { m_mimeType = mimeType; }

  void addHeader(const String& name, const String& value) {
    StringBuilder header;
    header.append(name);
    header.append(": ");
    header.append(value);
    m_headers.append(header.toString());
  }

 private:
  DialHttpResponse() : m_responseCode(404) {}
  friend class DialServer;

  String m_path;
  String m_method;
  String m_body;
  String m_mimeType;
  int m_responseCode;
  Vector<String> m_headers;

};

}  // namespace WebCore

#endif  // ENABLE(DIAL_SERVER)
#endif  // DialHttpResponse_h
