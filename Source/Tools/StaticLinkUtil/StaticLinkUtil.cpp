#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/CommandLineUtils.h>

/* When statically linking libraries into an application the linker will only pull in all the functions and variables that are inside
translation units (CPP files) that somehow get referenced.

In XII a lot of stuff happens automatically (e.g. types register themselves etc.), which is accomplished through global variables
that execute code in their constructor during the applications startup phase. This only works when those global variables are actually
put into the application by the linker. If the linker does not do that, functionality will not work as intended.

Contrary to common conception, the linker is NOT ALLOWED to optimize away global variables. The only reason for not including a global
variable into the final binary, is when the entire translation unit where a variable is defined in, is never referenced and thus never
even looked at by the linker.

To fix this, this tool inserts macros into each and every file which reference each other. Afterwards every file in a library will
have reference every other file in that same library and thus once a library is used in any way in some program, the entire library
will be pulled in and will then work as intended.

These references are accomplished through empty functions that are called in one central location (where XII_STATICLINK_LIBRARY is defined),
though the code actually never really calls those functions, but it is enough to force the linker to look at all the other files.

Usage of this tool:

Call this tool with the path to the root folder of some library as the sole command line argument:

StaticLinkUtil.exe "C:\XII\Source\Engine\Foundation"

Note: Do not add a trailing slash after the path.

This will iterate over all files below that folder and insert the proper macros.
Also make sure that exactly one file in each library contains the text 'XII_STATICLINK_LIBRARY();'

The parameters and function body will be automatically generated and later updated, you do not need to provide more.

See the Return Codes at the end of the BeforeCoreSystemsShutdown function.
*/

class xiiStaticLinkerApp : public xiiApplication
{
private:
  xiiString m_sSearchDir;
  bool      m_bHadErrors;
  bool      m_bHadSeriousWarnings;
  bool      m_bHadWarnings;
  bool      m_bModifiedFiles;
  bool      m_bAnyFileChanged;

  struct FileContent
  {
    FileContent() { m_bFileHasChanged = false; }

    bool      m_bFileHasChanged;
    xiiString m_sFileContent;
  };

  xiiSet<xiiString> m_AllRefPoints;
  xiiString         m_sRefPointGroupFile;

  xiiSet<xiiString> m_GlobalIncludes;

  xiiMap<xiiString, FileContent> m_ModifiedFiles;


public:
  using SUPER = xiiApplication;

  xiiStaticLinkerApp() :
    xiiApplication("StaticLinkerApp")
  {
    m_bHadErrors          = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings        = false;
    m_bModifiedFiles      = false;
    m_bAnyFileChanged     = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const xiiLoggingEventData& eventData)
  {
    xiiStaticLinkerApp* app = (xiiStaticLinkerApp*)xiiApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case xiiLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case xiiLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case xiiLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }

  virtual void AfterCoreSystemsStartup() override
  {
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(LogInspector);

    if (GetArgumentCount() != 2)
      xiiLog::Error("This tool requires exactly one command-line argument: A path to the top-level folder of a library.");

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    xiiStringBuilder sSearchDir = xiiOSFile::MakePathAbsoluteWithCWD(GetArgument(1));

    m_sSearchDir = sSearchDir;

    // Add the empty data directory to access files via absolute paths
    xiiFileSystem::AddDataDirectory("", "App", ":", xiiDataDirUsage::AllowWrites).IgnoreResult();

    // Use such a path to write to an absolute file
    // ':abs/C:/some/file.txt"
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if ((m_bHadSeriousWarnings || m_bHadErrors) && m_bModifiedFiles)
      xiiLog::SeriousWarning("There were issues while writing out the updated files. The source will be in an inconsistent state, please revert the changes.");
    else if (m_bHadWarnings || m_bHadSeriousWarnings || m_bHadErrors)
    {
      xiiLog::Warning("There have been errors or warnings, see log for details.");
    }

    if (m_bModifiedFiles)
    {
      if (m_bHadErrors || m_bHadSeriousWarnings)
        SetReturnCode(3); // Errors or Serious Warnings, yet files modified, this is unusual, requires reverting the source from outside.
      else if (m_bHadWarnings)
        SetReturnCode(2); // Warnings, files still modified. (normal operation)
      else
        SetReturnCode(1); // No issues, files modified. (normal operation)
    }
    else
    {
      if (m_bHadErrors || m_bHadSeriousWarnings)
        SetReturnCode(-3); // Errors or serious warnings, no files modified (but might need to be), user needs to look at it.
      else if (m_bHadWarnings)
        SetReturnCode(-2); // Warnings, but no files were modified anyway, user should look at it though. (normal operation)
      else
        SetReturnCode(-1); // No issues, no file modifications, everything is up to date apparently. (normal operation)
    }

    // Return Codes:
    // All negative requires no RCS operations (no changes were made)
    // All positive require RCS operations (either commit or revert)
    // -1 and 1 are perfectly fine
    // -2 and 2 mean there were warnings that a user should look at, but nothing that prevented this tool from doing its work
    // -3 and 3 mean something went wrong
    // thus 3 means the changes it made need to be reverted from outside
    // 1 and 2 mean the changes need to be committed

    xiiGlobalLog::RemoveLogWriter(LogInspector);
    xiiGlobalLog::RemoveLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::RemoveLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
  }

  xiiString GetLibraryMarkerName()
  {
    xiiStringBuilder tmp;
    return xiiPathUtils::GetFileName(m_sSearchDir.GetData()).GetData(tmp);
  }

  void SanitizeSourceCode(xiiStringBuilder& ref_sInOut)
  {
    ref_sInOut.ReplaceAll("\r\n", "\n");

    if (!ref_sInOut.EndsWith("\n"))
      ref_sInOut.Append("\n");

    while (ref_sInOut.EndsWith("\n\n\n\n"))
      ref_sInOut.Shrink(0, 1);
  }

  xiiResult ReadEntireFile(const char* szFile, xiiStringBuilder& ref_sOut)
  {
    ref_sOut.Clear();

    // If we have that file cached already, just return the cached (and possibly modified) content
    if (!m_ModifiedFiles[szFile].m_sFileContent.IsEmpty())
    {
      ref_sOut = m_ModifiedFiles[szFile].m_sFileContent.GetData();
      return XII_SUCCESS;
    }

    xiiFileReader File;
    if (File.Open(szFile) == XII_FAILURE)
    {
      xiiLog::Error("Could not open for reading: '{0}'", szFile);
      return XII_FAILURE;
    }

    xiiDynamicArray<xiiUInt8> FileContent;

    xiiUInt8  Temp[1024];
    xiiUInt64 uiRead = File.ReadBytes(Temp, 1024);

    while (uiRead > 0)
    {
      FileContent.PushBackRange(xiiArrayPtr<xiiUInt8>(Temp, (xiiUInt32)uiRead));

      uiRead = File.ReadBytes(Temp, 1024);
    }

    FileContent.PushBack(0);

    if (!xiiUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
    {
      xiiLog::Error("The file \"{0}\" contains characters that are not valid Utf8. This often happens when you type special characters in an editor "
                    "that does not save the file in Utf8 encoding.",
                    szFile);
      return XII_FAILURE;
    }

    ref_sOut = (const char*)&FileContent[0];

    m_ModifiedFiles[szFile].m_sFileContent = ref_sOut;

    SanitizeSourceCode(ref_sOut);

    return XII_SUCCESS;
  }

  void OverwriteFile(const char* szFile, const xiiStringBuilder& sFileContent)
  {
    xiiStringBuilder sOut = sFileContent;
    SanitizeSourceCode(sOut);

    if (m_ModifiedFiles[szFile].m_sFileContent == sOut)
      return;

    m_bAnyFileChanged                         = true;
    m_ModifiedFiles[szFile].m_bFileHasChanged = true;
    m_ModifiedFiles[szFile].m_sFileContent    = sOut;
  }

  void OverwriteModifiedFiles()
  {
    XII_LOG_BLOCK("Overwriting modified files");

    if (m_bHadSeriousWarnings || m_bHadErrors)
    {
      xiiLog::Info("There have been errors or warnings previously, no files will be modified.");
      return;
    }

    if (!m_bAnyFileChanged)
    {
      xiiLog::Success("No files needed modification.");
      return;
    }

    for (auto it = m_ModifiedFiles.GetIterator(); it.IsValid(); ++it)
    {
      if (!it.Value().m_bFileHasChanged)
        continue;

      xiiFileWriter FileOut;
      if (FileOut.Open(it.Key().GetData()) == XII_FAILURE)
      {
        xiiLog::Error("Could not open the file for writing: '{0}'", it.Key());
        return;
      }
      else
      {
        m_bModifiedFiles = true;
        FileOut.WriteBytes(it.Value().m_sFileContent.GetData(), it.Value().m_sFileContent.GetElementCount()).IgnoreResult();

        xiiLog::Success("File has been modified: '{0}'", it.Key());
      }
    }
  }

  void FindIncludes(xiiStringBuilder& ref_sFileContent)
  {
    const char*     szStartPos   = ref_sFileContent.GetData();
    const xiiString sLibraryName = GetLibraryMarkerName();

    while (true)
    {
      const char* szI = ref_sFileContent.FindSubString("#i", szStartPos);

      if (szI == nullptr)
        return;

      szStartPos = szI + 1;

      if (xiiStringUtils::IsEqualN(szI, "#if", 3))
      {
        szStartPos = ref_sFileContent.FindSubString("#endif", szStartPos);

        if (szStartPos == nullptr)
          return;

        ++szStartPos;
        continue; // Next search will be for #i again
      }

      if (xiiStringUtils::IsEqualN(szI, "#include", 8))
      {
        szI += 8; // Skip the "#include" string

        const char* szLineEnd = xiiStringUtils::FindSubString(szI, "\n");

        xiiStringView si(szI, szLineEnd);

        xiiStringBuilder sInclude = si;

        if (sInclude.ReplaceAll("\\", "/") > 0)
        {
          xiiLog::Info("Replacing backslashes in #include path with front slashes: '{0}'", sInclude);
          ref_sFileContent.ReplaceSubString(szI, szLineEnd, sInclude.GetData());
        }

        while (sInclude.StartsWith(" ") || sInclude.StartsWith("\t") || sInclude.StartsWith("<"))
          sInclude.Shrink(1, 0);

        while (sInclude.EndsWith(" ") || sInclude.EndsWith("\t") || sInclude.EndsWith(">"))
          sInclude.Shrink(0, 1);

        // Ignore relative includes, they will not work as expected from the PCH
        if (sInclude.StartsWith("\""))
          continue;

        // Ignore includes into the own library
        if (sInclude.StartsWith(sLibraryName.GetData()))
          continue;

        // Ignore third-party includes
        if (sInclude.FindSubString_NoCase("ThirdParty"))
        {
          xiiLog::Dev("Skipping ThirdParty Include: '{0}'", sInclude);
          continue;
        }

        xiiStringBuilder sCanFindInclude = m_sSearchDir.GetData();
        sCanFindInclude.PathParentDirectory();
        sCanFindInclude.AppendPath(sInclude.GetData());

        xiiStringBuilder sCanFindInclude2 = m_sSearchDir.GetData();
        sCanFindInclude2.PathParentDirectory(2);
        sCanFindInclude2.AppendPath("Source/Engine");
        sCanFindInclude2.AppendPath(sInclude.GetData());

        // Ignore includes to files that cannot be found (ie. they are not part of the XII Engine source tree)
        if (!xiiFileSystem::ExistsFile(sCanFindInclude.GetData()) && !xiiFileSystem::ExistsFile(sCanFindInclude2.GetData()))
        {
          xiiLog::Dev("Skipping non-Engine Include: '{0}'", sInclude);
          continue;
        }

        // Warn about includes that have 'implementation' in their path
        if (sInclude.FindSubString_NoCase("Implementation"))
        {
          xiiLog::Warning("This file includes an implementation header from another library: '{0}'", sInclude);
        }

        xiiLog::Dev("Found Include: '{0}'", sInclude);

        m_GlobalIncludes.Insert(sInclude);
      }
    }
  }

  bool RemoveLineWithPrefix(xiiStringBuilder& ref_sFile, const char* szLineStart)
  {
    const char* szSkipAhead = ref_sFile.FindSubString("// <StaticLinkUtil::StartHere>");

    const char* szStart = ref_sFile.FindSubString(szLineStart, szSkipAhead);

    if (szStart == nullptr)
      return false;

    const char* szEnd = ref_sFile.FindSubString("\n", szStart);

    if (szEnd == nullptr)
      szEnd = ref_sFile.GetData() + ref_sFile.GetElementCount();

    ref_sFile.ReplaceSubString(szStart, szEnd, "");

    return true;
  }

  void RewritePrecompiledHeaderIncludes()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    xiiStringBuilder sPCHFile = m_sSearchDir.GetData();
    sPCHFile.AppendPath("PCH.h");

    {
      xiiFileReader File;
      if (File.Open(sPCHFile.GetData()) == XII_FAILURE)
      {
        xiiLog::Warning("This project has no PCH file.");
        return;
      }
    }

    xiiLog::Info("Rewriting PCH: '{0}'", sPCHFile);

    xiiStringBuilder sFileContent;
    if (ReadEntireFile(sPCHFile.GetData(), sFileContent) == XII_FAILURE)
      return;

    while (RemoveLineWithPrefix(sFileContent, "#include"))
    {
      // Do this
    }

    SanitizeSourceCode(sFileContent);

    xiiStringBuilder sAllIncludes;

    for (auto it = m_GlobalIncludes.GetIterator(); it.IsValid(); ++it)
    {
      sAllIncludes.AppendFormat("#include <{0}>\n", it.Key());
    }

    sAllIncludes.ReplaceAll("\\", "/");

    sFileContent.Append(sAllIncludes.GetData());

    OverwriteFile(sPCHFile.GetData(), sFileContent);
  }

  void FixFileContents(const char* szFile)
  {
    xiiStringBuilder sFileContent;
    if (ReadEntireFile(szFile, sFileContent) == XII_FAILURE)
      return;

    if (xiiStringUtils::EndsWith(szFile, "PCH.h"))
      xiiLog::Dev("Skipping PCH for #include search: '{0}'", szFile);
    else
      FindIncludes(sFileContent);

    // Rewrite the entire file
    OverwriteFile(szFile, sFileContent);
  }

  xiiString GetFileMarkerName(const char* szFile)
  {
    xiiStringBuilder sRel = szFile;
    sRel.MakeRelativeTo(m_sSearchDir.GetData()).IgnoreResult();

    xiiStringBuilder sRefPointName = xiiPathUtils::GetFileName(m_sSearchDir.GetData());
    sRefPointName.Append("_");
    sRefPointName.Append(sRel.GetData());
    sRefPointName.ReplaceAll("\\", "_");
    sRefPointName.ReplaceAll("/", "_");
    sRefPointName.ReplaceAll(".cpp", "");

    return sRefPointName;
  }

  void InsertRefPoint(const char* szFile)
  {
    XII_LOG_BLOCK("InsertRefPoint", szFile);

    xiiStringBuilder sFileContent;
    if (ReadEntireFile(szFile, sFileContent) == XII_FAILURE)
      return;

    // If we find this macro in here, we don't need to insert XII_STATICLINK_FILE in this file
    // but once we are done with all files, we want to come back to this file and rewrite the XII_STATICLINK_LIBRARY
    // part such that it will reference all the other files
    if (sFileContent.FindSubString("XII_STATICLINK_LIBRARY"))
      return;

    xiiString sLibraryMarker = GetLibraryMarkerName();
    xiiString sFileMarker    = GetFileMarkerName(szFile);

    xiiStringBuilder sNewMarker;
    sNewMarker.SetFormat("XII_STATICLINK_FILE({0}, {1});", sLibraryMarker, sFileMarker);

    m_AllRefPoints.Insert(sFileMarker.GetData());

    const char* szMarker = sFileContent.FindSubString("XII_STATICLINK_FILE");

    // If the marker already exists, replace it with the updated string
    if (szMarker != nullptr)
    {
      const char* szMarkerEnd = szMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '\n')
        ++szMarkerEnd;

      sFileContent.ReplaceSubString(szMarker, szMarkerEnd, sNewMarker.GetData());
    }
    else
    {
      // Otherwise insert it at the end of the file
      sFileContent.AppendFormat("\n\n{0}\n\n", sNewMarker);
    }

    // Rewrite the entire file
    OverwriteFile(szFile, sFileContent);
  }

  void UpdateStaticLinkLibraryBlock()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    const char* szFile = m_sRefPointGroupFile.GetData();

    XII_LOG_BLOCK("RewriteRefPointGroup", szFile);

    xiiLog::Info("Replacing macro XII_STATICLINK_LIBRARY in file '{0}'.", m_sRefPointGroupFile);

    xiiStringBuilder sFileContent;
    if (ReadEntireFile(szFile, sFileContent) == XII_FAILURE)
      return;

    // Remove all instances of XII_STATICLINK_FILE from this file, it already contains XII_STATICLINK_LIBRARY
    const char* szMarker = sFileContent.FindSubString("XII_STATICLINK_FILE");
    while (szMarker != nullptr)
    {
      xiiLog::Warning("Found macro XII_STATICLINK_FILE inside the same file where XII_STATICLINK_LIBRARY is located. Removing it.");

      const char* szMarkerEnd = szMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '\n')
        ++szMarkerEnd;

      // No ref point allowed in a file that has already a ref point group
      sFileContent.Remove(szMarker, szMarkerEnd);

      szMarker = sFileContent.FindSubString("XII_STATICLINK_FILE");
    }

    xiiStringBuilder sNewGroupMarker;

    // Generate the code that should be inserted into this file
    // This code will reference all the other files in the library
    {
      sNewGroupMarker.SetFormat("XII_STATICLINK_LIBRARY({0})\n{\n  if (bReturn)\n    return;\n\n", GetLibraryMarkerName());

      auto it = m_AllRefPoints.GetIterator();

      while (it.IsValid())
      {
        sNewGroupMarker.AppendFormat("  XII_STATICLINK_REFERENCE({0});\n", it.Key());
        ++it;
      }

      sNewGroupMarker.Append("}\n");
    }

    const char* szGroupMarker = sFileContent.FindSubString("XII_STATICLINK_LIBRARY");

    if (szGroupMarker != nullptr)
    {
      // If we could find the macro XII_STATICLINK_LIBRARY, just replace it with the new code

      const char* szMarkerEnd = szGroupMarker;

      bool bFoundOpenBraces = false;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '}')
      {
        ++szMarkerEnd;

        if (*szMarkerEnd == '{')
          bFoundOpenBraces = true;
        if (!bFoundOpenBraces && *szMarkerEnd == ';')
          break;
      }

      if (*szMarkerEnd == '}' || *szMarkerEnd == ';')
        ++szMarkerEnd;
      if (*szMarkerEnd == '\n')
        ++szMarkerEnd;

      // Now replace the existing XII_STATICLINK_LIBRARY and its code block with the new block
      sFileContent.ReplaceSubString(szGroupMarker, szMarkerEnd, sNewGroupMarker.GetData());
    }
    else
    {
      // If we can't find the macro, append it to the end of the file.
      // This can only happen, if we ever extend this tool such that it picks one file to auto-insert this macro
      sFileContent.AppendFormat("\n\n{0}\n\n", sNewGroupMarker);
    }

    OverwriteFile(szFile, sFileContent);
  }

  void IterateOverFiles()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) || defined(XII_DOCS)
    const xiiUInt32 uiSearchDirLength = m_sSearchDir.GetElementCount() + 1;

    // Retrieve a directory iterator for the search directory
    xiiFileSystemIterator it;
    it.StartSearch(m_sSearchDir.GetData(), xiiFileSystemIteratorFlags::ReportFilesRecursive);

    if (it.IsValid())
    {
      xiiStringBuilder b, sExt;

      // While there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // Build the absolute path to the current file
        b = it.GetCurrentPath();
        b.AppendPath(it.GetStats().m_sName.GetData());

        // File extensions are always converted to lower-case actually
        sExt = b.GetFileExtension();

        if (sExt.IsEqual_NoCase("h") || sExt.IsEqual_NoCase("inl"))
        {
          XII_LOG_BLOCK("Header", &b.GetData()[uiSearchDirLength]);
          FixFileContents(b.GetData());
          continue;
        }

        if (sExt.IsEqual_NoCase("cpp"))
        {
          XII_LOG_BLOCK("Source", &b.GetData()[uiSearchDirLength]);
          FixFileContents(b.GetData());

          InsertRefPoint(b.GetData());
          continue;
        }
      }
    }
    else
      xiiLog::Error("Could not search the directory '{0}'", m_sSearchDir);

#else
    XII_REPORT_FAILURE("No file system iterator support, StaticLinkUtil sample can't run.");
#endif
  }

  void MakeSureStaticLinkLibraryMacroExists()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    // The macro XII_STATICLINK_LIBRARY was not found in any cpp file,
    // try to insert it into a PCH.cpp, if there is one
    if (!m_sRefPointGroupFile.IsEmpty())
      return;

    xiiStringBuilder sFilePath;
    sFilePath.AppendPath(m_sSearchDir.GetData(), "PCH.cpp");

    auto it = m_ModifiedFiles.Find(sFilePath);

    if (it.IsValid())
    {
      xiiStringBuilder sPCHcpp = it.Value().m_sFileContent.GetData();
      sPCHcpp.Append("\n\n\n\nXII_STATICLINK_LIBRARY() { }");

      OverwriteFile(sFilePath.GetData(), sPCHcpp);

      m_sRefPointGroupFile = sFilePath;

      xiiLog::Warning("No XII_STATICLINK_LIBRARY found in any cpp file, inserting it into the PCH.cpp file.");
    }
    else
      xiiLog::Error("The macro XII_STATICLINK_LIBRARY was not found in any cpp file in this library. It is required that it exists in exactly one "
                    "file, otherwise the generated code will not compile.");
  }

  void GatherInformation()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    XII_LOG_BLOCK("FindRefPointGroupFile");

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) || defined(XII_DOCS)
    // Retrieve a directory iterator for the search directory
    xiiFileSystemIterator it;
    it.StartSearch(m_sSearchDir.GetData(), xiiFileSystemIteratorFlags::ReportFilesRecursive);

    if (it.IsValid())
    {
      xiiStringBuilder sFile, sExt;

      // While there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // Build the absolute path to the current file
        sFile = it.GetCurrentPath();
        sFile.AppendPath(it.GetStats().m_sName.GetData());

        // File extensions are always converted to lower-case actually
        sExt = sFile.GetFileExtension();

        if (sExt.IsEqual_NoCase("cpp"))
        {
          xiiStringBuilder sFileContent;
          if (ReadEntireFile(sFile.GetData(), sFileContent) == XII_FAILURE)
            return;

          // If we find this macro in here, we don't need to insert XII_STATICLINK_FILE in this file
          // but once we are done with all files, we want to come back to this file and rewrite the XII_STATICLINK_LIBRARY
          // part such that it will reference all the other files
          if (sFileContent.FindSubString("XII_STATICLINK_LIBRARY"))
          {
            xiiLog::Info("Found macro 'XII_STATICLINK_LIBRARY' in file '{0}'.", &sFile.GetData()[m_sSearchDir.GetElementCount() + 1]);

            if (!m_sRefPointGroupFile.IsEmpty())
              xiiLog::Error("The macro 'XII_STATICLINK_LIBRARY' was already found in file '{0}' before. You cannot have this macro twice in the same library!", m_sRefPointGroupFile);
            else
              m_sRefPointGroupFile = sFile;
          }
        }
      }
    }
    else
      xiiLog::Error("Could not search the directory '{0}'", m_sSearchDir);
#else
    XII_REPORT_FAILURE("No file system iterator support, StaticLinkUtil sample can't run.");
#endif
    MakeSureStaticLinkLibraryMacroExists();
  }

  virtual xiiApplication::Execution Run() override
  {
    // Something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return xiiApplication::Execution::Quit;

    GatherInformation();

    IterateOverFiles();

    UpdateStaticLinkLibraryBlock();

    // RewritePrecompiledHeaderIncludes();

    OverwriteModifiedFiles();

    return xiiApplication::Execution::Quit;
  }
};

XII_CONSOLEAPP_ENTRY_POINT(xiiStaticLinkerApp);
