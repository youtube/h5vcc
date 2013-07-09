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

#ifndef ImageReaderShell_h
#define ImageReaderShell_h

#include <config.h>

#include "ImageDecoder.h"
#include "ImageSource.h"
#include "PlatformScreen.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

    // Real image decoding happens in the subclasses of this class.
    class ImageReaderShell {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        ImageReaderShell()
            : m_state(ReaderState_OPEN)
            , m_imageSize()
            , m_frameCount(0)
            , m_orientation()
            , m_colorProfile() {}
        virtual ~ImageReaderShell() {}

        // Image type. File extension and type string used in WebCore
        virtual String filenameExtension() const = 0;
        virtual String imageTypeString() const = 0;

        // Decode
        bool decode(const char* data, size_t size,
                    ImageFrameVector* frames);

        // Reader state
        // Is image decoding done
        bool decoded() const { return m_state != ReaderState_OPEN; }
        // Is image decoding completed successfully
        bool succeeded() const { return m_state == ReaderState_SUCCEEDED; }
        // Is there any error happened
        bool failed() const { return m_state == ReaderState_FAILED; }

        // Image properties
        size_t width() const { return m_imageSize.width(); }
        size_t height() const { return m_imageSize.height(); }
        const IntSize& size() const { return m_imageSize; }
        size_t frameCount() const { return m_frameCount; }

        const ColorProfile& colorProfile() const { return m_colorProfile; }
        const ImageOrientation& orientation() const { return m_orientation; }

    protected:
        // member variables accessible from sub classes
        void setSize(size_t width, size_t height)
        {
            ASSERT(m_imageSize.width() == 0);
            ASSERT(m_imageSize.height() == 0);

            m_imageSize.setWidth(width);
            m_imageSize.setHeight(height);
        }

        void setColorProfile(const ColorProfile& profile)
        {
            m_colorProfile = profile;
        }

        void setOrientation(const ImageOrientation& orientation)
        {
            m_orientation = orientation;
        }

        void setFrameCount(size_t count)
        {
            ASSERT(!m_frameCount);
            m_frameCount = count;
        }

        void createFrames(ImageFrameVector* frames)
        {
            ASSERT(m_frameCount);
            ASSERT(!frames->size());
            ASSERT(width());
            ASSERT(height());

            frames->resize(m_frameCount);
        }

        void premultiplyAlpha(ImageFrame* frame);

    private:
        // States for image readers
        enum ReaderState {
            ReaderState_OPEN,                       // Reader just opened
            ReaderState_SUCCEEDED,                  // Decoding succeeded
            ReaderState_FAILED                      // Decoding failed
        };

        void setState(ReaderState state) { m_state = state; }
        bool decodeInternal(const char* data, size_t size,
                            ImageFrameVector* frames);

        /* decoding functions to be implemented by sub classes, begin */

        // Create native image decoder object [optional].
        virtual bool setUpNativeObject(const char* data, size_t size)
        {
              return true;
        }
        // Decode image header. frame count should be available
        // after this call (set with setFrameCount()), if
        // possible && succeeded. In special cases that frame count
        // is not available, leave the frame count unset so that
        // ImageReader knows to skip the creation of the frames. The
        // Sub class needs to call createFrames() manually in this case.
        virtual bool decodeHeader(const char* data, size_t size) = 0;
        // Decode image frames.
        virtual bool decodeFrames(const char* data, size_t size,
                                  ImageFrameVector* frames) = 0;
        // Destroy native image decoder object [optional].
        virtual void destroyNativeObject() {}

        /* decoding functions to be implemented by sub classes, end */

        ReaderState m_state;
        IntSize m_imageSize;
        size_t m_frameCount;
        ImageOrientation m_orientation;
        ColorProfile m_colorProfile;
    };
}  // namespace WebCore
#endif  // ImageReaderShell_h
