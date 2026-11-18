/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

  m_pServer = XII_DEFAULT_NEW(xiiFileserver);

  m_pServer->m_Events.AddEventHandler(xiiMakeDelegate(&xiiFileserverApp::FileserverEventHandler, this));

#ifndef XII_USE_QT
  m_pServer->m_Events.AddEventHandler(xiiMakeDelegate(&xiiFileserverApp::FileserverEventHandlerConsole, this));
  m_pServer->StartServer();
  if (!m_pServer->IsServerRunning())
  {
    SetReturnCode(1);
    RequestQuit();
    return;
  }
#endif

  xiiPlugin::LoadPlugin("xiiShaderCompiler", xiiPluginLoadFlags::PluginIsOptional).IgnoreResult();

  m_pServer->SetShaderCompileHandler(xiiMakeDelegate(&xiiFileserverApp::CompileShader, this));

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
  m_pServer->StopServer();
  m_pServer.Clear();

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

  m_pServer->UpdateServer(xiiTime::MakeFromMilliseconds(10));

  return xiiApplication::Execution::Continue;
}
