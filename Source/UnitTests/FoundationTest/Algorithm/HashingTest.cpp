#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Strings/HashedString.h>

XII_CREATE_SIMPLE_TEST_GROUP(Algorithm);

// Warning for overflow in compile time executed static_assert(xiiHashingUtils::MurmurHash32...)
// Todo: Why is this not happening elsewhere?
#pragma warning(disable : 4307)

XII_CREATE_SIMPLE_TEST(Algorithm, Hashing)
{
  // check whether compile time hashing gives the same value as runtime hashing
  const char*      szString      = "This is a test string. 1234";
  const char*      szStringLower = "this is a test string. 1234";
  const char*      szString2     = "THiS iS A TESt sTrInG. 1234";
  xiiStringBuilder sb            = szString;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Hashfunction")
  {
    xiiUInt32           uiHashRT = xiiHashingUtils::MurmurHash32String(sb.GetData());
    constexpr xiiUInt32 uiHashCT = xiiHashingUtils::MurmurHash32String("This is a test string. 1234");
    XII_TEST_INT(uiHashRT, 0xb999d6c4);
    XII_TEST_INT(uiHashRT, uiHashCT);

    // Static assert to ensure this is happening at compile time!
    static_assert(xiiHashingUtils::MurmurHash32String("This is a test string. 1234") == static_cast<xiiUInt32>(0xb999d6c4), "Error in compile time murmur hash calculation!");

    {
      // Test short inputs (< 16 characters) of xx hash at compile time
      xiiUInt32 uixxHashRT = xiiHashingUtils::xxHash32("Test string", 11, 0);
      xiiUInt32 uixxHashCT = xiiHashingUtils::xxHash32String("Test string", 0);
      XII_TEST_INT(uixxHashRT, uixxHashCT);
      static_assert(xiiHashingUtils::xxHash32String("Test string") == 0x1b50ee03);

      // Test long inputs ( > 16 characters) of xx hash at compile time
      xiiUInt32 uixxHashRTLong = xiiHashingUtils::xxHash32String(sb.GetData());
      xiiUInt32 uixxHashCTLong = xiiHashingUtils::xxHash32String("This is a test string. 1234");
      XII_TEST_INT(uixxHashRTLong, uixxHashCTLong);
      static_assert(xiiHashingUtils::xxHash32String("This is a test string. 1234") == 0xff35b049);
    }

    {
      // Test short inputs (< 32 characters) of xx hash 64 at compile time
      xiiUInt64 uixxHash64RT = xiiHashingUtils::xxHash64("Test string", 11, 0);
      xiiUInt64 uixxHash64CT = xiiHashingUtils::xxHash64String("Test string", 0);
      XII_TEST_INT(uixxHash64RT, uixxHash64CT);
      static_assert(xiiHashingUtils::xxHash64String("Test string") == 0xcf0f91eece7c88feULL);

      // Test long inputs ( > 32 characters) of xx hash 64 at compile time
      xiiUInt64 uixxHash64RTLong = xiiHashingUtils::xxHash64String(xiiStringView("This is a longer test string for 64-bit. 123456"));
      xiiUInt64 uixxHash64CTLong = xiiHashingUtils::xxHash64String("This is a longer test string for 64-bit. 123456");
      XII_TEST_INT(uixxHash64RTLong, uixxHash64CTLong);
      static_assert(xiiHashingUtils::xxHash64String("This is a longer test string for 64-bit. 123456") == 0xb85d007925299bacULL);
    }

    {
      // Test short inputs (< 32 characters) of xx hash 64 at compile time
      xiiUInt64 uixxHash64RT = xiiHashingUtils::StringHash(xiiStringView("Test string"));
      xiiUInt64 uixxHash64CT = xiiHashingUtils::StringHash("Test string");
      XII_TEST_INT(uixxHash64RT, uixxHash64CT);
      static_assert(xiiHashingUtils::StringHash("Test string") == 0xcf0f91eece7c88feULL);

      // Test long inputs ( > 32 characters) of xx hash 64 at compile time
      xiiUInt64 uixxHash64RTLong = xiiHashingUtils::StringHash(xiiStringView("This is a longer test string for 64-bit. 123456"));
      xiiUInt64 uixxHash64CTLong = xiiHashingUtils::StringHash("This is a longer test string for 64-bit. 123456");
      XII_TEST_INT(uixxHash64RTLong, uixxHash64CTLong);
      static_assert(xiiHashingUtils::StringHash("This is a longer test string for 64-bit. 123456") == 0xb85d007925299bacULL);
    }

    // Check MurmurHash for unaligned inputs
    const char* alignmentTestString = "12345678_12345678__12345678___12345678";
    xiiUInt32   uiHash1             = xiiHashingUtils::MurmurHash32(alignmentTestString, 8);
    xiiUInt32   uiHash2             = xiiHashingUtils::MurmurHash32(alignmentTestString + 9, 8);
    xiiUInt32   uiHash3             = xiiHashingUtils::MurmurHash32(alignmentTestString + 19, 8);
    xiiUInt32   uiHash4             = xiiHashingUtils::MurmurHash32(alignmentTestString + 30, 8);
    XII_TEST_INT(uiHash1, uiHash2);
    XII_TEST_INT(uiHash1, uiHash3);
    XII_TEST_INT(uiHash1, uiHash4);

    // check 64bit hashes
    const xiiUInt64 uiMurmurHash64 = xiiHashingUtils::MurmurHash64(sb.GetData(), sb.GetElementCount());
    XII_TEST_INT(uiMurmurHash64, 0xf8ebc5e8cb110786);

    // Check MurmurHash64 for unaligned inputs
    xiiUInt64 uiHash1_64 = xiiHashingUtils::MurmurHash64(alignmentTestString, 8);
    xiiUInt64 uiHash2_64 = xiiHashingUtils::MurmurHash64(alignmentTestString + 9, 8);
    xiiUInt64 uiHash3_64 = xiiHashingUtils::MurmurHash64(alignmentTestString + 19, 8);
    xiiUInt64 uiHash4_64 = xiiHashingUtils::MurmurHash64(alignmentTestString + 30, 8);
    XII_TEST_INT(uiHash1_64, uiHash2_64);
    XII_TEST_INT(uiHash1_64, uiHash3_64);
    XII_TEST_INT(uiHash1_64, uiHash4_64);

    // test crc32
    const xiiUInt32 uiCrc32 = xiiHashingUtils::CRC32Hash(sb.GetData(), sb.GetElementCount());
    XII_TEST_INT(uiCrc32, 0x73b5e898);

    // Check crc32 for unaligned inputs
    uiHash1 = xiiHashingUtils::CRC32Hash(alignmentTestString, 8);
    uiHash2 = xiiHashingUtils::CRC32Hash(alignmentTestString + 9, 8);
    uiHash3 = xiiHashingUtils::CRC32Hash(alignmentTestString + 19, 8);
    uiHash4 = xiiHashingUtils::CRC32Hash(alignmentTestString + 30, 8);
    XII_TEST_INT(uiHash1, uiHash2);
    XII_TEST_INT(uiHash1, uiHash3);
    XII_TEST_INT(uiHash1, uiHash4);

    // 32 Bit xxHash
    const xiiUInt32 uiXXHash32 = xiiHashingUtils::xxHash32(sb.GetData(), sb.GetElementCount());
    XII_TEST_INT(uiXXHash32, 0xff35b049);

    // Check xxHash for unaligned inputs
    uiHash1 = xiiHashingUtils::xxHash32(alignmentTestString, 8);
    uiHash2 = xiiHashingUtils::xxHash32(alignmentTestString + 9, 8);
    uiHash3 = xiiHashingUtils::xxHash32(alignmentTestString + 19, 8);
    uiHash4 = xiiHashingUtils::xxHash32(alignmentTestString + 30, 8);
    XII_TEST_INT(uiHash1, uiHash2);
    XII_TEST_INT(uiHash1, uiHash3);
    XII_TEST_INT(uiHash1, uiHash4);

    // 64 Bit xxHash
    const xiiUInt64 uiXXHash64 = xiiHashingUtils::xxHash64(sb.GetData(), sb.GetElementCount());
    XII_TEST_INT(uiXXHash64, 0x141fb89c0bf32020);
    // Check xxHash64 for unaligned inputs
    uiHash1_64 = xiiHashingUtils::xxHash64(alignmentTestString, 8);
    uiHash2_64 = xiiHashingUtils::xxHash64(alignmentTestString + 9, 8);
    uiHash3_64 = xiiHashingUtils::xxHash64(alignmentTestString + 19, 8);
    uiHash4_64 = xiiHashingUtils::xxHash64(alignmentTestString + 30, 8);
    XII_TEST_INT(uiHash1_64, uiHash2_64);
    XII_TEST_INT(uiHash1_64, uiHash3_64);
    XII_TEST_INT(uiHash1_64, uiHash4_64);

    xiiUInt32 uixxHash32RTEmpty = xiiHashingUtils::xxHash32("", 0, 0);
    xiiUInt32 uixxHash32CTEmpty = xiiHashingUtils::xxHash32String("", 0);
    XII_TEST_BOOL(uixxHash32RTEmpty == uixxHash32CTEmpty);

    xiiUInt64 uixxHash64RTEmpty = xiiHashingUtils::xxHash64("", 0, 0);
    xiiUInt64 uixxHash64CTEmpty = xiiHashingUtils::xxHash64String("", 0);
    XII_TEST_BOOL(uixxHash64RTEmpty == uixxHash64CTEmpty);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HashHelper")
  {
    xiiUInt32 uiHash = xiiHashHelper<xiiStringBuilder>::Hash(sb);
    XII_TEST_INT(uiHash, 0x0bf32020);

    const char* szTest = "This is a test string. 1234";
    uiHash             = xiiHashHelper<const char*>::Hash(szTest);
    XII_TEST_INT(uiHash, 0x0bf32020);
    XII_TEST_BOOL(xiiHashHelper<const char*>::Equal(szTest, sb.GetData()));

    xiiHashedString hs;
    hs.Assign(szTest);
    uiHash = xiiHashHelper<xiiHashedString>::Hash(hs);
    XII_TEST_INT(uiHash, 0x0bf32020);

    xiiTempHashedString ths(szTest);
    uiHash = xiiHashHelper<xiiHashedString>::Hash(ths);
    XII_TEST_INT(uiHash, 0x0bf32020);
    XII_TEST_BOOL(xiiHashHelper<xiiHashedString>::Equal(hs, ths));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HashHelperString_NoCase")
  {
    const xiiUInt32 uiHash = xiiHashHelper<const char*>::Hash(szStringLower);
    XII_TEST_INT(uiHash, 0x19404167);
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(szString));
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(szStringLower));
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(szString2));
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(sb));
    xiiStringBuilder sb2 = szString2;
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(sb2));
    xiiString sL = szStringLower;
    xiiString s1 = sb;
    xiiString s2 = sb2;
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(s1));
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(s2));
    xiiStringView svL = szStringLower;
    xiiStringView sv1 = szString;
    xiiStringView sv2 = szString2;
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(svL));
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(sv1));
    XII_TEST_INT(uiHash, xiiHashHelperString_NoCase::Hash(sv2));

    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(sb, sb2));
    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(sb, szString2));
    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(sb, sv2));
    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(s1, sb2));
    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(s1, szString2));
    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(s1, sv2));
    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(sv1, sb2));
    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(sv1, szString2));
    XII_TEST_BOOL(xiiHashHelperString_NoCase::Equal(sv1, sv2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HashStream32")
  {
    const char* szTest      = "This is a test string. 1234";
    const char* szTestHalf1 = "This is a test";
    const char* szTestHalf2 = " string. 1234";

    auto test = [szTest, szTestHalf1, szTestHalf2](bool bFlush, xiiUInt32* pHash) {
      xiiHashStreamWriter32 writer1;
      writer1.WriteBytes(szTest, std::strlen(szTest)).IgnoreResult();
      if (bFlush)
      {
        writer1.Flush().IgnoreResult();
      }

      const xiiUInt32 uiHash1 = writer1.GetHashValue();

      xiiHashStreamWriter32 writer2;
      writer2.WriteBytes(szTestHalf1, std::strlen(szTestHalf1)).IgnoreResult();
      if (bFlush)
      {
        writer2.Flush().IgnoreResult();
      }

      writer2.WriteBytes(szTestHalf2, std::strlen(szTestHalf2)).IgnoreResult();
      if (bFlush)
      {
        writer2.Flush().IgnoreResult();
      }

      const xiiUInt32 uiHash2 = writer2.GetHashValue();

      xiiHashStreamWriter32 writer3;
      for (xiiUInt64 i = 0; szTest[i] != 0; ++i)
      {
        writer3.WriteBytes(szTest + i, 1).IgnoreResult();

        if (bFlush)
        {
          writer3.Flush().IgnoreResult();
        }
      }
      const xiiUInt32 uiHash3 = writer3.GetHashValue();

      XII_TEST_INT(uiHash1, uiHash2);
      XII_TEST_INT(uiHash1, uiHash3);

      *pHash = uiHash1;
    };

    xiiUInt32 uiHash1 = 0, uiHash2 = 1;
    test(true, &uiHash1);
    test(false, &uiHash2);
    XII_TEST_INT(uiHash1, uiHash2);

    const xiiUInt64 uiHash3 = xiiHashingUtils::xxHash32(szTest, std::strlen(szTest));
    XII_TEST_INT(uiHash1, uiHash3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HashStream64")
  {
    const char* szTest      = "This is a test string. 1234";
    const char* szTestHalf1 = "This is a test";
    const char* szTestHalf2 = " string. 1234";

    auto test = [szTest, szTestHalf1, szTestHalf2](bool bFlush, xiiUInt64* pHash) {
      xiiHashStreamWriter64 writer1;
      writer1.WriteBytes(szTest, std::strlen(szTest)).IgnoreResult();

      if (bFlush)
      {
        writer1.Flush().IgnoreResult();
      }

      const xiiUInt64 uiHash1 = writer1.GetHashValue();

      xiiHashStreamWriter64 writer2;
      writer2.WriteBytes(szTestHalf1, std::strlen(szTestHalf1)).IgnoreResult();
      if (bFlush)
        writer2.Flush().IgnoreResult();
      writer2.WriteBytes(szTestHalf2, std::strlen(szTestHalf2)).IgnoreResult();
      if (bFlush)
        writer2.Flush().IgnoreResult();

      const xiiUInt64 uiHash2 = writer2.GetHashValue();

      xiiHashStreamWriter64 writer3;
      for (xiiUInt64 i = 0; szTest[i] != 0; ++i)
      {
        writer3.WriteBytes(szTest + i, 1).IgnoreResult();
        if (bFlush)
          writer3.Flush().IgnoreResult();
      }
      const xiiUInt64 uiHash3 = writer3.GetHashValue();

      XII_TEST_INT(uiHash1, uiHash2);
      XII_TEST_INT(uiHash1, uiHash3);

      *pHash = uiHash1;
    };

    xiiUInt64 uiHash1 = 0, uiHash2 = 1;
    test(true, &uiHash1);
    test(false, &uiHash2);
    XII_TEST_INT(uiHash1, uiHash2);

    const xiiUInt64 uiHash3 = xiiHashingUtils::xxHash64(szTest, std::strlen(szTest));
    XII_TEST_INT(uiHash1, uiHash3);
  }
}

struct SimpleHashableStruct : public xiiHashableStruct<SimpleHashableStruct>
{
  xiiUInt32 m_uiTestMember1;
  xiiUInt8  m_uiTestMember2;
  xiiUInt64 m_uiTestMember3;
};

struct SimpleStruct
{
  xiiUInt32 m_uiTestMember1;
  xiiUInt8  m_uiTestMember2;
  xiiUInt64 m_uiTestMember3;
};

XII_CREATE_SIMPLE_TEST(Algorithm, HashableStruct)
{
  SimpleHashableStruct AutomaticInst;
  XII_TEST_INT(AutomaticInst.m_uiTestMember1, 0);
  XII_TEST_INT(AutomaticInst.m_uiTestMember2, 0);
  XII_TEST_INT(AutomaticInst.m_uiTestMember3, 0);

  SimpleStruct NonAutomaticInst;
  xiiMemoryUtils::ZeroFill(&NonAutomaticInst, 1);

  XII_CHECK_AT_COMPILETIME(sizeof(AutomaticInst) == sizeof(NonAutomaticInst));

  XII_TEST_INT(xiiMemoryUtils::Compare<xiiUInt8>((xiiUInt8*)&AutomaticInst, (xiiUInt8*)&NonAutomaticInst, sizeof(AutomaticInst)), 0);

  AutomaticInst.m_uiTestMember2 = 0x42u;
  AutomaticInst.m_uiTestMember3 = 0x23u;

  xiiUInt32 uiAutomaticHash = AutomaticInst.CalculateHash();

  NonAutomaticInst.m_uiTestMember2 = 0x42u;
  NonAutomaticInst.m_uiTestMember3 = 0x23u;

  xiiUInt32 uiNonAutomaticHash = xiiHashingUtils::xxHash32(&NonAutomaticInst, sizeof(NonAutomaticInst));

  XII_TEST_INT(uiAutomaticHash, uiNonAutomaticHash);

  AutomaticInst.m_uiTestMember1 = 0x5u;
  uiAutomaticHash               = AutomaticInst.CalculateHash();

  XII_TEST_BOOL(uiAutomaticHash != uiNonAutomaticHash);
}
