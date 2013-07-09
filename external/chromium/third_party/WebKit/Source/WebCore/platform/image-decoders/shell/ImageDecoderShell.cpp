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

#include "ImageDecoderShell.h"

#include "IntRect.h"
#include "PlatformInstrumentation.h"
#include <wtf/PassOwnPtr.h>

// Header files below are needed because we use WebCore's built-in decoders
#include "BMPImageDecoder.h"  // We use WebCore's built-in BMP decoder
#include "BMPImageReaderShell.h"  // matchSignature()
#include "GIFImageDecoder.h"  // We use WebCore's built-in GIF decoder
#include "GIFImageReaderShell.h"  // matchSignature()
#include "ICOImageDecoder.h"  // We use WebCore's built-in ICO decoder
#include "JPEGImageReaderShell.h"
#include "PNGImageReaderShell.h"

namespace WebCore {

/* static */
ImageDecoder* ImageDecoderShell::create(const char* buf, size_t size,
        ImageSource::AlphaOption alpha,
        ImageSource::GammaAndColorProfileOption profile)
{
    if (GIFImageReaderShell::matchSignature(buf, size))
        return createGIFDecoder(alpha, profile);

    if (BMPImageReaderShell::matchSignature(buf, size))
        return createBMPDecoder(alpha, profile);

    if (JPEGImageReaderShell::matchSignature(buf, size))
        return createJPEGDecoder(alpha, profile);

    if (PNGImageReaderShell::matchSignature(buf, size))
        return createPNGDecoder(alpha, profile);

    return NULL;
}

/* static */
ImageDecoder* ImageDecoderShell::createBMPDecoder(
    ImageSource::AlphaOption alpha,
    ImageSource::GammaAndColorProfileOption profile)
{
    // We use WebCore's built-in BMP decoder. Note that the decoder uses
    // internal code instead of external lib to decode image.
    return new BMPImageDecoder(alpha, profile);
}

/* static */
ImageDecoder* ImageDecoderShell::createGIFDecoder(
    ImageSource::AlphaOption alpha,
    ImageSource::GammaAndColorProfileOption profile)
{
    // We use WebCore's built-in GIF decoder. Note that the decoder uses
    // internal code instead of external lib to decode image.
    return new GIFImageDecoder(alpha, profile);
}

/* static */
ImageDecoder* ImageDecoderShell::createJPEGDecoder(
    ImageSource::AlphaOption alpha,
    ImageSource::GammaAndColorProfileOption profile)
{
    ImageReaderShell* reader = createJPEGImageReaderShell();
    if (reader) {
        return new ImageDecoderShell(reader, alpha, profile);
    }
    return NULL;
}

/* static */
ImageDecoder* ImageDecoderShell::createPNGDecoder(
    ImageSource::AlphaOption alpha,
    ImageSource::GammaAndColorProfileOption profile)
{
    ImageReaderShell* reader = createPNGImageReaderShell();
    if (reader) {
        return new ImageDecoderShell(reader, alpha, profile);
    }
    return NULL;
}

/* static */
ImageDecoder* ImageDecoderShell::createICODecoder(
    ImageSource::AlphaOption alpha,
    ImageSource::GammaAndColorProfileOption profile)
{
    // We use WebCore's built-in ICO decoder. Note that the decoder uses
    // internal code instead of external lib to decode image. This function
    // is called from outside directly without calling
    // ImageDecoderShell::create().
    return new ICOImageDecoder(alpha, profile);
}

ImageDecoderShell::ImageDecoderShell(ImageReaderShell* reader,
        ImageSource::AlphaOption alpha,
        ImageSource::GammaAndColorProfileOption profile)
    : ImageDecoder(alpha, profile)
    , m_reader(WTF::adoptPtr(reader))
{
}

ImageDecoderShell::~ImageDecoderShell()
{
}

void ImageDecoderShell::setData(SharedBuffer* data, bool allDataReceived)
{
    if (failed())
        return;

    ImageDecoder::setData(data, allDataReceived);
}

// Is the dimensions of decoded image available.
bool ImageDecoderShell::isSizeAvailable()
{
    // If not decoded yet, call decode, if all data is received.
    // We dont have progressive decoding. So size and frames are all
    // decoded in one decode() call.
    if (isAllDataReceived() && !m_reader->decoded()) {
        // Size is set in decode()
        decode();
    }

    return ImageDecoder::isSizeAvailable();
}

ImageFrame* ImageDecoderShell::frameBufferAtIndex(size_t index)
{
    // If not decoded yet, call decode, if all data is received.
    if (isAllDataReceived() && !m_reader->decoded()) {
        decode();
    }
    // After decode() is called and decoding is done, the reader
    // should be in either SUCCEEDED state or FAILED state.
    // We only check and return a frame if the decoding process succeeded.
    if (m_reader->succeeded()) {
        size_t frame_count = m_reader->frameCount();
        ASSERT(frame_count == m_frameBufferCache.size());
        if (index < frame_count)
            return &m_frameBufferCache[index];
    }
    return NULL;
}

String ImageDecoderShell::filenameExtension() const
{
    return m_reader->filenameExtension();
}

bool ImageDecoderShell::decode()
{
    ASSERT(!m_reader->decoded());
    ASSERT(isAllDataReceived());

    PlatformInstrumentation::willDecodeImage(
        m_reader->imageTypeString());
    // Send data to image reader to decode. Frames are output into
    // m_frameBufferCache
    bool ret = m_reader->decode(m_data->data(), m_data->size(),
                                &m_frameBufferCache);
    if (ret) {
        setSize(m_reader->width(), m_reader->height());
    } else {
        setFailed();
    }
    PlatformInstrumentation::didDecodeImage();

    return ret;
}

}  // namespace WebCore
