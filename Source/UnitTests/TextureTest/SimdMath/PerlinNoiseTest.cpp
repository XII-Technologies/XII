/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TextureTest/TextureTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST_GROUP(SimdMath);

XII_CREATE_SIMPLE_TEST(SimdMath, PerlinNoise)
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

    for (xiiUInt32 uiOctaveCount = 1; uiOctaveCount <= 6; ++uiOctaveCount)
    {
      xiiColorLinearUB* pData = image.GetPixelPointer<xiiColorLinearUB>();

      for (xiiUInt32 y = 0; y < description.m_Size.width; ++y)
      {
        for (xiiUInt32 x = 0; x < description.m_Size.width / 4; ++x)
        {
          xiiSimdVec4f sX = (xiiSimdVec4f(x * 4.0f) + xOffset) / scale;
          xiiSimdVec4f sY = xiiSimdVec4f(y * 1.0f) / scale;

          xiiSimdVec4f noise = perlin.NoiseZeroToOne(sX, sY, xiiSimdVec4f::MakeZero(), uiOctaveCount);
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
      sOutFile.SetFormat(":output/SimdNoise/result-perlin_{}.tga", uiOctaveCount);

      XII_TEST_BOOL(image.SaveTo(sOutFile).Succeeded());

      xiiStringBuilder sInFile;
      sInFile.SetFormat("SimdNoise/perlin_{}.tga", uiOctaveCount);
      XII_TEST_BOOL_MSG(xiiFileSystem::ExistsFile(sInFile), "Noise image file is missing: '%s'", sInFile.GetData());

      XII_TEST_FILES(sOutFile, sInFile, "");
    }
  }

  xiiFileSystem::RemoveDataDirectoryGroup("SimdNoise");
}
