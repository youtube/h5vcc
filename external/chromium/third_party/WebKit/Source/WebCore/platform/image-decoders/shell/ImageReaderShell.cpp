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

#include "ImageReaderShell.h"

namespace WebCore {

// Decode
bool ImageReaderShell::decode(const char* data, size_t size,
                              ImageFrameVector* frames)
{
    if (!decoded()) {
        if (decodeInternal(data, size, frames))
            setState(ReaderState_SUCCEEDED);
        else
            setState(ReaderState_FAILED);
    }
    return succeeded();
}

bool ImageReaderShell::decodeInternal(const char* data, size_t size,
                                      ImageFrameVector* frames)
{
    ASSERT(!decoded());
    ASSERT(!frames->size());

    bool succeeded = false;
    /* Create a native decoder. */
    if (setUpNativeObject(data, size)) {
        /* Set parameter  */
        if (decodeHeader(data, size)) {
            /* Set up frames if information is available*/
            if (frameCount())
                createFrames(frames);

            /* Decode frames */
            if (decodeFrames(data, size, frames))
                succeeded = true;
        }
        /* Destroy native decoder. */
        destroyNativeObject();
    }
    return succeeded;
}

void ImageReaderShell::premultiplyAlpha(ImageFrame* frame)
{
    // Premultiply alpha
    if (frame->premultiplyAlpha()) {
        for (int y = 0; y < height(); ++y) {
            ImageFrame::PixelData* pixel = frame->getAddr(0, y);
            for (int x = 0; x < width(); ++x) {
#if USE(SKIA)
                frame->setRGBA(pixel,
                               SkGetPackedR32(*pixel),
                               SkGetPackedG32(*pixel),
                               SkGetPackedB32(*pixel),
                               SkGetPackedA32(*pixel));
#else
                frame->setRGBA(pixel,
                               (*pixel >> 16) & 0xff,
                               (*pixel >> 8) & 0xff,
                               (*pixel >> 0) & 0xff,
                               (*pixel >> 24) & 0xff);
#endif
                pixel++;
            }
        }
    }
}

}  // namespace WebCore
