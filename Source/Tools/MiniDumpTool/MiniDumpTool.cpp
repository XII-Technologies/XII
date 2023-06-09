#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

xiiCommandLineOptionInt opt_PID("_MiniDumpTool", "-PID", "Process ID of the application for which to create a crash dump.", 0);

xiiCommandLineOptionPath opt_DumpFile("_MiniDumpTool", "-f", "Path to the crash dump file to write.", "");

class xiiMiniDumpTool : public xiiApplication
{
  xiiUInt32        m_uiProcessID = 0;
  xiiStringBuilder m_sDumpFile;

public:
  using SUPER = xiiApplication;

  xiiMiniDumpTool() :
    xiiApplication("MiniDumpTool")
  {
  }

  xiiResult ParseArguments()
  {
    xiiCommandLineUtils* cmd = xiiCommandLineUtils::GetGlobalInstance();

    m_uiProcessID = cmd->GetUIntOption("-PID");

    m_sDumpFile = opt_DumpFile.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    m_sDumpFile.MakeCleanPath();

    if (m_uiProcessID == 0)
    {
      xiiLog::Error("Missing '-PID' argument");
      return XII_FAILURE;
    }

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

  virtual Execution Run() override
  {
    {
      xiiStringBuilder cmdHelp;
      if (xiiCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_MiniDumpTool"))
      {
        xiiLog::Print(cmdHelp);
        return xiiApplication::Execution::Quit;
      }
    }

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      return xiiApplication::Execution::Quit;
    }

    xiiMiniDumpUtils::WriteExternalProcessMiniDump(m_sDumpFile, m_uiProcessID);
    return xiiApplication::Execution::Quit;
  }
};

XII_CONSOLEAPP_ENTRY_POINT(xiiMiniDumpTool);
