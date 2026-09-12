/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>

namespace
{
  // Precomputed reference hashes.
  static constexpr xiiUInt64 s_PerlinHash[7] =
    {
      0,                     // unused
      0xDFE237CBAC0B7B33ULL, // octave 1
      0xF4475E3826424486ULL, // octave 2
      0x3CDBF15B1AB55974ULL, // octave 3
      0xF28CAE2573BEABF9ULL, // octave 4
      0x7FE442EB5A4DA646ULL, // octave 5
      0x59AF16D3CC97641DULL  // octave 6
  };
} // namespace

XII_CREATE_SIMPLE_TEST(SimdMath, SimdNoise)
{
  xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
  xiiStringBuilder sWriteDirectory = xiiTestFramework::GetInstance()->GetAbsOutputPath();

  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sReadDir, "SimdNoise") == XII_SUCCESS);
  XII_TEST_BOOL_MSG(xiiFileSystem::AddDataDirectory(sWriteDirectory, "SimdNoise", "output", xiiDataDirUsage::AllowWrites) == XII_SUCCESS, "Failed to mount data directory '{}'.", sWriteDirectory.GetData());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PerlinNoise")
  {
    xiiSimdPerlinNoise perlin(12345);

    const xiiUInt32 uiNumSamplesX   = 16;
    const xiiUInt32 uiNumSamplesY   = 16;
    const xiiUInt32 uiNumOctavesMax = 6;
    const float     fScale          = 100.0f;

    xiiTemporaryArray<float> buffer;
    buffer.SetCount(uiNumSamplesX * uiNumSamplesY);

    for (xiiUInt32 uiNumOctaves = 1; uiNumOctaves <= uiNumOctavesMax; ++uiNumOctaves)
    {
      xiiUInt32 uiIndex = 0;

      for (xiiUInt32 y = 0; y < uiNumSamplesY; ++y)
      {
        for (xiiUInt32 x = 0; x < uiNumSamplesX; x += 4)
        {
          xiiSimdVec4f sX((x + 0) / fScale, (x + 1) / fScale, (x + 2) / fScale, (x + 3) / fScale);
          xiiSimdVec4f sY(y / fScale, y / fScale, y / fScale, y / fScale);

          xiiSimdVec4f noise = perlin.NoiseZeroToOne(sX, sY, xiiSimdVec4f::MakeZero(), uiNumOctaves);

          buffer[uiIndex++] = noise.x();
          buffer[uiIndex++] = noise.y();
          buffer[uiIndex++] = noise.z();
          buffer[uiIndex++] = noise.w();
        }
      }

      xiiUInt64 uiHash = xiiHashingUtils::xxHash64(buffer.GetData(), buffer.GetCount() * sizeof(float));

      XII_TEST_INT(uiHash, s_PerlinHash[uiNumOctaves]);
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
