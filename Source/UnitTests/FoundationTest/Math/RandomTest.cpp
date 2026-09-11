/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Random.h>

// only works when also linking against CoreUtils
// #define USE_XIIIMAGE

#ifdef USE_XIIIMAGE
#  include <Texture/Image/Image.h>
#endif


XII_CREATE_SIMPLE_TEST(Math, Random)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "UIntInRange")
  {
    xiiRandom r;
    r.Initialize(0xAABBCCDDEEFF0011ULL);

    for (xiiUInt32 i = 2; i < 10000; ++i)
    {
      const xiiUInt32 val = r.UIntInRange(i);
      XII_TEST_BOOL(val < i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IntMinMax")
  {
    xiiRandom r;
    r.Initialize(0xBBCCDDEEFF0011AAULL);

    XII_TEST_INT(r.IntMinMax(5, 5), 5);
    XII_TEST_INT(r.IntMinMax(-5, -5), -5);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const xiiInt32 val = r.IntMinMax(i, i + i);
      XII_TEST_BOOL(val >= i);
      XII_TEST_BOOL(val <= i + i);
    }

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const xiiInt32 val = r.IntMinMax(-i, i);
      XII_TEST_BOOL(val >= -i);
      XII_TEST_BOOL(val <= i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IntMinMax")
  {
    xiiRandom r;
    r.Initialize(0xCCDDEEFF0011AABBULL);

    XII_TEST_INT(r.IntMinMax(5, 5), 5);
    XII_TEST_INT(r.IntMinMax(-5, -5), -5);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const xiiInt32 val = r.IntMinMax(i, 2 * i);
      XII_TEST_BOOL(val >= i);
      XII_TEST_BOOL(val <= i + i);
    }

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const xiiInt32 val = r.IntMinMax(-i, i);
      XII_TEST_BOOL(val >= -i);
      XII_TEST_BOOL(val <= i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Bool")
  {
    xiiRandom r;
    r.Initialize(0x11AABBCCDDEEFFULL);

    xiiUInt32               falseCount = 0;
    xiiUInt32               trueCount  = 0;
    xiiTemporaryArray<bool> values;
    values.SetCount(1000);

    for (int i = 0; i < 1000; ++i)
    {
      values[i] = r.Bool();
      if (values[i])
      {
        ++trueCount;
      }
      else
      {
        ++falseCount;
      }
    }

    // This could be more elaborate, one could also test the variance
    // and assert that approximately an uniform distribution is yielded
    XII_TEST_BOOL(trueCount > 0 && falseCount > 0);

    xiiRandom r2;
    r2.Initialize(0x11AABBCCDDEEFFULL);

    for (int i = 0; i < 1000; ++i)
    {
      XII_TEST_BOOL(values[i] == r2.Bool());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DoubleZeroToOneExclusive")
  {
    xiiRandom r;
    r.Initialize(0xDDEEFF0011AABBCCULL);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneExclusive();
      XII_TEST_BOOL(val >= 0.0);
      XII_TEST_BOOL(val < 1.0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DoubleZeroToOneInclusive")
  {
    xiiRandom r;
    r.Initialize(0xEEFF0011AABBCCDDULL);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleZeroToOneInclusive();
      XII_TEST_BOOL(val >= 0.0);
      XII_TEST_BOOL(val <= 1.0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DoubleInRange")
  {
    xiiRandom r;
    r.Initialize(0xFF0011AABBCCDDEEULL);

    XII_TEST_DOUBLE(r.DoubleMinMax(5, 5), 5, 0.0);
    XII_TEST_DOUBLE(r.DoubleMinMax(-5, -5), -5, 0.0);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(i, i + i);
      XII_TEST_BOOL(val >= i);
      XII_TEST_BOOL(val < i + i);
    }

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(-i, i);
      XII_TEST_BOOL(val >= -i);
      XII_TEST_BOOL(val < -i + 2 * i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DoubleMinMax")
  {
    xiiRandom r;
    r.Initialize(0x0011AABBCCDDEEFFULL);

    XII_TEST_DOUBLE(r.DoubleMinMax(5, 5), 5, 0.0);
    XII_TEST_DOUBLE(r.DoubleMinMax(-5, -5), -5, 0.0);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(i, 2 * i);
      XII_TEST_BOOL(val >= i);
      XII_TEST_BOOL(val <= i + i);
    }

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const double val = r.DoubleMinMax(-i, i);
      XII_TEST_BOOL(val >= -i);
      XII_TEST_BOOL(val <= i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FloatZeroToOneExclusive")
  {
    xiiRandom r;
    r.Initialize(0xDDEEFF0011AABBCCULL);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatZeroToOneExclusive();
      XII_TEST_BOOL(val >= 0.f);
      XII_TEST_BOOL(val < 1.f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FloatZeroToOneInclusive")
  {
    xiiRandom r;
    r.Initialize(0xEEFF0011AABBCCDDULL);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatZeroToOneInclusive();
      XII_TEST_BOOL(val >= 0.f);
      XII_TEST_BOOL(val <= 1.f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FloatInRange")
  {
    xiiRandom r;
    r.Initialize(0xFF0011AABBCCDDEEULL);

    XII_TEST_FLOAT(r.FloatMinMax(5, 5), 5, 0.f);
    XII_TEST_FLOAT(r.FloatMinMax(-5, -5), -5, 0.f);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(i), static_cast<float>(i + i));
      XII_TEST_BOOL(val >= i);
      XII_TEST_BOOL(val < i + i);
    }

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(-i), static_cast<float>(i));
      XII_TEST_BOOL(val >= -i);
      XII_TEST_BOOL(val < -i + 2 * i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FloatMinMax")
  {
    xiiRandom r;
    r.Initialize(0x0011AABBCCDDEEFFULL);

    XII_TEST_FLOAT(r.FloatMinMax(5, 5), 5, 0.f);
    XII_TEST_FLOAT(r.FloatMinMax(-5, -5), -5, 0.f);

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(i), static_cast<float>(2 * i));
      XII_TEST_BOOL(val >= i);
      XII_TEST_BOOL(val <= i + i);
    }

    for (xiiInt32 i = 2; i < 10000; ++i)
    {
      const float val = r.FloatMinMax(static_cast<float>(-i), static_cast<float>(i));
      XII_TEST_BOOL(val >= -i);
      XII_TEST_BOOL(val <= i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Save / Load")
  {
    xiiRandom r, r2;
    r.Initialize(0x0011AABBCCDDE11FULL);

    for (int i = 0; i < 1000; ++i)
      r.UInt();

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiMemoryStreamReader         reader(&storage);

    r.Save(writer);

    xiiTemporaryArray<xiiUInt32> temp;
    temp.SetCountUninitialized(1000);

    for (int i = 0; i < 1000; ++i)
      temp[i] = r.UInt();

    r2.Load(reader);

    for (int i = 0; i < 1000; ++i)
    {
      XII_TEST_INT(temp[i], r2.UInt());
    }
  }
}

static void SaveToImage(xiiDynamicArray<xiiUInt32>& ref_values, xiiUInt32 uiMaxValue, const char* szFile)
{
#ifdef USE_XIIIMAGE
  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory("", xiiDataDirUsage::AllowWrites, "Clear") == XII_SUCCESS);

  xiiImage img;
  img.SetWidth(Values.GetCount());
  img.SetHeight(100);
  img.SetImageFormat(xiiImageFormat::B8G8R8A8_UNORM);
  img.AllocateImageData();

  for (xiiUInt32 y = 0; y < img.GetHeight(); ++y)
  {
    for (xiiUInt32 x = 0; x < img.GetWidth(); ++x)
    {
      xiiUInt32* pPixel = img.GetPixelPointer<xiiUInt32>(0, 0, 0, x, y);
      *pPixel           = 0xFF000000;
    }
  }

  for (xiiUInt32 i = 0; i < Values.GetCount(); ++i)
  {
    double    val = ((double)Values[i] / (double)uiMaxValue) * 100.0;
    xiiUInt32 y   = 99 - xiiMath::Clamp<xiiUInt32>((xiiUInt32)val, 0, 99);

    xiiUInt32* pPixel = img.GetPixelPointer<xiiUInt32>(0, 0, 0, i, y);
    *pPixel           = 0xFFFFFFFF;
  }

  img.SaveTo(szFile);

  xiiFileSystem::RemoveDataDirectoryGroup("Clear");
#endif
}

XII_CREATE_SIMPLE_TEST(Math, RandomGauss)
{
  const float fVariance = 1.0f;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "UnsignedValue")
  {
    xiiRandomGauss r;
    r.Initialize(0xABCDEF0012345678ULL, 100, fVariance);

    xiiTemporaryArray<xiiUInt32> Values;
    Values.SetCount(100);

    xiiUInt32 uiMaxValue = 0;

    const xiiUInt32 factor = 10; // with a factor of 100 the bell curve becomes more pronounced, with less samples it has more exceptions
    for (xiiUInt32 i = 0; i < 10000 * factor; ++i)
    {
      auto val = r.UnsignedValue();

      XII_TEST_BOOL(val < 100);

      if (val < Values.GetCount())
      {
        Values[val]++;

        uiMaxValue = xiiMath::Max(uiMaxValue, Values[val]);
      }
    }

    SaveToImage(Values, uiMaxValue, "D:/GaussUnsigned.tga");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SignedValue")
  {
    xiiRandomGauss r;
    r.Initialize(0xABCDEF0012345678ULL, 100, fVariance);

    xiiTemporaryArray<xiiUInt32> Values;
    Values.SetCount(2 * 100);

    xiiUInt32 uiMaxValue = 0;

    const xiiUInt32 factor = 10; // with a factor of 100 the bell curve becomes more pronounced, with less samples it has more exceptions
    for (xiiUInt32 i = 0; i < 10000 * factor; ++i)
    {
      auto val = r.SignedValue();

      XII_TEST_BOOL(val > -100 && val < 100);

      val += 100;

      if (val < (xiiInt32)Values.GetCount())
      {
        Values[val]++;

        uiMaxValue = xiiMath::Max(uiMaxValue, Values[val]);
      }
    }

    SaveToImage(Values, uiMaxValue, "D:/GaussSigned.tga");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Save / Load")
  {
    xiiRandomGauss r, r2;
    r.Initialize(0x0011AABBCCDDE11FULL, 1000, 1.7f);

    for (int i = 0; i < 1000; ++i)
      r.UnsignedValue();

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiMemoryStreamReader         reader(&storage);

    r.Save(writer);

    xiiTemporaryArray<xiiUInt32> temp;
    temp.SetCountUninitialized(1000);

    for (int i = 0; i < 1000; ++i)
      temp[i] = r.UnsignedValue();

    r2.Load(reader);

    for (int i = 0; i < 1000; ++i)
    {
      XII_TEST_INT(temp[i], r2.UnsignedValue());
    }
  }
}
