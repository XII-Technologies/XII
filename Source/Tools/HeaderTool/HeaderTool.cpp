/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

// Build tool that exports the Engine Source Header Files into a specified directory.
class xiiHeaderTool : public xiiApplication
{
private:
  xiiString m_sSourceDirectory;
  xiiString m_sExportDirectory;

  bool m_bErrorEncountered;

public:
  using SUPER = xiiApplication;

  xiiHeaderTool() :
    xiiApplication("xiiHeaderTool")
  {
    m_sSourceDirectory = "";
    m_sExportDirectory = "";

    m_bErrorEncountered = false;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // The console log writer will pass all log messages to the standard console window
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    // The Visual Studio log writer will pass all messages to the output window in VS
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

    auto pCmd = xiiCommandLineUtils::GetGlobalInstance();

    // Pass the absolute path to the directory that should be exported to as the first parameter to this application
    if (pCmd->GetParameterCount() > 1)
    {
      xiiStringBuilder sXIIExport = xiiCommandLineUtils::GetGlobalInstance()->GetParameter(1);
      sXIIExport.MakeCleanPath();

      m_sExportDirectory = sXIIExport;
    }

    // Pass the absolute path to the directory that should be exported to as the second parameter to this application
    // The source directory should be the first parameter
    if (pCmd->GetParameterCount() > 2)
    {
      xiiStringBuilder sXIISource = xiiCommandLineUtils::GetGlobalInstance()->GetParameter(1);
      sXIISource.MakeCleanPath();
      m_sSourceDirectory = sXIISource;

      xiiStringBuilder sXIIExport = xiiCommandLineUtils::GetGlobalInstance()->GetParameter(2);
      sXIIExport.MakeCleanPath();
      m_sExportDirectory = sXIIExport;
    }

    if (m_sSourceDirectory.IsEmpty())
    {
      xiiStringBuilder sXIISource = xiiFileSystem::GetSdkRootDirectory();
      sXIISource.AppendPath("Source");
      sXIISource.MakeCleanPath();

      m_sSourceDirectory = sXIISource;
    }
    else
    {
      m_bErrorEncountered = true;
    }

    if (m_sExportDirectory.IsEmpty())
    {
      m_bErrorEncountered = true;
    }

    xiiLog::Info("Source Directory: {}", m_sSourceDirectory);
    xiiLog::Info("Export Directory: {}", m_sExportDirectory);

    // Then add a folder as a data directory (the previously registered Factory will take care of creating the proper handler)
    // As we only need access to files through global paths, we add the "empty data directory"
    // This data dir will manage all accesses through absolute paths, unless any other data directory can handle them
    // since we don't add any further data dirs, this is it
    xiiFileSystem::AddDataDirectory("", "", ":", xiiDataDirUsage::AllowWrites).IgnoreResult();
  }

  virtual xiiApplication::Execution Run() override
  {
    const xiiTime t0 = xiiTime::Now();

    xiiUInt32 uiDirectories = 0;
    xiiUInt32 uiFiles       = 0;

    // Exit if any error was encountered
    if (m_bErrorEncountered)
    {
      xiiLog::Info("Encountered an error with source and export directory paths.");

      const xiiTime t1    = xiiTime::Now();
      const xiiTime tdiff = t1 - t0;

      xiiLog::Info("");
      xiiLog::Info("Directories Read: {0}  -  Files Read: {1}", uiDirectories, uiFiles);
      xiiLog::Info("Export complete with time {}", tdiff);

      return xiiApplication::Execution::Quit;
    }

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

    xiiStringBuilder sPrintStr;

    // Store a set of extensions for files that will be copied (typically in lowercase)
    xiiSet<xiiString> ExtensionsSet;
    ExtensionsSet.Insert(".h");
    ExtensionsSet.Insert(".txt");
    ExtensionsSet.Insert(".json");
    ExtensionsSet.Insert(".cmake");
    ExtensionsSet.Insert("_inl.h");
    ExtensionsSet.Insert(".natvis");
    ExtensionsSet.Insert("PCH.cpp");
    ExtensionsSet.Insert("ArchitectureDetect.c");

    // Store a set of extensions for files that will be ignored
    xiiSet<xiiString> IgnoreExtensionsSet;
    // IgnoreExtensionsSet.Insert("_inl.h");

    // Get a directory iterator for the search directory
    xiiFileSystemIterator it;
    it.StartSearch(m_sSourceDirectory, xiiFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

    if (it.IsValid())
    {
      xiiStringBuilder sPath, sExt;

      // While there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // Build the absolute path to the current file
        sPath = it.GetCurrentPath();
        sPath.AppendPath(it.GetStats().m_sName.GetData());

        // Log current directory / file
        // xiiLog::Info("{0}: {1}", it.GetStats().m_bIsDirectory ? "Directory" : "File", sPath);

        if (it.GetStats().m_bIsDirectory)
        {
          ++uiDirectories;
        }
        else
        {
          // File extensions are always converted to lowercase
          sExt = sPath.GetFileExtension();

          bool bShouldSkip = true;

          // Do not skip if a valid extension is found
          for (auto it = ExtensionsSet.GetIterator(); it.IsValid(); it.Next())
          {
            if (sPath.EndsWith_NoCase(it.Key()))
            {
              bShouldSkip = false;
              break;
            }
          }

          // Skip path endings that are marked as ignored
          for (auto it = IgnoreExtensionsSet.GetIterator(); it.IsValid(); it.Next())
          {
            if (sPath.EndsWith_NoCase(it.Key()))
            {
              bShouldSkip = true;
              break;
            }
          }

          if (bShouldSkip)
            continue;

          // Strip current source path and append the export path
          xiiStringBuilder sExportDirectoryPath = sPath;
          sExportDirectoryPath.ReplaceFirst(m_sSourceDirectory, m_sExportDirectory);
          sExportDirectoryPath.MakeCleanPath();

          xiiLog::Info("Copying file '{0}' to '{1}'", sPath, sExportDirectoryPath);

          if (xiiOSFile::ExistsFile(sExportDirectoryPath))
          {
            // Skip existing files. Should typically be run on a fresh directory.
            continue;

            // Delete file it already exists so we can update it
            // xiiOSFile::DeleteFile(sExportDirectoryPath).AssertSuccess("Failed to delete existing file at '{}'", sPath);
          }

          sPrintStr.SetFormat("Failed to copy file '{}' to '{}'", sPath, sExportDirectoryPath);
          xiiOSFile::CopyFile(sPath, sExportDirectoryPath).AssertSuccess(sPrintStr);

          ++uiFiles;
        }
      }
    }
    else
    {
      xiiLog::Error("Could export engine headers from the directory '{0}'", m_sSourceDirectory);
    }
#else
    XII_REPORT_FAILURE("No file system iterator support, Header Tool cannot be run.");
#endif

    const xiiTime t1    = xiiTime::Now();
    const xiiTime tdiff = t1 - t0;

    xiiLog::Info("");
    xiiLog::Info("Directories Read: {0}  -  Files Read: {1}", uiDirectories, uiFiles);
    xiiLog::Info("Export complete with time {}", tdiff);

    return xiiApplication::Execution::Quit;
  }
};

XII_CONSOLEAPP_ENTRY_POINT(xiiHeaderTool);
