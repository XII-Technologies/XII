/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/IterateBits.h>

namespace
{
  template <typename T>
  void TestEmptyIntegerBitValues()
  {
    xiiUInt32 uiNextBit = 1;
    for (auto bit : xiiIterateBitValues(static_cast<T>(0)))
    {
      XII_TEST_BOOL_MSG(false, "No bit should be present");
    }
  }

  template <typename T>
  void TestFullIntegerBitValues()
  {
    constexpr xiiUInt64 uiBitCount = sizeof(T) * 8;
    xiiUInt64           uiNextBit  = 1;
    xiiUInt64           uiCount    = 0;
    for (auto bit : xiiIterateBitValues(xiiMath::MaxValue<T>()))
    {
      XII_TEST_INT(bit, uiNextBit);
      uiNextBit *= 2;
      uiCount++;
    }
    XII_TEST_INT(uiBitCount, uiCount);
  }

  template <typename T>
  void TestEmptyIntegerBitIndices()
  {
    xiiUInt32 uiNextBit = 1;
    for (auto bit : xiiIterateBitIndices(static_cast<T>(0)))
    {
      XII_TEST_BOOL_MSG(false, "No bit should be present");
    }
  }

  template <typename T>
  void TestFullIntegerBitIndices()
  {
    constexpr xiiUInt64 uiBitCount     = sizeof(T) * 8;
    xiiUInt64           uiNextBitIndex = 0;
    for (auto bit : xiiIterateBitIndices(xiiMath::MaxValue<T>()))
    {
      XII_TEST_INT(bit, uiNextBitIndex);
      ++uiNextBitIndex;
    }
    XII_TEST_INT(uiBitCount, uiNextBitIndex);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Containers, IterateBits)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiIterateBitValues")
  {
    {
      // Empty set
      TestEmptyIntegerBitValues<xiiUInt8>();
      TestEmptyIntegerBitValues<xiiUInt16>();
      TestEmptyIntegerBitValues<xiiUInt32>();
      TestEmptyIntegerBitValues<xiiUInt64>();
    }

    {
      // Full sets
      TestFullIntegerBitValues<xiiUInt8>();
      TestFullIntegerBitValues<xiiUInt16>();
      TestFullIntegerBitValues<xiiUInt32>();
      TestFullIntegerBitValues<xiiUInt64>();
    }

    {
      // Some bits set
      xiiUInt64                    uiBitMask = 0b1101;
      xiiHybridArray<xiiUInt64, 3> bits;
      bits.PushBack(0b0001);
      bits.PushBack(0b0100);
      bits.PushBack(0b1000);

      for (xiiUInt64 bit : xiiIterateBitValues(uiBitMask))
      {
        XII_TEST_INT(bit, bits[0]);
        bits.RemoveAtAndCopy(0);
      }
      XII_TEST_BOOL(bits.IsEmpty());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiIterateBitIndices")
  {
    {
      // Empty set
      TestEmptyIntegerBitIndices<xiiUInt8>();
      TestEmptyIntegerBitIndices<xiiUInt16>();
      TestEmptyIntegerBitIndices<xiiUInt32>();
      TestEmptyIntegerBitIndices<xiiUInt64>();
    }

    {
      // Full sets
      TestFullIntegerBitIndices<xiiUInt8>();
      TestFullIntegerBitIndices<xiiUInt16>();
      TestFullIntegerBitIndices<xiiUInt32>();
      TestFullIntegerBitIndices<xiiUInt64>();
    }

    {
      // Some bits set
      xiiUInt64                    uiBitMask = 0b1101;
      xiiHybridArray<xiiUInt64, 3> bits;
      bits.PushBack(0);
      bits.PushBack(2);
      bits.PushBack(3);

      for (xiiUInt64 bit : xiiIterateBitIndices(uiBitMask))
      {
        XII_TEST_INT(bit, bits[0]);
        bits.RemoveAtAndCopy(0);
      }
      XII_TEST_BOOL(bits.IsEmpty());
    }
  }
}
