#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

XII_IMPLEMENT_SINGLETON(xiiQtEditorApp);

xiiEvent<const xiiEditorAppEvent&> xiiQtEditorApp::m_Events;

xiiQtEditorApp::xiiQtEditorApp() :
  m_SingletonRegistrar(this), m_RecentProjects(20), m_RecentDocuments(100)
{
  m_bSavePreferencesAfterOpenProject = false;

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

  if (m_bWroteCrashIndicatorFile)
  {
    m_bWroteCrashIndicatorFile = false;
    QTimer::singleShot(2000, []() {
      xiiStringBuilder sTemp = xiiOSFile::GetTempDataFolder("xiiEditor");
      sTemp.AppendPath("xiiEditorCrashIndicator");
      xiiOSFile::DeleteFile(sTemp).IgnoreResult();
      //
    });
  }

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
    if (m_VersionChecker.IsLatestNewer())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation(
        xiiFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A "
               "href=\"https://github.com/xiiEngine/xiiEngine/releases\">Releases</A> for details.</html>",
               m_VersionChecker.GetKnownLatestVersion(), m_VersionChecker.GetOwnVersion()));
    }
    else
    {
      xiiStringBuilder tmp("You have the latest version: \n");
      tmp.Append(m_VersionChecker.GetOwnVersion());

      xiiQtUiServices::GetSingleton()->MessageBoxInformation(tmp);
    }
  }

  if (m_VersionChecker.IsLatestNewer())
  {
    xiiQtUiServices::GetSingleton()->ShowGlobalStatusBarMessage(
      xiiFmt("New version '{}' available, please update.", m_VersionChecker.GetKnownLatestVersion()));
  }
}

void xiiQtEditorApp::EngineProcessMsgHandler(const xiiEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case xiiEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiUpdateReflectionTypeMsgToEditor>())
      {
        const xiiUpdateReflectionTypeMsgToEditor* pMsg = static_cast<const xiiUpdateReflectionTypeMsgToEditor*>(e.m_pMsg);
        xiiPhantomRttiManager::RegisterType(pMsg->m_desc);
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
    m_VersionChecker.Check(true);
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
        pDoc->SaveDocument();
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
  sFile.Format(":project/Editor/{}.txt", sEnumName);

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
