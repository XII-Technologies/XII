#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#include <QFileDialog>

XII_IMPLEMENT_SINGLETON(xiiQtEditorApp);

xiiEvent<const xiiEditorAppEvent&> xiiQtEditorApp::m_Events;

xiiQtEditorApp::xiiQtEditorApp() :
  m_SingletonRegistrar(this), m_RecentProjects(20), m_RecentDocuments(100)
{
  m_bSavePreferencesAfterOpenProject = false;
  m_pVersionChecker                  = XII_DEFAULT_NEW(xiiQtVersionChecker);

  m_pTimer = new QTimer(nullptr);
}

xiiQtEditorApp::~xiiQtEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;

  CloseSplashScreen();
}

xiiInt32 xiiQtEditorApp::RunEditor()
{
  xiiInt32 ret = m_pQtApplication->exec();
  return ret;
}

void xiiQtEditorApp::SlotTimedUpdate()
{
  if (xiiToolsProject::IsProjectOpen())
  {
    if (xiiEditorEngineProcessConnection::GetSingleton())
      xiiEditorEngineProcessConnection::GetSingleton()->Update();

    xiiAssetCurator::GetSingleton()->MainThreadTick(true);
  }
  xiiTaskSystem::FinishFrameTasks();

  // Close the splash screen when we get to the first idle event
  CloseSplashScreen();

  Q_EMIT IdleEvent();

  RestartEngineProcessIfPluginsChanged(false);

  m_pTimer->start(1);
}

void xiiQtEditorApp::SlotSaveSettings()
{
  SaveSettings();
}

void xiiQtEditorApp::SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced)
{
  // Close the splash screen so it doesn't become the parent window of our message boxes.
  CloseSplashScreen();

  if (bForced || bNewVersionReleased)
  {
    if (m_pVersionChecker->IsLatestNewer())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation(xiiFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A href=\"https://github.com/xiiEngine/xiiEngine/releases\">Releases</A> for details.</html>",
                                                                    m_pVersionChecker->GetKnownLatestVersion(), m_pVersionChecker->GetOwnVersion()));
    }
    else
    {
      xiiStringBuilder tmp("You have the latest version: \n");
      tmp.Append(m_pVersionChecker->GetOwnVersion());

      xiiQtUiServices::GetSingleton()->MessageBoxInformation(tmp);
    }
  }

  if (m_pVersionChecker->IsLatestNewer())
  {
    xiiQtUiServices::GetSingleton()->ShowGlobalStatusBarMessage(xiiFmt("New version '{}' available, please update.", m_pVersionChecker->GetKnownLatestVersion()));
  }
}

void xiiQtEditorApp::EngineProcessMsgHandler(const xiiEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case xiiEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (auto pTypeMsg = xiiDynamicCast<const xiiUpdateReflectionTypeMsgToEditor*>(e.m_pMsg))
      {
        xiiPhantomRttiManager::RegisterType(pTypeMsg->m_desc);
      }
      if (auto pDynEnumMsg = xiiDynamicCast<const xiiDynamicStringEnumMsgToEditor*>(e.m_pMsg))
      {
        auto& dynEnum = xiiDynamicStringEnum::CreateDynamicEnum(pDynEnumMsg->m_sEnumName);
        for (auto& sEnumValue : pDynEnumMsg->m_EnumValues)
        {
          dynEnum.AddValidValue(sEnumValue);
        }
        dynEnum.SortValues();
      }
      else if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiProjectReadyMsgToEditor>())
      {
        // This message is waited upon (blocking) but does not contain any data.
      }
    }
    break;

    case xiiEditorEngineProcessConnection::Event::Type::ProcessRestarted:
      StoreEnginePluginModificationTimes();
      break;

    default:
      return;
  }
}

void xiiQtEditorApp::UiServicesEvents(const xiiQtUiServices::Event& e)
{
  if (e.m_Type == xiiQtUiServices::Event::Type::CheckForUpdates)
  {
    m_pVersionChecker->Check(true);
  }
}

void xiiQtEditorApp::SaveAllOpenDocuments()
{
  for (auto pMan : xiiDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : pMan->xiiDocumentManager::GetAllOpenDocuments())
    {
      xiiQtDocumentWindow* pWnd = xiiQtDocumentWindow::FindWindowByDocument(pDoc);
      // Layers for example will share a window with the scene document and the window will always save the scene.
      if (pWnd && pWnd->GetDocument() == pDoc)
      {
        if (pWnd->SaveDocument().m_Result.Failed())
          return;
      }
      // There might be no window for this document.
      else
      {
        pDoc->SaveDocument().LogFailure();
      }
    }
  }
}

bool xiiQtEditorApp::IsProgressBarProcessingEvents() const
{
  return m_pQtProgressbar != nullptr && m_pQtProgressbar->IsProcessingEvents();
}

void xiiQtEditorApp::OnDemandDynamicStringEnumLoad(xiiStringView sEnumName, xiiDynamicStringEnum& e)
{
  xiiStringBuilder sFile;
  sFile.SetFormat(":project/Editor/{}.txt", sEnumName);

  // enums loaded this way are user editable
  e.SetStorageFile(sFile);
  e.ReadFromStorage();

  m_DynamicEnumStringsToClear.Insert(sEnumName);
}

bool ContainsPlugin(const xiiDynamicArray<xiiApplicationPluginConfig::PluginConfig>& all, const char* szPlugin)
{
  for (const xiiApplicationPluginConfig::PluginConfig& one : all)
  {
    if (one.m_sAppDirRelativePath == szPlugin)
      return true;
  }

  return false;
}

xiiResult xiiQtEditorApp::AddBundlesInOrder(xiiDynamicArray<xiiApplicationPluginConfig::PluginConfig>& order, const xiiPluginBundleSet& bundles, const xiiString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const
{
  const xiiPluginBundle& bundle = bundles.m_Plugins.Find(start).Value();

  for (const xiiString& req : bundle.m_RequiredBundles)
  {
    auto it = bundles.m_Plugins.Find(req);

    if (!it.IsValid())
    {
      xiiLog::Error("Plugin bundle '{}' has a dependency on bundle '{}' which does not exist.", start, req);
      return XII_FAILURE;
    }

    XII_SUCCEED_OR_RETURN(AddBundlesInOrder(order, bundles, req, bEditor, bEditorEngine, bRuntime));
  }

  if (bRuntime)
  {
    for (const xiiString& dll : bundle.m_RuntimePlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        xiiApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath                     = dll;
        p.m_bLoadCopy                               = bundle.m_bLoadCopy;
      }
    }
  }

  if (bEditorEngine)
  {
    for (const xiiString& dll : bundle.m_EditorEnginePlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        xiiApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath                     = dll;
        p.m_bLoadCopy                               = bundle.m_bLoadCopy;
      }
    }
  }

  if (bEditor)
  {
    for (const xiiString& dll : bundle.m_EditorPlugins)
    {
      if (!ContainsPlugin(order, dll))
      {
        xiiApplicationPluginConfig::PluginConfig& p = order.ExpandAndGetRef();
        p.m_sAppDirRelativePath                     = dll;
        p.m_bLoadCopy                               = bundle.m_bLoadCopy;
      }
    }
  }

  return XII_SUCCESS;
}

xiiStatus xiiQtEditorApp::MakeRemoteProjectLocal(xiiStringBuilder& inout_sFilePath)
{
  // already a local project?
  if (inout_sFilePath.EndsWith_NoCase("xiiProject"))
    return xiiStatus(XII_SUCCESS);

  {
    xiiStringBuilder tmp = inout_sFilePath;
    tmp.AppendPath("xiiProject");

    if (xiiOSFile::ExistsFile(tmp))
    {
      inout_sFilePath = tmp;
      return xiiStatus(XII_SUCCESS);
    }
  }

  XII_LOG_BLOCK("Open Remote Project", inout_sFilePath.GetData());

  xiiStringBuilder sRedirFile = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder(inout_sFilePath);
  sRedirFile.AppendPath("LocalCheckout.txt");

  // read redirection file, if available
  {
    xiiOSFile file;
    if (file.Open(sRedirFile, xiiFileOpenMode::Read).Succeeded())
    {
      xiiDataBuffer content;
      file.ReadAll(content);

      const xiiStringView sContent((const char*)content.GetData(), content.GetCount());

      if (sContent.EndsWith_NoCase("xiiProject") && xiiOSFile::ExistsFile(sContent))
      {
        inout_sFilePath = sContent;
        return xiiStatus(XII_SUCCESS);
      }
    }
  }

  xiiString sName;
  xiiString sType;
  xiiString sUrl;
  xiiString sProjectFile;

  // read the info about the remote project from the OpenDDL config file
  {
    xiiOSFile file;
    if (file.Open(inout_sFilePath, xiiFileOpenMode::Read).Failed())
    {
      return xiiStatus(xiiFmt("Remote project file '{}' doesn't exist.", inout_sFilePath));
    }

    xiiDataBuffer content;
    file.ReadAll(content);

    xiiMemoryStreamContainerWrapperStorage<xiiDataBuffer> storage(&content);
    xiiMemoryStreamReader                                 reader(&storage);

    xiiOpenDdlReader ddl;
    if (ddl.ParseDocument(reader).Failed())
    {
      return xiiStatus("Error in remote project DDL config file");
    }

    if (auto pRoot = ddl.GetRootElement())
    {
      if (auto pProject = pRoot->FindChildOfType("RemoteProject"))
      {
        if (auto pName = pProject->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Name"))
        {
          sName = pName->GetPrimitivesString()[0];
        }
        if (auto pType = pProject->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Type"))
        {
          sType = pType->GetPrimitivesString()[0];
        }
        if (auto pUrl = pProject->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Url"))
        {
          sUrl = pUrl->GetPrimitivesString()[0];
        }
        if (auto pProjectFile = pProject->FindChildOfType(xiiOpenDdlPrimitiveType::String, "ProjectFile"))
        {
          sProjectFile = pProjectFile->GetPrimitivesString()[0];
        }
      }
    }
  }

  if (sType.IsEmpty() || sName.IsEmpty())
  {
    return xiiStatus(xiiFmt("Remote project '{}' DDL configuration is invalid.", inout_sFilePath));
  }

  xiiQtUiServices::GetSingleton()->MessageBoxInformation("This is a 'remote' project, meaning the data is not yet available on your machine.\n\nPlease select a folder where the project should be downloaded to.");

  static QString sPreviousFolder = xiiOSFile::GetUserDocumentsFolder().GetData();

  QString sSelectedDir = QFileDialog::getExistingDirectory(QApplication::activeWindow(), QLatin1String("Choose Folder"), sPreviousFolder, QFileDialog::Option::ShowDirsOnly | QFileDialog::Option::DontResolveSymlinks);

  if (sSelectedDir.isEmpty())
  {
    return xiiStatus("");
  }

  sPreviousFolder             = sSelectedDir;
  xiiStringBuilder sTargetDir = sSelectedDir.toUtf8().data();

  // if it is a git repository, clone it
  if (sType == "git" && !sUrl.IsEmpty())
  {
    QStringList args;
    args << "clone";
    args << xiiMakeQString(sUrl);
    args << xiiMakeQString(sName);

    QProcess proc;
    proc.setWorkingDirectory(sTargetDir.GetData());
    proc.start("git.exe", args);

    if (!proc.waitForStarted())
    {
      return xiiStatus(xiiFmt("Running 'git' to download the remote project failed."));
    }

    proc.waitForFinished(60 * 1000);

    if (proc.exitStatus() != QProcess::ExitStatus::NormalExit)
    {
      return xiiStatus(xiiFmt("Failed to git clone the remote project '{}' from '{}'", sName, sUrl));
    }

    xiiLog::Success("Cloned remote project '{}' from '{}' to '{}'", sName, sUrl, sTargetDir);

    inout_sFilePath.SetFormat("{}/{}/{}", sTargetDir, sName, sProjectFile);

    // write redirection file
    {
      xiiOSFile file;
      if (file.Open(sRedirFile, xiiFileOpenMode::Write).Succeeded())
      {
        file.Write(inout_sFilePath.GetData(), inout_sFilePath.GetElementCount()).AssertSuccess();
      }
    }

    return xiiStatus(XII_SUCCESS);
  }

  return xiiStatus(xiiFmt("Unknown remote project type '{}' or invalid URL '{}'", sType, sUrl));
}

bool xiiQtEditorApp::ExistsPluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  xiiStringBuilder path = szProjectDir;
  path.MakeCleanPath();

  if (path.EndsWith_NoCase("/xiiProject"))
    path.PathParentDirectory();

  path.AppendPath("Editor/PluginSelection.ddl");

  return xiiOSFile::ExistsFile(path);
}

void xiiQtEditorApp::WritePluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return;

  xiiStringBuilder path = szProjectDir;
  path.AppendPath("Editor/PluginSelection.ddl");

  xiiFileWriter file;
  file.Open(path).AssertSuccess();

  xiiOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  m_PluginBundles.WriteStateToDDL(ddl);
}

void xiiQtEditorApp::CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate)
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return;

  xiiStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();

  for (auto it : m_PluginBundles.m_Plugins)
  {
    xiiPluginBundle& bundle = it.Value();

    bundle.m_bSelected = bundle.m_EnabledInTemplates.Contains(szTemplate);
  }

  WritePluginSelectionStateDDL(sPath);
}

void xiiQtEditorApp::LoadPluginBundleDlls(const char* szProjectFile)
{
  xiiStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();
  sPath.AppendPath("Editor/PluginSelection.ddl");

  xiiFileReader file;
  if (file.Open(sPath).Succeeded())
  {
    xiiOpenDdlReader ddl;
    if (ddl.ParseDocument(file).Failed())
    {
      xiiLog::Error("Syntax error in plugin bundle file '{}'", sPath);
    }
    else
    {
      auto pState = ddl.GetRootElement()->FindChildOfType("PluginState");
      while (pState)
      {
        if (auto pName = pState->FindChildOfType(xiiOpenDdlPrimitiveType::String, "ID"))
        {
          const xiiString sID = pName->GetPrimitivesString()[0];

          bool bExisted = false;
          auto itPlug   = m_PluginBundles.m_Plugins.FindOrAdd(sID, &bExisted);

          if (!bExisted)
          {
            xiiPluginBundle& bundle       = itPlug.Value();
            bundle.m_bMissing             = true;
            bundle.m_sDisplayName         = sID;
            bundle.m_sDescription         = "This plugin bundle is referenced by the project, but doesn't exist. Please check that all plugins are built correctly and their respective *.xiiPluginBundle files are copied to the binary directory.";
            bundle.m_LastModificationTime = xiiTimestamp::CurrentTimestamp();
          }
        }

        pState = pState->GetSibling();
      }

      m_PluginBundles.ReadStateFromDDL(ddl);
    }
  }

  xiiDynamicArray<xiiApplicationPluginConfig::PluginConfig> order;

  // first all the mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (!it.Value().m_bMandatory)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      xiiQtUiServices::MessageBoxWarning("The mandatory plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the xiiPluginBundle files correctly reference each other.");

      return;
    }
  }

  // now the non-mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || !it.Value().m_bSelected)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      xiiQtUiServices::MessageBoxWarning("The plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the xiiPluginBundle files correctly reference each other.");

      return;
    }
  }

  xiiSet<xiiString> NotLoaded;
  for (const xiiApplicationPluginConfig::PluginConfig& it : order)
  {
    if (xiiPlugin::LoadPlugin(it.m_sAppDirRelativePath, it.m_bLoadCopy ? xiiPluginLoadFlags::LoadCopy : xiiPluginLoadFlags::Default).Failed())
    {
      NotLoaded.Insert(it.m_sAppDirRelativePath);
    }
  }

  if (!NotLoaded.IsEmpty())
  {
    xiiStringBuilder s = "The following plugins could not be loaded. Scenes may not load correctly.\n\n";

    for (auto it = NotLoaded.GetIterator(); it.IsValid(); ++it)
    {
      s.AppendFormat(" '{0}' \n", it.Key());
    }

    xiiQtUiServices::MessageBoxWarning(s);
  }
}

void xiiQtEditorApp::LaunchEditor(const char* szProject, bool bCreate)
{
  if (m_bWroteCrashIndicatorFile)
  {
    // orderly shutdown -> make sure the crash indicator file is gone
    xiiStringBuilder sTemp = xiiOSFile::GetTempDataFolder("xiiEditor");
    sTemp.AppendPath("xiiEditorCrashIndicator");
    xiiOSFile::DeleteFile(sTemp).IgnoreResult();
    m_bWroteCrashIndicatorFile = false;
  }

  xiiStringBuilder app;
  app = xiiOSFile::GetApplicationDirectory();
  app.AppendPath("Editor");
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  app.Append(".exe");
#endif
  app.MakeCleanPath();

  // TODO: pass through all command line arguments ?

  QStringList args;
  args << "-nosplash";
  args << (bCreate ? "-newproject" : "-project");
  args << QString::fromUtf8(szProject);

  if (m_StartupFlags.IsSet(StartupFlags::SafeMode))
    args << "-safe";
  if (m_StartupFlags.IsSet(StartupFlags::NoRecent))
    args << "-noRecent";

  QProcess proc;
  proc.startDetached(QString::fromUtf8(app, app.GetElementCount()), args);
}

const xiiApplicationPluginConfig xiiQtEditorApp::GetRuntimePluginConfig(bool bIncludeEditorPlugins) const
{
  xiiApplicationPluginConfig cfg;

  xiiHybridArray<xiiString, 16> order;
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (it.Value().m_bMandatory || it.Value().m_bSelected)
    {
      AddBundlesInOrder(cfg.m_Plugins, m_PluginBundles, it.Key(), false, bIncludeEditorPlugins, true).IgnoreResult();
    }
  }

  return cfg;
}

void xiiQtEditorApp::ReloadEngineResources()
{
  xiiSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ReloadResources";
  msg.m_sPayload  = "ReloadAllResources";
  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
