#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/MemoryStream.h>

XII_CREATE_SIMPLE_TEST_GROUP(IO);

XII_CREATE_SIMPLE_TEST(IO, MemoryStream)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Memory Stream Reading / Writing")
  {
    xiiDefaultMemoryStreamStorage StreamStorage;

    // Create reader
    xiiMemoryStreamReader StreamReader(&StreamStorage);

    // Create writer
    xiiMemoryStreamWriter StreamWriter(&StreamStorage);

    // Temp read pointer
    xiiUInt8* pPointer = reinterpret_cast<xiiUInt8*>(0x41); // Should crash when accessed

    // Try reading from an empty stream (should not crash, just return 0 bytes read)
    xiiUInt64 uiBytesRead = StreamReader.ReadBytes(pPointer, 128);

    XII_TEST_BOOL(uiBytesRead == 0);


    // Now try writing data to the stream and reading it back
    xiiUInt32 uiData[1024];
    for (xiiUInt32 i = 0; i < 1024; i++)
      uiData[i] = rand();

    // Calculate the hash so we can reuse the array
    const xiiUInt32 uiHashBeforeWriting = xiiHashingUtils::xxHash32(uiData, sizeof(xiiUInt32) * 1024);

    // Write the data
    XII_TEST_BOOL(StreamWriter.WriteBytes(reinterpret_cast<const xiiUInt8*>(uiData), sizeof(xiiUInt32) * 1024) == XII_SUCCESS);

    XII_TEST_BOOL(StreamWriter.GetByteCount64() == sizeof(xiiUInt32) * 1024);
    XII_TEST_BOOL(StreamWriter.GetByteCount64() == StreamReader.GetByteCount64());
    XII_TEST_BOOL(StreamWriter.GetByteCount64() == StreamStorage.GetStorageSize64());


    // Clear the array for the read back
    xiiMemoryUtils::ZeroFill(uiData, 1024);

    uiBytesRead = StreamReader.ReadBytes(reinterpret_cast<xiiUInt8*>(uiData), sizeof(xiiUInt32) * 1024);

    XII_TEST_BOOL(uiBytesRead == sizeof(xiiUInt32) * 1024);

    const xiiUInt32 uiHashAfterReading = xiiHashingUtils::xxHash32(uiData, sizeof(xiiUInt32) * 1024);

    XII_TEST_BOOL(uiHashAfterReading == uiHashBeforeWriting);

    // Modify data and test the Rewind() functionality of the writer
    uiData[0] = 0x42;
    uiData[1] = 0x23;

    const xiiUInt32 uiHashOfModifiedData = xiiHashingUtils::xxHash32(uiData, sizeof(xiiUInt32) * 4); // Only test the first 4 elements now

    StreamWriter.SetWritePosition(0);

    StreamWriter.WriteBytes(uiData, sizeof(xiiUInt32) * 4).IgnoreResult();

    // Clear the array for the read back
    xiiMemoryUtils::ZeroFill(uiData, 4);

    // Test the rewind of the reader as well
    StreamReader.SetReadPosition(0);

    uiBytesRead = StreamReader.ReadBytes(uiData, sizeof(xiiUInt32) * 4);

    XII_TEST_BOOL(uiBytesRead == sizeof(xiiUInt32) * 4);

    const xiiUInt32 uiHashAfterReadingOfModifiedData = xiiHashingUtils::xxHash32(uiData, sizeof(xiiUInt32) * 4);

    XII_TEST_BOOL(uiHashAfterReadingOfModifiedData == uiHashOfModifiedData);

    // Test skipping
    StreamReader.SetReadPosition(0);

    StreamReader.SkipBytes(sizeof(xiiUInt32));

    xiiUInt32 uiTemp;

    uiBytesRead = StreamReader.ReadBytes(&uiTemp, sizeof(xiiUInt32));

    XII_TEST_BOOL(uiBytesRead == sizeof(xiiUInt32));

    // We skipped over the first 0x42 element, so this should be 0x23
    XII_TEST_BOOL(uiTemp == 0x23);

    // Skip more bytes than available
    xiiUInt64 uiBytesSkipped = StreamReader.SkipBytes(0xFFFFFFFFFF);

    XII_TEST_BOOL(uiBytesSkipped < 0xFFFFFFFFFF);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Raw Memory Stream Reading")
  {
    xiiDynamicArray<xiiUInt8> OrigStorage;
    OrigStorage.SetCountUninitialized(1000);

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      OrigStorage[i] = i % 256;
    }

    {
      xiiRawMemoryStreamReader reader(OrigStorage);

      xiiDynamicArray<xiiUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(static_cast<xiiUInt32>(reader.GetByteCount()));
      reader.ReadBytes(CopyStorage.GetData(), reader.GetByteCount());

      XII_TEST_BOOL(OrigStorage == CopyStorage);
    }

    {
      xiiRawMemoryStreamReader reader(OrigStorage.GetData() + 510, 490);

      xiiDynamicArray<xiiUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(static_cast<xiiUInt32>(reader.GetByteCount()));
      reader.ReadBytes(CopyStorage.GetData(), reader.GetByteCount());

      XII_TEST_BOOL(OrigStorage != CopyStorage);

      for (xiiUInt32 i = 0; i < 490; ++i)
      {
        CopyStorage[i] = (i + 10) % 256;
      }
    }

    {
      xiiRawMemoryStreamReader reader(OrigStorage.GetData(), 1000);
      reader.SkipBytes(510);

      xiiDynamicArray<xiiUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(490);
      reader.ReadBytes(CopyStorage.GetData(), 490);

      XII_TEST_BOOL(OrigStorage != CopyStorage);

      for (xiiUInt32 i = 0; i < 490; ++i)
      {
        CopyStorage[i] = (i + 10) % 256;
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Raw Memory Stream Writing")
  {
    xiiDynamicArray<xiiUInt8> OrigStorage;
    OrigStorage.SetCountUninitialized(1000);

    xiiRawMemoryStreamWriter writer0;
    XII_TEST_INT(writer0.GetNumWrittenBytes(), 0);
    XII_TEST_INT(writer0.GetStorageSize(), 0);

    xiiRawMemoryStreamWriter writer(OrigStorage.GetData(), OrigStorage.GetCount());

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      writer << static_cast<xiiUInt8>(i % 256);

      XII_TEST_INT(writer.GetNumWrittenBytes(), i + 1);
      XII_TEST_INT(writer.GetStorageSize(), 1000);
    }

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      XII_TEST_INT(OrigStorage[i], i % 256);
    }

    {
      xiiRawMemoryStreamWriter writer2(OrigStorage);
      XII_TEST_INT(writer2.GetNumWrittenBytes(), 0);
      XII_TEST_INT(writer2.GetStorageSize(), 1000);
    }
  }
}

XII_CREATE_SIMPLE_TEST(IO, LargeMemoryStream)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Large Memory Stream Reading / Writing")
  {
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiMemoryStreamReader         reader(&storage);

    const xiiUInt8 pattern[] = {11, 10, 27, 4, 14, 3, 21, 6};

    xiiUInt64           uiSize      = 0;
    constexpr xiiUInt64 bytesToTest = 0x8000000llu; // tested with up to 8 GB, but that just takes too long

    // writes n gigabyte
    for (xiiUInt32 n = 0; n < 8; ++n)
    {
      // writes one gigabyte
      for (xiiUInt32 gb = 0; gb < 1024; ++gb)
      {
        // writes one megabyte
        for (xiiUInt32 mb = 0; mb < 1024 * 1024 / XII_ARRAY_SIZE(pattern); ++mb)
        {
          writer.WriteBytes(pattern, XII_ARRAY_SIZE(pattern)).IgnoreResult();
          uiSize += XII_ARRAY_SIZE(pattern);

          if (uiSize == bytesToTest)
            goto check;
        }
      }
    }

  check:
    XII_TEST_BOOL(uiSize == bytesToTest);
    XII_TEST_BOOL(writer.GetWritePosition() == bytesToTest);
    uiSize = 0;

    // reads n gigabyte
    for (xiiUInt32 n = 0; n < 8; ++n)
    {
      // reads one gigabyte
      for (xiiUInt32 gb = 0; gb < 1024; ++gb)
      {
        // reads one megabyte
        for (xiiUInt32 mb = 0; mb < 1024 * 1024 / XII_ARRAY_SIZE(pattern); ++mb)
        {
          xiiUInt8 pattern2[XII_ARRAY_SIZE(pattern)];

          const xiiUInt64 uiRead = reader.ReadBytes(pattern2, XII_ARRAY_SIZE(pattern));

          if (uiRead != XII_ARRAY_SIZE(pattern))
          {
            XII_TEST_BOOL(uiRead == 0);
            XII_TEST_BOOL(uiSize == bytesToTest);
            goto endTest;
          }

          uiSize += uiRead;

          if (xiiMemoryUtils::RawByteCompare(pattern, pattern2, XII_ARRAY_SIZE(pattern)) != 0)
          {
            XII_TEST_BOOL_MSG(false, "Memory read comparison failed.");
            goto endTest;
          }
        }
      }
    }

  endTest:;
    XII_TEST_BOOL(reader.GetReadPosition() == bytesToTest);
  }
}
