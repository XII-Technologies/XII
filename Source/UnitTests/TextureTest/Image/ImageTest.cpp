/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>


#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(Image);

XII_CREATE_SIMPLE_TEST(Image, Image)
{
  const xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
  const xiiStringBuilder sWriteDir = xiiTestFramework::GetInstance()->GetAbsOutputPath();

  XII_TEST_BOOL(xiiOSFile::CreateDirectoryStructure(sWriteDir) == XII_SUCCESS);

  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sReadDir, "ImageTest") == XII_SUCCESS);
  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sWriteDir, "ImageTest", "output", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "BMP - Good")
  {
    const char* testImagesGood[] = {
      "BMPTestImages/Good/pal1",
      "BMPTestImages/Good/pal1bg",
      "BMPTestImages/Good/pal1wb",
      "BMPTestImages/Good/pal4",
      "BMPTestImages/Good/pal4rle",
      "BMPTestImages/Good/pal8",
      "BMPTestImages/Good/pal8-0",
      "BMPTestImages/Good/pal8nonsquare",
      "BMPTestImages/Good/pal8os2",
      "BMPTestImages/Good/pal8rle",
      "BMPTestImages/Good/pal8topdown",
      "BMPTestImages/Good/pal8v4",
      "BMPTestImages/Good/pal8v5",
      "BMPTestImages/Good/pal8w124",
      "BMPTestImages/Good/pal8w125",
      "BMPTestImages/Good/pal8w126",
      "BMPTestImages/Good/rgb16",
      "BMPTestImages/Good/rgb16-565pal",
      "BMPTestImages/Good/rgb24",
      "BMPTestImages/Good/rgb24pal",
      "BMPTestImages/Good/rgb32",
      "BMPTestImages/Good/rgb32bf"
    };

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testImagesGood); ++i)
    {
      xiiImage image;
      {
        xiiStringBuilder fileName;
        fileName.SetFormat("{0}.bmp", testImagesGood[i]);

        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(image.LoadFrom(fileName) == XII_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        xiiStringBuilder fileName;
        fileName.SetFormat(":output/{0}_out.bmp", testImagesGood[i]);

        XII_TEST_BOOL_MSG(image.SaveTo(fileName) == XII_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "BMP - Bad")
  {
    const char* testImagesBad[] = {
      "BMPTestImages/Bad/badbitcount",
      "BMPTestImages/Bad/badheadersize",
      "BMPTestImages/Bad/badpalettesize",
      "BMPTestImages/Bad/badwidth",
      "BMPTestImages/Bad/reallybig",
      "BMPTestImages/Bad/shortfile",
    };

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testImagesBad); ++i)
    {
      xiiImage image;
      {
        xiiStringBuilder fileName;
        fileName.SetFormat("{0}.bmp", testImagesBad[i]);

        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "File does not exist: '%s'", fileName.GetData());

        XII_LOG_BLOCK_MUTE();

        XII_TEST_BOOL_MSG(image.LoadFrom(fileName) == XII_FAILURE, "Reading image should have failed: '%s'", fileName.GetData());
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TGA")
  {
    const char* testImagesGood[] = {
      "TGATestImages/Good/RGBA",
      "TGATestImages/Good/RGBA_RLE",
    };

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(testImagesGood); ++i)
    {
      xiiImage image;
      {
        xiiStringBuilder fileName;
        fileName.SetFormat("{0}.tga", testImagesGood[i]);

        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(image.LoadFrom(fileName) == XII_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        xiiStringBuilder fileName;
        fileName.SetFormat(":output/{0}_out.bmp", testImagesGood[i]);

        xiiStringBuilder fileNameExpected;
        fileNameExpected.SetFormat("{0}_expected.bmp", testImagesGood[i]);

        XII_TEST_BOOL_MSG(image.SaveTo(fileName) == XII_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        XII_TEST_FILES(fileName, fileNameExpected, "");
      }

      {
        xiiStringBuilder fileName;
        fileName.SetFormat(":output/{0}_out.tga", testImagesGood[i]);

        xiiStringBuilder fileNameExpected;
        fileNameExpected.SetFormat("{0}_expected.tga", testImagesGood[i]);

        XII_TEST_BOOL_MSG(image.SaveTo(fileName) == XII_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        XII_TEST_FILES(fileName, fileNameExpected, "");
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Write Image Formats")
  {
    struct ImgTest
    {
      const char* szImage;
      const char* szFormat;
      xiiUInt32   uiMSE;
    };

    ImgTest imgTests[] = {
      {"RGBA", "tga", 0},
      {"RGBA", "png", 0},
      {"RGBA", "jpeg", 16670},
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      {"RGBA", "tif", 0},
#endif
    };

    const char* szTestImagePath = "TGATestImages/Good";

    for (xiiUInt32 uiIndex = 0; uiIndex < XII_ARRAY_SIZE(imgTests); ++uiIndex)
    {
      xiiImage image;
      {
        xiiStringBuilder fileName;
        fileName.SetFormat("{}/{}.tga", szTestImagePath, imgTests[uiIndex].szImage);

        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(image.LoadFrom(fileName) == XII_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        xiiStringBuilder fileName;
        fileName.SetFormat(":output/WriteImageTest/{}.{}", imgTests[uiIndex].szImage, imgTests[uiIndex].szFormat);

        xiiFileSystem::DeleteFile(fileName);

        XII_TEST_BOOL_MSG(image.SaveTo(fileName) == XII_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        xiiImage image2;
        XII_TEST_BOOL_MSG(image2.LoadFrom(fileName).Succeeded(), "Reading written image failed: '%s'", fileName.GetData());

        image.Convert(xiiGALResourceFormat::RGBA8UNormalizedSRGB).IgnoreResult();
        image2.Convert(xiiGALResourceFormat::RGBA8UNormalizedSRGB).IgnoreResult();

        xiiImage diff;
        xiiImageUtils::ComputeImageDifferenceABS(image, image2, diff);

        const xiiUInt32 uiMSE = xiiImageUtils::ComputeMeanSquareError(diff, 32);

        XII_TEST_BOOL_MSG(uiMSE <= imgTests[uiIndex].uiMSE, "MSE %u is larger than %u for image '%s'", uiMSE, imgTests[uiIndex].uiMSE, fileName.GetData());
      }
    }
  }

  xiiFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
