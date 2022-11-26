#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/EndianHelper.h>

namespace
{
  struct TempStruct
  {
    float     fVal;
    xiiUInt32 uiDVal;
    xiiUInt16 uiWVal1;
    xiiUInt16 uiWVal2;
    char      pad[4];
  };

  struct FloatAndInt
  {
    union
    {
      float     fVal;
      xiiUInt32 uiVal;
    };
  };
} // namespace


XII_CREATE_SIMPLE_TEST(Memory, Endian)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basics")
  {
// Test if the IsBigEndian() delivers the same result as the #define
#if XII_ENABLED(XII_PLATFORM_LITTLE_ENDIAN)
    XII_TEST_BOOL(!xiiEndianHelper::IsBigEndian());
#elif XII_ENABLED(XII_PLATFORM_BIG_ENDIAN)
    XII_TEST_BOOL(xiiEndianHelper::IsBigEndian());
#endif

    // Test conversion functions for single elements
    XII_TEST_BOOL(xiiEndianHelper::Switch(static_cast<xiiUInt16>(0x15FF)) == 0xFF15);
    XII_TEST_BOOL(xiiEndianHelper::Switch(static_cast<xiiUInt32>(0x34AA12FF)) == 0xFF12AA34);
    XII_TEST_BOOL(xiiEndianHelper::Switch(static_cast<xiiUInt64>(0x34AA12FFABC3421E)) == 0x1E42C3ABFF12AA34);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Switching Arrays")
  {
    xiiArrayPtr<xiiUInt16> p16BitArray     = XII_DEFAULT_NEW_ARRAY(xiiUInt16, 1024);
    xiiArrayPtr<xiiUInt16> p16BitArrayCopy = XII_DEFAULT_NEW_ARRAY(xiiUInt16, 1024);

    xiiArrayPtr<xiiUInt32> p32BitArray     = XII_DEFAULT_NEW_ARRAY(xiiUInt32, 1024);
    xiiArrayPtr<xiiUInt32> p32BitArrayCopy = XII_DEFAULT_NEW_ARRAY(xiiUInt32, 1024);

    xiiArrayPtr<xiiUInt64> p64BitArray     = XII_DEFAULT_NEW_ARRAY(xiiUInt64, 1024);
    xiiArrayPtr<xiiUInt64> p64BitArrayCopy = XII_DEFAULT_NEW_ARRAY(xiiUInt64, 1024);

    for (xiiUInt32 i = 0; i < 1024; i++)
    {
      xiiInt32 iRand = rand();
      p16BitArray[i] = static_cast<xiiUInt16>(iRand);
      p32BitArray[i] = static_cast<xiiUInt32>(iRand);
      p64BitArray[i] = static_cast<xiiUInt64>(iRand | static_cast<xiiUInt64>((iRand % 3)) << 32);
    }

    p16BitArrayCopy.CopyFrom(p16BitArray);
    p32BitArrayCopy.CopyFrom(p32BitArray);
    p64BitArrayCopy.CopyFrom(p64BitArray);

    xiiEndianHelper::SwitchWords(p16BitArray.GetPtr(), 1024);
    xiiEndianHelper::SwitchDWords(p32BitArray.GetPtr(), 1024);
    xiiEndianHelper::SwitchQWords(p64BitArray.GetPtr(), 1024);

    for (xiiUInt32 i = 0; i < 1024; i++)
    {
      XII_TEST_BOOL(p16BitArray[i] == xiiEndianHelper::Switch(p16BitArrayCopy[i]));
      XII_TEST_BOOL(p32BitArray[i] == xiiEndianHelper::Switch(p32BitArrayCopy[i]));
      XII_TEST_BOOL(p64BitArray[i] == xiiEndianHelper::Switch(p64BitArrayCopy[i]));

      // Test in place switcher
      xiiEndianHelper::SwitchInPlace(&p16BitArrayCopy[i]);
      XII_TEST_BOOL(p16BitArray[i] == p16BitArrayCopy[i]);

      xiiEndianHelper::SwitchInPlace(&p32BitArrayCopy[i]);
      XII_TEST_BOOL(p32BitArray[i] == p32BitArrayCopy[i]);

      xiiEndianHelper::SwitchInPlace(&p64BitArrayCopy[i]);
      XII_TEST_BOOL(p64BitArray[i] == p64BitArrayCopy[i]);
    }


    XII_DEFAULT_DELETE_ARRAY(p16BitArray);
    XII_DEFAULT_DELETE_ARRAY(p16BitArrayCopy);

    XII_DEFAULT_DELETE_ARRAY(p32BitArray);
    XII_DEFAULT_DELETE_ARRAY(p32BitArrayCopy);

    XII_DEFAULT_DELETE_ARRAY(p64BitArray);
    XII_DEFAULT_DELETE_ARRAY(p64BitArrayCopy);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Switching Structs")
  {
    TempStruct instance = {42.0f, 0x34AA12FF, 0x15FF, 0x23FF, {'E', 'Z', 'F', 'T'}};

    xiiEndianHelper::SwitchStruct(&instance, "ddwwcccc");

    xiiIntFloatUnion floatHelper(42.0f);
    xiiIntFloatUnion floatHelper2(instance.fVal);

    XII_TEST_BOOL(floatHelper2.i == xiiEndianHelper::Switch(floatHelper.i));
    XII_TEST_BOOL(instance.uiDVal == xiiEndianHelper::Switch(static_cast<xiiUInt32>(0x34AA12FF)));
    XII_TEST_BOOL(instance.uiWVal1 == xiiEndianHelper::Switch(static_cast<xiiUInt16>(0x15FF)));
    XII_TEST_BOOL(instance.uiWVal2 == xiiEndianHelper::Switch(static_cast<xiiUInt16>(0x23FF)));
    XII_TEST_BOOL(instance.pad[0] == 'E');
    XII_TEST_BOOL(instance.pad[1] == 'Z');
    XII_TEST_BOOL(instance.pad[2] == 'F');
    XII_TEST_BOOL(instance.pad[3] == 'T');
  }
}
