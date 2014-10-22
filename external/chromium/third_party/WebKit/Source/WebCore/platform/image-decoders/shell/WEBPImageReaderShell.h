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

#ifndef WEBPImageReaderShell_h
#define WEBPImageReaderShell_h

#include "ImageReaderShell.h"

namespace WebCore {
    class WEBPImageReaderShell : public ImageReaderShell {
    public:
        static bool matchSignature(const char* signature_str, int str_len)
        {
            const int kSignatureLength = 8 + 6;
            if (str_len < kSignatureLength)
                return false;
            return !memcmp(signature_str, "RIFF", 4)
                    && !memcmp(signature_str + 8, "WEBPVP", 6);
        }

        virtual String filenameExtension() const OVERRIDE { return "webp"; }
        virtual String imageTypeString() const OVERRIDE { return "WEBP"; }
    };
}

#endif  // WEBPImageReaderShell_h
