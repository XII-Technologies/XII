#include <Foundation/Application/Application.h>
#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>

/* ArchiveTool command line options:

-out <path>
    Path to a file or folder.

    -out specifies the target to pack or unpack things to.
    For packing mode it has to be a file. The file will be overwritten, if it already exists.
    For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.

    If no -out is specified, it is determined to be where the input file is located.

-unpack <paths>
    One or multiple paths to xiiArchive files that shall be extracted.

    Example:
      -unpack "path/to/file.xiiArchive" "another/file.xiiArchive"

-pack <paths>
    One or multiple paths to folders that shall be packed.

    Example:
      -pack "path/to/folder" "path/to/another/folder"

Description:
    -pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)
    or to unpack multiple archives at the same time.

    If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.
    If all inputs are folders, the mode is 'pack'.
    If all inputs are files, the mode is 'unpack'.

Examples:
    xiiArchiveTool.exe "C:/Stuff"
      Packs all data in "C:/Stuff" into "C:/Stuff.xiiArchive"

    xiiArchiveTool.exe "C:/Stuff" -out "C:/MyStuff.xiiArchive"
      Packs all data in "C:/Stuff" into "C:/MyStuff.xiiArchive"

    xiiArchiveTool.exe "C:/Stuff.xiiArchive"
      Unpacks all data from the archive into "C:/Stuff"

    xiiArchiveTool.exe "C:/Stuff.xiiArchive" -out "C:/MyStuff"
      Unpacks all data from the archive into "C:/MyStuff"
*/

xiiCommandLineOptionPath opt_Out("_ArchiveTool", "-out", "\
Path to a file or folder.\n\
\n\
-out specifies the target to pack or unpack things to.\n\
For packing mode it has to be a file. The file will be overwritten, if it already exists.\n\
For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.\n\
\n\
If no -out is specified, it is determined to be where the input file is located.\n\
",
                                 "");

xiiCommandLineOptionDoc opt_Unpack("_ArchiveTool", "-unpack", "<paths>", "\
One or multiple paths to xiiArchive files that shall be extracted.\n\
\n\
Example:\n\
  -unpack \"path/to/file.xiiArchive\" \"another/file.xiiArchive\"\n\
",
                                   "");

xiiCommandLineOptionDoc opt_Pack("_ArchiveTool", "-pack", "<paths>", "\
One or multiple paths to folders that shall be packed.\n\
\n\
Example:\n\
  -pack \"path/to/folder\" \"path/to/another/folder\"\n\
",
                                 "");

xiiCommandLineOptionDoc opt_Desc("_ArchiveTool", "Description:", "", "\
-pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)\n\
or to unpack multiple archives at the same time.\n\
\n\
If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.\n\
If all inputs are folders, the mode is 'pack'.\n\
If all inputs are files, the mode is 'unpack'.\n\
",
                                 "");

xiiCommandLineOptionDoc opt_Examples("_ArchiveTool", "Examples:", "", "\
xiiArchiveTool.exe \"C:/Stuff\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/Stuff.xiiArchive\"\n\
\n\
xiiArchiveTool.exe \"C:/Stuff\" -out \"C:/MyStuff.xiiArchive\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/MyStuff.xiiArchive\"\n\
\n\
xiiArchiveTool.exe \"C:/Stuff.xiiArchive\"\n\
  Unpacks all data from the archive into \"C:/Stuff\"\n\
\n\
xiiArchiveTool.exe \"C:/Stuff.xiiArchive\" -out \"C:/MyStuff\"\n\
  Unpacks all data from the archive into \"C:/MyStuff\"\n\
",
                                     "");

class xiiArchiveBuilderImpl : public xiiArchiveBuilder
{
protected:
  virtual void WriteFileResultCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, xiiStringView sSourceFile, xiiUInt64 uiSourceSize, xiiUInt64 uiStoredSize, xiiTime duration) const override
  {
    const xiiUInt64 uiPercentage = (uiSourceSize == 0) ? 100 : (uiStoredSize * 100 / uiSourceSize);
    xiiLog::Info(" [{}%%] {} ({}%%) - {}", xiiArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile, uiPercentage, duration);
  }
};

class xiiArchiveReaderImpl : public xiiArchiveReader
{
public:
protected:
  virtual bool ExtractNextFileCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, xiiStringView sSourceFile) const override
  {
    xiiLog::Info(" [{}%%] {}", xiiArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile);
    return true;
  }


  virtual bool ExtractFileProgressCallback(xiiUInt64 bytesWritten, xiiUInt64 bytesTotal) const override
  {
    // xiiLog::Dev("   {}%%", xiiArgU(100 * bytesWritten / bytesTotal));
    return true;
  }
};

class xiiArchiveTool : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  enum class ArchiveMode
  {
    Auto,
    Pack,
    Unpack,
  };

  ArchiveMode m_Mode = ArchiveMode::Auto;

  xiiDynamicArray<xiiString> m_sInputs;
  xiiString                  m_sOutput;

  xiiArchiveTool() :
    xiiApplication("ArchiveTool")
  {
  }

  xiiResult ParseArguments()
  {
    if (GetArgumentCount() <= 1)
    {
      xiiLog::Error("No arguments given");
      return XII_FAILURE;
    }

    xiiCommandLineUtils& cmd = *xiiCommandLineUtils::GetGlobalInstance();

    m_sOutput = opt_Out.GetOptionValue(xiiCommandLineOption::LogMode::Always);

    xiiStringBuilder path;

    if (cmd.GetStringOptionArguments("-pack") > 0)
    {
      m_Mode               = ArchiveMode::Pack;
      const xiiUInt32 args = cmd.GetStringOptionArguments("-pack");

      if (args == 0)
      {
        xiiLog::Error("-pack option expects at least one argument");
        return XII_FAILURE;
      }

      for (xiiUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-pack", a));

        if (!xiiOSFile::ExistsDirectory(m_sInputs.PeekBack()))
        {
          xiiLog::Error("-pack input path is not a valid directory: '{}'", m_sInputs.PeekBack());
          return XII_FAILURE;
        }
      }
    }
    else if (cmd.GetStringOptionArguments("-unpack") > 0)
    {
      m_Mode               = ArchiveMode::Unpack;
      const xiiUInt32 args = cmd.GetStringOptionArguments("-unpack");

      if (args == 0)
      {
        xiiLog::Error("-unpack option expects at least one argument");
        return XII_FAILURE;
      }

      for (xiiUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-unpack", a));

        if (!xiiOSFile::ExistsFile(m_sInputs.PeekBack()))
        {
          xiiLog::Error("-unpack input file does not exist: '{}'", m_sInputs.PeekBack());
          return XII_FAILURE;
        }
      }
    }
    else
    {
      bool bInputsFolders = true;
      bool bInputsFiles   = true;

      for (xiiUInt32 a = 1; a < GetArgumentCount(); ++a)
      {
        xiiStringView sArg = GetArgument(a);

        if (sArg.IsEqual_NoCase("-out"))
          break;

        m_sInputs.PushBack(xiiOSFile::MakePathAbsoluteWithCWD(sArg));

        if (!xiiOSFile::ExistsDirectory(m_sInputs.PeekBack()))
          bInputsFolders = false;
        if (!xiiOSFile::ExistsFile(m_sInputs.PeekBack()))
          bInputsFiles = false;
      }

      if (bInputsFolders && !bInputsFiles)
      {
        m_Mode = ArchiveMode::Pack;
      }
      else if (bInputsFiles && !bInputsFolders)
      {
        m_Mode = ArchiveMode::Unpack;
      }
      else
      {
        xiiLog::Error("Inputs are ambiguous. Specify only folders for packing or only files for unpacking. Use -out as last argument to "
                      "specify a target.");
        return XII_FAILURE;
      }
    }

    xiiLog::Info("Mode is: {}", m_Mode == ArchiveMode::Pack ? "pack" : "unpack");
    xiiLog::Info("Inputs:");

    for (const auto& input : m_sInputs)
    {
      xiiLog::Info("  '{}'", input);
    }

    xiiLog::Info("Output: '{}'", m_sOutput);

    return XII_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add the empty data directory to access files via absolute paths
    xiiFileSystem::AddDataDirectory("", "App", ":", xiiFileSystem::AllowWrites).IgnoreResult();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    xiiGlobalLog::RemoveLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::RemoveLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  static xiiArchiveBuilder::InclusionMode PackFileCallback(xiiStringView sFile)
  {
    const xiiStringView ext = xiiPathUtils::GetFileExtension(sFile);

    if (ext.IsEqual_NoCase("jpg") || ext.IsEqual_NoCase("jpeg") || ext.IsEqual_NoCase("png"))
      return xiiArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("zip") || ext.IsEqual_NoCase("7z"))
      return xiiArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("mp3") || ext.IsEqual_NoCase("ogg"))
      return xiiArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("dds"))
      return xiiArchiveBuilder::InclusionMode::Compress_zstd_fast;

    return xiiArchiveBuilder::InclusionMode::Compress_zstd_average;
  }

  xiiResult Pack()
  {
    xiiArchiveBuilderImpl archive;

    for (const auto& folder : m_sInputs)
    {
      archive.AddFolder(folder, xiiArchiveCompressionMode::Compressed_zstd, PackFileCallback);
    }

    if (m_sOutput.IsEmpty())
    {
      xiiStringBuilder sArchive = m_sInputs[0];
      sArchive.Append(".xiiArchive");

      m_sOutput = sArchive;
    }

    m_sOutput = xiiOSFile::MakePathAbsoluteWithCWD(m_sOutput);

    xiiLog::Info("Writing archive to '{}'", m_sOutput);
    if (archive.WriteArchive(m_sOutput).Failed())
    {
      xiiLog::Error("Failed to write the xiiArchive");

      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  xiiResult Unpack()
  {
    for (const auto& file : m_sInputs)
    {
      xiiLog::Info("Extracting archive '{}'", file);

      // if the file has a custom archive file extension, just register it as 'allowed'
      // we assume that the user only gives us files that are xiiArchives
      if (!xiiArchiveUtils::IsAcceptedArchiveFileExtensions(xiiPathUtils::GetFileExtension(file)))
      {
        xiiArchiveUtils::GetAcceptedArchiveFileExtensions().PushBack(xiiPathUtils::GetFileExtension(file));
      }

      xiiArchiveReaderImpl reader;
      XII_SUCCEED_OR_RETURN(reader.OpenArchive(file));

      xiiStringBuilder sOutput = m_sOutput;

      if (sOutput.IsEmpty())
      {
        sOutput = file;
        sOutput.RemoveFileExtension();
      }

      if (reader.ExtractAllFiles(sOutput).Failed())
      {
        xiiLog::Error("File extraction failed.");
        return XII_FAILURE;
      }
    }

    return XII_SUCCESS;
  }

  virtual Execution Run() override
  {
    {
      xiiStringBuilder cmdHelp;
      if (xiiCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_ArchiveTool"))
      {
        xiiLog::Print(cmdHelp);
        return xiiApplication::Execution::Quit;
      }
    }

    xiiStopwatch sw;

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      return xiiApplication::Execution::Quit;
    }

    if (m_Mode == ArchiveMode::Pack)
    {
      if (Pack().Failed())
      {
        xiiLog::Error("Packaging files failed");
        SetReturnCode(2);
      }

      xiiLog::Success("Finished packing archive in {}", sw.GetRunningTotal());
      return xiiApplication::Execution::Quit;
    }

    if (m_Mode == ArchiveMode::Unpack)
    {
      if (Unpack().Failed())
      {
        xiiLog::Error("Extracting files failed");
        SetReturnCode(3);
      }

      xiiLog::Success("Finished extracting archive in {}", sw.GetRunningTotal());
      return xiiApplication::Execution::Quit;
    }

    xiiLog::Error("Unknown mode");
    return xiiApplication::Execution::Quit;
  }
};

XII_CONSOLEAPP_ENTRY_POINT(xiiArchiveTool);
