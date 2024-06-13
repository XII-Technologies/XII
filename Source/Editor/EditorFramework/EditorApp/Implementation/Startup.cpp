#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorFramework/EditorApp/CheckVersion.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/DynamicDefaultStateProvider.h>
#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeAngleManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ConeLengthManipulatorAdapter.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Manipulators/NonUniformBoxManipulatorAdapter.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <EditorFramework/Manipulators/TransformManipulatorAdapter.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedBoneWidget.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/GameObjectReferencePropertyWidget.moc.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QSvgRenderer>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ads/DockManager.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation",
    "PropertyGrid",
    "ManipulatorAdapterRegistry",
    "DefaultState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiDefaultState::RegisterDefaultStateProvider(xiiExposedParametersDefaultStateProvider::CreateProvider);
    xiiDefaultState::RegisterDefaultStateProvider(xiiDynamicDefaultStateProvider::CreateProvider);
    xiiProjectActions::RegisterActions();
    xiiAssetActions::RegisterActions();
    xiiViewActions::RegisterActions();
    xiiViewLightActions::RegisterActions();
    xiiGameObjectContextActions::RegisterActions();
    xiiGameObjectDocumentActions::RegisterActions();
    xiiGameObjectSelectionActions::RegisterActions();
    xiiQuadViewActions::RegisterActions();
    xiiTransformGizmoActions::RegisterActions();
    xiiTranslateGizmoAction::RegisterActions();
    xiiCommonAssetActions::RegisterActions();

    xiiActionMapManager::RegisterActionMap("SettingsTabMenuBar").IgnoreResult();
    xiiStandardMenus::MapActions("SettingsTabMenuBar", xiiStandardMenuTypes::Default);
    xiiProjectActions::MapActions("SettingsTabMenuBar");

    xiiActionMapManager::RegisterActionMap("AssetBrowserToolBar").IgnoreResult();
    xiiAssetActions::MapToolBarActions("AssetBrowserToolBar", false);

    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiFileBrowserAttribute>(), [](const xiiRTTI* pRtti)->xiiQtPropertyWidget* { return new xiiQtFilePropertyWidget(); });
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiAssetBrowserAttribute>(), [](const xiiRTTI* pRtti)->xiiQtPropertyWidget* { return new xiiQtAssetPropertyWidget(); });
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiDynamicEnumAttribute>(), [](const xiiRTTI* pRtti)->xiiQtPropertyWidget* { return new xiiQtDynamicEnumPropertyWidget(); });
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiDynamicStringEnumAttribute>(), [](const xiiRTTI* pRtti)->xiiQtPropertyWidget* { return new xiiQtDynamicStringEnumPropertyWidget(); });
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiExposedParametersAttribute>(), [](const xiiRTTI* pRtti)->xiiQtPropertyWidget* { return new xiiQtExposedParametersPropertyWidget(); });
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiGameObjectReferenceAttribute>(), [](const xiiRTTI* pRtti)->xiiQtPropertyWidget* { return new xiiQtGameObjectReferencePropertyWidget(); });
    xiiQtPropertyGridWidget::GetFactory().RegisterCreator(xiiGetStaticRTTI<xiiExposedBone>(), [](const xiiRTTI* pRtti)->xiiQtPropertyWidget* { return new xiiQtExposedBoneWidget(); });

    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiSphereManipulatorAttribute>(), [](const xiiRTTI* pRtti)->xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiSphereManipulatorAdapter); });
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiCapsuleManipulatorAttribute>(), [](const xiiRTTI* pRtti)->xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiCapsuleManipulatorAdapter); });
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiBoxManipulatorAttribute>(), [](const xiiRTTI* pRtti)->xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiBoxManipulatorAdapter); });
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiConeAngleManipulatorAttribute>(), [](const xiiRTTI* pRtti)->xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiConeAngleManipulatorAdapter); });
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiConeLengthManipulatorAttribute>(), [](const xiiRTTI* pRtti)->xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiConeLengthManipulatorAdapter); });
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiNonUniformBoxManipulatorAttribute>(), [](const xiiRTTI* pRtti)->xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiNonUniformBoxManipulatorAdapter); });
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiTransformManipulatorAttribute>(), [](const xiiRTTI* pRtti)->xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiTransformManipulatorAdapter); });
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiBoneManipulatorAttribute>(), [](const xiiRTTI* pRtti)->xiiManipulatorAdapter* { return XII_DEFAULT_NEW(xiiBoneManipulatorAdapter); });

    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiBoxVisualizerAttribute>(), [](const xiiRTTI* pRtti)->xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiBoxVisualizerAdapter); });
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiSphereVisualizerAttribute>(), [](const xiiRTTI* pRtti)->xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiSphereVisualizerAdapter); });
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiCapsuleVisualizerAttribute>(), [](const xiiRTTI* pRtti)->xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiCapsuleVisualizerAdapter); });
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiCylinderVisualizerAttribute>(), [](const xiiRTTI* pRtti)->xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiCylinderVisualizerAdapter); });
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiDirectionVisualizerAttribute>(), [](const xiiRTTI* pRtti)->xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiDirectionVisualizerAdapter); });
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiConeVisualizerAttribute>(), [](const xiiRTTI* pRtti)->xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiConeVisualizerAdapter); });
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(xiiGetStaticRTTI<xiiCameraVisualizerAttribute>(), [](const xiiRTTI* pRtti)->xiiVisualizerAdapter* { return XII_DEFAULT_NEW(xiiCameraVisualizerAdapter); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiDefaultState::UnregisterDefaultStateProvider(xiiExposedParametersDefaultStateProvider::CreateProvider);
    xiiDefaultState::UnregisterDefaultStateProvider(xiiDynamicDefaultStateProvider::CreateProvider);
    xiiProjectActions::UnregisterActions();
    xiiAssetActions::UnregisterActions();
    xiiViewActions::UnregisterActions();
    xiiViewLightActions::UnregisterActions();
    xiiGameObjectContextActions::UnregisterActions();
    xiiGameObjectDocumentActions::UnregisterActions();
    xiiGameObjectSelectionActions::UnregisterActions();
    xiiQuadViewActions::UnregisterActions();
    xiiTransformGizmoActions::UnregisterActions();
    xiiTranslateGizmoAction::UnregisterActions();
    xiiCommonAssetActions::UnregisterActions();

    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiFileBrowserAttribute>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiAssetBrowserAttribute>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiDynamicEnumAttribute>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiDynamicStringEnumAttribute>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiGameObjectReferenceAttribute>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiExposedParametersAttribute>());
    xiiQtPropertyGridWidget::GetFactory().UnregisterCreator(xiiGetStaticRTTI<xiiExposedBone>());

    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiSphereManipulatorAttribute>());
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiCapsuleManipulatorAttribute>());
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiBoxManipulatorAttribute>());
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiConeAngleManipulatorAttribute>());
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiConeLengthManipulatorAttribute>());
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiNonUniformBoxManipulatorAttribute>());
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiTransformManipulatorAttribute>());
    xiiManipulatorAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiBoneManipulatorAttribute>());

    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiBoxVisualizerAttribute>());
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiSphereVisualizerAttribute>());
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiCapsuleVisualizerAttribute>());
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiCylinderVisualizerAttribute>());
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiDirectionVisualizerAttribute>());
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiConeVisualizerAttribute>());
    xiiVisualizerAdapterRegistry::GetSingleton()->m_Factory.UnregisterCreator(xiiGetStaticRTTI<xiiCameraVisualizerAttribute>());
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiCommandLineOptionBool opt_Safe("_Editor", "-safe", "In safe-mode the editor minimizes the risk of crashing, for instance by not loading previous projects and scenes.", false);
xiiCommandLineOptionBool opt_NoRecent("_Editor", "-noRecent", "Disables automatic loading of recent projects and documents.", false);

void xiiQtEditorApp::StartupEditor()
{
  {
    xiiStringBuilder sTemp = xiiOSFile::GetTempDataFolder("xiiEditor");
    sTemp.AppendPath("xiiEditorCrashIndicator");

    if (xiiOSFile::ExistsFile(sTemp))
    {
      xiiOSFile::DeleteFile(sTemp).IgnoreResult();

      if (xiiQtUiServices::GetSingleton()->MessageBoxQuestion("It seems the editor ran into problems last time.\n\nDo you want to run it in safe mode, to deactivate automatic project loading and document restoration?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes)
      {
        opt_Safe.GetOptions(sTemp);
        xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(sTemp);
      }
    }
  }

  xiiBitflags<StartupFlags> startupFlags;

  startupFlags.AddOrRemove(StartupFlags::SafeMode, opt_Safe.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
  startupFlags.AddOrRemove(StartupFlags::NoRecent, opt_NoRecent.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));

  StartupEditor(startupFlags);
}

void xiiQtEditorApp::StartupEditor(xiiBitflags<StartupFlags> startupFlags, const char* szUserDataFolder)
{
  XII_PROFILE_SCOPE("StartupEditor");

  QCoreApplication::setOrganizationDomain("www.xiitechnologies.com");
  QCoreApplication::setOrganizationName("XII Technologies");
  QCoreApplication::setApplicationName(xiiApplication::GetApplicationInstance()->GetApplicationName().GetData());
  QCoreApplication::setApplicationVersion("1.0.0");

  m_StartupFlags = startupFlags;

  auto* pCmd = xiiCommandLineUtils::GetGlobalInstance();

  if (!IsInHeadlessMode())
  {
    SetupAndShowSplashScreen();

    m_pProgressbar   = XII_DEFAULT_NEW(xiiProgress);
    m_pQtProgressbar = XII_DEFAULT_NEW(xiiQtProgressbar);

    xiiProgress::SetGlobalProgressbar(m_pProgressbar);
    m_pQtProgressbar->SetProgressbar(m_pProgressbar);
  }

  // custom command line arguments
  {
    // Make sure to disable the fileserve plugin
    pCmd->InjectCustomArgument("-fs_off");
  }

  const bool bNoRecent = m_StartupFlags.IsAnySet(StartupFlags::UnitTest | StartupFlags::SafeMode | StartupFlags::Headless | StartupFlags::NoRecent);

  const xiiString sApplicationName = pCmd->GetStringOption("-appname", 0, xiiApplication::GetApplicationInstance()->GetApplicationName());
  xiiApplication::GetApplicationInstance()->SetApplicationName(sApplicationName);

  QLocale::setDefault(QLocale(QLocale::English));

  m_pEngineViewProcess = new xiiEditorEngineProcessConnection;

  m_LongOpControllerManager.Startup(&m_pEngineViewProcess->GetCommunicationChannel());

  if (!IsInHeadlessMode())
  {
    XII_PROFILE_SCOPE("xiiQtContainerWindow");
    SetStyleSheet();

    xiiQtContainerWindow* pContainer = new xiiQtContainerWindow();
    pContainer->show();
  }

  xiiDocumentManager::s_Requests.AddEventHandler(xiiMakeDelegate(&xiiQtEditorApp::DocumentManagerRequestHandler, this));
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEditorApp::DocumentManagerEventHandler, this));
  xiiDocument::s_EventsAny.AddEventHandler(xiiMakeDelegate(&xiiQtEditorApp::DocumentEventHandler, this));
  xiiToolsProject::s_Requests.AddEventHandler(xiiMakeDelegate(&xiiQtEditorApp::ProjectRequestHandler, this));
  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEditorApp::ProjectEventHandler, this));
  xiiEditorEngineProcessConnection::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEditorApp::EngineProcessMsgHandler, this));
  xiiQtDocumentWindow::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEditorApp::DocumentWindowEventHandler, this));
  xiiQtUiServices::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEditorApp::UiServicesEvents, this));

  xiiStartup::StartupCoreSystems();

  // prevent restoration of window layouts when in safe mode
  xiiQtDocumentWindow::s_bAllowRestoreWindowLayout = !IsInSafeMode();

  {
    // Make sure that we have at least 4 worker threads for short running and 4 worker threads for long running tasks.
    // Otherwise the Editor might deadlock during asset transform.
    xiiInt32 iLongThreads  = xiiMath::Max(4, (xiiInt32)xiiTaskSystem::GetNumAllocatedWorkerThreads(xiiWorkerThreadType::LongTasks));
    xiiInt32 iShortThreads = xiiMath::Max(4, (xiiInt32)xiiTaskSystem::GetNumAllocatedWorkerThreads(xiiWorkerThreadType::ShortTasks));
    xiiTaskSystem::SetWorkerThreadCount(iShortThreads, iLongThreads);
  }

  {
    XII_PROFILE_SCOPE("Filesystem");
    xiiFileSystem::DetectSdkRootDirectory().IgnoreResult();

    const xiiString sAppDir   = xiiApplicationServices::GetSingleton()->GetApplicationDataFolder();
    xiiString       sUserData = xiiApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    if (!xiiStringUtils::IsNullOrEmpty(szUserDataFolder))
    {
      sUserData = szUserDataFolder;
    }
    // make sure these folders exist
    xiiFileSystem::CreateDirectoryStructure(sAppDir).IgnoreResult();
    xiiFileSystem::CreateDirectoryStructure(sUserData).IgnoreResult();

    xiiFileSystem::AddDataDirectory("", "AbsPaths", ":", xiiFileSystem::AllowWrites).IgnoreResult();             // for absolute paths
    xiiFileSystem::AddDataDirectory(">appdir/", "AppBin", "bin", xiiFileSystem::AllowWrites).IgnoreResult();     // writing to the binary directory
    xiiFileSystem::AddDataDirectory(sAppDir, "AppData", "app").IgnoreResult();                                   // app specific data
    xiiFileSystem::AddDataDirectory(sUserData, "AppData", "appdata", xiiFileSystem::AllowWrites).IgnoreResult(); // for writing app user data
  }

  {
    XII_PROFILE_SCOPE("Logging");
    xiiInt32         iApplicationID = pCmd->GetIntOption("-appid", 0);
    xiiStringBuilder sLogFile;
    sLogFile.SetFormat(":appdata/Log_{0}.htm", iApplicationID);
    m_LogHTML.BeginLog(sLogFile, sApplicationName);

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLoggingEvent::Handler(&xiiLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  }
  xiiUniquePtr<xiiTranslatorFromFiles> pTranslatorEn = XII_DEFAULT_NEW(xiiTranslatorFromFiles);
  m_pTranslatorFromFiles                             = pTranslatorEn.Borrow();

  // xiiUniquePtr<xiiTranslatorFromFiles> pTranslatorDe = XII_DEFAULT_NEW(xiiTranslatorFromFiles);

  pTranslatorEn->AddTranslationFilesFromFolder(":app/Localization/en");
  // pTranslatorDe->LoadTranslationFilesFromFolder(":app/Localization/de");

  xiiTranslationLookup::AddTranslator(XII_DEFAULT_NEW(xiiTranslatorMakeMoreReadable));
  // xiiTranslationLookup::AddTranslator(XII_DEFAULT_NEW(xiiTranslatorLogMissing));
  xiiTranslationLookup::AddTranslator(std::move(pTranslatorEn));
  // xiiTranslationLookup::AddTranslator(std::move(pTranslatorDe));

  LoadEditorPreferences();

  xiiQtUiServices::GetSingleton()->LoadState();

  if (!IsInHeadlessMode())
  {
    xiiActionManager::LoadShortcutAssignment();

    LoadRecentFiles();

    CreatePanels();

    ShowSettingsDocument();

    if (!IsInUnitTestMode())
    {
      connect(m_pVersionChecker.Borrow(), &xiiQtVersionChecker::VersionCheckCompleted, this, &xiiQtEditorApp::SlotVersionCheckCompleted, Qt::QueuedConnection);

      m_pVersionChecker->Initialize();
      m_pVersionChecker->Check(false);
    }
  }

  LoadEditorPlugins();
  CloseSplashScreen();

  {
    xiiEditorAppEvent e;
    e.m_Type = xiiEditorAppEvent::Type::EditorStarted;
    m_Events.Broadcast(e);
  }

  xiiEditorApplicationPreferences* pPreferences = xiiPreferences::QueryPreferences<xiiEditorApplicationPreferences>();

  if (pCmd->GetStringOptionArguments("-newproject") > 0)
  {
    CreateOrOpenProject(true, pCmd->GetAbsolutePathOption("-newproject")).IgnoreResult();
  }
  else if (pCmd->GetStringOptionArguments("-project") > 0)
  {
    for (xiiUInt32 doc = 0; doc < pCmd->GetStringOptionArguments("-documents"); ++doc)
    {
      m_DocumentsToOpen.PushBack(pCmd->GetStringOption("-documents", doc));
    }

    CreateOrOpenProject(false, pCmd->GetAbsolutePathOption("-project")).IgnoreResult();
  }
  else if (!bNoRecent && pPreferences->m_bLoadLastProjectAtStartup)
  {
    if (!m_RecentProjects.GetFileList().IsEmpty())
    {
      CreateOrOpenProject(false, m_RecentProjects.GetFileList()[0].m_File).IgnoreResult();
    }
  }
  else if (!IsInHeadlessMode() && !IsInSafeMode())
  {
    if (xiiQtContainerWindow::GetContainerWindow())
    {
      xiiQtContainerWindow::GetContainerWindow()->ScheduleRestoreWindowLayout();
    }
  }

  connect(m_pTimer, SIGNAL(timeout()), this, SLOT(SlotTimedUpdate()), Qt::QueuedConnection);
  m_pTimer->start(1);

  if (m_bWroteCrashIndicatorFile)
  {
    QTimer::singleShot(1000, [this]() {
      xiiStringBuilder sTemp = xiiOSFile::GetTempDataFolder("xiiEditor");
      sTemp.AppendPath("xiiEditorCrashIndicator");
      xiiOSFile::DeleteFile(sTemp).IgnoreResult();
      m_bWroteCrashIndicatorFile = false;
      //
    });
  }

  if (m_StartupFlags.AreNoneSet(StartupFlags::Headless | StartupFlags::UnitTest) && !xiiToolsProject::GetSingleton()->IsProjectOpen())
  {
    GuiOpenDashboard();
  }
}

void xiiQtEditorApp::ShutdownEditor()
{
  xiiToolsProject::SaveProjectState();

  m_pTimer->stop();

  xiiToolsProject::CloseProject();

  m_LongOpControllerManager.Shutdown();

  xiiEditorEngineProcessConnection::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorApp::EngineProcessMsgHandler, this));
  xiiToolsProject::s_Requests.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorApp::ProjectRequestHandler, this));
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorApp::ProjectEventHandler, this));
  xiiDocument::s_EventsAny.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorApp::DocumentEventHandler, this));
  xiiDocumentManager::s_Requests.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorApp::DocumentManagerRequestHandler, this));
  xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorApp::DocumentManagerEventHandler, this));
  xiiQtDocumentWindow::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorApp::DocumentWindowEventHandler, this));
  xiiQtUiServices::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorApp::UiServicesEvents, this));

  xiiQtUiServices::GetSingleton()->SaveState();

  CloseSettingsDocument();

  if (!IsInHeadlessMode())
  {
    delete xiiQtContainerWindow::GetContainerWindow();
  }
  // HACK to figure out why the panels are not always properly destroyed together with the ContainerWindows
  // if you run into this, please try to figure this out
  // every xiiQtApplicationPanel actually registers itself with a container window in its constructor
  // there its Qt 'parent' is set to the container window (there is only one)
  // that means, when the application is shut down, all xiiQtApplicationPanel instances should get deleted by their parent
  // ie. the container window
  // however, SOMETIMES this does not happen
  // it seems to be related to whether a panel has been opened/closed (ie. shown/hidden), and maybe also with the restored state
  {
    const auto& Panels      = xiiQtApplicationPanel::GetAllApplicationPanels();
    xiiUInt32   uiNumPanels = Panels.GetCount();

    XII_ASSERT_DEBUG(uiNumPanels == 0, "Not all panels have been cleaned up correctly");

    for (xiiUInt32 i = 0; i < uiNumPanels; ++i)
    {
      xiiQtApplicationPanel* pPanel = Panels[i];
      delete pPanel;
    }
  }

  QCoreApplication::sendPostedEvents();
  qApp->processEvents();

  delete m_pEngineViewProcess;

  // Unload potential plugin referenced clipboard data to prevent crash on shutdown.
  QApplication::clipboard()->clear();
  xiiPlugin::UnloadAllPlugins();

  if (m_bWroteCrashIndicatorFile)
  {
    // orderly shutdown -> make sure the crash indicator file is gone
    xiiStringBuilder sTemp = xiiOSFile::GetTempDataFolder("xiiEditor");
    sTemp.AppendPath("xiiEditorCrashIndicator");
    xiiOSFile::DeleteFile(sTemp).IgnoreResult();
    m_bWroteCrashIndicatorFile = false;
  }

  // make sure no one tries to load any further images in parallel
  xiiQtImageCache::GetSingleton()->StopRequestProcessing(true);

  xiiTranslationLookup::Clear();

  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::Console::LogMessageHandler);
  xiiGlobalLog::RemoveLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
  xiiGlobalLog::RemoveLogWriter(xiiLoggingEvent::Handler(&xiiLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  XII_DEFAULT_DELETE(m_pQtProgressbar);
  XII_DEFAULT_DELETE(m_pProgressbar);
}

void xiiQtEditorApp::CreatePanels()
{
  XII_PROFILE_SCOPE("CreatePanels");
  xiiQtApplicationPanel* pAssetBrowserPanel = new xiiQtAssetBrowserPanel();
  xiiQtApplicationPanel* pLogPanel          = new xiiQtLogPanel();
  xiiQtApplicationPanel* pLongOpsPanel      = new xiiQtLongOpsPanel();
  xiiQtApplicationPanel* pCVarPanel         = new xiiQtCVarPanel();
  xiiQtApplicationPanel* pAssetCuratorPanel = new xiiQtAssetCuratorPanel();

  xiiQtContainerWindow* pMainWnd     = xiiQtContainerWindow::GetContainerWindow();
  ads::CDockManager*    pDockManager = pMainWnd->GetDockManager();
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetBrowserPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLogPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pAssetCuratorPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pCVarPanel);
  pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pLongOpsPanel);

  pAssetBrowserPanel->raise();
}

xiiCommandLineOptionBool opt_NoSplashScreen("_Editor", "-NoSplash", "Disables the editor splash-screen", false);

void xiiQtEditorApp::SetupAndShowSplashScreen()
{
  XII_ASSERT_DEV(m_pSplashScreen == nullptr, "Splash screen shouldn't exist already.");

  if (m_StartupFlags.IsAnySet(xiiQtEditorApp::StartupFlags::UnitTest))
    return;

  if (opt_NoSplashScreen.GetOptionValue(xiiCommandLineOption::LogMode::Never))
    return;

  bool bShowSplashScreen = true;

  // preferences are not yet available here
  {
    QSettings s;
    s.beginGroup("EditorPreferences");
    bShowSplashScreen = s.value("ShowSplashscreen", true).toBool();
    s.endGroup();
  }

  if (!bShowSplashScreen)
    return;

// QSvgRenderer svgRenderer(QString(":/Splash/Splash/splash.svg"));

// const qreal PixelRatio = qApp->primaryScreen()->devicePixelRatio();

// TODO: When migrating to Qt 5.15 or newer this should have a fixed square size and
// let the aspect ratio mode of the svg renderer handle the difference
#if 0
  QPixmap splashPixmap(QSize(187, 256) * PixelRatio);
  splashPixmap.fill(Qt::transparent);
  {
    QPainter painter;
    painter.begin(&splashPixmap);
    svgRenderer.render(&painter);
    painter.end();
  }
#endif

  QPixmap splashPixmap(QString(":/Splash/Splash/splash.png"));

  // splashPixmap.setDevicePixelRatio(PixelRatio);

  m_pSplashScreen = new QSplashScreen(splashPixmap);
  m_pSplashScreen->setMask(splashPixmap.mask());

  // Don't set always on top if a debugger is attached to prevent it being stuck over the debugger.
  if (!xiiSystemInformation::IsDebuggerAttached())
  {
    m_pSplashScreen->setWindowFlag(Qt::WindowStaysOnTopHint, true);
  }
  m_pSplashScreen->show();
}

void xiiQtEditorApp::CloseSplashScreen()
{
  if (!m_pSplashScreen)
    return;

  XII_ASSERT_DEBUG(QThread::currentThread() == this->thread(), "CloseSplashScreen must be called from the main thread");
  QSplashScreen* pLocalSplashScreen = m_pSplashScreen;
  m_pSplashScreen                   = nullptr;

  pLocalSplashScreen->finish(xiiQtContainerWindow::GetContainerWindow());
  // if the deletion is done 'later', the splashscreen can end up as the parent window of other things
  // like messageboxes, and then the deletion will make the app crash
  delete pLocalSplashScreen;
}
