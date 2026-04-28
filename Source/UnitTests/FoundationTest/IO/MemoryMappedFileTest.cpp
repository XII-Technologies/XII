/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/OSFile.h>

#if XII_ENABLED(XII_SUPPORTS_MEMORY_MAPPED_FILE)

XII_CREATE_SIMPLE_TEST(IO, MemoryMappedFile)
{
  xiiStringBuilder sOutputFile = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile.MakeCleanPath();
  sOutputFile.AppendPath("IO");
  sOutputFile.AppendPath("MemoryMappedFile.dat");

  const xiiUInt32 uiFileSize = 1024 * 1024 * 16; // * 4

  // generate test data
  {
    xiiOSFile file;
    if (!XII_TEST_BOOL_MSG(file.Open(sOutputFile, xiiFileOpenMode::Write).Succeeded(), "File for memory mapping could not be created"))
      return;

    xiiDynamicArray<xiiUInt32> data;
    data.SetCountUninitialized(uiFileSize);

    for (xiiUInt32 i = 0; i < uiFileSize; ++i)
    {
      data[i] = i;
    }

    file.Write(data.GetData(), data.GetCount() * sizeof(xiiUInt32)).IgnoreResult();
    file.Close();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Memory map for writing")
  {
    xiiMemoryMappedFile memFile;

    if (!XII_TEST_BOOL_MSG(memFile.Open(sOutputFile, xiiMemoryMappedFile::Mode::ReadWrite).Succeeded(), "Memory mapping a file failed"))
      return;

    XII_TEST_BOOL(memFile.GetWritePointer() != nullptr);
    XII_TEST_INT(memFile.GetFileSize(), uiFileSize * sizeof(xiiUInt32));

    xiiUInt32* ptr = static_cast<xiiUInt32*>(memFile.GetWritePointer());

    for (xiiUInt32 i = 0; i < uiFileSize; ++i)
    {
      XII_TEST_INT(ptr[i], i);
      ptr[i] = ptr[i] + 1;
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Memory map for reading")
  {
    xiiMemoryMappedFile memFile;

    if (!XII_TEST_BOOL_MSG(memFile.Open(sOutputFile, xiiMemoryMappedFile::Mode::ReadOnly).Succeeded(), "Memory mapping a file failed"))
      return;

    XII_TEST_BOOL(memFile.GetReadPointer() != nullptr);
    XII_TEST_INT(memFile.GetFileSize(), uiFileSize * sizeof(xiiUInt32));

    const xiiUInt32* ptr = static_cast<const xiiUInt32*>(memFile.GetReadPointer());

    for (xiiUInt32 i = 0; i < uiFileSize; ++i)
    {
      XII_TEST_INT(ptr[i], i + 1);
    }

    // try to map it a second time
    xiiMemoryMappedFile memFile2;

    if (!XII_TEST_BOOL_MSG(memFile2.Open(sOutputFile, xiiMemoryMappedFile::Mode::ReadOnly).Succeeded(), "Memory mapping a file twice failed"))
      return;
  }

  xiiOSFile::DeleteFile(sOutputFile).IgnoreResult();
}
#endif
