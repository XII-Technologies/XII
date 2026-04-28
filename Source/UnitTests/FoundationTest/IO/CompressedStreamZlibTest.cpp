/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/CompressedStreamZlib.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

XII_CREATE_SIMPLE_TEST(IO, CompressedStreamZlib)
{
  xiiDynamicArray<xiiUInt32> TestData;

  // create the test data
  // a repetition of a counting sequence that is getting longer and longer, ie:
  // 0, 0,1, 0,1,2, 0,1,2,3, 0,1,2,3,4, ...
  {
    TestData.SetCountUninitialized(1024 * 1024 * 8);

    const xiiUInt32 uiItems    = TestData.GetCount();
    xiiUInt32       uiStartPos = 0;

    for (xiiUInt32 uiWrite = 1; uiWrite < uiItems; ++uiWrite)
    {
      uiWrite = xiiMath::Min(uiWrite, uiItems - uiStartPos);

      if (uiWrite == 0)
        break;

      for (xiiUInt32 i = 0; i < uiWrite; ++i)
      {
        TestData[uiStartPos + i] = i;
      }

      uiStartPos += uiWrite;
    }
  }


  xiiDefaultMemoryStreamStorage StreamStorage;

  xiiMemoryStreamWriter MemoryWriter(&StreamStorage);
  xiiMemoryStreamReader MemoryReader(&StreamStorage);

  xiiCompressedStreamReaderZlib CompressedReader(&MemoryReader);
  xiiCompressedStreamWriterZlib CompressedWriter(&MemoryWriter);

  const float fExpectedCompressionRatio = 25.0f; // this is a guess that is based on the current input data and size

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compress Data")
  {
    xiiUInt32 uiWrite = 1;
    for (xiiUInt32 i = 0; i < TestData.GetCount();)
    {
      uiWrite = xiiMath::Min<xiiUInt32>(uiWrite, TestData.GetCount() - i);

      XII_TEST_BOOL(CompressedWriter.WriteBytes(&TestData[i], sizeof(xiiUInt32) * uiWrite) == XII_SUCCESS);

      i += uiWrite;
      uiWrite += 17; // try different sizes to write
    }

    // flush all data
    CompressedWriter.CloseStream().IgnoreResult();

    const xiiUInt64 uiCompressed   = CompressedWriter.GetCompressedSize();
    const xiiUInt64 uiUncompressed = CompressedWriter.GetUncompressedSize();

    XII_TEST_INT(uiUncompressed, TestData.GetCount() * sizeof(xiiUInt32));

    XII_TEST_BOOL((float)uiCompressed <= (float)uiUncompressed / fExpectedCompressionRatio);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Uncompress Data")
  {
    bool      bSkip      = false;
    xiiUInt32 uiStartPos = 0;

    xiiDynamicArray<xiiUInt32> TestDataRead = TestData; // initialize with identical data, makes comparing the skipped parts easier

    // read the data in blocks that get larger and larger
    for (xiiUInt32 iRead = 1; iRead < TestData.GetCount(); ++iRead)
    {
      xiiUInt32 iToRead = xiiMath::Min(iRead, TestData.GetCount() - uiStartPos);

      if (iToRead == 0)
        break;

      if (bSkip)
      {
        const xiiUInt64 uiReadFromStream = CompressedReader.SkipBytes(sizeof(xiiUInt32) * iToRead);
        XII_TEST_BOOL(uiReadFromStream == sizeof(xiiUInt32) * iToRead);
      }
      else
      {
        // overwrite part we are going to read from the stream, to make sure it re-reads the correct data
        for (xiiUInt32 i = 0; i < iToRead; ++i)
        {
          TestDataRead[uiStartPos + i] = 0;
        }

        const xiiUInt64 uiReadFromStream = CompressedReader.ReadBytes(&TestDataRead[uiStartPos], sizeof(xiiUInt32) * iToRead);
        XII_TEST_BOOL(uiReadFromStream == sizeof(xiiUInt32) * iToRead);
      }

      bSkip = !bSkip;

      uiStartPos += iToRead;
    }

    XII_TEST_BOOL(TestData == TestDataRead);

    // test reading after the end of the stream
    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      xiiUInt32 uiTemp = 0;
      XII_TEST_BOOL(CompressedReader.ReadBytes(&uiTemp, sizeof(xiiUInt32)) == 0);
    }
  }
}

#endif
