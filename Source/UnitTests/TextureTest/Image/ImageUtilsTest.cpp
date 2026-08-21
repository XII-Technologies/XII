/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/ImageUtils.h>

XII_CREATE_SIMPLE_TEST(Image, ImageUtils)
{
  xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
  xiiStringBuilder sWriteDir = xiiTestFramework::GetInstance()->GetAbsOutputPath();

  XII_TEST_BOOL(xiiOSFile::CreateDirectoryStructure(sWriteDir) == XII_SUCCESS);

  xiiResult addDirectoryResult = xiiFileSystem::AddDataDirectory(sReadDir, "ImageTest");
  XII_TEST_BOOL(addDirectoryResult == XII_SUCCESS);

  if (addDirectoryResult.Failed())
    return;

  addDirectoryResult = xiiFileSystem::AddDataDirectory(sWriteDir, "ImageTest", "output", xiiDataDirUsage::AllowWrites);
  XII_TEST_BOOL(addDirectoryResult == XII_SUCCESS);

  if (addDirectoryResult.Failed())
    return;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ComputeImageDifferenceABS RGBA")
  {
    xiiImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGBA.tga").IgnoreResult();

    xiiImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/Diff_RGBA.tga").IgnoreResult();

    XII_TEST_FILES("ImageUtils/ExpectedDiff_RGBA.tga", "ImageUtils/Diff_RGBA.tga", "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Scaledown Half RGBA")
  {
    xiiImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    xiiImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();

    ImageAc.SaveTo(":output/ImageUtils/ScaledHalf_RGBA.tga").IgnoreResult();

    XII_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGBA.tga", "ImageUtils/ScaledHalf_RGBA.tga", "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CropImage RGBA")
  {
    xiiImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    xiiImageUtils::CropImage(ImageA, xiiVec2I32(100, 75), xiiSizeU32(300, 180), ImageAc);

    ImageAc.SaveTo(":output/ImageUtils/Crop_RGBA.tga").IgnoreResult();

    XII_TEST_FILES("ImageUtils/ExpectedCrop_RGBA.tga", "ImageUtils/Crop_RGBA.tga", "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ComputeMeanSquareError")
  {
    xiiImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGBA.tga").IgnoreResult();

    xiiImage ImageAc, ImageBc;
    xiiImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();
    xiiImageUtils::Scale(ImageB, ImageBc, ImageB.GetWidth() / 2, ImageB.GetHeight() / 2).IgnoreResult();

    xiiImageUtils::ComputeImageDifferenceABS(ImageAc, ImageBc, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/MeanSquareDiff_RGBA.tga").IgnoreResult();

    XII_TEST_FILES("ImageUtils/ExpectedMeanSquareDiff_RGBA.tga", "ImageUtils/MeanSquareDiff_RGBA.tga", "");

    xiiUInt32 uiError = xiiImageUtils::ComputeMeanSquareError(ImageDiff, 4);
    XII_TEST_INT(uiError, 3155);
  }

  xiiFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
