#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/System/Window.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/Configuration/InputConfig.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

void UpdateInputDynamicEnumValues();

void xiiQtEditorApp::CloseProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedCloseProject", Qt::ConnectionType::QueuedConnection);
}

void xiiQtEditorApp::SlotQueuedCloseProject()
{
  // purge the image loading queue when a project is closed, but keep the existing cache
  xiiQtImageCache::GetSingleton()->StopRequestProcessing(false);

  xiiToolsProject::CloseProject();

  // enable image loading again, the queue is purged now
  xiiQtImageCache::GetSingleton()->EnableRequestProcessing();
}

xiiResult xiiQtEditorApp::OpenProject(const char* szProject, bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return CreateOrOpenProject(false, szProject);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, szProject));
    return XII_SUCCESS;
  }
}

void xiiQtEditorApp::SlotQueuedOpenProject(QString sProject)
{
  CreateOrOpenProject(false, sProject.toUtf8().data()).IgnoreResult();
}

xiiResult xiiQtEditorApp::CreateOrOpenProject(bool bCreate, xiiStringView sFile0)
{
  xiiStringBuilder sFile = sFile0;
  if (!bCreate)
  {
    const xiiStatus status = MakeRemoteProjectLocal(sFile);
    if (status.Failed())
    {
      // if the message is empty, the user decided not to continue, so don't show an error message in this case
      if (!status.m_sMessage.IsEmpty())
      {
        xiiQtUiServices::GetSingleton()->MessageBoxStatus(status, "Opening remote project failed.");
      }

      return XII_FAILURE;
    }
  }

  // check that we don't attempt to open a project from a different repository, due to code changes this often doesn't work too well
  if (!IsInHeadlessMode() && !m_bAnyProjectOpened)
  {
    xiiStringBuilder sdkDirFromProject;
    if (xiiFileSystem::FindFolderWithSubPath(sdkDirFromProject, sFile, "Data/Base", "xiiSdkRoot.txt").Succeeded())
    {
      sdkDirFromProject.MakeCleanPath();
      sdkDirFromProject.Trim(nullptr, "/");

      xiiStringView sdkDir = xiiFileSystem::GetSdkRootDirectory();
      sdkDir.Trim(nullptr, "/");

      if (sdkDirFromProject != sdkDir)
      {
        if (xiiQtUiServices::MessageBoxQuestion(xiiFmt("You are attempting to open a project that's located in a different SDK directory.\n\nSDK location: '{}'\nProject path: '{}'\n\nThis may make problems.\n\nContinue anyway?", sdkDir, sFile), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
        {
          return XII_FAILURE;
        }
      }
    }
  }

  XII_PROFILE_SCOPE("CreateOrOpenProject");
  m_bLoadingProjectInProgress = true;
  XII_SCOPE_EXIT(m_bLoadingProjectInProgress = false;);

  CloseSplashScreen();

  xiiStringBuilder sProjectFile = sFile;
  sProjectFile.MakeCleanPath();

  if (bCreate == false && !sProjectFile.EndsWith_NoCase("/xiiProject"))
  {
    sProjectFile.AppendPath("xiiProject");
  }

  if (xiiToolsProject::IsProjectOpen() && xiiToolsProject::GetSingleton()->GetProjectFile() == sProjectFile)
  {
    xiiQtUiServices::MessageBoxInformation("The selected project is already open");
    return XII_FAILURE;
  }

  if (!xiiToolsProject::CanCloseProject())
    return XII_FAILURE;

  xiiToolsProject::CloseProject();

  // create default plugin selection
  if (!ExistsPluginSelectionStateDDL(sProjectFile))
    CreatePluginSelectionDDL(sProjectFile, "General3D");

  xiiStatus res;
  if (bCreate)
  {
    if (m_bAnyProjectOpened)
    {
      // if we opened any project before, spawn a new editor instance and open the project there
      // this way, a different set of editor plugins can be loaded
      LaunchEditor(sProjectFile, true);

      QApplication::closeAllWindows();
      return XII_SUCCESS;
    }
    else
    {
      // once we start loading any plugins, we can't reuse the same instance again for another project
      m_bAnyProjectOpened = true;

      LoadPluginBundleDlls(sProjectFile);

      res = xiiToolsProject::CreateProject(sProjectFile);
    }
  }
  else
  {
    if (m_bAnyProjectOpened)
    {
      // if we opened any project before, spawn a new editor instance and open the project there
      // this way, a different set of editor plugins can be loaded
      LaunchEditor(sProjectFile, false);

      QApplication::closeAllWindows();
      return XII_SUCCESS;
    }
    else
    {
      // once we start loading any plugins, we can't reuse the same instance again for another project
      m_bAnyProjectOpened = true;

      xiiStringBuilder sTemp = xiiOSFile::GetTempDataFolder("xiiEditor");
      sTemp.AppendPath("xiiEditorCrashIndicator");
      xiiOSFile f;
      if (f.Open(sTemp, xiiFileOpenMode::Write, xiiFileShareMode::Exclusive).Succeeded())
      {
        f.Write(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
        f.Close();
        m_bWroteCrashIndicatorFile = true;
      }

      {
        xiiStringBuilder sProjectDir = sProjectFile;
        sProjectDir.PathParentDirectory();

        xiiStringBuilder sSettingsFile = sProjectDir;
        sSettingsFile.AppendPath("Editor/CppProject.ddl");

        // first attempt to load project specific plugin bundles
        xiiCppSettings cppSettings;
        if (cppSettings.Load(sSettingsFile).Succeeded())
        {
          xiiQtEditorApp::GetSingleton()->DetectAvailablePluginBundles(xiiCppProject::GetPluginSourceDir(cppSettings, sProjectDir));
        }

        // now load the plugin DLLs
        LoadPluginBundleDlls(sProjectFile);
      }

      res = xiiToolsProject::OpenProject(sProjectFile);
    }
  }

  if (res.m_Result.Failed())
  {
    xiiStringBuilder s;
    s.SetFormat("Failed to open project:\n'{0}'", sProjectFile);

    xiiQtUiServices::MessageBoxStatus(res, s);
    return XII_FAILURE;
  }


  if (m_StartupFlags.AreNoneSet(StartupFlags::SafeMode | StartupFlags::Headless))
  {
    xiiStringBuilder sAbsPath;

    if (!m_DocumentsToOpen.IsEmpty())
    {
      for (const auto& doc : m_DocumentsToOpen)
      {
        sAbsPath = doc;

        if (MakeDataDirectoryRelativePathAbsolute(sAbsPath))
        {
          SlotQueuedOpenDocument(sAbsPath.GetData(), nullptr);
        }
        else
        {
          xiiLog::Error("Document '{}' does not exist in this project.", doc);
        }
      }

      // don't try to open the same documents when the user switches to another project
      m_DocumentsToOpen.Clear();
    }
    else if (!m_StartupFlags.IsSet(StartupFlags::NoRecent))
    {
      const xiiRecentFilesList allDocs = LoadOpenDocumentsList();

      // Unfortunately this crashes in Qt due to the processEvents in the QtProgressBar
      // xiiProgressRange range("Restoring Documents", allDocs.GetFileList().GetCount(), true);

      for (auto& doc : allDocs.GetFileList())
      {
        // if (range.WasCanceled())
        //    break;

        // range.BeginNextStep(doc.m_File);
        SlotQueuedOpenDocument(doc.m_File.GetData(), nullptr);
      }

      if (allDocs.GetFileList().IsEmpty())
      {
        OpenDemoDocument();
      }
    }

    if (!xiiQtEditorApp::GetSingleton()->IsInSafeMode())
    {
      xiiQtContainerWindow::GetContainerWindow()->ScheduleRestoreWindowLayout();
    }
  }
  return XII_SUCCESS;
}

void xiiQtEditorApp::ProjectEventHandler(const xiiToolsProjectEvent& r)
{
  switch (r.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectCreated:
      SetupNewProject();
      m_bSavePreferencesAfterOpenProject = true;
      break;

    case xiiToolsProjectEvent::Type::ProjectOpened:
    {
      XII_PROFILE_SCOPE("ProjectOpened");
      xiiDynamicStringEnum::s_RequestUnknownCallback = xiiMakeDelegate(&xiiQtEditorApp::OnDemandDynamicStringEnumLoad, this);
      LoadProjectPreferences();
      SetupDataDirectories();
      ReadTagRegistry();
      UpdateInputDynamicEnumValues();

      // add project specific translations
      // (these are currently never removed)
      {
        m_pTranslatorFromFiles->AddTranslationFilesFromFolder(":project/Editor/Localization/en");
      }

      // tell the engine process which file system and plugin configuration to use
      xiiEditorEngineProcessConnection::GetSingleton()->SetFileSystemConfig(m_FileSystemConfig);
      xiiEditorEngineProcessConnection::GetSingleton()->SetPluginConfig(GetRuntimePluginConfig(true));

      xiiAssetCurator::GetSingleton()->StartInitialize(m_FileSystemConfig);
      if (xiiEditorEngineProcessConnection::GetSingleton()->RestartProcess().Failed())
      {
        XII_PROFILE_SCOPE("ErrorLog");
        xiiLog::Error("Failed to start the engine process. Project loading incomplete.");
      }
      xiiAssetCurator::GetSingleton()->WaitForInitialize();

      m_sLastDocumentFolder = xiiToolsProject::GetSingleton()->GetProjectFile();
      m_sLastProjectFolder  = xiiToolsProject::GetSingleton()->GetProjectFile();

      m_RecentProjects.Insert(xiiToolsProject::GetSingleton()->GetProjectFile(), 0);

      xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();

      // Make sure preferences are saved, this is important when the project was just created.
      if (m_bSavePreferencesAfterOpenProject)
      {
        m_bSavePreferencesAfterOpenProject = false;
        SaveSettings();
      }
      else
      {
        // Save recent project list on project open in case of crashes or stopping the debugger.
        SaveRecentFiles();
      }

      if (m_StartupFlags.AreNoneSet(xiiQtEditorApp::StartupFlags::Headless | xiiQtEditorApp::StartupFlags::SafeMode | xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::Background))
      {
        if (xiiCppProject::ExistsProjectCMakeListsTxt())
        {
          xiiStatus compilerStatus = xiiCppProject::TestCompiler();
          if (compilerStatus.Failed())
          {
            xiiQtUiServices::MessageBoxWarning(xiiFmt("<html>The compiler preferences are invalid.<br><br>\
              This project has <a href='https://xiiengine.net/pages/docs/custom-code/cpp/cpp-project-generation.html'>a dedicated C++ plugin</a> with custom code.<br><br>\
              The compiler set in the preferences does not appear to work, as a result the plugin cannot be compiled <br><br><b>Error:</b> {}</html>",
                                                      compilerStatus.m_sMessage.GetView()));
            break;
          }
          else if (xiiCppProject::IsBuildRequired())
          {
            const auto clicked = xiiQtUiServices::MessageBoxQuestion("<html>Compile this project's C++ plugin?<br><br>\
Explanation: This project has <a href='https://xiiengine.net/pages/docs/custom-code/cpp/cpp-project-generation.html'>a dedicated C++ plugin</a> with custom code. The plugin is currently not compiled and therefore the project won't fully work and certain assets will fail to transform.<br><br>\
It is advised to compile the plugin now, but you can also do so later.</html>",
                                                                     QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Ignore, QMessageBox::StandardButton::Apply);

            if (clicked == QMessageBox::StandardButton::Ignore)
              break;

            QTimer::singleShot(1000, this, [this]() { xiiCppProject::EnsureCppPluginReady().IgnoreResult(); });
          }
        }


        xiiTimestamp lastTransform = xiiAssetCurator::GetSingleton()->GetLastFullTransformDate().GetTimestamp();

        if (pPreferences->m_bBackgroundAssetProcessing)
        {
          QTimer::singleShot(2000, this, [this]() { xiiAssetProcessor::GetSingleton()->StartProcessTask(); });
        }
        else if (!lastTransform.IsValid() || (xiiTimestamp::CurrentTimestamp() - lastTransform).GetHours() > 5 * 24)
        {
          const auto clicked = xiiQtUiServices::MessageBoxQuestion("<html>Apply asset transformation now?<br><br>\
Explanation: For assets to work properly, they must be <a href='https://xiiengine.net/pages/docs/assets/assets-overview.html#asset-transform'>transformed</a>. Otherwise they don't function as they should or don't even show up.<br>You can manually run the asset transform from the <a href='https://xiiengine.net/pages/docs/assets/asset-browser.html#transform-assets'>asset browser</a> at any time.</html>",
                                                                   QMessageBox::StandardButton::Apply | QMessageBox::StandardButton::Ignore, QMessageBox::StandardButton::Apply);

          if (clicked == QMessageBox::StandardButton::Ignore)
          {
            xiiAssetCurator::GetSingleton()->StoreFullTransformDate();
            break;
          }

          // check whether the project needs to be transformed
          QTimer::singleShot(2000, this, [this]() { xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::Default).IgnoreResult(); });
        }
      }

      break;
    }

    case xiiToolsProjectEvent::Type::ProjectSaveState:
    {
      m_RecentProjects.Insert(xiiToolsProject::GetSingleton()->GetProjectFile(), 0);
      SaveSettings();
      break;
    }

    case xiiToolsProjectEvent::Type::ProjectClosing:
    {
      xiiShutdownProcessMsgToEngine msg;
      xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
      break;
    }

    case xiiToolsProjectEvent::Type::ProjectClosed:
    {
      xiiEditorEngineProcessConnection::GetSingleton()->ShutdownProcess();

      xiiAssetCurator::GetSingleton()->Deinitialize();

      // remove all data directories that were loaded by the project configuration
      xiiApplicationFileSystemConfig::Clear();
      xiiFileSystem::SetSpecialDirectory("project", nullptr); // removes this directory

      m_ReloadProjectRequiredReasons.Clear();
      UpdateGlobalStatusBarMessage();

      xiiPreferences::ClearProjectPreferences();

      // remove all dynamic enums that were dynamically loaded from the project directory
      {
        for (const auto& val : m_DynamicEnumStringsToClear)
        {
          xiiDynamicStringEnum::RemoveEnum(val);
        }
        m_DynamicEnumStringsToClear.Clear();
      }

      break;
    }

    case xiiToolsProjectEvent::Type::SaveAll:
    {
      xiiToolsProject::SaveProjectState();
      SaveAllOpenDocuments();
      break;
    }

    default:
      break;
  }
}

void xiiQtEditorApp::ProjectRequestHandler(xiiToolsProjectRequest& r)
{
  switch (r.m_Type)
  {
    case xiiToolsProjectRequest::Type::CanCloseProject:
    case xiiToolsProjectRequest::Type::CanCloseDocuments:
    {
      if (r.m_bCanClose == false)
        return;

      xiiHybridArray<xiiDocument*, 32> ModifiedDocs;
      if (r.m_Type == xiiToolsProjectRequest::Type::CanCloseProject)
      {
        for (xiiDocumentManager* pMan : xiiDocumentManager::GetAllDocumentManagers())
        {
          for (xiiDocument* pDoc : pMan->GetAllOpenDocuments())
          {
            if (pDoc->IsModified())
              ModifiedDocs.PushBack(pDoc);
          }
        }
      }
      else
      {
        for (xiiDocument* pDoc : r.m_Documents)
        {
          if (pDoc->IsModified())
            ModifiedDocs.PushBack(pDoc);
        }
      }

      if (!ModifiedDocs.IsEmpty())
      {
        xiiQtModifiedDocumentsDlg dlg(QApplication::activeWindow(), ModifiedDocs);
        if (dlg.exec() == 0)
          r.m_bCanClose = false;
      }
    }
    break;
    case xiiToolsProjectRequest::Type::SuggestContainerWindow:
    {
      const auto&      docs       = GetRecentDocumentsList();
      xiiStringBuilder sCleanPath = r.m_Documents[0]->GetDocumentPath();
      sCleanPath.MakeCleanPath();

      for (auto& file : docs.GetFileList())
      {
        if (file.m_File == sCleanPath)
        {
          r.m_iContainerWindowUniqueIdentifier = file.m_iContainerWindow;
          break;
        }
      }
    }
    break;
    case xiiToolsProjectRequest::Type::GetPathForDocumentGuid:
    {
      if (xiiAssetCurator::xiiLockedSubAsset pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(r.m_documentGuid))
      {
        r.m_sAbsDocumentPath = pSubAsset->m_pAssetInfo->m_Path;
      }
    }
    break;
  }
}

void xiiQtEditorApp::SetupNewProject()
{
  xiiToolsProject::GetSingleton()->CreateSubFolder("Editor");
  xiiToolsProject::GetSingleton()->CreateSubFolder("RuntimeConfigs");

  // write the default window config
  {
    xiiStringBuilder sPath = xiiToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/Window.ddl");

    if (!xiiFileSystem::ExistsFile(sPath))
    {
      xiiWindowCreationDescription desc;
      desc.m_Title = xiiToolsProject::GetSingleton()->GetProjectName(false);
      desc.SaveToDDL(sPath).IgnoreResult();
    }
  }

  // write a stub input mapping
  {
    xiiStringBuilder sPath = xiiToolsProject::GetSingleton()->GetProjectDirectory();
    sPath.AppendPath("RuntimeConfigs/InputConfig.ddl");

    if (!xiiFileSystem::ExistsFile(sPath))
    {
      xiiDeferredFileWriter file;
      file.SetOutput(sPath);

      xiiHybridArray<xiiGameAppInputConfig, 4> actions;
      xiiGameAppInputConfig&                   a = actions.ExpandAndGetRef();
      a.m_sInputSet                              = "Default";
      a.m_sInputAction                           = "Interact";
      a.m_bApplyTimeScaling                      = false;
      a.m_sInputSlotTrigger[0]                   = xiiInputSlot_KeySpace;
      a.m_sInputSlotTrigger[1]                   = xiiInputSlot_MouseButton0;
      a.m_sInputSlotTrigger[2]                   = xiiInputSlot_Controller0_ButtonA;

      xiiGameAppInputConfig::WriteToDDL(file, actions);

      file.Close().IgnoreResult();
    }
  }
}
