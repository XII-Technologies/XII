#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/String.h>

XII_CREATE_SIMPLE_TEST(Strings, PathUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsPathSeparator")
  {
    for (int i = 0; i < 0xFFFF; ++i)
    {
      if (i == '/')
      {
        XII_TEST_BOOL(xiiPathUtils::IsPathSeparator(i));
      }
      else if (i == '\\')
      {
        XII_TEST_BOOL(xiiPathUtils::IsPathSeparator(i));
      }
      else
      {
        XII_TEST_BOOL(!xiiPathUtils::IsPathSeparator(i));
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindPreviousSeparator")
  {
    const char* szPath = "This/Is\\My//Path.dot\\file.extension";

    XII_TEST_BOOL(xiiPathUtils::FindPreviousSeparator(szPath, szPath + 35) == szPath + 20);
    XII_TEST_BOOL(xiiPathUtils::FindPreviousSeparator(szPath, szPath + 20) == szPath + 11);
    XII_TEST_BOOL(xiiPathUtils::FindPreviousSeparator(szPath, szPath + 11) == szPath + 10);
    XII_TEST_BOOL(xiiPathUtils::FindPreviousSeparator(szPath, szPath + 10) == szPath + 7);
    XII_TEST_BOOL(xiiPathUtils::FindPreviousSeparator(szPath, szPath + 7) == szPath + 4);
    XII_TEST_BOOL(xiiPathUtils::FindPreviousSeparator(szPath, szPath + 4) == nullptr);
    XII_TEST_BOOL(xiiPathUtils::FindPreviousSeparator(szPath, szPath) == nullptr);
    XII_TEST_BOOL(xiiPathUtils::FindPreviousSeparator(nullptr, nullptr) == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HasAnyExtension")
  {
    XII_TEST_BOOL(xiiPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file.extension"));
    XII_TEST_BOOL(!xiiPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file_no_extension"));
    XII_TEST_BOOL(!xiiPathUtils::HasAnyExtension(""));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HasExtension")
  {
    XII_TEST_BOOL(xiiPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Extension"));
    XII_TEST_BOOL(xiiPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "EXT"));
    XII_TEST_BOOL(!xiiPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "NEXT"));
    XII_TEST_BOOL(!xiiPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Ext"));
    XII_TEST_BOOL(!xiiPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", "sion"));
    XII_TEST_BOOL(!xiiPathUtils::HasExtension("", "ext"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileExtension")
  {
    XII_TEST_BOOL(xiiPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file.extension") == "extension");
    XII_TEST_BOOL(xiiPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file") == "");
    XII_TEST_BOOL(xiiPathUtils::GetFileExtension("") == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileNameAndExtension")
  {
    XII_TEST_BOOL(xiiPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file.extension") == "file.extension");
    XII_TEST_BOOL(xiiPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\.extension") == ".extension");
    XII_TEST_BOOL(xiiPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file") == "file");
    XII_TEST_BOOL(xiiPathUtils::GetFileNameAndExtension("\\file") == "file");
    XII_TEST_BOOL(xiiPathUtils::GetFileNameAndExtension("") == "");
    XII_TEST_BOOL(xiiPathUtils::GetFileNameAndExtension("/") == "");
    XII_TEST_BOOL(xiiPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\") == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileName")
  {
    XII_TEST_BOOL(xiiPathUtils::GetFileName("This/Is\\My//Path.dot\\file.extension") == "file");
    XII_TEST_BOOL(xiiPathUtils::GetFileName("This/Is\\My//Path.dot\\file") == "file");
    XII_TEST_BOOL(xiiPathUtils::GetFileName("\\file") == "file");
    XII_TEST_BOOL(xiiPathUtils::GetFileName("") == "");
    XII_TEST_BOOL(xiiPathUtils::GetFileName("/") == "");
    XII_TEST_BOOL(xiiPathUtils::GetFileName("This/Is\\My//Path.dot\\") == "");

    // so far we treat file and folders whose names start with a '.' as extensions
    XII_TEST_BOOL(xiiPathUtils::GetFileName("This/Is\\My//Path.dot\\.stupidfile") == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFileDirectory")
  {
    XII_TEST_BOOL(xiiPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file.extension") == "This/Is\\My//Path.dot\\");
    XII_TEST_BOOL(xiiPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\.extension") == "This/Is\\My//Path.dot\\");
    XII_TEST_BOOL(xiiPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file") == "This/Is\\My//Path.dot\\");
    XII_TEST_BOOL(xiiPathUtils::GetFileDirectory("\\file") == "\\");
    XII_TEST_BOOL(xiiPathUtils::GetFileDirectory("") == "");
    XII_TEST_BOOL(xiiPathUtils::GetFileDirectory("/") == "/");
    XII_TEST_BOOL(xiiPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\") == "This/Is\\My//Path.dot\\");
    XII_TEST_BOOL(xiiPathUtils::GetFileDirectory("This") == "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsAbsolutePath")
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    XII_TEST_BOOL(xiiPathUtils::IsAbsolutePath("C:\\temp.stuff"));
    XII_TEST_BOOL(xiiPathUtils::IsAbsolutePath("C:/temp.stuff"));
    XII_TEST_BOOL(xiiPathUtils::IsAbsolutePath("\\\\myserver\\temp.stuff"));
    XII_TEST_BOOL(!xiiPathUtils::IsAbsolutePath("\\myserver\\temp.stuff"));
    XII_TEST_BOOL(!xiiPathUtils::IsAbsolutePath("temp.stuff"));
    XII_TEST_BOOL(!xiiPathUtils::IsAbsolutePath("/temp.stuff"));
    XII_TEST_BOOL(!xiiPathUtils::IsAbsolutePath("\\temp.stuff"));
    XII_TEST_BOOL(!xiiPathUtils::IsAbsolutePath("..\\temp.stuff"));
    XII_TEST_BOOL(!xiiPathUtils::IsAbsolutePath(".\\temp.stuff"));
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
    XII_TEST_BOOL(xiiPathUtils::IsAbsolutePath("/usr/local/.stuff"));
    XII_TEST_BOOL(xiiPathUtils::IsAbsolutePath("/file.test"));
    XII_TEST_BOOL(!xiiPathUtils::IsAbsolutePath("./file.stuff"));
    XII_TEST_BOOL(!xiiPathUtils::IsAbsolutePath("file.stuff"));
#else
#  error "Unknown platform."
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRootedPathParts")
  {
    xiiStringView root, relPath;
    xiiPathUtils::GetRootedPathParts(":MyRoot\\folder\\file.txt", root, relPath);
    XII_TEST_BOOL(xiiPathUtils::GetRootedPathRootName(":MyRoot\\folder\\file.txt") == root);
    XII_TEST_BOOL(root == "MyRoot");
    XII_TEST_BOOL(relPath == "folder\\file.txt");
    xiiPathUtils::GetRootedPathParts("folder\\file2.txt", root, relPath);
    XII_TEST_BOOL(root.IsEmpty());
    XII_TEST_BOOL(relPath == "folder\\file2.txt");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsSubPath")
  {
    XII_TEST_BOOL(xiiPathUtils::IsSubPath("C:/DataDir", "C:/DataDir/SomeFolder"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath("C:/DataDir", "C:/DataDir"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath("C:/DataDir", "C:/DataDir/"));
    XII_TEST_BOOL(!xiiPathUtils::IsSubPath("C:/DataDir", "C:/DataDir2"));

    XII_TEST_BOOL(xiiPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir/SomeFolder"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir/"));
    XII_TEST_BOOL(!xiiPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir2"));

    XII_TEST_BOOL(!xiiPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir/SomeFolder"));
    XII_TEST_BOOL(!xiiPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir"));
    XII_TEST_BOOL(!xiiPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir/"));
    XII_TEST_BOOL(!xiiPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir2"));

    XII_TEST_BOOL(!xiiPathUtils::IsSubPath("C:/DataDir/SomeFolder", "C:/DataDir"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsSubPath_NoCase")
  {
    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir/SomeFolder"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir/"));
    XII_TEST_BOOL(!xiiPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir2"));

    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir/SomeFolder"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir/"));
    XII_TEST_BOOL(!xiiPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir2"));

    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir/SomeFolder"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir"));
    XII_TEST_BOOL(xiiPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir/"));
    XII_TEST_BOOL(!xiiPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir2"));

    XII_TEST_BOOL(!xiiPathUtils::IsSubPath_NoCase("C:/DataDir/SomeFolder", "C:/DataDir"));
  }
}
