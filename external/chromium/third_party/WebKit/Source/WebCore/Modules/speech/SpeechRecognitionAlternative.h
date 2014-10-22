/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SpeechRecognitionAlternative_h
#define SpeechRecognitionAlternative_h

#if ENABLE(SCRIPTED_SPEECH)

// We need WEBKIT_EXPORT to expose some WebCore functionality to the lbshell
// platform layer. This forces us to include public/WebCommon.h, which we
// shouldn't since that's a chromium specific header. Here we include it
// via including wtf/OSAllocator which includes public/WebCommon.h via a
// relative path.
#include <wtf/OSAllocator.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ScriptExecutionContext;

class WEBKIT_EXPORT SpeechRecognitionAlternative : public RefCounted<SpeechRecognitionAlternative> {
public:
    static PassRefPtr<SpeechRecognitionAlternative> create(const String&, double);

    const String& transcript() const { return m_transcript; }
    double confidence() const { return m_confidence; }

private:
    SpeechRecognitionAlternative(const String&, double);

    String m_transcript;
    double m_confidence;
};

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)

#endif // SpeechRecognitionAlternative_h
