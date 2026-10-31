/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/EditorPreferences.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#include <QFileDialog>

XII_IMPLEMENT_SINGLETON(xiiQtEditorApp);

xiiEvent<const xiiEditorAppEvent&> xiiQtEditorApp::m_Events;

void xiiQtEditorApp::AssetProcessorEventHandler(const xiiAssetProcessorEvent& e)
{
  if (e.m_Type != xiiAssetProcessorEvent::Type::TransformsFinished)
    return;
  const auto progress = e.m_Progress;
  QMetaObject::invokeMethod(this, [this, progress]() {
    if (!xiiToolsProject::IsProjectGenerationCurrent(progress.m_uiProjectGeneration) || !m_bIsRunning || !xiiToolsProject::IsProjectOpen() || xiiToolsProject::IsProjectClosing())
      return;
    if (xiiAssetCurator::GetSingleton()->WriteAssetTables().Failed())
      xiiLog::Error("Failed to publish asset lookup tables after background transformation.");
    if (progress.m_bAllAssets && !progress.m_bCancelRequested && progress.m_uiFailed == 0 && progress.m_uiNeedsImport == 0 && progress.m_uiCanceled == 0)
      xiiAssetCurator::GetSingleton()->StoreFullTransformDate();
    xiiStringBuilder text;
    text.SetFormat("Asset transformation: {} processed, {} failed, {} need import, {} canceled. See Asset Curator for details.",
      progress.m_uiCompleted, progress.m_uiFailed, progress.m_uiNeedsImport, progress.m_uiCanceled);
    xiiQtUiServices::ShowAllDocumentsTemporaryStatusBarMessage(text, xiiTime::MakeFromSeconds(10));
    if (progress.m_uiFailed > 0)
      xiiLog::Error("{}", text);
    else if (progress.m_uiNeedsImport > 0)
      xiiLog::Warning("{}", text);
    else
      xiiLog::Info("{}", text); }, Qt::QueuedConnection);
}

xiiQtEditorApp::xiiQtEditorApp() :
  m_SingletonRegistrar(this), m_RecentProjects(20), m_RecentDocuments(100)
{
  m_bSavePreferencesAfterOpenProject = false;
  m_pVersionChecker                  = XII_DEFAULT_NEW(xiiQtVersionChecker);

  m_pTimer         = new QTimer(nullptr);
  m_pAutoSaveTimer = new QTimer(nullptr);
}

xiiQtEditorApp::~xiiQtEditorApp()
{
  delete m_pTimer;
  m_pTimer = nullptr;

  delete m_pAutoSaveTimer;
  m_pAutoSaveTimer = nullptr;

  CloseSplashScreen();
}

xiiInt32 xiiQtEditorApp::RunEditor()
{
  if (m_bStartupQuitRequested)
    return 0;
  xiiInt32 ret = m_pQtApplication->exec();
  return ret;
}

void xiiQtEditorApp::ShowEditorWindow(QWidget* pWindow)
{
  if (pWindow == nullptr || GetSingleton()->IsStartingUp())
    return;
  pWindow->setWindowState(pWindow->windowState() & ~Qt::WindowMinimized);
  pWindow->show();
  QWidget* pTarget = QApplication::activeModalWidget();
  if (pTarget == nullptr)
    pTarget = pWindow;
  pTarget->raise();
  pTarget->activateWindow();
  // Qt follows Windows foreground restrictions; requesting attention is the fallback.
  if (!pTarget->isActiveWindow())
    QApplication::alert(pTarget);
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
  if (!m_bStartupInProgress)
    CloseSplashScreen();

  Q_EMIT IdleEvent();

  RestartEngineProcessIfPluginsChanged(false);

  m_pTimer->start(1);
}

void xiiQtEditorApp::SlotSaveSettings()
{
  SaveSettings();
}

void xiiQtEditorApp::SlotAutoSave()
{
  const auto* pPreferences = xiiEditorSettings::QuerySettings<xiiEditorPreferencesUser>();
  if (!pPreferences || pPreferences->m_uiAutoSaveMinutes == 0)
    return;

  const xiiTime tAutoSaveThreshold = xiiTime::MakeFromMinutes(pPreferences->m_uiAutoSaveMinutes);
  const xiiTime tNow               = xiiTime::Now();

  // Find the oldest modified document that exceeds the auto-save threshold.
  xiiDocument* pOldestDoc  = nullptr;
  xiiTime      tOldestTime = tNow;

  for (auto pManager : xiiDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDocumenent : pManager->xiiDocumentManager::GetAllOpenDocuments())
    {
      const xiiTime tModified = pDocumenent->GetModifiedTime();
      if (tModified.IsPositive() && (tNow - tModified) >= tAutoSaveThreshold && tModified < tOldestTime)
      {
        pOldestDoc  = pDocumenent;
        tOldestTime = tModified;
      }
    }
  }

  if (pOldestDoc == nullptr)
    return;

  xiiQtDocumentWindow* pWnd = xiiQtDocumentWindow::FindWindowByDocument(pOldestDoc);
  if (pWnd && pWnd->GetDocument() == pOldestDoc)
  {
    pWnd->SaveDocument().IgnoreResult();
  }
  else
  {
    pOldestDoc->SaveDocument().IgnoreResult();
  }
}

void xiiQtEditorApp::SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced)
{
  if (m_bStartupInProgress)
  {
    // Optional update UI must not dismiss startup presentation or activate another window.
    connect(this, &xiiQtEditorApp::IdleEvent, this, [this, bNewVersionReleased, bForced]() { SlotVersionCheckCompleted(bNewVersionReleased, bForced); }, Qt::SingleShotConnection);
    return;
  }
  if (bForced || bNewVersionReleased)
  {
    CloseSplashScreen();
    if (m_pVersionChecker->IsLatestNewer())
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation(xiiFmt("<html>A new version is available: {}<br><br>Your version is: {}<br><br>Please check the <A href=\"https://github.com/xiiEngine/xiiEngine/releases\">Releases</A> for details.</html>", m_pVersionChecker->GetKnownLatestVersion(), m_pVersionChecker->GetOwnVersion()));
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
    {
      StoreEnginePluginModificationTimes();
    }
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
        if (pWnd->SaveDocument().Failed())
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

xiiResult xiiQtEditorApp::AddBundlesInOrder(xiiDynamicArray<xiiApplicationPluginConfig::PluginConfig>& order, const xiiPluginBundleSet& bundles, const xiiString& start, bool bEditor, bool bEditorEngine, bool bRuntime, xiiDynamicArray<xiiString>* pDependencyStack) const
{
  xiiDynamicArray<xiiString> stack;
  if (pDependencyStack == nullptr)
    pDependencyStack = &stack;
  const auto found = bundles.m_Plugins.Find(start);
  if (!found.IsValid() || found.Value().m_bMissing || pDependencyStack->Contains(start))
  {
    xiiLog::Error("Required plugin bundle '{}' is missing or contains a dependency cycle.", start);
    return XII_FAILURE;
  }
  pDependencyStack->PushBack(start);
  XII_SCOPE_EXIT(pDependencyStack->PopBack());
  const xiiPluginBundle& bundle = found.Value();

  for (const xiiString& req : bundle.m_RequiredBundles)
  {
    auto it = bundles.m_Plugins.Find(req);

    if (!it.IsValid())
    {
      xiiLog::Error("Plugin bundle '{}' has a dependency on bundle '{}' which does not exist.", start, req);
      return XII_FAILURE;
    }

    XII_SUCCEED_OR_RETURN(AddBundlesInOrder(order, bundles, req, bEditor, bEditorEngine, bRuntime, pDependencyStack));
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
    return XII_SUCCESS;

  {
    xiiStringBuilder tmp = inout_sFilePath;
    tmp.AppendPath("xiiProject");

    if (xiiOSFile::ExistsFile(tmp))
    {
      inout_sFilePath = tmp;
      return XII_SUCCESS;
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
        return XII_SUCCESS;
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
    proc.setProcessChannelMode(QProcess::MergedChannels);

    xiiProgressRange progress("Downloading Project", true);
    bool             bRecursion = false;

    QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&]() {
      if (bRecursion)
        return;

      bRecursion = true;

      auto             data = proc.readAllStandardOutput();
      xiiStringBuilder str  = data.toStdString().c_str();
      if (const char* szPercent = str.FindLastSubString("%"))
      {
        str.SetSubString_FromTo(szPercent - 3, szPercent);
        str.Trim();

        xiiInt32 p;
        if (xiiConversionUtils::StringToInt(str, p).Succeeded())
        {
          progress.SetCompletion(p / 100.0f);
        }
      }

      if (progress.WasCanceled())
      {
        proc.close();
      }

      bRecursion = false;
    });

    QApplication::setOverrideCursor(Qt::WaitCursor);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    proc.start("git.exe", args);
#else
    proc.start("git", args);
#endif

    if (!proc.waitForStarted())
    {
      return xiiStatus(xiiFmt("Running 'git' to download the remote project failed."));
    }

    proc.waitForFinished(60 * 1000);
    QApplication::restoreOverrideCursor();

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

    return XII_SUCCESS;
  }

  return xiiStatus(xiiFmt("Unknown remote project type '{}' or invalid URL '{}'", sType, sUrl));
}

bool xiiQtEditorApp::ExistsPluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  xiiStringBuilder path = szProjectDir;
  path.MakeCleanPath();

  if (path.EndsWith_NoCase("/xiiProject"))
    path.PathParentDirectory();

  xiiProjectManifest manifest;
  if (xiiToolsProject::GetSingleton() && (path == ":project" || path == xiiToolsProject::GetSingleton()->GetProjectDirectory()))
    manifest = xiiToolsProject::GetSingleton()->GetManifest();
  else
  {
    const xiiStringBuilder projectFile(path, "/xiiProject");
    if (xiiOSFile::ExistsFile(projectFile))
      manifest.Load(projectFile).LogFailure();
  }
  path.AppendPath(manifest.m_sPluginSelection);

  return xiiOSFile::ExistsFile(path);
}

xiiStatus xiiQtEditorApp::WritePluginSelectionStateDDL(const char* szProjectDir /*= ":project"*/)
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return XII_SUCCESS;

  xiiStringBuilder   path = szProjectDir;
  xiiProjectManifest manifest;
  if (xiiToolsProject::GetSingleton() && (path == ":project" || path == xiiToolsProject::GetSingleton()->GetProjectDirectory()))
    manifest = xiiToolsProject::GetSingleton()->GetManifest();
  else
  {
    const xiiStringBuilder projectFile(path, "/xiiProject");
    if (xiiOSFile::ExistsFile(projectFile))
      XII_SUCCEED_OR_RETURN(manifest.Load(projectFile));
  }
  path.AppendPath(manifest.m_sPluginSelection);

  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(path));

  xiiOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  m_PluginBundles.WriteStateToDDL(ddl);
  return file.Flush();
}

xiiStatus xiiQtEditorApp::CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate)
{
  if (m_StartupFlags.IsAnySet(StartupFlags::Background | StartupFlags::Headless | StartupFlags::UnitTest))
    return XII_SUCCESS;

  xiiStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();

  for (auto it : m_PluginBundles.m_Plugins)
  {
    xiiPluginBundle& bundle = it.Value();

    bundle.m_bSelected = bundle.m_EnabledInTemplates.Contains(szTemplate);
  }

  return WritePluginSelectionStateDDL(sPath);
}

xiiStatus xiiQtEditorApp::LoadPluginBundleDlls(const char* szProjectFile)
{
  xiiStringBuilder sPath = szProjectFile;
  sPath.PathParentDirectory();
  xiiProjectManifest     manifest;
  const xiiStringBuilder projectFile(sPath, "/xiiProject");
  if (xiiOSFile::ExistsFile(projectFile))
    XII_SUCCEED_OR_RETURN(manifest.Load(projectFile));
  sPath.AppendPath(manifest.m_sPluginSelection);
  for (auto& entry : m_PluginBundles.m_Plugins)
    entry.Value().m_bSelected = false;

  xiiFileReader file;
  if (file.Open(sPath).Succeeded())
  {
    xiiOpenDdlReader ddl;
    if (ddl.ParseDocument(file).Failed())
    {
      return xiiStatus(xiiFmt("Syntax error in plugin bundle file '{}'.", sPath));
    }
    else
    {
      xiiSet<xiiString> ids;
      for (auto* state = ddl.GetRootElement()->GetFirstChild(); state; state = state->GetSibling())
      {
        if (!state->IsCustomType("PluginState"))
          continue;
        xiiSet<xiiString> names;
        for (auto* field = state->GetFirstChild(); field; field = field->GetSibling())
        {
          const auto name = field->GetName();
          if (!field->HasName() || names.Contains(name))
            return xiiStatus("Plugin selection fields must have unique names.");
          names.Insert(name);
          if ((name == "ID" || name == "Selected" || name == "LoadCopy") &&
              (field->GetNumPrimitives() != 1 || field->GetPrimitivesType() != (name == "ID" ? xiiOpenDdlPrimitiveType::String : xiiOpenDdlPrimitiveType::Bool)))
            return xiiStatus("Plugin selection ID requires one string; Selected and LoadCopy require one bool.");
        }
        const auto* id = state->FindChildOfType(xiiOpenDdlPrimitiveType::String, "ID");
        if (!id || id->GetPrimitivesString()[0].IsEmpty())
          return xiiStatus("Plugin selection requires a nonempty bundle ID.");
        xiiStringBuilder key(id->GetPrimitivesString()[0]);
        key.ToLower();
        if (ids.Contains(key))
          return xiiStatus("Plugin selection contains duplicate bundle IDs.");
        ids.Insert(key);
      }
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

  if (!file.IsOpen() && xiiOSFile::ExistsFile(sPath))
    return xiiStatus(xiiFmt("Could not read project plugin selection '{}'.", sPath));
  xiiDynamicArray<xiiApplicationPluginConfig::PluginConfig> order;

  // first all the mandatory bundles
  for (auto it : m_PluginBundles.m_Plugins)
  {
    if (!it.Value().m_bMandatory)
      continue;

    if (AddBundlesInOrder(order, m_PluginBundles, it.Key(), true, false, false).Failed())
    {
      xiiQtUiServices::MessageBoxWarning("The mandatory plugin bundles have non-existing dependencies. Please make sure all plugins are properly built and the xiiPluginBundle files correctly reference each other.");

      return xiiStatus("Project plugin bundles have missing dependencies.");
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

      return xiiStatus("Project plugin bundles have missing dependencies.");
    }
  }

  xiiSet<xiiString> NotLoaded;
  xiiUInt32         uiCompleted = 0;
  for (const xiiApplicationPluginConfig::PluginConfig& it : order)
  {
    UpdateStartupProgress(QStringLiteral("Loading project plugins"), xiiMakeQString(it.m_sAppDirRelativePath), uiCompleted, order.GetCount());
    if (m_bStartupInProgress && m_StartupFlags.AreNoneSet(StartupFlags::Headless | StartupFlags::Background | StartupFlags::UnitTest))
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    if (xiiPlugin::LoadPlugin(it.m_sAppDirRelativePath, it.m_bLoadCopy ? xiiPluginLoadFlags::LoadCopy : xiiPluginLoadFlags::Default).Failed())
    {
      NotLoaded.Insert(it.m_sAppDirRelativePath);
    }
    ++uiCompleted;
  }

  if (!NotLoaded.IsEmpty())
  {
    xiiStringBuilder s = "The following plugins could not be loaded. Scenes may not load correctly.\n\n";

    for (auto it = NotLoaded.GetIterator(); it.IsValid(); ++it)
    {
      s.AppendFormat(" '{0}' \n", it.Key());
    }

    return xiiStatus(s.GetView());
  }
  return XII_SUCCESS;
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
  app.AppendPath("xiiEditor");
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

  if (xiiCommandLineUtils::GetGlobalInstance()->HasOption("-renderer"))
  {
    xiiStringBuilder sRenderer = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer");
    args << "-renderer";
    args << sRenderer.GetData();
  }

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

  for (auto& plugin : cfg.m_Plugins)
  {
    if (!xiiCppProject::GetPluginBinaryDirectory(plugin.m_sAppDirRelativePath).IsEmpty())
      plugin.m_sPluginDirectory = xiiCppProject::GetPluginOutputDir(">project");
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

void xiiQtEditorApp::OpenDemoDocument()
{
  auto* pCurator = xiiAssetCurator::GetSingleton();
  auto  assets   = pCurator->GetKnownAssets();

  xiiStringBuilder sBestDoc;

  for (auto it = assets->GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->m_Path.GetDataDirRelativePath().GetFileName().IsEqual_NoCase("Main"))
    {
      sBestDoc = it.Value()->m_Path.GetAbsolutePath();
      break;
    }
  }

  if (!sBestDoc.IsEmpty())
  {
    SlotQueuedOpenDocument(sBestDoc.GetData(), nullptr);
  }
}
