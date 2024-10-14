#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/OSFile.h>

XII_CREATE_SIMPLE_TEST(IO, OSFile)
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

  const xiiUInt32 uiTextLen = sFileContent.GetElementCount();

  xiiStringBuilder sOutputFile = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile.MakeCleanPath();
  sOutputFile.AppendPath("IO", "SubFolder");
  sOutputFile.AppendPath("OSFile_TestFile.txt");

  xiiStringBuilder sOutputFile2 = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile2.MakeCleanPath();
  sOutputFile2.AppendPath("IO", "SubFolder2");
  sOutputFile2.AppendPath("OSFile_TestFileCopy.txt");

  xiiStringBuilder sOutputFile3 = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile3.MakeCleanPath();
  sOutputFile3.AppendPath("IO", "SubFolder2", "SubSubFolder");
  sOutputFile3.AppendPath("RandomFile.txt");

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Write File")
  {
    xiiOSFile f;
    XII_TEST_BOOL(f.Open(sOutputFile.GetData(), xiiFileOpenMode::Write) == XII_SUCCESS);
    XII_TEST_BOOL(f.IsOpen());
    XII_TEST_INT(f.GetFilePosition(), 0);
    XII_TEST_INT(f.GetFileSize(), 0);

    for (xiiUInt32 i = 0; i < uiTextLen; ++i)
    {
      XII_TEST_BOOL(f.Write(&sFileContent.GetData()[i], 1) == XII_SUCCESS);
      XII_TEST_INT(f.GetFilePosition(), i + 1);
      XII_TEST_INT(f.GetFileSize(), i + 1);
    }

    XII_TEST_INT(f.GetFilePosition(), uiTextLen);
    f.SetFilePosition(5, xiiFileSeekMode::FromStart);
    XII_TEST_INT(f.GetFileSize(), uiTextLen);

    XII_TEST_INT(f.GetFilePosition(), 5);
    // f.Close(); // The file should be closed automatically
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Append File")
  {
    xiiOSFile f;
    XII_TEST_BOOL(f.Open(sOutputFile.GetData(), xiiFileOpenMode::Append) == XII_SUCCESS);
    XII_TEST_BOOL(f.IsOpen());
    XII_TEST_INT(f.GetFilePosition(), uiTextLen);
    XII_TEST_BOOL(f.Write(sFileContent.GetData(), uiTextLen) == XII_SUCCESS);
    XII_TEST_INT(f.GetFilePosition(), uiTextLen * 2);
    f.Close();
    XII_TEST_BOOL(!f.IsOpen());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Read File")
  {
    const xiiUInt32 FS_MAX_PATH = 1024;
    char            szTemp[FS_MAX_PATH];

    xiiOSFile f;
    XII_TEST_BOOL(f.Open(sOutputFile.GetData(), xiiFileOpenMode::Read) == XII_SUCCESS);
    XII_TEST_BOOL(f.IsOpen());
    XII_TEST_INT(f.GetFilePosition(), 0);

    XII_TEST_INT(f.Read(szTemp, FS_MAX_PATH), uiTextLen * 2);
    XII_TEST_INT(f.GetFilePosition(), uiTextLen * 2);

    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), uiTextLen));
    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(&szTemp[uiTextLen], sFileContent.GetData(), uiTextLen));

    f.Close();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy File")
  {
    xiiOSFile::CopyFile(sOutputFile.GetData(), sOutputFile2.GetData()).IgnoreResult();

    xiiOSFile f;
    XII_TEST_BOOL(f.Open(sOutputFile2.GetData(), xiiFileOpenMode::Read) == XII_SUCCESS);

    const xiiUInt32 FS_MAX_PATH = 1024;
    char            szTemp[FS_MAX_PATH];

    XII_TEST_INT(f.Read(szTemp, FS_MAX_PATH), uiTextLen * 2);

    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), uiTextLen));
    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(&szTemp[uiTextLen], sFileContent.GetData(), uiTextLen));

    f.Close();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadAll")
  {
    xiiOSFile f;
    XII_TEST_BOOL(f.Open(sOutputFile, xiiFileOpenMode::Read) == XII_SUCCESS);

    xiiDynamicArray<xiiUInt8> fileContent;
    const xiiUInt64           bytes = f.ReadAll(fileContent);

    XII_TEST_INT(bytes, uiTextLen * 2);

    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(fileContent.GetData(), (const xiiUInt8*)sFileContent.GetData(), uiTextLen));

    f.Close();
  }

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "File Stats")
  {
    xiiFileStats s;

    xiiStringBuilder dir = sOutputFile2.GetFileDirectory();

    XII_TEST_BOOL(xiiOSFile::GetFileStats(sOutputFile2.GetData(), s) == XII_SUCCESS);
    // printf("%s Name: '%s' (%lli Bytes), Modified Time: %lli\n", s.m_bIsDirectory ? "Directory" : "File", s.m_sFileName.GetData(),
    // s.m_uiFileSize, s.m_LastModificationTime.GetInt64(xiiSIUnitOfTime::Microsecond));

    XII_TEST_BOOL(xiiOSFile::GetFileStats(dir.GetData(), s) == XII_SUCCESS);
    // printf("%s Name: '%s' (%lli Bytes), Modified Time: %lli\n", s.m_bIsDirectory ? "Directory" : "File", s.m_sFileName.GetData(),
    // s.m_uiFileSize, s.m_LastModificationTime.GetInt64(xiiSIUnitOfTime::Microsecond));
  }

#  if (XII_ENABLED(XII_SUPPORTS_CASE_INSENSITIVE_PATHS) && XII_ENABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS))
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileCasing")
  {
    xiiStringBuilder dir = sOutputFile2;
    dir.ToLower();

#    if XII_ENABLED(XII_PLATFORM_WINDOWS)
    // On Windows the drive letter will always be turned upper case by xiiOSFile::GetFileCasing()
    // ensure that our input data ('ground truth') also uses an upper case drive letter
    auto            driveLetterIterator = sOutputFile2.GetIteratorFront();
    const xiiUInt32 uiDriveLetter       = xiiStringUtils::ToUpperChar(driveLetterIterator.GetCharacter());
    sOutputFile2.ChangeCharacter(driveLetterIterator, uiDriveLetter);
#    endif

    xiiStringBuilder sCorrected;
    XII_TEST_BOOL(xiiOSFile::GetFileCasing(dir.GetData(), sCorrected) == XII_SUCCESS);

    // On Windows the drive letter will always be made to upper case
    XII_TEST_STRING(sCorrected.GetData(), sOutputFile2.GetData());
  }
#  endif // XII_SUPPORTS_CASE_INSENSITIVE_PATHS && XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif // XII_SUPPORTS_FILE_STATS

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "File Iterator")
  {
    // It is not really possible to test this stuff (with a guaranteed result), as long as we do not have
    // a test data folder with deterministic content
    // Therefore I tested it manually, and leave the code in, such that it is at least a 'does it compile and link' test.

    xiiStringBuilder sOutputFolder = xiiOSFile::GetApplicationDirectory();
    sOutputFolder.AppendPath("*");

    xiiStringBuilder sFullPath;

    xiiUInt32 uiFolders = 0;
    xiiUInt32 uiFiles   = 0;

    bool bSkipFolder = true;

    xiiFileSystemIterator it;
    for (it.StartSearch(sOutputFolder.GetData(), xiiFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
    {
      sFullPath = it.GetCurrentPath();
      sFullPath.AppendPath(it.GetStats().m_sName.GetData());

      it.GetStats();
      it.GetCurrentPath();

      if (it.GetStats().m_bIsDirectory)
      {
        ++uiFolders;
        bSkipFolder = !bSkipFolder;

        if (bSkipFolder)
        {
          it.SkipFolder(); // replaces the 'Next' call
          continue;
        }
      }
      else
      {
        ++uiFiles;
      }

      it.Next();
    }

// The binary folder will only have subdirectories on windows desktop
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
    XII_TEST_BOOL(uiFolders > 0);
#  endif
    XII_TEST_BOOL(uiFiles > 0);
  }

#endif

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Delete File")
  {
    XII_TEST_BOOL(xiiOSFile::DeleteFile(sOutputFile.GetData()) == XII_SUCCESS);
    XII_TEST_BOOL(xiiOSFile::DeleteFile(sOutputFile.GetData()) == XII_SUCCESS); // second time should still 'succeed'

    XII_TEST_BOOL(xiiOSFile::DeleteFile(sOutputFile2.GetData()) == XII_SUCCESS);
    XII_TEST_BOOL(xiiOSFile::DeleteFile(sOutputFile2.GetData()) == XII_SUCCESS); // second time should still 'succeed'

    xiiOSFile f;
    XII_TEST_BOOL(f.Open(sOutputFile.GetData(), xiiFileOpenMode::Read) == XII_FAILURE);  // file should not exist anymore
    XII_TEST_BOOL(f.Open(sOutputFile2.GetData(), xiiFileOpenMode::Read) == XII_FAILURE); // file should not exist anymore
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCurrentWorkingDirectory")
  {
    xiiStringBuilder cwd = xiiOSFile::GetCurrentWorkingDirectory();

    XII_TEST_BOOL(!cwd.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakePathAbsoluteWithCWD")
  {
    xiiStringBuilder cwd  = xiiOSFile::GetCurrentWorkingDirectory();
    xiiStringBuilder path = xiiOSFile::MakePathAbsoluteWithCWD("sub/folder");

    XII_TEST_BOOL(path.StartsWith(cwd));
    XII_TEST_BOOL(path.EndsWith("/sub/folder"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExistsFile")
  {
    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFile.GetData()) == false);
    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    {
      xiiOSFile f;
      XII_TEST_BOOL(f.Open(sOutputFile.GetData(), xiiFileOpenMode::Write) == XII_SUCCESS);
    }

    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFile.GetData()) == true);
    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    {
      xiiOSFile f;
      XII_TEST_BOOL(f.Open(sOutputFile2.GetData(), xiiFileOpenMode::Write) == XII_SUCCESS);
    }

    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFile.GetData()) == true);
    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFile2.GetData()) == true);

    XII_TEST_BOOL(xiiOSFile::DeleteFile(sOutputFile.GetData()) == XII_SUCCESS);
    XII_TEST_BOOL(xiiOSFile::DeleteFile(sOutputFile2.GetData()) == XII_SUCCESS);

    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFile.GetData()) == false);
    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    xiiStringBuilder sOutputFolder = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    // We should not report folders as files.
    XII_TEST_BOOL(xiiOSFile::ExistsFile(sOutputFolder) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExistsDirectory")
  {
    // files are not folders
    XII_TEST_BOOL(xiiOSFile::ExistsDirectory(sOutputFile.GetData()) == false);
    XII_TEST_BOOL(xiiOSFile::ExistsDirectory(sOutputFile2.GetData()) == false);

    xiiStringBuilder sOutputFolder = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    XII_TEST_BOOL(xiiOSFile::ExistsDirectory(sOutputFolder) == true);

    sOutputFile.AppendPath("IO");
    XII_TEST_BOOL(xiiOSFile::ExistsDirectory(sOutputFolder) == true);

    sOutputFile.AppendPath("SubFolder");
    XII_TEST_BOOL(xiiOSFile::ExistsDirectory(sOutputFolder) == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetApplicationDirectory")
  {
    xiiStringView sAppDir = xiiOSFile::GetApplicationDirectory();
    XII_TEST_BOOL(!sAppDir.IsEmpty());
  }

#if (XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) && XII_ENABLED(XII_SUPPORTS_FILE_STATS))

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DeleteFolder")
  {
    {
      xiiOSFile f;
      XII_TEST_BOOL(f.Open(sOutputFile3.GetData(), xiiFileOpenMode::Write) == XII_SUCCESS);
    }

    xiiStringBuilder SubFolder2 = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    SubFolder2.MakeCleanPath();
    SubFolder2.AppendPath("IO", "SubFolder2");

    XII_TEST_BOOL(xiiOSFile::DeleteFolder(SubFolder2).Succeeded());
    XII_TEST_BOOL(!xiiOSFile::ExistsDirectory(SubFolder2));
  }

#endif
}
