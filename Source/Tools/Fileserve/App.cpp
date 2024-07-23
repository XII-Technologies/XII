#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Utilities/CommandLineUtils.h>

void xiiFileserverApp::AfterCoreSystemsStartup()
{
  xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

  // Add the empty data directory to access files via absolute paths
  xiiFileSystem::AddDataDirectory("", "App", ":", xiiDataDirUsage::AllowWrites).IgnoreResult();

  XII_DEFAULT_NEW(xiiFileserver);

  xiiFileserver::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiFileserverApp::FileserverEventHandler, this));

#ifndef XII_USE_QT
  xiiFileserver::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiFileserverApp::FileserverEventHandlerConsole, this));
  xiiFileserver::GetSingleton()->StartServer();
#endif

  // TODO: CommandLine Option
  m_CloseAppTimeout = xiiTime::MakeFromSeconds(xiiCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_close_timeout", 0));
  m_TimeTillClosing = xiiTime::MakeFromSeconds(xiiCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_wait_timeout", 0));

  if (m_TimeTillClosing.GetSeconds() > 0)
  {
    m_TimeTillClosing += xiiTime::Now();
  }
}

void xiiFileserverApp::BeforeCoreSystemsShutdown()
{
  xiiFileserver::GetSingleton()->StopServer();

#ifndef XII_USE_QT
  xiiFileserver::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiFileserverApp::FileserverEventHandlerConsole, this));
#endif

  xiiFileserver::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiFileserverApp::FileserverEventHandler, this));

  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

xiiApplication::Execution xiiFileserverApp::Run()
{
  // if there are no more connections, and we have a timeout to close when no connections are left, we return Quit
  if (m_uiConnections == 0 && m_TimeTillClosing > xiiTime::MakeFromSeconds(0) && xiiTime::Now() > m_TimeTillClosing)
  {
    return xiiApplication::Execution::Quit;
  }

  if (xiiFileserver::GetSingleton()->UpdateServer() == false)
  {
    m_uiSleepCounter++;

    if (m_uiSleepCounter > 1000)
    {
      // only sleep when no work had to be done in a while
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    else if (m_uiSleepCounter > 10)
    {
      // only sleep when no work had to be done in a while
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    }
  }
  else
  {
    m_uiSleepCounter = 0;
  }

  return xiiApplication::Execution::Continue;
}
