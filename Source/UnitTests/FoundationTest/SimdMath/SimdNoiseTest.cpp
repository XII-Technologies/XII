/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdNoise)
{
  xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
  xiiStringBuilder sWriteDirectory = xiiTestFramework::GetInstance()->GetAbsOutputPath();

  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sReadDir, "SimdNoise") == XII_SUCCESS);
  XII_TEST_BOOL_MSG(xiiFileSystem::AddDataDirectory(sWriteDirectory, "SimdNoise", "output", xiiDataDirUsage::AllowWrites) == XII_SUCCESS, "Failed to mount data directory '{}'.", sWriteDirectory);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Perlin")
  {
    xiiGALTextureCreationDescription description;
    description.m_Type   = xiiGALResourceDimension::Texture2D;
    description.m_Size   = xiiSizeU32(128, 128);
    description.m_Format = xiiGALResourceFormat::RGBA8UNormalized;

    xiiImage image;
    image.ResetAndAlloc(description);

    xiiSimdPerlinNoise perlin(12345);
    xiiSimdVec4f       xOffset(0, 1, 2, 3);
    xiiSimdFloat       scale(100);

    for (xiiUInt32 uiNumOctaves = 1; uiNumOctaves <= 6; ++uiNumOctaves)
    {
      xiiColorLinearUB* pData = image.GetPixelPointer<xiiColorLinearUB>();

      for (xiiUInt32 y = 0; y < description.m_Size.width; ++y)
      {
        for (xiiUInt32 x = 0; x < description.m_Size.width / 4; ++x)
        {
          xiiSimdVec4f sX = (xiiSimdVec4f(x * 4.0f) + xOffset) / scale;
          xiiSimdVec4f sY = xiiSimdVec4f(y * 1.0f) / scale;

          xiiSimdVec4f noise = perlin.NoiseZeroToOne(sX, sY, xiiSimdVec4f::MakeZero(), uiNumOctaves);
          float        fP[4];
          fP[0] = noise.x();
          fP[1] = noise.y();
          fP[2] = noise.z();
          fP[3] = noise.w();

          xiiUInt32 uiPixelIndex = y * description.m_Size.width + x * 4;

          for (xiiUInt32 i = 0; i < 4; ++i)
          {
            pData[uiPixelIndex + i] = xiiColor(fP[i], fP[i], fP[i]);
          }
        }
      }

      xiiStringBuilder sOutFile;
      sOutFile.SetFormat(":output/SimdNoise/result-perlin_{}.tga", uiNumOctaves);

      XII_TEST_BOOL(image.SaveTo(sOutFile).Succeeded());

      xiiStringBuilder sInFile;
      sInFile.SetFormat("SimdNoise/perlin_{}.tga", uiNumOctaves);
      XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(sInFile), "Noise image file is missing: '%s'", sInFile.GetData());

      XII_TEST_FILES(sOutFile, sInFile, "");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Random")
  {
    xiiUInt32 histogram[256] = {};

    for (xiiUInt32 i = 0; i < 10000; ++i)
    {
      xiiSimdVec4u seed              = xiiSimdVec4u(i);
      xiiSimdVec4f randomValues      = xiiSimdRandom::FloatMinMax(xiiSimdVec4i(0, 1, 2, 3), xiiSimdVec4f::MakeZero(), xiiSimdVec4f(256.0f), seed);
      xiiSimdVec4i randomValuesAsInt = xiiSimdVec4i::Truncate(randomValues);

      ++histogram[randomValuesAsInt.x()];
      ++histogram[randomValuesAsInt.y()];
      ++histogram[randomValuesAsInt.z()];
      ++histogram[randomValuesAsInt.w()];

      randomValues      = xiiSimdRandom::FloatMinMax(xiiSimdVec4i(32, 33, 34, 35), xiiSimdVec4f::MakeZero(), xiiSimdVec4f(256.0f), seed);
      randomValuesAsInt = xiiSimdVec4i::Truncate(randomValues);

      ++histogram[randomValuesAsInt.x()];
      ++histogram[randomValuesAsInt.y()];
      ++histogram[randomValuesAsInt.z()];
      ++histogram[randomValuesAsInt.w()];
    }

    const char* szOutFile = ":output/SimdNoise/result-random.csv";
    {
      xiiFileWriter fileWriter;
      XII_TEST_BOOL(fileWriter.Open(szOutFile).Succeeded());

      xiiStringBuilder sLine;
      for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(histogram); ++i)
      {
        sLine.SetFormat("{},\n", histogram[i]);
        fileWriter.WriteBytes(sLine.GetData(), sLine.GetElementCount()).IgnoreResult();
      }
    }

    const char* szInFile = "SimdNoise/random.csv";
    XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(szInFile), "Random histogram file is missing: '%s'", szInFile);

    XII_TEST_TEXT_FILES(szOutFile, szInFile, "");
  }

  xiiFileSystem::RemoveDataDirectoryGroup("SimdNoise");
}
