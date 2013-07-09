/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GIFImageReaderShell_h
#define GIFImageReaderShell_h

#include "ImageReaderShell.h"

namespace WebCore {
    class GIFImageReaderShell : public ImageReaderShell {
    public:
        static bool matchSignature(const char* signature_str, int str_len)
        {
            const int kSignatureLength = 6;
            if (str_len < kSignatureLength)
                return false;
            return !memcmp(signature_str, "GIF87a", kSignatureLength)
                   || !memcmp(signature_str, "GIF89a", kSignatureLength);
        }

        virtual String filenameExtension() const OVERRIDE { return "gif"; }
        virtual String imageTypeString() const OVERRIDE { return "GIF"; }
    };
}

#endif  // GIFImageReaderShell_h
