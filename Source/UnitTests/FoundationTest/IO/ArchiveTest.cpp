#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#if (XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) && XII_ENABLED(XII_SUPPORTS_FILE_STATS) && defined(BUILDSYSTEM_HAS_ARCHIVE_TOOL))

XII_CREATE_SIMPLE_TEST(IO, Archive)
{
  xiiStringBuilder sOutputFolder = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFolder.AppendPath("ArchiveTest");
  sOutputFolder.MakeCleanPath();

  // make sure it is empty
  xiiOSFile::DeleteFolder(sOutputFolder).IgnoreResult();
  xiiOSFile::CreateDirectoryStructure(sOutputFolder).IgnoreResult();

  if (!XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder, "Clear", "output", xiiFileSystem::AllowWrites).Succeeded()))
    return;

  const char* szTestData     = "TestData";
  const char* szUnpackedData = "Unpacked";

  // write a couple of files for packaging
  const char* szFileList[] = {
    "File1.txt",
    "FolderA/File2.jpg", // should get stored uncompressed
    "FolderB/File3.txt",
    "FolderA/FolderC/File4.zip", // should get stored uncompressed
    "FolderA/FolderD/File5.txt",
    "File6.txt",
  };

  const xiiUInt32 uiMinFileSize = 1024 * 128;


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Generate Data")
  {
    xiiUInt64 uiValue = 0;

    xiiStringBuilder fileName;

    for (xiiUInt32 uiFileIdx = 0; uiFileIdx < XII_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      fileName.Set(":output/", szTestData, "/", szFileList[uiFileIdx]);

      xiiFileWriter file;
      if (!XII_TEST_BOOL(file.Open(fileName).Succeeded()))
        return;

      for (xiiUInt32 i = 0; i < uiMinFileSize * uiFileIdx; ++i)
      {
        file << uiValue;
        ++uiValue;
      }
    }
  }

  const xiiStringBuilder sArchiveFolder(sOutputFolder, "/", szTestData);
  const xiiStringBuilder sUnpackFolder(sOutputFolder, "/", szUnpackedData);
  const xiiStringBuilder sArchiveFile(sOutputFolder, "/", szTestData, ".xiiArchive");

  xiiStringBuilder pathToArchiveTool = xiiCommandLineUtils::GetGlobalInstance()->GetParameter(0);
  pathToArchiveTool.PathParentDirectory();
  pathToArchiveTool.AppendPath("ArchiveTool.exe");

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create a Package")
  {

    xiiProcessOptions opt;
    opt.m_sProcess = pathToArchiveTool;
    opt.m_Arguments.PushBack(sArchiveFolder);

    xiiInt32 iReturnValue = 1;

    xiiProcess ArchiveToolProc;
    if (!XII_TEST_BOOL(ArchiveToolProc.Execute(opt, &iReturnValue).Succeeded()))
      return;

    XII_TEST_INT(iReturnValue, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Unpack the Package")
  {

    xiiProcessOptions opt;
    opt.m_sProcess = pathToArchiveTool;
    opt.m_Arguments.PushBack("-unpack");
    opt.m_Arguments.PushBack(sArchiveFile);
    opt.m_Arguments.PushBack("-out");
    opt.m_Arguments.PushBack(sUnpackFolder);

    xiiInt32 iReturnValue = 1;

    xiiProcess ArchiveToolProc;
    if (!XII_TEST_BOOL(ArchiveToolProc.Execute(opt, &iReturnValue).Succeeded()))
      return;

    XII_TEST_INT(iReturnValue, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compare unpacked data")
  {
    xiiUInt64 uiValue = 0;

    xiiStringBuilder sFileSrc;
    xiiStringBuilder sFileDst;

    for (xiiUInt32 uiFileIdx = 0; uiFileIdx < XII_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileSrc.Set(sOutputFolder, "/", szTestData, "/", szFileList[uiFileIdx]);
      sFileDst.Set(sOutputFolder, "/", szUnpackedData, "/", szFileList[uiFileIdx]);

      XII_TEST_FILES(sFileSrc, sFileDst, "Unpacked file should be identical");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Mount as Data Dir")
  {
    if (!XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sArchiveFile, "Clear", "archive", xiiFileSystem::ReadOnly) == XII_SUCCESS))
      return;

    xiiStringBuilder sFileSrc;
    xiiStringBuilder sFileDst;

    // test opening multiple files in parallel and keeping them open
    xiiFileReader readers[XII_ARRAY_SIZE(szFileList)];
    for (xiiUInt32 uiFileIdx = 0; uiFileIdx < XII_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileDst.Set(":archive/", szFileList[uiFileIdx]);
      XII_TEST_BOOL(readers[uiFileIdx].Open(sFileDst).Succeeded());

      // advance the reader a bit
      XII_TEST_INT(readers[uiFileIdx].SkipBytes(uiMinFileSize * uiFileIdx), uiMinFileSize * uiFileIdx);
    }

    for (xiiUInt32 uiFileIdx = 0; uiFileIdx < XII_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileSrc.Set(":output/", szTestData, "/", szFileList[uiFileIdx]);
      sFileDst.Set(":archive/", szFileList[uiFileIdx]);

      XII_TEST_FILES(sFileSrc, sFileDst, "Unpacked file should be identical");
    }

    // mount a second time
    if (!XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sArchiveFile, "Clear", "archive2", xiiFileSystem::ReadOnly) == XII_SUCCESS))
      return;
  }

  xiiFileSystem::RemoveDataDirectoryGroup("Clear");
}

#endif
