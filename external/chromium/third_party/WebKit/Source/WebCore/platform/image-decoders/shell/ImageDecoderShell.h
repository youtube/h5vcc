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

#ifndef ImageDecoderShell_h
#define ImageDecoderShell_h

#include <config.h>

#include "ImageDecoder.h"
#include <wtf/OwnPtr.h>


namespace WebCore {
    class ImageReaderShell;

    class ImageDecoderShell : public ImageDecoder {
    public:
        // Create image decoder according to signature. Caller needs
        // to make sure the length of signature data is enough.
        static ImageDecoder* create(const char* buf, size_t size,
                ImageSource::AlphaOption alpha,
                ImageSource::GammaAndColorProfileOption profile);
        virtual ~ImageDecoderShell();

        // Create decoder with type type.
        static ImageDecoder* createBMPDecoder(
            ImageSource::AlphaOption alpha,
            ImageSource::GammaAndColorProfileOption profile);
        static ImageDecoder* createPNGDecoder(
            ImageSource::AlphaOption alpha,
            ImageSource::GammaAndColorProfileOption profile);
        static ImageDecoder* createJPEGDecoder(
            ImageSource::AlphaOption alpha,
            ImageSource::GammaAndColorProfileOption profile);
        static ImageDecoder* createGIFDecoder(
            ImageSource::AlphaOption alpha,
            ImageSource::GammaAndColorProfileOption profile);
        static ImageDecoder* createICODecoder(
            ImageSource::AlphaOption alpha,
            ImageSource::GammaAndColorProfileOption profile);
        static ImageDecoder* createWEBPDecoder(
            ImageSource::AlphaOption alpha,
            ImageSource::GammaAndColorProfileOption profile);

        // Set encoded image data.
        virtual void setData(SharedBuffer*, bool allDataReceived) OVERRIDE;
        // Is image size available. We dont support progressive decoding
        // so this size is available only when all data is received and
        // the image is successfully decoded. Decoding will be triggered if
        // needed.
        virtual bool isSizeAvailable() OVERRIDE;
        // Get buffer of specified frame. If the image is not decoded
        // or the frame number is greater or equal to frame count,
        // NULL is returned.  Decoding will be triggered if needed.
        virtual ImageFrame* frameBufferAtIndex(size_t index)  OVERRIDE;
        // Return the filename extension string of current image format.
        virtual String filenameExtension() const  OVERRIDE;

    private:
        ImageDecoderShell(ImageReaderShell*,
                          ImageSource::AlphaOption,
                          ImageSource::GammaAndColorProfileOption);
        bool decode();
        // Internal reader object
        OwnPtr<ImageReaderShell> m_reader;
    };
}

#endif  // ImageDecoderShell_h
