/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 * Copyright (C) 2013 Google Inc.  All rights reserved.
 *
 * Portions are Copyright (C) 2001 mozilla.org
 *
 * Other contributors:
 *   Stuart Parmenter <stuart@mozilla.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#ifndef PNGImageReaderLibPng_h
#define PNGImageReaderLibPng_h

/*
 This is a PNG image decoder using libpng. It is is a modified/refactored
 version of the built-in PNGImageReader at ../png/PNGImageDecoder.cpp.
*/

#include "png.h"
#include "PNGImageReaderShell.h"

namespace WebCore {
    class PNGImageReaderLibPng : public PNGImageReaderShell {
    public:
        static PNGImageReaderShell* create();
        virtual ~PNGImageReaderLibPng();

    protected:
        // Callbacks from libpng
        void headerAvailableCallBack();
        void rowAvailableCallBack(unsigned char* rowBuffer, unsigned rowIndex,
                                  int interlacePass);
        void pngCompleteCallBack();

    private:
        PNGImageReaderLibPng();

        // Callbck functions
        static void PNGAPI decodingFailed(png_structp png, png_const_charp);
        static void PNGAPI decodingWarning(png_structp png,
                                           png_const_charp warningMsg);
        static void PNGAPI headerAvailable(png_structp png, png_infop);
        static void PNGAPI rowAvailable(png_structp png,
                                        png_bytep rowBuffer,
                                        png_uint_32 rowIndex,
                                        int interlacePass);
        static void PNGAPI pngComplete(png_structp png, png_infop);

        /* decoding functions to be implemented by sub classes, begin */

        // Create native image decoder object [optional].
        virtual bool setUpNativeObject(const char* data, size_t size) OVERRIDE;
        // Decode image header.
        virtual bool decodeHeader(const char* data, size_t size) OVERRIDE;
        // Decode image frames.
        virtual bool decodeFrames(const char* data, size_t size,
                                  ImageFrameVector* frames) OVERRIDE;
        // Destroy native image decoder object [optional].
        virtual void destroyNativeObject() OVERRIDE;

        /* decoding functions to be implemented by sub classes, end */

        void readColorProfile(png_structp png, png_infop info);

        png_structp m_png;
        png_infop m_info;
        unsigned m_readOffset;
        unsigned m_currentBufferSize;
        bool m_hasAlpha;
        png_bytep m_interlaceBuffer;
        ImageFrameVector* m_frames;
    };
}
#endif  // PNGImageReaderLibPng_h
