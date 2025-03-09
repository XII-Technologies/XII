#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

xiiString xiiQtEditorApp::FindToolApplication(const char* szToolName)
{
  xiiStringBuilder toolExe = szToolName;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  toolExe.ChangeFileExtension("exe");
#else
  toolExe.RemoveFileExtension();
#endif

  szToolName = toolExe;

  xiiEditorPreferencesUser* pPref = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();

  xiiHybridArray<xiiString, 3> sFolders;

  if (pPref->m_bUsePrecompiledTools)
  {
    if (!pPref->m_sCustomPrecompiledToolsFolder.IsEmpty() && xiiOSFile::ExistsDirectory(pPref->m_sCustomPrecompiledToolsFolder))
    {
      xiiStringBuilder customToolsFolder = pPref->m_sCustomPrecompiledToolsFolder;
      customToolsFolder.MakeCleanPath();
      sFolders.PushBack(customToolsFolder);
    }

    sFolders.PushBack(xiiApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(true));
    sFolders.PushBack(xiiApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(false));
  }
  else
  {
    sFolders.PushBack(xiiApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(false));
    sFolders.PushBack(xiiApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(true));
  }

  xiiStringBuilder sTool;
  for (auto& folder : sFolders)
  {
    sTool = folder;
    sTool.AppendPath(szToolName);

    if (xiiOSFile::ExistsFile(sTool))
      return sTool;
  }

  // just try the one in the same folder as the editor
  return szToolName;
}

xiiStatus xiiQtEditorApp::ExecuteTool(const char* szTool, const QStringList& arguments, xiiUInt32 uiSecondsTillTimeout, xiiLogInterface* pLogOutput /*= nullptr*/, xiiLogMsgType::Enum logLevel /*= xiiLogMsgType::InfoMsg*/, const char* szCWD /*= nullptr*/)
{
  // this block is supposed to be in the global log, not the given log interface
  XII_LOG_BLOCK("Executing Tool", szTool);

  xiiStringBuilder toolExe = szTool;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  toolExe.ChangeFileExtension("exe");
#else
  toolExe.RemoveFileExtension();
#endif

  szTool = toolExe;

  xiiStringBuilder cmd;
  for (xiiInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  xiiLog::Debug("{}{}", szTool, cmd);


  QProcess proc;

  if (szCWD != nullptr)
  {
    proc.setWorkingDirectory(szCWD);
  }

  QString logoutput;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setReadChannel(QProcess::StandardOutput);
  QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&proc, &logoutput]() { logoutput.append(proc.readAllStandardOutput()); });
  xiiString toolPath = xiiQtEditorApp::GetSingleton()->FindToolApplication(szTool);
  proc.start(QString::fromUtf8(toolPath, toolPath.GetElementCount()), arguments);

  if (!proc.waitForStarted(uiSecondsTillTimeout * 1000))
    return xiiStatus(xiiFmt("{0} could not be started", szTool));

  if (!proc.waitForFinished(uiSecondsTillTimeout * 1000))
    return xiiStatus(xiiFmt("{0} timed out", szTool));

  if (pLogOutput)
  {
    xiiStringBuilder tmp;

    struct LogBlockData
    {
      LogBlockData(xiiLogInterface* pInterface, const char* szName) :
        m_Name(szName), m_Block(pInterface, m_Name)
      {
      }

      xiiString   m_Name;
      xiiLogBlock m_Block;
    };

    xiiHybridArray<xiiUniquePtr<LogBlockData>, 8> blocks;

    QTextStream logoutputStream(&logoutput);
    while (!logoutputStream.atEnd())
    {
      tmp = logoutputStream.readLine().toUtf8().data();
      tmp.Trim(" \n");

      const char*         szMsg   = nullptr;
      xiiLogMsgType::Enum msgType = xiiLogMsgType::None;

      if (tmp.StartsWith("Error: "))
      {
        szMsg   = &tmp.GetData()[7];
        msgType = xiiLogMsgType::ErrorMsg;
      }
      else if (tmp.StartsWith("Warning: "))
      {
        szMsg   = &tmp.GetData()[9];
        msgType = xiiLogMsgType::WarningMsg;
      }
      else if (tmp.StartsWith("Seriously: "))
      {
        szMsg   = &tmp.GetData()[11];
        msgType = xiiLogMsgType::SeriousWarningMsg;
      }
      else if (tmp.StartsWith("Success: "))
      {
        szMsg   = &tmp.GetData()[9];
        msgType = xiiLogMsgType::SuccessMsg;
      }
      else if (tmp.StartsWith("+++++ "))
      {
        tmp.Trim("+ ");
        if (tmp.EndsWith("()"))
          tmp.Trim("() ");

        szMsg = tmp.GetData();
        blocks.PushBack(XII_DEFAULT_NEW(LogBlockData, pLogOutput, szMsg));
        continue;
      }
      else if (tmp.StartsWith("----- "))
      {
        if (!blocks.IsEmpty())
          blocks.PopBack();

        continue;
      }
      else
      {
        szMsg   = &tmp.GetData()[0];
        msgType = xiiLogMsgType::InfoMsg;

        // TODO: output all logged data in one big message, if the tool failed
      }

      if (msgType > logLevel || szMsg == nullptr)
        continue;

      xiiLog::BroadcastLoggingEvent(pLogOutput, msgType, szMsg);
    }

    blocks.Clear();
  }

  if (proc.exitStatus() == QProcess::ExitStatus::CrashExit)
  {
    return xiiStatus(xiiFmt("{0} crashed during execution", szTool));
  }
  else if (proc.exitCode() != 0)
  {
    return xiiStatus(xiiFmt("{0} returned error code {1}", szTool, proc.exitCode()));
  }

  return xiiStatus(XII_SUCCESS);
}

xiiString xiiQtEditorApp::BuildFileserveCommandLine() const
{
  const xiiStringBuilder sToolPath   = xiiQtEditorApp::GetSingleton()->FindToolApplication("xiiFileserve");
  const xiiStringBuilder sProjectDir = xiiToolsProject::GetSingleton()->GetProjectDirectory();
  xiiStringBuilder       params;

  xiiStringBuilder cmd;
  cmd.Set(sToolPath, " -specialdirs project \"", sProjectDir, "\"");

  return cmd;
}

void xiiQtEditorApp::RunFileserve()
{
  const xiiStringBuilder sToolPath   = xiiQtEditorApp::GetSingleton()->FindToolApplication("xiiFileserve");
  const xiiStringBuilder sProjectDir = xiiToolsProject::GetSingleton()->GetProjectDirectory();

  QStringList args;
  args << "-specialdirs" << "project" << sProjectDir.GetData() << "-fs_start";

  QProcess::startDetached(sToolPath.GetData(), args);
}

void xiiQtEditorApp::RunInspector()
{
  const xiiStringBuilder sToolPath = xiiQtEditorApp::GetSingleton()->FindToolApplication("xiiInspector");
  QStringList            args;

  QProcess::startDetached(sToolPath.GetData(), args);
}

void xiiQtEditorApp::RunTracy()
{
#if BUILDSYSTEM_ENABLE_TRACY_SUPPORT == 0
  xiiQtUiServices::MessageBoxInformation("<html>This build of XII was compiled without support for Tracy profiling.<br><br>See <a href='https://xiiengine.net/pages/docs/debugging/tracy.html'>the documentation</a> for how to enable it.</html>");
#else
  const xiiStringBuilder sToolPath = xiiQtEditorApp::GetSingleton()->FindToolApplication("tracy-profiler");
  QStringList            args;

  QProcess::startDetached(sToolPath.GetData(), args);
#endif
}
