/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "base/file_path.h"
#include "base/path_service.h"
#include "gtest/gtest.h"
#include "ImageDecoder.h"
// For exporting frame buffers to PNG files. Note that there are a few
// "PNGImageEncoder.h"s in the project. The skia version is included here.
#include "PNGImageEncoder.h"
#include "SharedBuffer.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

using namespace WebCore;

struct ImageDecoderTestData
{
    // Image file. Reference image file is m_fileName#.{frame_num}.bmp
    const char* m_fileName;
    // Information to verify
    const char* m_extension;        // File type
    bool m_hasAlpha;                // Image has alpha channel or not
    int m_frameCount;               // Frame count
};

class ImageDecoderShellTest
    : public ::testing::TestWithParam<ImageDecoderTestData>
{
public:
    virtual void SetUp()
    {
        ImageDecoderTestData data = GetParam();
        m_decoder = WTF::adoptPtr(CreateDecoderFromFile(data.m_fileName));
    }

    virtual void TearDown()
    {
    }

    ImageDecoder* CreateDecoderFromFile(const char* filePath)
    {
        ImageDecoder* decoder = NULL;
        // Open file
        RefPtr<SharedBuffer> data = ReadFile(filePath);
        if (data)
        {
            // Create decoder
            decoder = ImageDecoder::create(*data,
                WebCore::ImageSource::AlphaPremultiplied,
                WebCore::ImageSource::GammaAndColorProfileApplied);
            // Set data
            if (decoder)
                decoder->setData(data.get(), true);
        }
        else
        {
            fprintf(stdout, "Failed to read image file %s\n", filePath);
        }
        return decoder;
    }

protected:
    RefPtr<SharedBuffer> ReadFile(const char* filePath)
    {
        struct stat statBuf;
        FilePath dataPath;
        PathService::Get(base::DIR_SOURCE_ROOT, &dataPath);
        dataPath = dataPath.Append(FILE_PATH_LITERAL(filePath));

        const char* path = dataPath.value().c_str();

        if (stat(path, &statBuf))
        {
            return NULL;
        }

        WTF::Vector<char> contents;
        contents.resize(statBuf.st_size);
        if (contents.size() != statBuf.st_size)
        {
            return NULL;  // Out of memory
        }
        FILE* fin(fopen(path, "rb"));
        if (!fin)
        {
            return NULL;
        }
        size_t bytes_read(fread(contents.data(), 1, contents.size(), fin));
        fclose(fin);

        if (bytes_read != contents.size())
        {
            return NULL;
        }
        // Create buffer
        return SharedBuffer::create(contents.data(), contents.size());
    }

    // Write frame to PNG file using build in PNGImageEncoder, so that
    // we can use external tools to compare.
    void OutputFrame(size_t index, const char* refFileName)
    {
        ImageFrame* frame = m_decoder->frameBufferAtIndex(index);
        ASSERT_NE(static_cast<ImageFrame*>(NULL), frame);

        // Convert to PNG
        Vector<unsigned char> pngData;
        bool res = PNGImageEncoder::encode(frame->getSkBitmap(), &pngData);
        ASSERT_TRUE(res) << "Failed to encode image for image file: "
                         << refFileName
                         << " frame: " << index;

        // Write file
        char filename[256];
        snprintf(filename, 256, "%s.png", refFileName);
        FilePath dataPath;
        PathService::Get(base::DIR_SOURCE_ROOT, &dataPath);
        dataPath = dataPath.Append(FILE_PATH_LITERAL(filename));
        const char* path = dataPath.value().c_str();

        FILE* fp = fopen(path, "wb");
        ASSERT_NE(static_cast<FILE*>(NULL), fp)
            << "Failed to write reference image file: " << path;
        fwrite(pngData.data(), pngData.size(), 1, fp);
        fclose(fp);
    }

    OwnPtr<ImageDecoder> m_decoder;
};

TEST_P(ImageDecoderShellTest, ImageInfo)
{
    ImageDecoderTestData data = GetParam();
    fprintf(stdout, "Running ImageInfo test with file %s\n", data.m_fileName);
    ASSERT_NE(static_cast<ImageDecoder*>(NULL), m_decoder);

    EXPECT_EQ(data.m_extension, m_decoder->filenameExtension());
}

TEST_P(ImageDecoderShellTest, ImageFrame)
{
    ImageDecoderTestData data = GetParam();
    fprintf(stdout, "Running ImageFrame test with file %s\n", data.m_fileName);
    ASSERT_NE(static_cast<ImageDecoder*>(NULL), m_decoder);

    const char* subStr = strstr(data.m_fileName, "/bad.");
    if (subStr) {
        // This is a broken image
        EXPECT_FALSE(m_decoder->isSizeAvailable());
        EXPECT_TRUE(m_decoder->failed());
        return;  // Bail out
    }
    EXPECT_EQ(data.m_frameCount, m_decoder->frameCount());
    // Size should be available
    EXPECT_TRUE(m_decoder->isSizeAvailable());
    EXPECT_FALSE(m_decoder->failed());

    // Some images might be slightly different using different decoder,
    // hence comparing pixels directly may cause false alarm.
    // Here we write frames to image files so that we can use
    // external tools to compare the input and output images to verify.
    if (data.m_frameCount > 1)
    {
        for (int i = 0; i < data.m_frameCount; ++i)
        {
            // Check current frame
            ImageFrame* frame = m_decoder->frameBufferAtIndex(i);
            ASSERT_NE(static_cast<ImageFrame*>(NULL), frame);
            EXPECT_EQ(data.m_hasAlpha, frame->hasAlpha());
            EXPECT_EQ(ImageFrame::FrameComplete, frame->status());

            char filenameBase[256];
            // Generate ref image file name
            snprintf(filenameBase, 256, "%s-%d", data.m_fileName, i);
            // Create ref decoder
            OutputFrame(i, filenameBase);
        }
    }
    else
    {
        // Check current frame
        ImageFrame* frame = m_decoder->frameBufferAtIndex(0);
        ASSERT_NE(static_cast<ImageFrame*>(NULL), frame);
        EXPECT_EQ(data.m_hasAlpha, frame->hasAlpha());
        EXPECT_EQ(ImageFrame::FrameComplete, frame->status());

        // Create ref decoder
        OutputFrame(0, data.m_fileName);
    }
}

// Image file data
static ImageDecoderTestData s_decoderTestData[] =
{
    // Good image files.
    { "imagedecoder/test_13_13.png", "png", false, 1 },
    { "imagedecoder/baboon.png", "png", false, 1 },
    { "imagedecoder/text_test1.png", "png", false, 1 },
    { "imagedecoder/text_test2.png", "png", false, 1 },
    { "imagedecoder/text_test3.png", "png", false, 1 },
    { "imagedecoder/trans_text_test1.png", "png", true, 1 },
    { "imagedecoder/trans_text_test2.png", "png", true, 1 },
    { "imagedecoder/trans_text_test3.png", "png", true, 1 },

    { "imagedecoder/test_13_13.gif", "gif", false, 1 },
    { "imagedecoder/baboon.gif", "gif", false, 1 },
    { "imagedecoder/text_test1.gif", "gif", false, 1 },
    { "imagedecoder/text_test2.gif", "gif", false, 1 },
    { "imagedecoder/text_test3.gif", "gif", false, 1 },
    { "imagedecoder/trans_text_test1.gif", "gif", true, 1 },
    { "imagedecoder/trans_text_test2.gif", "gif", true, 1 },
    { "imagedecoder/trans_text_test3.gif", "gif", true, 1 },
    { "imagedecoder/text_test_anim.gif", "gif", false, 3 },
    { "imagedecoder/trans_text_test_anim.gif", "gif", true, 3 },
    { "imagedecoder/spinner.gif", "gif", false, 12 },

    { "imagedecoder/test_13_13.jpg", "jpg", false, 1 },
    { "imagedecoder/baboon.jpg", "jpg", false, 1 },
    { "imagedecoder/lena.jpg", "jpg", false, 1 },
    { "imagedecoder/text_test1.jpg", "jpg", false, 1 },
    { "imagedecoder/text_test2.jpg", "jpg", false, 1 },
    { "imagedecoder/text_test3.jpg", "jpg", false, 1 },

    { "imagedecoder/test_13_13.bmp", "bmp", false, 1 },
    { "imagedecoder/test_13_13_256b.bmp", "bmp", false, 1 },
    { "imagedecoder/baboon.bmp", "bmp", false, 1 },
    // It seems webcore believes these bmp files have alpha channel.
    { "imagedecoder/text_test1.bmp", "bmp", true, 1 },
    { "imagedecoder/text_test2.bmp", "bmp", true, 1 },
    { "imagedecoder/text_test3.bmp", "bmp", true, 1 },

    // Broken images files. To make sure image decoders wont crash on broken
    // image data. Especially some libraries such as libjpeg and libpng
    // use setjmp/longjmp. It seems that bmp and gif decoders are pretty
    // forgivable to missing data at the end of file. So for these two
    // formats only broken files of missing data at beginning are included.

    // Delete about 100 bytes from beginning of file (after signature)
    { "imagedecoder/bad.baboon1.png", "png", false, 1 },
    // Delete 1k-2k at the end of file
    { "imagedecoder/bad.baboon2.png", "png", false, 1 },
    // Delete about 100 bytes from beginning of file (after signature)
    { "imagedecoder/bad.baboon1.gif", "gif", false, 1 },
    // Delete about 100 bytes from beginning of file (after signature)
    { "imagedecoder/bad.baboon1.jpg", "jpg", false, 1 },
    // Delete 1k-2k at the end of file
    { "imagedecoder/bad.baboon2.jpg", "jpg", false, 1 },
    // Delete about 100 bytes from beginning of file (after signature)
    { "imagedecoder/bad.baboon1.bmp", "bmp", false, 1 },
};

// Use INSTANTIATE_TEST_CASE_P to instantiate the test cases.
INSTANTIATE_TEST_CASE_P(ImageDecoderShellTestInstance,
                        ImageDecoderShellTest,
                        ::testing::ValuesIn(s_decoderTestData));
