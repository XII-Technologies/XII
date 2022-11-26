#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

XII_CREATE_SIMPLE_TEST(IO, DeferredFileWriter)
{
  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory("", "", ":", xiiFileSystem::AllowWrites) == XII_SUCCESS);

  const xiiStringBuilder szOutputFolder = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  xiiStringBuilder       sOutputFolderResolved;
  xiiFileSystem::ResolveSpecialDirectory(szOutputFolder, sOutputFolderResolved).IgnoreResult();

  xiiStringBuilder sTempFile = sOutputFolderResolved;
  sTempFile.AppendPath("Temp.tmp");

  // make sure the file does not exist
  xiiFileSystem::DeleteFile(sTempFile);
  XII_TEST_BOOL(!xiiFileSystem::ExistsFile(sTempFile));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DeferredFileWriter")
  {
    xiiDeferredFileWriter writer;
    writer.SetOutput(sTempFile);

    for (xiiUInt64 i = 0; i < 1'000'000; ++i)
    {
      writer << i;
    }

    // does not exist yet
    XII_TEST_BOOL(!xiiFileSystem::ExistsFile(sTempFile));
  }

  // now it exists
  XII_TEST_BOOL(xiiFileSystem::ExistsFile(sTempFile));

  // check content is correct
  {
    xiiFileReader reader;
    XII_TEST_BOOL(reader.Open(sTempFile).Succeeded());

    for (xiiUInt64 i = 0; i < 1'000'000; ++i)
    {
      xiiUInt64 v;
      reader >> v;
      XII_TEST_BOOL(v == i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DeferredFileWriter2")
  {
    xiiDeferredFileWriter writer;
    writer.SetOutput(sTempFile);

    for (xiiUInt64 i = 1; i < 100'000; ++i)
    {
      writer << i;
    }

    // does exist from earlier
    XII_TEST_BOOL(xiiFileSystem::ExistsFile(sTempFile));

    // check content is as previous correct
    {
      xiiFileReader reader;
      XII_TEST_BOOL(reader.Open(sTempFile).Succeeded());

      for (xiiUInt64 i = 0; i < 1'000'000; ++i)
      {
        xiiUInt64 v;
        reader >> v;
        XII_TEST_BOOL(v == i);
      }
    }
  }

  // exist but now was overwritten
  XII_TEST_BOOL(xiiFileSystem::ExistsFile(sTempFile));

  // check content is as previous correct
  {
    xiiFileReader reader;
    XII_TEST_BOOL(reader.Open(sTempFile).Succeeded());

    for (xiiUInt64 i = 1; i < 100'000; ++i)
    {
      xiiUInt64 v;
      reader >> v;
      XII_TEST_BOOL(v == i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Discard")
  {
    xiiStringBuilder sTempFile2 = sOutputFolderResolved;
    sTempFile2.AppendPath("Temp2.tmp");
    {
      xiiDeferredFileWriter writer;
      writer.SetOutput(sTempFile2);
      writer << 10;
      writer.Discard();
    }
    XII_TEST_BOOL(!xiiFileSystem::ExistsFile(sTempFile2));
  }

  xiiFileSystem::DeleteFile(sTempFile);
  xiiFileSystem::ClearAllDataDirectories();
}
