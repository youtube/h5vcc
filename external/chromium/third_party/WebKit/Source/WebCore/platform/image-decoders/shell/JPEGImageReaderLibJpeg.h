/*
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * Portions are Copyright (C) 2001-6 mozilla.org
 *
 * Other contributors:
 *   Stuart Parmenter <stuart@mozilla.com>
 *
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2013 Google Inc.
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
#ifndef JPEGImageReaderLibJpeg_h
#define JPEGImageReaderLibJpeg_h

/*
 This is a JPEG image decoder using libjpeg. It is is a modified/refactored
 version of the built-in JPEGImageDecoder at ../jpeg/JpegImageDecoder.cpp.
*/

#include "JPEGImageReaderShell.h"

extern "C" {
#if USE(ICCJPEG)
#include "iccjpeg.h"
#endif
#include "jpeglib.h"
#include <setjmp.h>
}

namespace WebCore {
    class JPEGImageReaderLibJpeg : public JPEGImageReaderShell {
    public:
        static JPEGImageReaderShell* create();
        virtual ~JPEGImageReaderLibJpeg() {}

    private:
        enum { iccColorProfileHeaderLength = 128 };
        static const int exifMarker = JPEG_APP0 + 1;

        JPEGImageReaderLibJpeg();

        // Decode image
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

        // Output image data
        bool outputRGB(ImageFrame* buffer);
        bool outputScanlines(ImageFrame* buffer);

        void readImageOrientation();
        void readColorProfile();

        static unsigned readUint16(JOCTET* data, bool isBigEndian);
        static unsigned readUint32(JOCTET* data, bool isBigEndian);
        static bool checkExifHeader(jpeg_saved_marker_ptr marker,
                                    bool& isBigEndian,
                                    unsigned& ifdOffset);
        static bool rgbColorProfile(const char* profileData,
                                    unsigned profileLength);
        static bool inputDeviceColorProfile(const char* profileData,
                                            unsigned profileLength);

        // Callback functions from libjpeg, via jpeg_source_mgr
        static void error_exit(j_common_ptr cinfo);
        static void init_source(j_decompress_ptr);
        static void skip_input_data(j_decompress_ptr jd, long num_bytes);
        static boolean fill_input_buffer(j_decompress_ptr);
        static void term_source(j_decompress_ptr jd);
        static boolean jpeg_resync_to_restart(j_decompress_ptr cinfo,
                                              int desired);

        // Used internally, called from libjpeg callback functions.
        jmp_buf& getJmpPoint() { return m_setjmpPoint; }
        void skipBytes(long numBytes);

        jpeg_decompress_struct m_info;
        struct jpeg_source_mgr m_source;
        struct jpeg_error_mgr m_error;

        jmp_buf m_setjmpPoint;
    };
}
#endif  // JPEGImageReaderLibJpeg_h
