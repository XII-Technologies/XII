#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
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

// In general it is not possible to have global or static variables that (indirectly) require an allocator.
// If you create a variable that somehow needs to have an allocator, an assert will fail.
// Instead you should initialize such variables dynamically (e.g. make it into a pointer and create it at startup,
// and destroy it before shutdown, to prevent messages about memory leaks).
// However, if you absolutely need a global/static variable and cannot initialize it dynamically, wrap it inside the 'xiiStatic'
// template. This will make sure that the variable uses a different allocator, such that it won't be counted as a memory leak
// at shutdown.
xiiLogWriter::HTML g_HtmlLog;

struct FileStats
{
  FileStats()
  {
    m_uiFileCount  = 0;
    m_uiLines      = 0;
    m_uiEmptyLines = 0;
    m_uiBytes      = 0;
    m_uiCharacters = 0;
    m_uiWords      = 0;
  }

  void operator+=(const FileStats& rhs)
  {
    m_uiFileCount += rhs.m_uiFileCount;
    m_uiLines += rhs.m_uiLines;
    m_uiEmptyLines += rhs.m_uiEmptyLines;
    m_uiBytes += rhs.m_uiBytes;
    m_uiCharacters += rhs.m_uiCharacters;
    m_uiWords += rhs.m_uiWords;
  }

  xiiUInt32 m_uiFileCount;
  xiiUInt32 m_uiLines;
  xiiUInt32 m_uiEmptyLines;
  xiiUInt32 m_uiBytes;
  xiiUInt32 m_uiCharacters;
  xiiUInt32 m_uiWords;
};

xiiResult ReadCompleteFile(const char* szFile, xiiDynamicArray<xiiUInt8>& out_fileContent)
{
  out_fileContent.Clear();

  xiiFileReader File;
  if (File.Open(szFile) == XII_FAILURE)
    return XII_FAILURE;

  xiiUInt8 uiTemp[1024];
  while (true)
  {
    const xiiUInt64 uiRead = File.ReadBytes(uiTemp, 1023);

    if (uiRead == 0)
      return XII_SUCCESS; // file is automatically closed here

    out_fileContent.PushBackRange(xiiArrayPtr<xiiUInt8>(uiTemp, (xiiUInt32)uiRead));
  }

  return XII_SUCCESS; // file is automatically closed here
}

// Removes all spaces and tabs from the front and end of a line
void TrimWhitespaces(xiiStringBuilder& ref_sLine)
{
  bool b = true;

  while (b)
  {
    b = false;

    if (ref_sLine.EndsWith(" "))
    {
      b = true;
      ref_sLine.Shrink(0, 1);
    }
    if (ref_sLine.EndsWith("\t"))
    {
      b = true;
      ref_sLine.Shrink(0, 1);
    }
    if (ref_sLine.StartsWith(" "))
    {
      b = true;
      ref_sLine.Shrink(1, 0);
    }
    if (ref_sLine.StartsWith("\t"))
    {
      b = true;
      ref_sLine.Shrink(1, 0);
    }
  }
}

FileStats GetFileStats(const char* szFile)
{
  FileStats s;

  xiiDynamicArray<xiiUInt8> FileContent;
  if (ReadCompleteFile(szFile, FileContent) == XII_FAILURE)
    return s;

  FileContent.PushBack('\0');

  if (!xiiUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
  {
    xiiLog::Warning("File is not valid Utf-8: '{0}'", szFile);
    return s;
  }

  // We should not append that directly at the xiiStringBuilder, as the file read operations may end
  // in between a Utf8 sequence and then xiiStringBuilder will complain about invalid Utf8 strings.
  xiiStringBuilder sContent = (const char*)&FileContent[0];

  // Count the number of lines
  {
    xiiDynamicArray<xiiString> Lines;
    sContent.ReplaceAll("\r", ""); // Remove carriage return

    // Splits the string at occurrence of '\n' and adds each line to the 'Lines' container
    sContent.Split(true, Lines, "\n");

    xiiStringBuilder sLine;

    for (xiiUInt32 l = 0; l < Lines.GetCount(); ++l)
    {
      sLine = Lines[l].GetData();

      TrimWhitespaces(sLine);

      if (sLine.IsEmpty())
        ++s.m_uiEmptyLines;
      else
      {
        ++s.m_uiLines;

        xiiStringView LineIt = sLine;

        bool bIsInWord = false;
        while (!LineIt.IsEmpty())
        {
          const bool bNewWord = xiiStringUtils::IsIdentifierDelimiter_C_Code(LineIt.GetCharacter());

          if (bIsInWord != bNewWord)
          {
            // Count every whole word as one word and everything in between as another word
            ++s.m_uiWords;
            bIsInWord = bNewWord;
          }

          ++LineIt;
        }
      }
    }
  }

  s.m_uiBytes += sContent.GetElementCount();
  s.m_uiCharacters += sContent.GetCharacterCount();

  return s;
}


class xiiLineCountApp : public xiiApplication
{
private:
  xiiString m_sSearchDir;

public:
  using SUPER = xiiApplication;

  xiiLineCountApp() :
    xiiApplication("LineCountApp")
  {
    m_sSearchDir = "";
  }

  virtual void AfterCoreSystemsStartup() override
  {
    auto pCmd = xiiCommandLineUtils::GetGlobalInstance();

    // Pass the absolute path to the directory that should be scanned as the first parameter to this application
    if (pCmd->GetParameterCount() > 1)
      m_sSearchDir = pCmd->GetParameter(1);

    if (m_sSearchDir.IsEmpty())
    {
      xiiStringBuilder sXIISource = xiiFileSystem::GetSdkRootDirectory();
      sXIISource.AppendPath("Source");
      sXIISource.MakeCleanPath();

      m_sSearchDir = sXIISource;
    }

    xiiLog::Info("Search-dir: {}", m_sSearchDir);

    // Then add a folder as a data directory (the previously registered Factory will take care of creating the proper handler)
    // As we only need access to files through global paths, we add the "empty data directory"
    // This data dir will manage all accesses through absolute paths, unless any other data directory can handle them
    // since we don't add any further data dirs, this is it
    xiiFileSystem::AddDataDirectory("", "", ":", xiiFileSystem::AllowWrites).IgnoreResult();


    // Now we can set up the logging system (we could do it earlier, but the HTML writer needs access to the file system)

    xiiStringBuilder sLogPath = m_sSearchDir;
    sLogPath.PathParentDirectory(); // Go one folder up
    sLogPath.AppendPath("CodeStatistics.htm");

    // The console log writer will pass all log messages to the standard console window
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    // The Visual Studio log writer will pass all messages to the output window in VS
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
    // The HTML log writer will write all log messages to an HTML file
    g_HtmlLog.BeginLog(sLogPath.GetData(), "Code Statistics");
    xiiGlobalLog::AddLogWriter(xiiLoggingEvent::Handler(&xiiLogWriter::HTML::LogMessageHandler, &g_HtmlLog));
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // Close the HTML log, from now on no more log messages are written to the file
    g_HtmlLog.EndLog();
  }

  virtual xiiApplication::Execution Run() override
  {
#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS) || defined(XII_DOCS)

    xiiUInt32                    uiDirectories = 0;
    xiiUInt32                    uiFiles       = 0;
    xiiMap<xiiString, FileStats> FileTypeStatistics;

    // Get a directory iterator for the search directory
    xiiFileSystemIterator it;
    it.StartSearch(m_sSearchDir);

    if (it.IsValid())
    {
      xiiStringBuilder b, sExt;

      // While there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // Build the absolute path to the current file
        b = it.GetCurrentPath();
        b.AppendPath(it.GetStats().m_sName.GetData());

        // Log some info
        xiiLog::Info("{0}: {1}", it.GetStats().m_bIsDirectory ? "Directory" : "File", b);

        if (it.GetStats().m_bIsDirectory)
        {
          ++uiDirectories;
        }
        else
        {
          // File extensions are always converted to lower-case actually
          sExt = b.GetFileExtension();

          if (sExt.IsEqual_NoCase("cpp") || sExt.IsEqual_NoCase("h") || sExt.IsEqual_NoCase("hpp") || sExt.IsEqual_NoCase("inl"))
          {
            ++uiFiles;

            // Get additional stats and add them to the overall stats
            FileStats& TypeStats = FileTypeStatistics[sExt.GetData()];
            ++TypeStats.m_uiFileCount;

            TypeStats += GetFileStats(b.GetData());
          }
        }
      }

      // Now output some statistics
      xiiLog::Info("Directories: {0}, Files: {1}, Avg. Files per Dir: {2}", uiDirectories, uiFiles, xiiArgF(uiFiles / (float)uiDirectories, 1));

      FileStats AllTypes;

      // Iterate over all elements in the amp
      xiiMap<xiiString, FileStats>::Iterator MapIt = FileTypeStatistics.GetIterator();
      while (MapIt.IsValid())
      {
        xiiLog::Info("File Type: '{0}': {1} Files, {2} Lines, {3} Empty Lines, Bytes: {4}, Non-ASCII Characters: {5}, Words: {6}", MapIt.Key(), MapIt.Value().m_uiFileCount, MapIt.Value().m_uiLines, MapIt.Value().m_uiEmptyLines, MapIt.Value().m_uiBytes,
                     MapIt.Value().m_uiBytes - MapIt.Value().m_uiCharacters, MapIt.Value().m_uiWords);

        AllTypes += MapIt.Value();

        ++MapIt;
      }

      xiiLog::Info("File Type: '{0}': {1} Files, {2} Lines, {3} Empty Lines, All Lines: {4}, Bytes: {5}, Non-ASCII Characters: {6}, Words: {7}", "all", AllTypes.m_uiFileCount, AllTypes.m_uiLines, AllTypes.m_uiEmptyLines, AllTypes.m_uiLines + AllTypes.m_uiEmptyLines, AllTypes.m_uiBytes,
                   AllTypes.m_uiBytes - AllTypes.m_uiCharacters, AllTypes.m_uiWords);
    }
    else
    {
      xiiLog::Error("Could not search the directory '{0}'", m_sSearchDir);
    }
#else
    XII_REPORT_FAILURE("No file system iterator support, LineCount sample can't run.");
#endif
    return xiiApplication::Execution::Quit;
  }
};

XII_CONSOLEAPP_ENTRY_POINT(xiiLineCountApp);
