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

/*
 This is a JPEG image decoder using libjpeg. It is is a modified/refactored
 version of the built-in JPEGImageReader at ../jpeg/JpegImageDecoder.cpp.
*/

#if !__LB_ENABLE_SHELL_NATIVE_JPEG_DECODER__

#include "JPEGImageReaderLibJpeg.h"

namespace WebCore {

// Create JPEGImageReaderLibJpeg object to be used as JPEGImageReaderShell
// in project.
JPEGImageReaderShell* createJPEGImageReaderShell()
{
    return JPEGImageReaderLibJpeg::create();
}

/* static */
JPEGImageReaderShell* JPEGImageReaderLibJpeg::create()
{
    return new JPEGImageReaderLibJpeg();
}

/* Callback functions for libjpeg source manager */
/* static  */
void JPEGImageReaderLibJpeg::error_exit(j_common_ptr cinfo)
{
    // Return control to the setjmp point.
    JPEGImageReaderLibJpeg* client =
        static_cast<JPEGImageReaderLibJpeg*>(cinfo->client_data);

    longjmp(client->getJmpPoint(), -1);
}

/* static  */
void JPEGImageReaderLibJpeg::init_source(j_decompress_ptr)
{
}

/* static  */
void JPEGImageReaderLibJpeg::skip_input_data(j_decompress_ptr jd,
                                             long num_bytes)
{
    JPEGImageReaderLibJpeg* client =
        static_cast<JPEGImageReaderLibJpeg*>(jd->client_data);

    client->skipBytes(num_bytes);
}

/* static  */
boolean JPEGImageReaderLibJpeg::fill_input_buffer(j_decompress_ptr)
{
    // Our decode step always sets things up properly, so if this method is ever
    // called, then we have hit the end of the buffer.  A return value of false
    // indicates that we have no data to supply yet.
    return false;
}

/* static  */
boolean JPEGImageReaderLibJpeg::jpeg_resync_to_restart(j_decompress_ptr cinfo,
                                                       int desired)
{
    return false;
}

/* static  */
void JPEGImageReaderLibJpeg::term_source(j_decompress_ptr jd)
{
}


JPEGImageReaderLibJpeg::JPEGImageReaderLibJpeg()
    : JPEGImageReaderShell()
{
    memset(&m_info, 0, sizeof(m_info));
    memset(&m_source, 0, sizeof(m_source));
}

bool JPEGImageReaderLibJpeg::outputRGB(ImageFrame* buffer)
{
    // Allocate a buffer big enough for a row
    JSAMPARRAY samples = (*m_info.mem->alloc_sarray)((j_common_ptr) &m_info,
        JPOOL_IMAGE, m_info.output_width * 4, 1);

    while (m_info.output_scanline < m_info.output_height) {
        // jpeg_read_scanlines will increase the scanline counter, so we
        // save the scanline before calling it.
        int sourceY = m_info.output_scanline;
        /* Request one scanline.  Returns 0 or 1 scanlines. */
        if (jpeg_read_scanlines(&m_info, samples, 1) != 1)
            return false;

        ImageFrame::PixelData* currentAddress = buffer->getAddr(0, sourceY);
        JSAMPLE* jsample = *samples;
        for (int x = 0; x < m_info.output_width; ++x) {
                // Convert RGB data to RGBA data
            buffer->setRGBA(currentAddress, jsample[0], jsample[1], jsample[2],
                                       0xFF);
            ++currentAddress;
            jsample += 3;  // for RGB data
        }
    }
    return true;
}

bool JPEGImageReaderLibJpeg::outputScanlines(ImageFrame* buffer)
{
    ASSERT(buffer);

    buffer->setSize(width(), height());
    buffer->setStatus(ImageFrame::FramePartial);
    // The buffer is transparent outside the decoded area while the image is
    // loading. The completed image will be marked fully opaque when
    // decoding is done.
    buffer->setHasAlpha(true);
    buffer->setColorProfile(colorProfile());

    // For JPEGs, the frame always fills the entire image.
    buffer->setOriginalFrameRect(IntRect(IntPoint(), size()));

    return outputRGB(buffer);
}

bool JPEGImageReaderLibJpeg::setUpNativeObject(const char* data, size_t size)
{
    ASSERT(!decoded());
    // We set up the normal JPEG error routines, then override error_exit.
    m_info.err = jpeg_std_error(&m_error);
    m_error.error_exit = JPEGImageReaderLibJpeg::error_exit;

    // Allocate and initialize JPEG decompression object.
    jpeg_create_decompress(&m_info);

    m_info.src = (jpeg_source_mgr*)&m_source;

    // Set up callback functions for source manager.
    m_source.init_source = JPEGImageReaderLibJpeg::init_source;
    m_source.fill_input_buffer = JPEGImageReaderLibJpeg::fill_input_buffer;
    m_source.skip_input_data = JPEGImageReaderLibJpeg::skip_input_data;
    m_source.resync_to_restart = JPEGImageReaderLibJpeg::jpeg_resync_to_restart;
    m_source.term_source = JPEGImageReaderLibJpeg::term_source;

    // Setup client data of m_info.
    m_info.client_data = this;

#if USE(ICCJPEG)
    // Retain ICC color profile markers for color management.
    setup_read_icc_profile(&m_info);
#endif

    // Keep APP1 blocks, for obtaining exif data.
    jpeg_save_markers(&m_info, exifMarker, 0xFFFF);

    m_info.src->bytes_in_buffer = size;
    m_info.src->next_input_byte = (JOCTET*)(data);

    return true;
}

// Decode image header.
bool JPEGImageReaderLibJpeg::decodeHeader(const char* data, size_t size)
{
    ASSERT(!decoded());

    // We need to do the setjmp here. Otherwise bad things will happen
    if (setjmp(m_setjmpPoint))
        return false;

    // Read header
    if (jpeg_read_header(&m_info, true) == JPEG_SUSPENDED)
        return false; // I/O suspension. Shouldn't happen

    // We can fill in the size now that the header is available.
    setSize(m_info.image_width, m_info.image_height);

    // We support only single frame JPEG
    setFrameCount(1);
    return true;
}

// Decode image frames.
bool JPEGImageReaderLibJpeg::decodeFrames(const char* data, size_t size,
                                          ImageFrameVector* frames)
{
    ASSERT(!decoded());
    ASSERT(frames->size() == 1);

    // We need to do the setjmp here. Otherwise bad things will happen
    if (setjmp(m_setjmpPoint))
        return false;

    readImageOrientation();

    m_info.out_color_space = JCS_RGB;
    // Don't allocate a giant and superfluous memory buffer when the
    // image is a sequential JPEG.
    m_info.buffered_image = jpeg_has_multiple_scans(&m_info);

    // Used to set up image size so arrays can be allocated.
    jpeg_calc_output_dimensions(&m_info);

    // Start image decompress
    // Set parameters for decompression.
    m_info.dct_method = JDCT_ISLOW;
    m_info.dither_mode = JDITHER_FS;
    m_info.do_fancy_upsampling = false;
    m_info.enable_2pass_quant = false;
    m_info.do_block_smoothing = true;

    // Start decompressor.
    if (!jpeg_start_decompress(&m_info))
        return false; // I/O suspension. Shouldn't happen

    // JPEG format supports only 1 frame
    ImageFrame* buffer = &(*frames)[0];

    if (m_info.buffered_image) {  // PROGRESSIVE
        int status;
        do {
            status = jpeg_consume_input(&m_info);
        } while ((status != JPEG_SUSPENDED) && (status != JPEG_REACHED_EOI));

        for (;;) {
            if (!m_info.output_scanline) {
                int scan = m_info.input_scan_number;

                // If we haven't displayed anything yet
                // (output_scan_number == 0) and we have enough data for
                // a complete scan, force output of the last full scan.
                if (!m_info.output_scan_number && (scan > 1)
                    && (status != JPEG_REACHED_EOI))
                    --scan;

                if (!jpeg_start_output(&m_info, scan))
                    return false; // I/O suspension. Shouldn't happen
            }

            if (m_info.output_scanline == 0xffffff)
                m_info.output_scanline = 0;

            if (!outputScanlines(buffer)) {
                if (!m_info.output_scanline)
                    // Didn't manage to read any lines - flag so we
                    // don't call jpeg_start_output() multiple times for
                    // the same scan.
                    m_info.output_scanline = 0xffffff;

                return false; // I/O suspension. Shouldn't happen
            }

            if (m_info.output_scanline == m_info.output_height) {
                if (!jpeg_finish_output(&m_info))
                    return false; // I/O suspension. Shouldn't happen

                if (jpeg_input_complete(&m_info)
                    && (m_info.input_scan_number == m_info.output_scan_number))
                    break;

                m_info.output_scanline = 0;
            }
        }
    } else {  // SEQUENTIAL
        if (!outputScanlines(buffer))
            return false; // I/O suspension. Shouldn't happen

        // If we've completed image output...
        ASSERT(m_info.output_scanline == m_info.output_height);
    }

    if (jpeg_finish_decompress(&m_info)) {
        // Finish decompression.
        buffer->setHasAlpha(false);
        buffer->setStatus(ImageFrame::FrameComplete);
        return true;
    } else {
        return false;
    }
}

void JPEGImageReaderLibJpeg::destroyNativeObject()
{
    jpeg_destroy_decompress(&m_info);
}

/* static */
unsigned JPEGImageReaderLibJpeg::readUint16(JOCTET* data, bool isBigEndian)
{
    if (isBigEndian)
        return (GETJOCTET(data[0]) << 8) | GETJOCTET(data[1]);
    return (GETJOCTET(data[1]) << 8) | GETJOCTET(data[0]);
}

/* static */
unsigned JPEGImageReaderLibJpeg::readUint32(JOCTET* data, bool isBigEndian)
{
    if (isBigEndian)
        return (GETJOCTET(data[0]) << 24) | (GETJOCTET(data[1]) << 16)
               | (GETJOCTET(data[2]) << 8) | GETJOCTET(data[3]);
    return (GETJOCTET(data[3]) << 24) | (GETJOCTET(data[2]) << 16)
           | (GETJOCTET(data[1]) << 8) | GETJOCTET(data[0]);
}

/* static */
bool JPEGImageReaderLibJpeg::checkExifHeader(jpeg_saved_marker_ptr marker,
                                         bool& isBigEndian,
                                         unsigned& ifdOffset)
{
    // For exif data, the APP1 block is followed by 'E', 'x', 'i', 'f', '\0',
    // then a fill byte, and then a tiff file that contains the metadata.
    // A tiff file starts with 'I', 'I' (intel / little endian byte order) or
    // 'M', 'M' (motorola / big endian byte order), followed by (uint16_t)42,
    // followed by an uint32_t with the offset to the tag block, relative to the
    // tiff file start.
    const unsigned exifHeaderSize = 14;
    if (!(marker->marker == exifMarker
        && marker->data_length >= exifHeaderSize
        && marker->data[0] == 'E'
        && marker->data[1] == 'x'
        && marker->data[2] == 'i'
        && marker->data[3] == 'f'
        && marker->data[4] == '\0'
        // data[5] is a fill byte
        && ((marker->data[6] == 'I' && marker->data[7] == 'I')
            || (marker->data[6] == 'M' && marker->data[7] == 'M'))))
        return false;

    isBigEndian = marker->data[6] == 'M';
    if (readUint16(marker->data + 8, isBigEndian) != 42)
        return false;

    ifdOffset = readUint32(marker->data + 10, isBigEndian);
    return true;
}

/* static */
bool JPEGImageReaderLibJpeg::rgbColorProfile(const char* profileData,
                                         unsigned profileLength)
{
    ASSERT_UNUSED(profileLength, profileLength >= iccColorProfileHeaderLength);

    return !memcmp(&profileData[16], "RGB ", 4);
}

/* static */
bool JPEGImageReaderLibJpeg::inputDeviceColorProfile(const char* profileData,
                                                 unsigned profileLength)
{
    ASSERT_UNUSED(profileLength, profileLength >= iccColorProfileHeaderLength);

    return !memcmp(&profileData[12], "mntr", 4)
           || !memcmp(&profileData[12], "scnr", 4);
}

void JPEGImageReaderLibJpeg::readImageOrientation()
{
    // The JPEG decoder looks at EXIF metadata.
    // FIXME: Possibly implement XMP and IPTC support.
    const unsigned orientationTag = 0x112;
    const unsigned shortType = 3;
    for (jpeg_saved_marker_ptr marker = m_info.marker_list;
         marker; marker = marker->next) {
        bool isBigEndian;
        unsigned ifdOffset;
        if (!checkExifHeader(marker, isBigEndian, ifdOffset))
            continue;
        // Account for 'Exif\0<fill byte>' header.
        const unsigned offsetToTiffData = 6;
        if (marker->data_length < offsetToTiffData
            || ifdOffset >= marker->data_length - offsetToTiffData)
            continue;
        ifdOffset += offsetToTiffData;

        // The jpeg exif container format contains a tiff block for metadata.
        // A tiff image file directory (ifd) consists of a uint16_t describing
        // the number of ifd entries, followed by that many entries.
        // When touching this code, it's useful to look at the tiff spec:
        // http://partners.adobe.com/public/developer/en/tiff/TIFF6.pdf
        JOCTET* ifd = marker->data + ifdOffset;
        JOCTET* end = marker->data + marker->data_length;
        if (end - ifd < 2)
            continue;
        unsigned tagCount = readUint16(ifd, isBigEndian);
        ifd += 2; // Skip over the uint16 that was just read.

        // Every ifd entry is 2 bytes of tag, 2 bytes of contents datatype,
        // 4 bytes of number-of-elements, and 4 bytes of either offset to the
        // tag data, or if the data is small enough, the inlined data itself.
        const int ifdEntrySize = 12;
        for (unsigned i = 0; i < tagCount && end - ifd >= ifdEntrySize;
             ++i, ifd += ifdEntrySize) {
            unsigned tag = readUint16(ifd, isBigEndian);
            unsigned type = readUint16(ifd + 2, isBigEndian);
            unsigned count = readUint32(ifd + 4, isBigEndian);
            if (tag == orientationTag && type == shortType && count == 1)
                setOrientation(
                    ImageOrientation::fromEXIFValue(readUint16(ifd + 8,
                                                    isBigEndian)));
        }
    }
}

void JPEGImageReaderLibJpeg::readColorProfile()
{
    JOCTET* profile;
    unsigned int profileLength;

    if (!read_icc_profile(&m_info, &profile, &profileLength))
        return;

    // Only accept RGB color profiles from input class devices.
    bool ignoreProfile = false;
    char* profileData = reinterpret_cast<char*>(profile);
    if (profileLength < iccColorProfileHeaderLength)
        ignoreProfile = true;
    else if (!rgbColorProfile(profileData, profileLength))
        ignoreProfile = true;
    else if (!inputDeviceColorProfile(profileData, profileLength))
        ignoreProfile = true;

    ColorProfile colorProfile;
    if (!ignoreProfile)
        colorProfile.append(profileData, profileLength);
    free(profile);
    setColorProfile(colorProfile);
}

void JPEGImageReaderLibJpeg::skipBytes(long numBytes)
{
    jpeg_source_mgr* src = (jpeg_source_mgr*)m_info.src;
    long bytesToSkip = std::min(numBytes, (long)src->bytes_in_buffer);
    src->bytes_in_buffer -= (size_t)bytesToSkip;
    src->next_input_byte += bytesToSkip;
}

}  // namespace WebCore

#endif  // #if !__LB_ENABLE_SHELL_NATIVE_JPEG_DECODER__
