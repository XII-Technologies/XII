#include <FoundationTest/FoundationTestPCH.h>


#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
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
      "BMPTestImages/good/pal1", "BMPTestImages/good/pal1bg", "BMPTestImages/good/pal1wb", "BMPTestImages/good/pal4", "BMPTestImages/good/pal4rle",
      "BMPTestImages/good/pal8", "BMPTestImages/good/pal8-0", "BMPTestImages/good/pal8nonsquare",
      /*"BMPTestImages/good/pal8os2",*/ "BMPTestImages/good/pal8rle",
      /*"BMPTestImages/good/pal8topdown",*/ "BMPTestImages/good/pal8v4", "BMPTestImages/good/pal8v5", "BMPTestImages/good/pal8w124",
      "BMPTestImages/good/pal8w125", "BMPTestImages/good/pal8w126", "BMPTestImages/good/rgb16", "BMPTestImages/good/rgb16-565pal",
      "BMPTestImages/good/rgb24", "BMPTestImages/good/rgb24pal", "BMPTestImages/good/rgb32", /*"BMPTestImages/good/rgb32bf"*/
    };

    for (int i = 0; i < XII_ARRAY_SIZE(testImagesGood); i++)
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
    const char* testImagesBad[] = {"BMPTestImages/bad/badbitcount", "BMPTestImages/bad/badbitssize",
                                   /*"BMPTestImages/bad/baddens1", "BMPTestImages/bad/baddens2", "BMPTestImages/bad/badfilesize", "BMPTestImages/bad/badheadersize",*/
                                   "BMPTestImages/bad/badpalettesize",
                                   /*"BMPTestImages/bad/badplanes",*/ "BMPTestImages/bad/badrle", "BMPTestImages/bad/badwidth",
                                   /*"BMPTestImages/bad/pal2",*/ "BMPTestImages/bad/pal8badindex", "BMPTestImages/bad/reallybig", "BMPTestImages/bad/rletopdown",
                                   "BMPTestImages/bad/shortfile"};


    for (int i = 0; i < XII_ARRAY_SIZE(testImagesBad); i++)
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
    const char* testImagesGood[] = {"TGATestImages/good/RGB", "TGATestImages/good/RGBA", "TGATestImages/good/RGB_RLE", "TGATestImages/good/RGBA_RLE"};

    for (int i = 0; i < XII_ARRAY_SIZE(testImagesGood); i++)
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
      {"RGB", "tga", 0},
      {"RGBA", "tga", 0},
      {"RGB", "png", 0},
      {"RGBA", "png", 0},
      {"RGB", "jpg", 4650},
      {"RGBA", "jpeg", 16670},
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      {"RGB", "tif", 0},
      {"RGBA", "tif", 0},
#endif
    };

    const char* szTestImagePath = "TGATestImages/good";

    for (int idx = 0; idx < XII_ARRAY_SIZE(imgTests); ++idx)
    {
      xiiImage image;
      {
        xiiStringBuilder fileName;
        fileName.SetFormat("{}/{}.tga", szTestImagePath, imgTests[idx].szImage);

        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(image.LoadFrom(fileName) == XII_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        xiiStringBuilder fileName;
        fileName.SetFormat(":output/WriteImageTest/{}.{}", imgTests[idx].szImage, imgTests[idx].szFormat);

        xiiFileSystem::DeleteFile(fileName);

        XII_TEST_BOOL_MSG(image.SaveTo(fileName) == XII_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        xiiImage image2;
        XII_TEST_BOOL_MSG(image2.LoadFrom(fileName).Succeeded(), "Reading written image failed: '%s'", fileName.GetData());

        image.Convert(xiiImageFormat::R8G8B8A8_UNORM_SRGB).IgnoreResult();
        image2.Convert(xiiImageFormat::R8G8B8A8_UNORM_SRGB).IgnoreResult();

        xiiImage diff;
        xiiImageUtils::ComputeImageDifferenceABS(image, image2, diff);

        const xiiUInt32 uiMSE = xiiImageUtils::ComputeMeanSquareError(diff, 32);

        XII_TEST_BOOL_MSG(uiMSE <= imgTests[idx].uiMSE, "MSE %u is larger than %u for image '%s'", uiMSE, imgTests[idx].uiMSE, fileName.GetData());
      }
    }
  }

  xiiFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
