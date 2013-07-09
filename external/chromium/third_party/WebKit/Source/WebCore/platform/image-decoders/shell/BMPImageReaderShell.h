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

#ifndef BMPImageReaderShell_h
#define BMPImageReaderShell_h

#include "ImageReaderShell.h"

namespace WebCore {
    class BMPImageReaderShell : public ImageReaderShell {
    public:
        static bool matchSignature(const char* signature_str, int str_len)
        {
            const int kSignatureLength = 2;
            if (str_len < kSignatureLength)
                return false;
            return !memcmp(signature_str, "BM", kSignatureLength);
        }

        virtual String filenameExtension() const OVERRIDE { return "bmp"; }
        virtual String imageTypeString() const OVERRIDE { return "BMP"; }
    };
}
#endif  // BMPImageReaderShell_h
