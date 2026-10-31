/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if XII_ENABLED(XII_SUPPORTS_LONG_PATHS)
#  define LongPath                                                                                                                                   \
    "AVeryLongSubFolderPathNameThatShouldExceedThePathLengthLimitOnPlatformsLikeWindowsWhereOnly260CharactersAreAllowedOhNoesIStillNeedMoreThisIsNo" \
    "tLongEnoughAaaaaaaaaaaaaaahhhhStillTooShortAaaaaaaaaaaaaaaaaaaaaahImBoredNow"
#else
#  define LongPath "AShortPathBecaueThisPlatformDoesntSupportLongOnes"
#endif

XII_CREATE_SIMPLE_TEST(IO, AssetIdentifiers)
{
  const xiiStringBuilder first(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/AssetIdentifiers/First");
  const xiiStringBuilder second(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/AssetIdentifiers/Second");
  const xiiString        previousTable               = xiiDataDirectory::FolderType::s_sRedirectionFile;
  const xiiString        previousPrefix              = xiiDataDirectory::FolderType::s_sRedirectionPrefix;
  xiiDataDirectory::FolderType::s_sRedirectionFile   = "Lookup.xiiAidlt";
  xiiDataDirectory::FolderType::s_sRedirectionPrefix = "AssetCache/";
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("AssetIdentifiers"); xiiDataDirectory::FolderType::s_sRedirectionFile = previousTable; xiiDataDirectory::FolderType::s_sRedirectionPrefix = previousPrefix;);
  auto write = [](xiiStringView path, xiiStringView text) {
    xiiOSFile file;
    XII_TEST_RESULT(file.Open(path, xiiFileOpenMode::Write));
    XII_TEST_RESULT(file.Write(text.GetStartPointer(), text.GetElementCount()));
  };
  const char*      firstGuid  = "{ 12345678-abcd-4321-1234-123456789abc }";
  const char*      secondGuid = "{ abcdef00-1234-abcd-ABCD-0123456789ab }";
  xiiStringBuilder canonicalFirst, canonicalSecond;
  xiiConversionUtils::ToString(xiiConversionUtils::ConvertStringToUuid(firstGuid), canonicalFirst);
  xiiConversionUtils::ToString(xiiConversionUtils::ConvertStringToUuid(secondGuid), canonicalSecond);
  // Exercise CRLF and a final record without a trailing newline.
  write(xiiStringBuilder(first, "/Lookup.xiiAidlt"), xiiStringBuilder(firstGuid, ";Common/Asset.bin\r\nCommon/Asset.xiiAsset;", firstGuid));
  write(xiiStringBuilder(first, "/AssetCache/Common/Asset.bin"), "compiled");
  write(xiiStringBuilder(first, "/Common/Asset.xiiAsset"), "authored");
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(first, "AssetIdentifiers", "AssetFirst"));
  xiiStringBuilder resolved;
  const xiiString  forms[] = {firstGuid, "Common/Asset.xiiAsset", "Common/./Other/../Asset.xiiAsset", "First/Common/Asset.xiiAsset", ":AssetFirst/Common/Asset.xiiAsset", xiiStringBuilder(first, "/Common/Asset.xiiAsset")};
  for (const auto& reference : forms)
  {
    XII_TEST_STATUS(xiiFileSystem::ResolveAssetIdentifier(reference, resolved));
    XII_TEST_STRING(resolved, canonicalFirst);
  }
  resolved = "Common/Asset.xiiAsset";
  XII_TEST_STATUS(xiiFileSystem::ResolveAssetIdentifier(resolved, resolved));
  XII_TEST_STRING(resolved, canonicalFirst);
  XII_TEST_STATUS(xiiFileSystem::ResolveAssetIdentifier("Native/GeneratedResource", resolved));
  XII_TEST_STRING(resolved, "Native/GeneratedResource");
  XII_TEST_STATUS(xiiFileSystem::ResolveAssetIdentifier("AssetCache/Common/Asset.bin", resolved));
  XII_TEST_STRING(resolved, "AssetCache/Common/Asset.bin");
  {
    xiiFileReader source, resource;
    XII_TEST_RESULT(source.Open(":AssetFirst/Common/Asset.xiiAsset"));
    XII_TEST_RESULT(resource.Open(canonicalFirst));
    char sourceBytes[8] = {}, resourceBytes[8] = {};
    XII_TEST_INT(source.ReadBytes(sourceBytes, 8), 8);
    XII_TEST_INT(resource.ReadBytes(resourceBytes, 8), 8);
    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(sourceBytes, "authored", 8));
    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(resourceBytes, "compiled", 8));
  }
  write(xiiStringBuilder(second, "/Lookup.xiiAidlt"), xiiStringBuilder(secondGuid, ";Other.bin\nCommon/Asset.xiiAsset;", secondGuid, "\n"));
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(second, "AssetIdentifiers", "AssetSecond"));
  const auto ambiguous = xiiFileSystem::ResolveAssetIdentifier("Common/Asset.xiiAsset", resolved);
  XII_TEST_BOOL(ambiguous.Failed());
  XII_TEST_BOOL(ambiguous.GetMessageString().FindSubString("Ambiguous") != nullptr);
  XII_TEST_BOOL(resolved.IsEmpty());
  XII_TEST_STATUS(xiiFileSystem::ResolveAssetIdentifier(":AssetSecond/Common/Asset.xiiAsset", resolved));
  XII_TEST_STRING(resolved, canonicalSecond);
  XII_TEST_BOOL(xiiFileSystem::ResolveAssetIdentifier(":AssetFirst/../AssetSecond/Common/Asset.xiiAsset", resolved).Failed());
  XII_TEST_BOOL(resolved.IsEmpty());
  XII_TEST_STATUS(xiiFileSystem::ResolveAssetIdentifier(xiiStringBuilder(first, "/Common/Asset.xiiAsset"), resolved));
  XII_TEST_STRING(resolved, canonicalFirst);
  write(xiiStringBuilder(first, "/Lookup.xiiAidlt"), xiiStringBuilder(firstGuid, ";Relocated.bin\nMoved/Asset.xiiAsset;", firstGuid));
  xiiFileSystem::ReloadAllExternalDataDirectoryConfigs();
  XII_TEST_STATUS(xiiFileSystem::ResolveAssetIdentifier(":AssetFirst/Moved/Asset.xiiAsset", resolved));
  XII_TEST_STRING(resolved, canonicalFirst);
  XII_TEST_STATUS(xiiFileSystem::ResolveAssetIdentifier(":AssetFirst/Common/Asset.xiiAsset", resolved));
  XII_TEST_STRING(resolved, ":AssetFirst/Common/Asset.xiiAsset");
  XII_TEST_BOOL(xiiFileSystem::ResolveAssetRedirection(canonicalFirst, resolved));
  XII_TEST_STRING(resolved, "AssetCache/Relocated.bin");
}

XII_CREATE_SIMPLE_TEST(IO, FileSystem)
{
  xiiStringBuilder sFileContent = "Lyrics to Taste The Cake:\n\
Turret: Who's there?\n\
Turret: Is anyone there?\n\
Turret: I see you.\n\
\n\
Chell rises from a stasis inside of a glass box\n\
She isn't greeted by faces,\n\
Only concrete and clocks.\n\
...";

  xiiStringBuilder szOutputFolder = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  szOutputFolder.MakeCleanPath();

  xiiStringBuilder sOutputFolderResolved;
  xiiFileSystem::ResolveSpecialDirectory(szOutputFolder, sOutputFolderResolved).IgnoreResult();

  xiiStringBuilder sOutputFolder1 = szOutputFolder;
  sOutputFolder1.AppendPath("IO", "SubFolder");
  xiiStringBuilder sOutputFolder1Resolved;
  xiiFileSystem::ResolveSpecialDirectory(sOutputFolder1, sOutputFolder1Resolved).IgnoreResult();

  xiiStringBuilder sOutputFolder2 = szOutputFolder;
  sOutputFolder2.AppendPath("IO", "SubFolder2");
  xiiStringBuilder sOutputFolder2Resolved;
  xiiFileSystem::ResolveSpecialDirectory(sOutputFolder2, sOutputFolder2Resolved).IgnoreResult();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Setup Data Dirs")
  {
    // adding the same factory three times would actually not make a difference
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);

    // xiiFileSystem::ClearAllDataDirectoryFactories();

    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);

    // for absolute paths
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory("", "", ":", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(szOutputFolder, "Clear", "output", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);

    xiiStringBuilder sTempFile = sOutputFolder1Resolved;
    sTempFile.AppendPath(LongPath);
    sTempFile.AppendPath("Temp.tmp");

    xiiFileWriter TempFile;
    XII_TEST_BOOL(TempFile.Open(sTempFile) == XII_SUCCESS);
    TempFile.Close();

    sTempFile = sOutputFolder2Resolved;
    sTempFile.AppendPath("Temp.tmp");

    XII_TEST_BOOL(TempFile.Open(sTempFile) == XII_SUCCESS);
    TempFile.Close();

    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder1, "Clear", "output1", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder2, "Clear") == XII_SUCCESS);

    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder1, "Remove") == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder2, "Remove") == XII_SUCCESS);

    XII_TEST_INT(xiiFileSystem::RemoveDataDirectoryGroup("Remove"), 3);

    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder1, "Remove") == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder2, "Remove") == XII_SUCCESS);

    xiiFileSystem::ClearAllDataDirectories();

    XII_TEST_INT(xiiFileSystem::RemoveDataDirectoryGroup("Remove"), 0);
    XII_TEST_INT(xiiFileSystem::RemoveDataDirectoryGroup("Clear"), 0);

    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder1, "", "output1", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder2) == XII_SUCCESS);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Add / Remove Data Dirs")
  {
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory("", "xyz-rooted", "xyz", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);

    XII_TEST_BOOL(xiiFileSystem::FindDataDirectoryWithRoot("xyz") != nullptr);

    XII_TEST_BOOL(xiiFileSystem::RemoveDataDirectory("xyz") == true);

    XII_TEST_BOOL(xiiFileSystem::FindDataDirectoryWithRoot("xyz") == nullptr);

    XII_TEST_BOOL(xiiFileSystem::RemoveDataDirectory("xyz") == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Write File")
  {
    xiiFileWriter FileOut;

    xiiStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    XII_TEST_BOOL(FileOut.Open(":output1/FileSystemTest.txt") == XII_SUCCESS);

    XII_TEST_STRING(FileOut.GetFilePathRelative(), "FileSystemTest.txt");
    XII_TEST_STRING(FileOut.GetFilePathAbsolute(), sAbs);

    XII_TEST_INT(FileOut.GetFileSize(), 0);

    XII_TEST_BOOL(FileOut.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()) == XII_SUCCESS);

    FileOut.Flush().IgnoreResult();
    XII_TEST_INT(FileOut.GetFileSize(), sFileContent.GetElementCount());

    FileOut.Close();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Read File")
  {
    xiiFileReader FileIn;

    xiiStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    XII_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == XII_SUCCESS);

    XII_TEST_STRING(FileIn.GetFilePathRelative(), "FileSystemTest.txt");
    XII_TEST_STRING(FileIn.GetFilePathAbsolute(), sAbs);

    XII_TEST_INT(FileIn.GetFileSize(), sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    XII_TEST_INT(FileIn.ReadBytes(szTemp, 1024 * 2), sFileContent.GetElementCount());

    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Read File (Absolute Path)")
  {
    xiiFileReader FileIn;

    xiiStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    XII_TEST_BOOL(FileIn.Open(sAbs) == XII_SUCCESS);

    XII_TEST_STRING(FileIn.GetFilePathRelative(), "FileSystemTest.txt");
    XII_TEST_STRING(FileIn.GetFilePathAbsolute(), sAbs);

    XII_TEST_INT(FileIn.GetFileSize(), sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    XII_TEST_INT(FileIn.ReadBytes(szTemp, 1024 * 2), sFileContent.GetElementCount());

    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Delete File / Exists File")
  {
    {
      XII_TEST_BOOL(xiiFileSystem::ExistsFile(":output1/FileSystemTest.txt"));
      xiiFileSystem::DeleteFile(":output1/FileSystemTest.txt");
      XII_TEST_BOOL(!xiiFileSystem::ExistsFile("FileSystemTest.txt"));

      xiiFileReader FileIn;
      XII_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == XII_FAILURE);
    }

    // very long path names
    {
      xiiStringBuilder sTempFile = ":output1";
      sTempFile.AppendPath(LongPath);
      sTempFile.AppendPath("Temp.tmp");

      xiiFileWriter TempFile;
      XII_TEST_BOOL(TempFile.Open(sTempFile) == XII_SUCCESS);
      TempFile.Close();

      XII_TEST_BOOL(xiiFileSystem::ExistsFile(sTempFile));
      xiiFileSystem::DeleteFile(sTempFile);
      XII_TEST_BOOL(!xiiFileSystem::ExistsFile(sTempFile));

      xiiFileReader FileIn;
      XII_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == XII_FAILURE);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileStats")
  {
    const char* szPath = ":output1/" LongPath "/FileSystemTest.txt";

    // Create file
    {
      xiiFileWriter    FileOut;
      xiiStringBuilder sAbs = sOutputFolder1Resolved;
      sAbs.AppendPath("FileSystemTest.txt");
      XII_TEST_BOOL(FileOut.Open(szPath) == XII_SUCCESS);
      FileOut.WriteBytes("Test", 4).IgnoreResult();
    }

    xiiFileStats stat;

    XII_TEST_BOOL(xiiFileSystem::GetFileStats(szPath, stat).Succeeded());

    XII_TEST_BOOL(!stat.m_bIsDirectory);
    XII_TEST_STRING(stat.m_sName, "FileSystemTest.txt");
    XII_TEST_INT(stat.m_uiFileSize, 4);

    xiiFileSystem::DeleteFile(szPath);
    XII_TEST_BOOL(xiiFileSystem::GetFileStats(szPath, stat).Failed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ResolvePath")
  {
    xiiStringBuilder sRel, sAbs;

    XII_TEST_BOOL(xiiFileSystem::ResolvePath(":output1/FileSystemTest2.txt", &sAbs, &sRel) == XII_SUCCESS);

    xiiStringBuilder sExpectedAbs = sOutputFolder1Resolved;
    sExpectedAbs.AppendPath("FileSystemTest2.txt");

    XII_TEST_STRING(sAbs, sExpectedAbs);
    XII_TEST_STRING(sRel, "FileSystemTest2.txt");

    // create a file in the second dir
    {
      XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);

      {
        xiiFileWriter FileOut;
        XII_TEST_BOOL(FileOut.Open(":output2/FileSystemTest2.txt") == XII_SUCCESS);
      }

      XII_TEST_INT(xiiFileSystem::RemoveDataDirectoryGroup("Remove"), 1);
    }

    // find the path to an existing file
    {
      XII_TEST_BOOL(xiiFileSystem::ResolvePath("FileSystemTest2.txt", &sAbs, &sRel) == XII_SUCCESS);

      sExpectedAbs = sOutputFolder2Resolved;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      XII_TEST_STRING(sAbs, sExpectedAbs);
      XII_TEST_STRING(sRel, "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      XII_TEST_BOOL(xiiFileSystem::ResolvePath(":output1/FileSystemTest2.txt", &sAbs, &sRel) == XII_SUCCESS);

      sExpectedAbs = sOutputFolder1Resolved;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      XII_TEST_STRING(sAbs, sExpectedAbs);
      XII_TEST_STRING(sRel, "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      XII_TEST_BOOL(xiiFileSystem::ResolvePath(":output1/SubSub/FileSystemTest2.txt", &sAbs, &sRel) == XII_SUCCESS);

      sExpectedAbs = sOutputFolder1Resolved;
      sExpectedAbs.AppendPath("SubSub/FileSystemTest2.txt");

      XII_TEST_STRING(sAbs, sExpectedAbs);
      XII_TEST_STRING(sRel, "SubSub/FileSystemTest2.txt");
    }

    xiiFileSystem::DeleteFile(":output1/FileSystemTest2.txt");
    xiiFileSystem::DeleteFile(":output2/FileSystemTest2.txt");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindFolderWithSubPath")
  {
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(szOutputFolder, "remove", "toplevel", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder2, "remove", "output2", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);

    xiiStringBuilder StartPath;
    xiiStringBuilder SubPath;
    xiiStringBuilder result, expected;

    // make sure this exists
    {
      xiiFileWriter FileOut;
      XII_TEST_BOOL(FileOut.Open(":output2/FileSystemTest2.txt") == XII_SUCCESS);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("DoesNotExist");

      XII_TEST_BOOL(xiiFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Failed());
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("SubFolder2");
      expected.Set(sOutputFolderResolved, "/IO/");

      XII_TEST_BOOL(xiiFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      XII_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub");
      SubPath.Set("IO/SubFolder2");
      expected.Set(sOutputFolderResolved, "/");

      XII_TEST_BOOL(xiiFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      XII_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub");
      SubPath.Set("IO/SubFolder2");
      expected.Set(sOutputFolderResolved, "/");

      XII_TEST_BOOL(xiiFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      XII_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("SubFolder2/FileSystemTest2.txt");
      expected.Set(sOutputFolderResolved, "/IO/");

      XII_TEST_BOOL(xiiFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      XII_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(":toplevel/IO/SubFolder");
      SubPath.Set("IO/SubFolder2");
      expected.Set(":toplevel/");

      XII_TEST_BOOL(xiiFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      XII_TEST_STRING(result, expected);
    }

    {
      // Project executables can live on a different drive from the SDK.
      // Absolute redirections must work without appending a drive-qualified path.
      const xiiStringBuilder redirection(sOutputFolder1Resolved, "/xiiSdkRoot.txt");
      for (xiiStringView target : {sOutputFolder2Resolved.GetView(), "../SubFolder2"_xiisv})
      {
        {
          xiiOSFile writer;
          if (!XII_TEST_RESULT(writer.Open(redirection, xiiFileOpenMode::Write)))
            return;
          XII_TEST_RESULT(writer.Write(target.GetStartPointer(), target.GetElementCount()));
        }
        XII_TEST_RESULT(xiiFileSystem::FindFolderWithSubPath(result, sOutputFolder1Resolved, "FileSystemTest2.txt", "xiiSdkRoot.txt"));
        XII_TEST_STRING(result, sOutputFolder2Resolved);
      }
      XII_TEST_RESULT(xiiOSFile::DeleteFile(redirection));
    }

    xiiFileSystem::DeleteFile(":output1/FileSystemTest2.txt");
    xiiFileSystem::DeleteFile(":output2/FileSystemTest2.txt");

    xiiFileSystem::RemoveDataDirectoryGroup("remove");
  }
}
