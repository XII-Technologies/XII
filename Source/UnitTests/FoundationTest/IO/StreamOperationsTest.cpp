#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  struct SerializableStructWithMethods
  {

    XII_DECLARE_POD_TYPE();

    xiiResult Serialize(xiiStreamWriter& ref_stream) const
    {
      ref_stream << m_uiMember1;
      ref_stream << m_uiMember2;

      return XII_SUCCESS;
    }

    xiiResult Deserialize(xiiStreamReader& ref_stream)
    {
      ref_stream >> m_uiMember1;
      ref_stream >> m_uiMember2;

      return XII_SUCCESS;
    }

    xiiInt32 m_uiMember1 = 0x42;
    xiiInt32 m_uiMember2 = 0x23;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(IO, StreamOperation)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Binary Stream Basic Operations (built-in types)")
  {
    xiiDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    xiiMemoryStreamWriter StreamWriter(&StreamStorage);

    StreamWriter << (xiiUInt8)0x42;
    StreamWriter << (xiiUInt16)0x4223;
    StreamWriter << (xiiUInt32)0x42232342;
    StreamWriter << (xiiUInt64)0x4223234242232342;
    StreamWriter << 42.0f;
    StreamWriter << 23.0;
    StreamWriter << (xiiInt8)0x23;
    StreamWriter << (xiiInt16)0x2342;
    StreamWriter << (xiiInt32)0x23422342;
    StreamWriter << (xiiInt64)0x2342234242232342;

    // Arrays
    {
      xiiDynamicArray<xiiUInt32> DynamicArray;
      DynamicArray.PushBack(42);
      DynamicArray.PushBack(23);
      DynamicArray.PushBack(13);
      DynamicArray.PushBack(5);
      DynamicArray.PushBack(0);

      StreamWriter.WriteArray(DynamicArray).IgnoreResult();
    }

    // Create reader
    xiiMemoryStreamReader StreamReader(&StreamStorage);

    // Read back
    {
      xiiUInt8 uiVal;
      StreamReader >> uiVal;
      XII_TEST_BOOL(uiVal == (xiiUInt8)0x42);
    }
    {
      xiiUInt16 uiVal;
      StreamReader >> uiVal;
      XII_TEST_BOOL(uiVal == (xiiUInt16)0x4223);
    }
    {
      xiiUInt32 uiVal;
      StreamReader >> uiVal;
      XII_TEST_BOOL(uiVal == (xiiUInt32)0x42232342);
    }
    {
      xiiUInt64 uiVal;
      StreamReader >> uiVal;
      XII_TEST_BOOL(uiVal == (xiiUInt64)0x4223234242232342);
    }

    {
      float fVal;
      StreamReader >> fVal;
      XII_TEST_BOOL(fVal == 42.0f);
    }
    {
      double dVal;
      StreamReader >> dVal;
      XII_TEST_BOOL(dVal == 23.0f);
    }


    {
      xiiInt8 iVal;
      StreamReader >> iVal;
      XII_TEST_BOOL(iVal == (xiiInt8)0x23);
    }
    {
      xiiInt16 iVal;
      StreamReader >> iVal;
      XII_TEST_BOOL(iVal == (xiiInt16)0x2342);
    }
    {
      xiiInt32 iVal;
      StreamReader >> iVal;
      XII_TEST_BOOL(iVal == (xiiInt32)0x23422342);
    }
    {
      xiiInt64 iVal;
      StreamReader >> iVal;
      XII_TEST_BOOL(iVal == (xiiInt64)0x2342234242232342);
    }

    {
      xiiDynamicArray<xiiUInt32> ReadBackDynamicArray;

      // This element will be removed by the ReadArray function
      ReadBackDynamicArray.PushBack(0xAAu);

      StreamReader.ReadArray(ReadBackDynamicArray).IgnoreResult();

      XII_TEST_INT(ReadBackDynamicArray.GetCount(), 5);

      XII_TEST_INT(ReadBackDynamicArray[0], 42);
      XII_TEST_INT(ReadBackDynamicArray[1], 23);
      XII_TEST_INT(ReadBackDynamicArray[2], 13);
      XII_TEST_INT(ReadBackDynamicArray[3], 5);
      XII_TEST_INT(ReadBackDynamicArray[4], 0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Binary Stream Arrays of Structs")
  {
    xiiDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    xiiMemoryStreamWriter StreamWriter(&StreamStorage);

    // Write out a couple of the structs
    {
      xiiStaticArray<SerializableStructWithMethods, 16> WriteArray;
      WriteArray.ExpandAndGetRef().m_uiMember1 = 0x5;
      WriteArray.ExpandAndGetRef().m_uiMember1 = 0x6;

      StreamWriter.WriteArray(WriteArray).IgnoreResult();
    }

    // Read back in
    {
      // Create reader
      xiiMemoryStreamReader StreamReader(&StreamStorage);

      // This intentionally uses a different array type for the read back
      // to verify that it is a) compatible and b) all arrays are somewhat tested
      xiiHybridArray<SerializableStructWithMethods, 1> ReadArray;

      StreamReader.ReadArray(ReadArray).IgnoreResult();

      XII_TEST_INT(ReadArray.GetCount(), 2);

      XII_TEST_INT(ReadArray[0].m_uiMember1, 0x5);
      XII_TEST_INT(ReadArray[0].m_uiMember2, 0x23);

      XII_TEST_INT(ReadArray[1].m_uiMember1, 0x6);
      XII_TEST_INT(ReadArray[1].m_uiMember2, 0x23);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiSet Stream Operators")
  {
    xiiDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    xiiMemoryStreamWriter StreamWriter(&StreamStorage);

    xiiSet<xiiString> TestSet;
    TestSet.Insert("Hello");
    TestSet.Insert("World");
    TestSet.Insert("!");

    StreamWriter.WriteSet(TestSet).IgnoreResult();

    xiiSet<xiiString> TestSetReadBack;

    TestSetReadBack.Insert("Shouldn't be there after deserialization.");

    xiiMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadSet(TestSetReadBack).IgnoreResult();

    XII_TEST_INT(TestSetReadBack.GetCount(), 3);

    XII_TEST_BOOL(TestSetReadBack.Contains("Hello"));
    XII_TEST_BOOL(TestSetReadBack.Contains("!"));
    XII_TEST_BOOL(TestSetReadBack.Contains("World"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiMap Stream Operators")
  {
    xiiDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    xiiMemoryStreamWriter StreamWriter(&StreamStorage);

    xiiMap<xiiUInt64, xiiString> TestMap;
    TestMap.Insert(42, "Hello");
    TestMap.Insert(23, "World");
    TestMap.Insert(5, "!");

    StreamWriter.WriteMap(TestMap).IgnoreResult();

    xiiMap<xiiUInt64, xiiString> TestMapReadBack;

    TestMapReadBack.Insert(1, "Shouldn't be there after deserialization.");

    xiiMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadMap(TestMapReadBack).IgnoreResult();

    XII_TEST_INT(TestMapReadBack.GetCount(), 3);

    XII_TEST_BOOL(TestMapReadBack.Contains(42));
    XII_TEST_BOOL(TestMapReadBack.Contains(5));
    XII_TEST_BOOL(TestMapReadBack.Contains(23));

    XII_TEST_BOOL(TestMapReadBack.GetValue(42)->IsEqual("Hello"));
    XII_TEST_BOOL(TestMapReadBack.GetValue(5)->IsEqual("!"));
    XII_TEST_BOOL(TestMapReadBack.GetValue(23)->IsEqual("World"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiHashTable Stream Operators")
  {
    xiiDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    xiiMemoryStreamWriter StreamWriter(&StreamStorage);

    xiiHashTable<xiiUInt64, xiiString> TestHashTable;
    TestHashTable.Insert(42, "Hello");
    TestHashTable.Insert(23, "World");
    TestHashTable.Insert(5, "!");

    StreamWriter.WriteHashTable(TestHashTable).IgnoreResult();

    xiiMap<xiiUInt64, xiiString> TestHashTableReadBack;

    TestHashTableReadBack.Insert(1, "Shouldn't be there after deserialization.");

    xiiMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadMap(TestHashTableReadBack).IgnoreResult();

    XII_TEST_INT(TestHashTableReadBack.GetCount(), 3);

    XII_TEST_BOOL(TestHashTableReadBack.Contains(42));
    XII_TEST_BOOL(TestHashTableReadBack.Contains(5));
    XII_TEST_BOOL(TestHashTableReadBack.Contains(23));

    XII_TEST_BOOL(TestHashTableReadBack.GetValue(42)->IsEqual("Hello"));
    XII_TEST_BOOL(TestHashTableReadBack.GetValue(5)->IsEqual("!"));
    XII_TEST_BOOL(TestHashTableReadBack.GetValue(23)->IsEqual("World"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "String Deduplication")
  {
    xiiDefaultMemoryStreamStorage StreamStorageNonDeduplicated(4096);
    xiiDefaultMemoryStreamStorage StreamStorageDeduplicated(4096);

    xiiHybridString<4> str1 = "Hello World";
    xiiDynamicString   str2 = "Hello World 2";
    xiiStringBuilder   str3 = "Hello Schlumpf";

    // Non deduplicated serialization
    {
      xiiMemoryStreamWriter StreamWriter(&StreamStorageNonDeduplicated);

      StreamWriter << str1;
      StreamWriter << str2;
      StreamWriter << str1;
      StreamWriter << str3;
      StreamWriter << str1;
      StreamWriter << str2;
    }

    // Deduplicated serialization
    {
      xiiMemoryStreamWriter StreamWriter(&StreamStorageDeduplicated);

      xiiStringDeduplicationWriteContext StringDeduplicationContext(StreamWriter);
      auto&                              DeduplicationWriter = StringDeduplicationContext.Begin();

      DeduplicationWriter << str1;
      DeduplicationWriter << str2;
      DeduplicationWriter << str1;
      DeduplicationWriter << str3;
      DeduplicationWriter << str1;
      DeduplicationWriter << str2;

      StringDeduplicationContext.End().IgnoreResult();

      XII_TEST_INT(StringDeduplicationContext.GetUniqueStringCount(), 3);
    }

    XII_TEST_BOOL(StreamStorageDeduplicated.GetStorageSize64() < StreamStorageNonDeduplicated.GetStorageSize64());

    // Read the deduplicated strings back
    {
      xiiMemoryStreamReader StreamReader(&StreamStorageDeduplicated);

      xiiStringDeduplicationReadContext StringDeduplicationReadContext(StreamReader);

      xiiHybridString<16> szRead0, szRead1, szRead2;
      xiiStringBuilder    szRead3, szRead4, szRead5;

      StreamReader >> szRead0;
      StreamReader >> szRead1;
      StreamReader >> szRead2;
      StreamReader >> szRead3;
      StreamReader >> szRead4;
      StreamReader >> szRead5;

      XII_TEST_STRING(szRead0, szRead2);
      XII_TEST_STRING(szRead0, szRead4);
      XII_TEST_STRING(szRead1, szRead5);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Array Serialization Performance (bytes)")
  {
    constexpr xiiUInt32 uiCount = 1024 * 1024 * 10;

    xiiContiguousMemoryStreamStorage storage(uiCount + 16);

    xiiMemoryStreamWriter writer(&storage);
    xiiMemoryStreamReader reader(&storage);

    xiiDynamicArray<xiiUInt8> DynamicArray;
    DynamicArray.SetCountUninitialized(uiCount);

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      DynamicArray[i] = i & 0xFF;
    }

    {
      xiiStopwatch sw;

      writer.WriteArray(DynamicArray).AssertSuccess();

      xiiTime          t = sw.GetRunningTotal();
      xiiStringBuilder s;
      s.Format("Write {} byte array: {}", xiiArgFileSize(uiCount), t);
      xiiTestFramework::Output(xiiTestOutput::Details, s);
    }

    {
      xiiStopwatch sw;

      reader.ReadArray(DynamicArray).IgnoreResult();

      xiiTime          t = sw.GetRunningTotal();
      xiiStringBuilder s;
      s.Format("Read {} byte array: {}", xiiArgFileSize(uiCount), t);
      xiiTestFramework::Output(xiiTestOutput::Details, s);
    }

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      XII_TEST_INT(DynamicArray[i], i & 0xFF);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Array Serialization Performance (xiiVec3)")
  {
    constexpr xiiUInt32 uiCount = 1024 * 1024 * 10;

    xiiContiguousMemoryStreamStorage storage(uiCount * sizeof(xiiVec3) + 16);

    xiiMemoryStreamWriter writer(&storage);
    xiiMemoryStreamReader reader(&storage);

    xiiDynamicArray<xiiVec3> DynamicArray;
    DynamicArray.SetCountUninitialized(uiCount);

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      DynamicArray[i].Set(i, i + 1, i + 2);
    }

    {
      xiiStopwatch sw;

      writer.WriteArray(DynamicArray).AssertSuccess();

      xiiTime          t = sw.GetRunningTotal();
      xiiStringBuilder s;
      s.Format("Write {} vec3 array: {}", xiiArgFileSize(uiCount * sizeof(xiiVec3)), t);
      xiiTestFramework::Output(xiiTestOutput::Details, s);
    }

    {
      xiiStopwatch sw;

      reader.ReadArray(DynamicArray).AssertSuccess();

      xiiTime          t = sw.GetRunningTotal();
      xiiStringBuilder s;
      s.Format("Read {} vec3 array: {}", xiiArgFileSize(uiCount * sizeof(xiiVec3)), t);
      xiiTestFramework::Output(xiiTestOutput::Details, s);
    }

    for (xiiUInt32 i = 0; i < uiCount; ++i)
    {
      XII_TEST_VEC3(DynamicArray[i], xiiVec3(i, i + 1, i + 2), 0.01f);
    }
  }
}
