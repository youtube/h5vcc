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

/*
 This is a PNG image decoder using libpng. It is is a modified/refactored
 version of the built-in PNGImageReader at ../png/PNGImageDecoder.cpp.
*/

#if !__LB_ENABLE_SHELL_NATIVE_PNG_DECODER__

#include "PNGImageReaderLibPng.h"

#if defined(PNG_LIBPNG_VER_MAJOR) && defined(PNG_LIBPNG_VER_MINOR) && (PNG_LIBPNG_VER_MAJOR > 1 || (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR >= 4))
#define JMPBUF(png_ptr) png_jmpbuf(png_ptr)
#else
#define JMPBUF(png_ptr) png_ptr->jmpbuf
#endif

namespace WebCore {

// Create PNGImageReaderLibPng object to be used as PNGImageReaderShell
// in project.
PNGImageReaderShell* createPNGImageReaderShell()
{
    return PNGImageReaderLibPng::create();
}

/* static */
PNGImageReaderShell* PNGImageReaderLibPng::create()
{
    return new PNGImageReaderLibPng();
}

// Gamma constants.
const double cMaxGamma = 21474.83;
const double cDefaultGamma = 2.2;
const double cInverseGamma = 0.45455;

// Protect against large PNGs. See Mozilla's bug #251381 for more info.
const unsigned long cMaxPNGSize = 1000000UL;

// Call back functions from libpng
/* static */
void PNGAPI PNGImageReaderLibPng::decodingFailed(png_structp png,
                                                 png_const_charp)
{
    PNGImageReaderLibPng* reader =
        static_cast<PNGImageReaderLibPng*>(png_get_progressive_ptr(png));
    longjmp(JMPBUF(png), 1);
}

/* static */
void PNGAPI PNGImageReaderLibPng::decodingWarning(png_structp png,
                                                  png_const_charp warningMsg)
{
    // Mozilla did this, so we will too.
    // Convert a tRNS warning to be an error (see
    // http://bugzilla.mozilla.org/show_bug.cgi?id=251381 )
    if (!strncmp(warningMsg, "Missing PLTE before tRNS", 24))
        png_error(png, warningMsg);
}

// Called when we have obtained the header information (including the size).
/* static */
void PNGAPI PNGImageReaderLibPng::headerAvailable(png_structp png, png_infop)
{
    PNGImageReaderLibPng* reader =
        static_cast<PNGImageReaderLibPng*>(png_get_progressive_ptr(png));
    reader->headerAvailableCallBack();
}

// Called when a row is ready.
/* static */
void PNGAPI PNGImageReaderLibPng::rowAvailable(png_structp png,
                                               png_bytep rowBuffer,
                                               png_uint_32 rowIndex,
                                               int interlacePass)
{
    PNGImageReaderLibPng* reader =
        static_cast<PNGImageReaderLibPng*>(png_get_progressive_ptr(png));
    reader->rowAvailableCallBack(rowBuffer, rowIndex, interlacePass);
}

// Called when we have completely finished decoding the image.
/* static */
void PNGAPI PNGImageReaderLibPng::pngComplete(png_structp png, png_infop)
{
    PNGImageReaderLibPng* reader =
        static_cast<PNGImageReaderLibPng*>(png_get_progressive_ptr(png));
    reader->pngCompleteCallBack();
}

PNGImageReaderLibPng::PNGImageReaderLibPng()
    : PNGImageReaderShell()
    , m_png(NULL)
    , m_info(NULL)
    , m_readOffset(0)
    , m_hasAlpha(false)
    , m_interlaceBuffer(0)
{
}

PNGImageReaderLibPng::~PNGImageReaderLibPng()
{
    delete[] m_interlaceBuffer;
    m_interlaceBuffer = 0;
    m_readOffset = 0;
    ASSERT(!m_info);
    ASSERT(!m_png);
}

bool PNGImageReaderLibPng::setUpNativeObject(const char* data, size_t size)
{
    ASSERT(!decoded());
    m_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, decodingFailed,
                                   decodingWarning);
    m_info = png_create_info_struct(m_png);
    png_set_progressive_read_fn(m_png, this, headerAvailable, rowAvailable,
                                pngComplete);
    return true;
}

// Decode image header.
bool PNGImageReaderLibPng::decodeHeader(const char* data, size_t size)
{
    ASSERT(!decoded());
    // Libpng decoding header with image data. Simply returns true here
    // without real decoding. We need to handle setSize and setFrameCount
    // in decodeFrame().
    // In fact we support only single frame PNG images, so the frame count
    // is available indeed, however other header information isn't.
    // I postpone the setFrameCount call to the time when we have real
    // header information so that (1) this function is more consistent;
    // (2) demonstrate what to do in this special case (header decoding can not
    // be separated from frame decoding).
    return true;
}

// Decode image frames.
bool PNGImageReaderLibPng::decodeFrames(const char* data, size_t size,
                                        ImageFrameVector* frames)
{
    ASSERT(!decoded());
    // We need to do the setjmp here. Otherwise bad things will happen.
    if (setjmp(JMPBUF(m_png)))
    {
        return false;
    }
    // Since size and frame count is not available in decodeHeader. Frames
    // shouldn't have been created. We should manually create them by
    // setting size and frame count and then call createFrames().
    ASSERT(!frames->size());

    m_frames = frames;

    png_process_data(m_png,
        m_info, reinterpret_cast<png_bytep>(const_cast<char*>(data)), size);

    bool succeeded = (*m_frames)[0].status() == ImageFrame::FrameComplete;
    m_frames = NULL;
    return succeeded;
}

void PNGImageReaderLibPng::destroyNativeObject()
{
    // Both are created at the same time. So they should be both zero
    // or both non-zero. Use && here to be safer.
    if (m_png && m_info)
        // This will zero the pointers.
        png_destroy_read_struct(&m_png, &m_info, 0);
    m_png = NULL;
    m_info = NULL;
}

void PNGImageReaderLibPng::headerAvailableCallBack()
{
    png_structp png = m_png;
    png_infop info = m_info;
    png_uint_32 width = png_get_image_width(png, info);
    png_uint_32 height = png_get_image_height(png, info);

    // Protect against large images.
    if (width > cMaxPNGSize || height > cMaxPNGSize) {
        longjmp(JMPBUF(png), 1);
        return;
    }

    setSize(width, height);
    // PNG should have 1 frame only
    setFrameCount(1);
    // Ask base class to create frames for us, since we didn't provide
    // the information in decodeHeader().
    createFrames(m_frames);

    int bitDepth, colorType, interlaceType, compressionType, filterType,
        channels;
    png_get_IHDR(png, info, &width, &height, &bitDepth, &colorType,
                 &interlaceType, &compressionType, &filterType);

    // The options we set here match what Mozilla does.

    // Expand to ensure we use 24-bit for RGB and 32-bit for RGBA.
    if (colorType == PNG_COLOR_TYPE_PALETTE
           || (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8))
        png_set_expand(png);

    png_bytep trns = 0;
    int trnsCount = 0;
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_get_tRNS(png, info, &trns, &trnsCount, 0);
        png_set_expand(png);
    }

    if (bitDepth == 16)
        png_set_strip_16(png);

    if (colorType == PNG_COLOR_TYPE_GRAY
        || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    if (colorType & PNG_COLOR_MASK_COLOR) {
        // We only support color profiles for color PALETTE and RGB[A] PNG. Supporting
        // color profiles for gray-scale images is slightly tricky, at least using the
        // CoreGraphics ICC library, because we expand gray-scale images to RGB but we
        // do not similarly transform the color profile. We'd either need to transform
        // the color profile or we'd need to decode into a gray-scale image buffer and
        // hand that to CoreGraphics.
        readColorProfile(png, info);
    }

    // Deal with gamma and keep it under our control.
    double gamma;
    if (png_get_gAMA(png, info, &gamma)) {
        if ((gamma <= 0.0) || (gamma > cMaxGamma)) {
            gamma = cInverseGamma;
            png_set_gAMA(png, info, gamma);
        }
        png_set_gamma(png, cDefaultGamma, gamma);
    } else {
        png_set_gamma(png, cDefaultGamma, cInverseGamma);
    }

    // Tell libpng to send us rows for interlaced pngs.
    if (interlaceType == PNG_INTERLACE_ADAM7)
        png_set_interlace_handling(png);

    // Update our info now.
    png_read_update_info(png, info);
    channels = png_get_channels(png, info);
    ASSERT(channels == 3 || channels == 4);

    m_hasAlpha = (channels == 4);
    // Set up frame
    ImageFrame* buffer = &(*m_frames)[0];
    if (PNG_INTERLACE_ADAM7 == png_get_interlace_type(png, m_info)) {
        size_t size = channels * width, height;
        m_interlaceBuffer = new png_byte[size];
        if (!m_interlaceBuffer) {
            longjmp(JMPBUF(png), 1);
            return;
        }
    }
    buffer->setSize(width, height);
    buffer->setStatus(ImageFrame::FramePartial);
    buffer->setHasAlpha(m_hasAlpha);
    buffer->setColorProfile(colorProfile());

    // For PNGs, the frame always fills the entire image.
    buffer->setOriginalFrameRect(IntRect(IntPoint(), size()));
}

void PNGImageReaderLibPng::rowAvailableCallBack(unsigned char* rowBuffer,
                                                unsigned rowIndex, int)
{
    ASSERT(m_frames);

    /* libpng comments (here to explain what follows).
     *
     * this function is called for every row in the image.  If the
     * image is interlacing, and you turned on the interlace handler,
     * this function will be called for every row in every pass.
     * Some of these rows will not be changed from the previous pass.
     * When the row is not changed, the new_row variable will be NULL.
     * The rows and passes are called in order, so you don't really
     * need the row_num and pass, but I'm supplying them because it
     * may make your life easier.
     */

    // Nothing to do if the row is unchanged, or the row is outside
    // the image bounds: libpng may send extra rows, ignore them to
    // make our lives easier.
    if (!rowBuffer)
        return;
    int y = rowIndex;

    /* libpng comments (continued).
     *
     * For the non-NULL rows of interlaced images, you must call
     * png_progressive_combine_row() passing in the row and the
     * old row.  You can call this function for NULL rows (it will
     * just return) and for non-interlaced images (it just does the
     * memcpy for you) if it will make the code easier.  Thus, you
     * can just do this for all cases:
     *
     *    png_progressive_combine_row(png_ptr, old_row, new_row);
     *
     * where old_row is what was displayed for previous rows.  Note
     * that the first pass (pass == 0 really) will completely cover
     * the old row, so the rows do not have to be initialized.  After
     * the first pass (and only for interlaced images), you will have
     * to pass the current row, and the function will combine the
     * old row and the new row.
     */

    bool hasAlpha = m_hasAlpha;
    unsigned colorChannels = hasAlpha ? 4 : 3;
    png_bytep row = rowBuffer;

    if (m_interlaceBuffer) {
        row = m_interlaceBuffer + (rowIndex * colorChannels * width());
        png_progressive_combine_row(m_png, row, rowBuffer);
    }

    // Write the decoded row pixels to the frame buffer.
    ImageFrame* buffer = &(*m_frames)[0];
    ImageFrame::PixelData* address = buffer->getAddr(0, y);
    bool nonTrivialAlpha = false;

    png_bytep pixel = row;
    for (int x = 0; x < width(); ++x, pixel += colorChannels) {
        unsigned alpha = hasAlpha ? pixel[3] : 255;
        buffer->setRGBA(address++, pixel[0], pixel[1], pixel[2], alpha);
        nonTrivialAlpha |= alpha < 255;
    }

    buffer->setHasAlpha(nonTrivialAlpha);
}

void PNGImageReaderLibPng::pngCompleteCallBack()
{
    ASSERT(m_frames);
    ImageFrame* buffer = &(*m_frames)[0];
    buffer->setStatus(ImageFrame::FrameComplete);
}

void PNGImageReaderLibPng::readColorProfile(png_structp png,
                                            png_infop info)
{
    ColorProfile colorProfile;

#ifdef PNG_iCCP_SUPPORTED
    char* profileName;
    int compressionType;
#if (PNG_LIBPNG_VER < 10500)
    png_charp profile;
#else
    png_bytep profile;
#endif
    png_uint_32 profileLength;
    if (!png_get_iCCP(png, info, &profileName, &compressionType, &profile,
                      &profileLength))
        return;

    // Only accept RGB color profiles from input class devices.
    bool ignoreProfile = false;
    char* profileData = reinterpret_cast<char*>(profile);
    if (profileLength < ImageDecoder::iccColorProfileHeaderLength)
        ignoreProfile = true;
    else if (!ImageDecoder::rgbColorProfile(profileData, profileLength))
        ignoreProfile = true;
    else if (!ImageDecoder::inputDeviceColorProfile(profileData, profileLength))
        ignoreProfile = true;

    if (!ignoreProfile)
        colorProfile.append(profileData, profileLength);
#endif
    setColorProfile(colorProfile);
}

}  // namespace WebCore
#endif  // #if !__LB_ENABLE_SHELL_NATIVE_PNG_DECODER__
