/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFrameworkTest/AssetDependenciesTest.h>
#include <EditorFrameworkTest/AssetDependencyTrackingTest.h>
#include <EditorFrameworkTest/AssetProcessorTest.h>
#include <EditorFrameworkTest/AssetProfileTest.h>
#include <EditorFrameworkTest/AssetReplacementTest.h>
#include <EditorFrameworkTest/AssetResaveTest.h>
#include <EditorFrameworkTest/DocumentSaveUiTest.h>
#include <EditorFrameworkTest/PropertyAnimationTestUtils.h>
#include <EditorFrameworkTest/RenderPipelineSettingsEditorTest.h>
#include <EditorFrameworkTest/SceneWindowTest.h>

#include <Core/Physics/SurfaceResource.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QPointer>
#include <QProgressBar>
#include <QProgressDialog>
#include <QTimer>
#include <QTreeView>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Project/ToolsProject.h>

XII_CREATE_SIMPLE_TEST_GROUP(EditorIntegration);

XII_CREATE_SIMPLE_TEST(EditorIntegration, Startup)
{
  auto* editor = xiiQtEditorApp::GetSingleton();
  if (!XII_TEST_BOOL(editor != nullptr))
    return;
  // Linked test plugins initialize before the Editor's data mounts. Keep their initial
  // startup separate, then use the same read-only tool-data identity as EditorProcessor.
  xiiStartup::StartupCoreSystems();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  xiiStringBuilder preferences = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  preferences.AppendPath("EditorStartupPreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  editor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor->ShutdownEditor());
  XII_TEST_BOOL(editor->IsRunning());
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Processor rejects work before project readiness")
  {
    xiiAssetProcessor::GetSingleton()->StartProcessTask();
    XII_TEST_BOOL(xiiAssetProcessor::GetSingleton()->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Stopped);
    XII_TEST_BOOL(xiiAssetProcessor::GetSingleton()->QueueTransformAllAssets().Failed());
  }
  xiiStringBuilder preferenceRoot = xiiApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
  preferenceRoot.Trim("/\\");
  preferences.Trim("/\\");
  XII_TEST_STRING(preferenceRoot, preferences);
  XII_TEST_BOOL(xiiQtContainerWindow::GetContainerWindow() != nullptr);
  QApplication::processEvents();
  XII_TEST_BOOL(!xiiQtDocumentWindow::GetAllDocumentWindows().IsEmpty());
  const xiiDocumentTypeDescriptor* source = nullptr;
  XII_TEST_RESULT(xiiDocumentManager::FindDocumentTypeFromPath("Existing.xiiPropertyAnimAsset", false, source));
  if (XII_TEST_BOOL(source != nullptr))
  {
    XII_TEST_BOOL(!source->m_bCanCreate);
    XII_TEST_STRING(source->m_sDocumentTypeName, "PropertyAnim");
  }
  const xiiDocumentTypeDescriptor* created = nullptr;
  XII_TEST_BOOL(xiiDocumentManager::FindDocumentTypeFromPath("New.xiiPropertyAnimAsset", true, created).Failed());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Project creation and a production source window")
  {
    xiiStringBuilder project = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    project.AppendPath("SourceWindowProject", "xiiProject");
    if (!XII_TEST_RESULT(editor->CreateOrOpenProject(true, project)))
      return;
    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Graceful processor stop and restart")
    {
      auto* processor = xiiAssetProcessor::GetSingleton();
      processor->StartProcessTask();
      XII_TEST_BOOL(processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Running);
      processor->StopProcessTask(false);
      processor->StopProcessTask(false, true);
      XII_TEST_BOOL(processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Stopped);
      processor->StartProcessTask();
      processor->StopProcessTask(false, true);
      XII_TEST_BOOL(processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Stopped);
    }

    XII_TEST_BOOL(xiiToolsProject::IsProjectOpen());
    TestDocumentSaveUi(*editor, project.GetFileDirectory());
    TestAssetReplacement(*editor, project.GetFileDirectory());
    xiiStringBuilder path = project.GetFileDirectory();
    path.AppendPath("Source.xiiPropertyAnimAsset");
    PropertyAnimationSourceManager fixtures;
    xiiDocument*                   base = nullptr;
    XII_TEST_STATUS(fixtures.CreateDocument("PropertyAnim", path, base, xiiDocumentFlags::EmptyDocument));
    if (!XII_TEST_BOOL(base != nullptr))
      return;
    auto* fixture  = static_cast<xiiPropertyAnimAssetDocument*>(base);
    auto* accessor = fixture->GetObjectAccessor();
    accessor->StartTransaction("Create source fixture");
    xiiUuid track;
    XII_TEST_STATUS(accessor->AddObjectByName(fixture->GetPropertyObject(), "Tracks", -1, xiiGetStaticRTTI<xiiPropertyAnimationTrack>(), track));
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(track), "ObjectPath", "Root/Child"));
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(track), "Property", "LocalPosition"));
    accessor->FinishTransaction();
    fixture->InsertCurveCpAt(track, 240, 7.5);
    const auto identity = fixture->GetGuid();
    XII_TEST_STATUS(fixture->SaveDocument());
    fixtures.CloseDocument(fixture);

    auto* document = static_cast<xiiPropertyAnimAssetDocument*>(editor->OpenDocument(path, xiiDocumentFlags::RequestWindow));
    if (!XII_TEST_BOOL(document != nullptr))
      return;
    XII_TEST_BOOL(document->GetGuid() == identity);
    XII_TEST_BOOL(document->GetEditorEngineConnection() == nullptr);
    auto* window = xiiQtDocumentWindow::FindWindowByDocument(document);
    if (!XII_TEST_BOOL(window != nullptr))
      return;
    XII_TEST_STRING(window->GetWindowLayoutGroupName(), "PropertyAnimAsset");

    auto pump = []() {
      QElapsedTimer elapsed;
      elapsed.start();
      while (elapsed.elapsed() < 150)
        QApplication::processEvents(QEventLoop::AllEvents, 20);
    };
    pump();
    QTreeView* tree = nullptr;
    for (auto* candidate : window->findChildren<QTreeView*>())
    {
      if (xiiStringView(candidate->metaObject()->className()) == "xiiQtPropertyAnimAssetTreeView")
        tree = candidate;
    }
    if (!XII_TEST_BOOL(tree != nullptr))
      return;
    QModelIndex leaf = tree->model()->index(0, 0);
    while (tree->model()->rowCount(leaf) > 0)
      leaf = tree->model()->index(0, 0, leaf);
    if (!XII_TEST_BOOL(leaf.isValid()))
      return;
    tree->selectionModel()->select(leaf, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    // Exercise the interval before the queued Qt model rebuild, when native mirror objects have already been deleted/recreated.
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      auto* sourceAccessor = document->GetObjectAccessor();
      sourceAccessor->StartTransaction("Remove selected source track");
      XII_TEST_STATUS(sourceAccessor->RemoveObject(sourceAccessor->GetObject(track)));
      sourceAccessor->FinishTransaction();
      XII_TEST_BOOL(QMetaObject::invokeMethod(window, "onTreeItemDoubleClicked", Qt::DirectConnection, Q_ARG(QModelIndex, leaf)));
      XII_TEST_STATUS(document->GetCommandHistory()->Undo());
      XII_TEST_BOOL(QMetaObject::invokeMethod(window, "onTreeItemDoubleClicked", Qt::DirectConnection, Q_ARG(QModelIndex, leaf)));
      XII_TEST_STATUS(document->GetCommandHistory()->Redo());
      XII_TEST_STATUS(document->GetCommandHistory()->Undo());
    }
    pump();
    XII_TEST_INT(document->GetProperties()->m_Tracks.GetCount(), 1);
    XII_TEST_STATUS(window->SaveDocument());
    QPointer<xiiQtDocumentWindow> closedWindow = window;
    document->GetDocumentManager()->CloseDocument(document);
    pump();
    XII_TEST_BOOL(closedWindow.isNull());
    TestRenderPipelineSettingsWindow(*editor, project.GetFileDirectory());
    TestSceneWindow(*editor, project.GetFileDirectory());
    TestExposedAssetDependencies(*editor, project.GetFileDirectory());
    TestAssetResave(*editor, project.GetFileDirectory());
    TestAssetDependencyTracking(*editor, project.GetFileDirectory());
    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Production asset processor IPC")
    {
      TestAssetProcessor(*editor, project.GetFileDirectory());
    }
    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native asset profile evaluation and switching")
    {
      TestAssetProfiles(*editor, project.GetFileDirectory());
      TestAssetProfileDialog(project.GetFileDirectory());
    }
    editor->CloseProject();
    pump();
    XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
  }
}


XII_CREATE_SIMPLE_TEST_GROUP(EditorBackgroundTransforms);

XII_CREATE_SIMPLE_TEST_GROUP(EditorBackgroundImports);

XII_CREATE_SIMPLE_TEST_GROUP(EditorBackgroundAssetChecks);

XII_CREATE_SIMPLE_TEST(EditorBackgroundAssetChecks, ResponsiveRescanAndShutdown)
{
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/CheckPreferences");
  const xiiStringBuilder project(preferences, "/Project/xiiProject");
  XII_TEST_BOOL(xiiToolsProject::CreateProject(project).Succeeded());
  xiiToolsProject::CloseProject();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor.ShutdownEditor());
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, project)))
    return;
  auto* curator   = xiiAssetCurator::GetSingleton();
  auto* processor = xiiAssetProcessor::GetSingleton();
  auto* files     = xiiFileSystemModel::GetSingleton();
  class ModalObserver : public QObject
  {
  public:
    bool eventFilter(QObject* object, QEvent* event) override
    {
      if (event->type() == QEvent::Show && qobject_cast<QProgressDialog*>(object))
        ++m_uiDialogs;
      return false;
    }
    xiiUInt32 m_uiDialogs = 0;
  } observer;
  qApp->installEventFilter(&observer);
  XII_SCOPE_EXIT(qApp->removeEventFilter(&observer));
  xiiThreadSignal   release;
  std::atomic<bool> entered      = false;
  auto              subscription = files->m_FileChangedEvents.AddEventHandler([&](const xiiFileChangedEvent& event) {
    if (event.m_Type == xiiFileChangedEvent::Type::ModelReset && !xiiThreadUtils::IsMainThread() && !entered.exchange(true))
    {
      release.WaitForSignal(xiiTime::MakeFromSeconds(10));
    }
  });
  XII_SCOPE_EXIT(release.RaiseSignal(); editor.CloseProject(); files->m_FileChangedEvents.RemoveEventHandler(subscription));
  xiiUInt32 started = 0, finished = 0;
  auto      curatorSubscription = curator->m_Events.AddEventHandler([&](const xiiAssetCuratorEvent& event) {
    if (event.m_Type == xiiAssetCuratorEvent::Type::FileSystemCheckStarted)
    {
      XII_TEST_BOOL(xiiThreadUtils::IsMainThread());
      ++started;
    }
    if (event.m_Type == xiiAssetCuratorEvent::Type::AssetListReset && !curator->IsCheckingFileSystem())
    {
      XII_TEST_BOOL(xiiThreadUtils::IsMainThread());
      ++finished;
    }
  });
  XII_SCOPE_EXIT(curator->m_Events.RemoveEventHandler(curatorSubscription));
  const xiiStringBuilder source(preferences, "/Project/CheckFixture.txt");
  {
    xiiOSFile file;
    XII_TEST_RESULT(file.Open(source, xiiFileOpenMode::Write));
    XII_TEST_RESULT(file.Write("fixture", 7));
  }
  xiiQtCuratorControl control(nullptr);
  // Exercise the actual Start Background Processing button, including its rescan.
  XII_TEST_BOOL(QMetaObject::invokeMethod(&control, "BackgroundProcessClicked", Qt::DirectConnection, Q_ARG(bool, true)));
  XII_TEST_BOOL(curator->IsCheckingFileSystem());
  curator->QueueCheckFileSystem();
  XII_TEST_INT(started, 1);                // Coalesce repeated requests while the first scan is active.
  processor->StopProcessTask(false, true); // Stop remains responsive while scanning.
  QLineEdit editing;
  QTimer    heartbeat;
  xiiUInt32 heartbeats = 0;
  QObject::connect(&heartbeat, &QTimer::timeout, &editing, [&]() {
    editing.setText(QString::number(++heartbeats));
    XII_TEST_BOOL(QApplication::activeModalWidget() == nullptr);
    XII_TEST_BOOL(!editor.IsProgressBarProcessingEvents());
    if (entered && heartbeats >= 3)
      release.RaiseSignal();
  });
  heartbeat.start(5);
  auto*         progress = control.findChild<QProgressBar*>();
  QElapsedTimer timeout;
  timeout.start();
  while (curator->IsCheckingFileSystem() && timeout.elapsed() < 15000)
  {
    QApplication::processEvents(QEventLoop::AllEvents, 20);
    if (progress && curator->IsCheckingFileSystem())
    {
      XII_TEST_BOOL(!progress->isHidden());
      XII_TEST_INT(progress->maximum(), 0);
    }
    curator->MainThreadTick(true);
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  }
  QApplication::processEvents();
  XII_TEST_BOOL(entered);
  XII_TEST_BOOL(!curator->IsCheckingFileSystem());
  XII_TEST_BOOL(heartbeats >= 3 && !editing.text().isEmpty());
  XII_TEST_INT(finished, 1);
  XII_TEST_BOOL(progress && progress->isHidden());
  XII_TEST_INT(observer.m_uiDialogs, 0);
  xiiFileStatus status;
  XII_TEST_RESULT(files->FindFile(source, status));
  XII_TEST_BOOL(processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Stopped);
  // A direct synchronous caller must also stay free of global modal progress.
  curator->CheckFileSystem();
  XII_TEST_INT(observer.m_uiDialogs, 0);
  // Closing the project joins a queued scan before destroying its file model.
  curator->QueueCheckFileSystem();
  editor.CloseProject();
  timeout.restart();
  while (xiiToolsProject::IsProjectOpen() && timeout.elapsed() < 15000)
  {
    QApplication::processEvents(QEventLoop::AllEvents, 20);
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  }
  XII_TEST_BOOL(!curator->IsCheckingFileSystem());
  XII_TEST_BOOL(!xiiToolsProject::IsProjectOpen());
}

XII_CREATE_SIMPLE_TEST(EditorBackgroundImports, MeshAndSkeleton)
{
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/ImportPreferences");
  const xiiStringBuilder project(preferences, "/Project/xiiProject");
  XII_TEST_BOOL(xiiToolsProject::CreateProject(project).Succeeded());
  xiiToolsProject::CloseProject();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor.ShutdownEditor());
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, project)))
    return;
  auto* processor = xiiAssetProcessor::GetSingleton();
  auto* curator   = xiiAssetCurator::GetSingleton();
  class ReservationLog : public xiiTestLogInterface
  {
    void HandleLogMessage(const xiiLoggingEventData& e) override
    {
      // UnitTest mode retains synchronous after-save transforms. The real
      // background reservation must reject that additional legacy attempt.
      if (e.m_EventType != xiiLogMsgType::ErrorMsg || !e.m_sText.FindSubString("Asset is being transformed in the background") || !e.m_sText.FindSubString("Triangle.xiiSkeletonAsset"))
        xiiTestLogInterface::HandleLogMessage(e);
    }
  } reservationLog;
  xiiTestLogSystemScope  logScope(&reservationLog);
  const xiiStringBuilder source(preferences, "/Project/Triangle.obj");
  {
    xiiOSFile           file;
    const xiiStringView geometry = "o Triangle\nv 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    XII_TEST_RESULT(file.Open(source, xiiFileOpenMode::Write));
    XII_TEST_RESULT(file.Write(geometry.GetStartPointer(), geometry.GetElementCount()));
  }
  class ModalObserver : public QObject
  {
  public:
    bool eventFilter(QObject* object, QEvent* event) override
    {
      if (event->type() == QEvent::Show && qobject_cast<QProgressDialog*>(object))
        ++m_uiDialogs;
      return false;
    }
    xiiUInt32 m_uiDialogs = 0;
  } observer;
  qApp->installEventFilter(&observer);
  XII_SCOPE_EXIT(qApp->removeEventFilter(&observer));
  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  xiiAssetDocumentGenerator::CreateGenerators(generators);
  XII_SCOPE_EXIT(xiiAssetDocumentGenerator::DestroyGenerators(generators));
  xiiAssetDocumentGenerator* meshGenerator = nullptr;
  for (auto* generator : generators)
    if (generator->GetDocumentExtension() == "xiiMeshAsset")
      meshGenerator = generator;
  if (!XII_TEST_BOOL(meshGenerator != nullptr))
    return;
  xiiStringBuilder meshPath(source);
  meshPath.ChangeFileExtension("xiiMeshAsset");
  xiiOSFile::FindFreeFilename(meshPath);
  XII_TEST_STATUS(meshGenerator->Import(source, "MeshImport.NoMaterials", false));
  auto* mesh = xiiDynamicCast<xiiAssetDocument*>(editor.OpenDocument(meshPath, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(mesh != nullptr))
    return;
  auto* skeleton = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(xiiStringBuilder(preferences, "/Project/Triangle.xiiSkeletonAsset"), xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(skeleton != nullptr))
    return;
  auto* skeletonAccessor = skeleton->GetObjectAccessor();
  skeletonAccessor->StartTransaction("Set import source");
  XII_TEST_STATUS(skeletonAccessor->SetValueByName(skeleton->GetObjectManager()->GetRootObject()->GetChildren()[0], "File", source.GetView()));
  skeletonAccessor->FinishTransaction();
  XII_TEST_STATUS(skeleton->SaveDocument());
  const xiiUuid assets[] = {mesh->GetGuid(), skeleton->GetGuid()};
  QLineEdit     editing;
  QTimer        heartbeat;
  xiiUInt32     heartbeats = 0;
  QObject::connect(&heartbeat, &QTimer::timeout, &editing, [&]() {
    editing.setText(QString::number(++heartbeats));
    XII_TEST_BOOL(QApplication::activeModalWidget() == nullptr);
    XII_TEST_BOOL(!editor.IsProgressBarProcessingEvents());
  });
  heartbeat.start(5);
  auto wait = [&]() {
    QElapsedTimer timeout;
    timeout.start();
    while ((processor->GetTransformProgress().m_bActive || processor->GetProcessTaskState() != xiiAssetProcessor::ProcessTaskState::Stopped) && timeout.elapsed() < 90000)
    {
      QApplication::processEvents(QEventLoop::AllEvents, 20);
      curator->MainThreadTick(true);
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    }
    XII_TEST_BOOL(!processor->GetTransformProgress().m_bActive);
    XII_TEST_BOOL(processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Stopped);
  };
  XII_TEST_STATUS(processor->QueueTransformAssets(assets, xiiTransformFlags::TriggeredManually | xiiTransformFlags::ForceTransform));
  wait();
  XII_TEST_INT(processor->GetTransformProgress().m_uiFailed, 0);
  XII_TEST_INT(processor->GetTransformProgress().m_uiNeedsImport, 0);
  XII_TEST_BOOL(heartbeats > 0);
  XII_TEST_INT(observer.m_uiDialogs, 0);
  XII_TEST_BOOL(mesh->GetObjectAccessor()->GetCountByName(mesh->GetObjectManager()->GetRootObject()->GetChildren()[0], "Materials") > 0);
  XII_TEST_BOOL(skeletonAccessor->GetCountByName(skeleton->GetObjectManager()->GetRootObject()->GetChildren()[0], "Children") > 0);
  for (const auto& guid : assets)
    XII_TEST_BOOL(!curator->IsAssetBeingTransformed(guid));
  // A live edit must survive an in-flight import. Defer reconciliation until saved.
  auto* accessor = mesh->GetObjectAccessor();
  accessor->StartTransaction("Clear mesh slots for reimport");
  auto* properties = mesh->GetObjectManager()->GetRootObject()->GetChildren()[0];
  XII_TEST_STATUS(accessor->ClearByName(properties, "Materials"));
  XII_TEST_STATUS(accessor->SetValueByName(properties, "ImportMaterials", true));
  accessor->FinishTransaction();
  XII_TEST_STATUS(mesh->SaveDocument());
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(assets, 1), xiiTransformFlags::TriggeredManually | xiiTransformFlags::ForceTransform));
  QElapsedTimer reserved;
  reserved.start();
  while (!curator->IsAssetBeingTransformed(assets[0]) && reserved.elapsed() < 5000)
  {
    QApplication::processEvents();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  }
  accessor->StartTransaction("Edit while importing");
  XII_TEST_STATUS(accessor->SetValueByName(properties, "UniformScaling", 2.0f));
  accessor->FinishTransaction();
  wait();
  xiiVariant scaling;
  XII_TEST_STATUS(accessor->GetValueByName(properties, "UniformScaling", scaling));
  XII_TEST_FLOAT(scaling.Get<float>(), 2.0f, 0.0f);
  XII_TEST_BOOL(mesh->IsModified());
  XII_TEST_INT(accessor->GetCountByName(properties, "Materials"), 0);
  XII_TEST_STATUS(mesh->SaveDocument());
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(assets, 1)));
  wait();
  XII_TEST_BOOL(accessor->GetCountByName(properties, "Materials") > 0);
  XII_TEST_INT(processor->GetTransformProgress().m_uiFailed, 0);
  const auto* slot = accessor->GetChildObjectByName(properties, "Materials", 0);
  if (XII_TEST_BOOL(slot != nullptr))
  {
    xiiVariant material;
    XII_TEST_STATUS(accessor->GetValueByName(slot, "Resource", material));
    XII_TEST_BOOL(!material.Get<xiiString>().IsEmpty());
    XII_TEST_BOOL(curator->FindSubAsset(material.Get<xiiString>()).isValid());
  }
  // Closing with an import in flight must not wait for the GUI's next tick.
  accessor->StartTransaction("Request import before shutdown");
  XII_TEST_STATUS(accessor->ClearByName(properties, "Materials"));
  accessor->FinishTransaction();
  XII_TEST_STATUS(mesh->SaveDocument());
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(assets, 1), xiiTransformFlags::TriggeredManually | xiiTransformFlags::ForceTransform));
  reserved.restart();
  while (!curator->IsAssetBeingTransformed(assets[0]) && reserved.elapsed() < 5000)
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  XII_TEST_BOOL(curator->IsAssetBeingTransformed(assets[0]));
  processor->StopProcessTask(false, true);
  XII_TEST_BOOL(processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Stopped);
  XII_TEST_BOOL(!curator->IsAssetBeingTransformed(assets[0]));
  XII_TEST_INT(accessor->GetCountByName(properties, "Materials"), 0);
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(assets, 1)));
  wait();
  XII_TEST_BOOL(accessor->GetCountByName(properties, "Materials") > 0);
  XII_TEST_INT(observer.m_uiDialogs, 0);
}

XII_CREATE_SIMPLE_TEST_GROUP(EditorAssetProcessor);

XII_CREATE_SIMPLE_TEST(EditorAssetProcessor, ProtocolAndProfiles)
{
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/ProcessorPreferences");
  const xiiStringBuilder project(preferences, "/Project/xiiProject");
  XII_TEST_BOOL(xiiToolsProject::CreateProject(project).Succeeded());
  xiiToolsProject::CloseProject();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor.ShutdownEditor());
  if (XII_TEST_RESULT(editor.CreateOrOpenProject(false, project)))
    TestAssetProcessor(editor, project.GetFileDirectory());
}

XII_CREATE_SIMPLE_TEST(EditorBackgroundTransforms, BatchFailureAndCancellation)
{
  auto&           editor       = *xiiQtEditorApp::GetSingleton();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/BackgroundPreferences");
  const xiiStringBuilder project(preferences, "/Project/xiiProject");
  const xiiStringBuilder previousFailure(preferences, "/Project/Batch/Failure.xiiSurfaceAsset");
  if (xiiOSFile::ExistsFile(previousFailure))
    XII_TEST_RESULT(xiiOSFile::DeleteFile(previousFailure));
  XII_TEST_BOOL(xiiToolsProject::CreateProject(project).Succeeded());
  xiiToolsProject::CloseProject();
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor.ShutdownEditor());
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(false, project)))
    return;
  auto*                    processor = xiiAssetProcessor::GetSingleton();
  auto*                    curator   = xiiAssetCurator::GetSingleton();
  xiiDynamicArray<xiiUuid> assets;
  for (xiiUInt32 i = 0; i < 32; ++i)
  {
    xiiStringBuilder path(preferences);
    path.AppendFormat("/Project/Batch/Surface{}.xiiSurfaceAsset", i);
    auto* document = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(document != nullptr))
      return;
    XII_TEST_STATUS(document->SaveDocument());
    assets.PushBack(document->GetGuid());
    document->GetDocumentManager()->CloseDocument(document);
  }
  const xiiStringBuilder firstPath(preferences, "/Project/Batch/Surface0.xiiSurfaceAsset");
  auto*                  dependent = xiiDynamicCast<xiiAssetDocument*>(editor.OpenDocument(firstPath, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(dependent != nullptr))
    return;
  xiiStringBuilder dependencyId;
  xiiConversionUtils::ToString(assets.PeekBack(), dependencyId);
  auto* dependencyAccessor = dependent->GetObjectAccessor();
  dependencyAccessor->StartTransaction("Set shared background dependency");
  XII_TEST_STATUS(dependencyAccessor->SetValueByName(dependent->GetObjectManager()->GetRootObject()->GetChildren()[0], "BaseSurface", dependencyId.GetView()));
  dependencyAccessor->FinishTransaction();
  XII_TEST_STATUS(dependent->SaveDocument());
  dependent->GetDocumentManager()->CloseDocument(dependent);
  class ModalObserver : public QObject
  {
  public:
    bool eventFilter(QObject* object, QEvent* event) override
    {
      if (event->type() == QEvent::Show && qobject_cast<QProgressDialog*>(object))
        ++m_uiDialogs;
      return false;
    }
    xiiUInt32 m_uiDialogs = 0;
  } observer;
  qApp->installEventFilter(&observer);
  XII_SCOPE_EXIT(qApp->removeEventFilter(&observer));
  QLineEdit editing;
  QTimer    heartbeat;
  xiiUInt32 uiHeartbeats = 0;
  QObject::connect(&heartbeat, &QTimer::timeout, &editing, [&]() {
    ++uiHeartbeats;
    editing.setText(QString::number(uiHeartbeats));
    XII_TEST_BOOL(QApplication::activeModalWidget() == nullptr);
    XII_TEST_BOOL(!editor.IsProgressBarProcessingEvents());
  });
  heartbeat.start(5);
  auto wait = [&]() {
    QElapsedTimer timeout;
    timeout.start();
    while ((processor->GetTransformProgress().m_bActive || processor->GetProcessTaskState() != xiiAssetProcessor::ProcessTaskState::Stopped) && timeout.elapsed() < 90000)
    {
      QApplication::processEvents(QEventLoop::AllEvents, 20);
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    }
    QApplication::processEvents();
    XII_TEST_BOOL(!processor->GetTransformProgress().m_bActive);
    XII_TEST_BOOL(processor->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Stopped);
  };
  XII_TEST_STATUS(processor->QueueTransformAssets(assets, xiiTransformFlags::TriggeredManually | xiiTransformFlags::ForceTransform));
  XII_TEST_STATUS(processor->QueueTransformAssets(assets)); // Coalesce duplicate submissions.
  XII_TEST_INT(processor->GetTransformProgress().m_uiTotal, assets.GetCount());
  wait();
  auto progress = processor->GetTransformProgress();
  XII_TEST_INT(progress.m_uiCompleted, assets.GetCount());
  XII_TEST_INT(progress.m_uiFailed, 0);
  XII_TEST_BOOL(uiHeartbeats > 0 && !editing.text().isEmpty());
  XII_TEST_INT(observer.m_uiDialogs, 0);
  for (const auto& guid : assets)
  {
    XII_TEST_BOOL(!curator->IsAssetBeingTransformed(guid));
    xiiProcessAssetMsg request;
    XII_TEST_RESULT(xiiAssetProcessor::PrepareAssetRequest(guid, request));
    xiiUInt64 hash = 0, thumb = 0, package = 0;
    XII_TEST_BOOL(curator->IsAssetUpToDate(guid, curator->GetActiveAssetProfile(), nullptr, hash, thumb, package, true) == xiiAssetInfo::UpToDate);
  }
  // A saved edit after reservation must trigger another request before publishing/reloading.
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(assets.GetData(), 1), xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually));
  QElapsedTimer editTimeout;
  editTimeout.start();
  while (!curator->IsAssetBeingTransformed(assets[0]) && editTimeout.elapsed() < 5000)
  {
    QApplication::processEvents(QEventLoop::AllEvents, 20);
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  }
  XII_TEST_BOOL(curator->IsAssetBeingTransformed(assets[0]));
  auto* edited = xiiDynamicCast<xiiAssetDocument*>(editor.OpenDocument(firstPath, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(edited != nullptr))
    return;
  auto* editAccessor = edited->GetObjectAccessor();
  editAccessor->StartTransaction("Edit while transforming");
  XII_TEST_STATUS(editAccessor->SetValueByName(edited->GetObjectManager()->GetRootObject()->GetChildren()[0], "Restitution", 0.75f));
  editAccessor->FinishTransaction();
  // Saved edits are observed by the reserved job without a competing auto-transform or error.
  XII_TEST_STATUS(edited->SaveDocument());
  edited->GetDocumentManager()->CloseDocument(edited);
  wait();
  progress = processor->GetTransformProgress();
  XII_TEST_BOOL(progress.m_uiTotal >= 2);
  XII_TEST_INT(progress.m_uiFailed, 0);
  const xiiDocumentTypeDescriptor* descriptor = nullptr;
  XII_TEST_RESULT(xiiDocumentManager::FindDocumentTypeFromPath(firstPath, false, descriptor));
  const auto*   assetDescriptor = static_cast<const xiiAssetDocumentTypeDescriptor*>(descriptor);
  const auto*   manager         = static_cast<const xiiAssetDocumentManager*>(descriptor->m_pManager);
  const auto    output          = manager->GetAbsoluteOutputFileName(assetDescriptor, firstPath, "");
  xiiFileReader reader;
  XII_TEST_RESULT(reader.Open(output));
  xiiAssetFileHeader header;
  XII_TEST_RESULT(header.Read(reader));
  xiiSurfaceResourceDescriptor surface;
  surface.Load(reader);
  XII_TEST_FLOAT(surface.m_fPhysicsRestitution, 0.75f, 0.0001f);
  reader.Close();

  // A stronger request arriving after dispatch needs one additional pass. Identical
  // pending requests still coalesce, and completion waits for both passes of the asset.
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(assets.GetData(), 1), xiiTransformFlags::ForceTransform));
  QElapsedTimer upgradeTimeout;
  upgradeTimeout.start();
  while (!curator->IsAssetBeingTransformed(assets[0]) && upgradeTimeout.elapsed() < 5000)
  {
    QApplication::processEvents(QEventLoop::AllEvents, 20);
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  }
  XII_TEST_BOOL(curator->IsAssetBeingTransformed(assets[0]));
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(assets.GetData(), 1), xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually));
  XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(assets.GetData(), 1), xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually));
  XII_TEST_INT(processor->GetTransformProgress().m_uiTotal, 2);
  wait();
  XII_TEST_INT(processor->GetTransformProgress().m_uiCompleted, 2);
  XII_TEST_INT(processor->GetTransformProgress().m_uiFailed, 0);

  // A missing dependency produces a stored diagnostic and one batch outcome, with no dialog.
  class MissingDependencyLog : public xiiTestLogInterface
  {
  public:
    void HandleLogMessage(const xiiLoggingEventData& e) override
    {
      if (e.m_EventType == xiiLogMsgType::ErrorMsg &&
          (e.m_sText.FindSubString("11111111-2222-3333-4444-555555555555") || e.m_sText.FindSubString("Failure.xiiSurfaceAsset")))
        ++m_uiDiagnostics;
      else
        xiiTestLogInterface::HandleLogMessage(e);
    }
    std::atomic<xiiUInt32> m_uiDiagnostics = 0;
  } failureLog;
  xiiTestLogSystemScope failureScope(&failureLog, true);
  failureLog.ExpectMessage("Asset transformation:", xiiLogMsgType::ErrorMsg);
  const xiiStringBuilder badPath(preferences, "/Project/Batch/Failure.xiiSurfaceAsset");
  auto*                  bad = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(badPath, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(bad != nullptr))
    return;
  auto* accessor = bad->GetObjectAccessor();
  accessor->StartTransaction("Set missing dependency");
  XII_TEST_STATUS(accessor->SetValueByName(bad->GetObjectManager()->GetRootObject()->GetChildren()[0], "BaseSurface", "{ 11111111-2222-3333-4444-555555555555 }"));
  accessor->FinishTransaction();
  XII_TEST_STATUS(bad->SaveDocument());
  const xiiUuid badGuid = bad->GetGuid();
  bad->GetDocumentManager()->CloseDocument(bad);
  {
    XII_TEST_STATUS(processor->QueueTransformAssets(xiiMakeArrayPtr(&badGuid, 1)));
    wait();
    XII_TEST_INT(processor->GetTransformProgress().m_uiFailed, 1);
    XII_TEST_BOOL(failureLog.m_uiDiagnostics > 0);
  }
  XII_TEST_STATUS(processor->QueueTransformAssets(assets, xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually));
  processor->StopProcessTask(false); // Nonblocking cancellation; drain active requests.
  wait();
  progress = processor->GetTransformProgress();
  XII_TEST_BOOL(progress.m_uiCanceled > 0);
  XII_TEST_INT(progress.m_uiCompleted + progress.m_uiCanceled, progress.m_uiTotal);
  XII_TEST_INT(progress.m_uiFailed, 0);
  for (const auto& guid : assets)
    XII_TEST_BOOL(!curator->IsAssetBeingTransformed(guid));
  XII_TEST_INT(observer.m_uiDialogs, 0);
  // A forced shutdown must also release all reservations before the project is destroyed.
  XII_TEST_STATUS(processor->QueueTransformAssets(assets, xiiTransformFlags::ForceTransform));
  QElapsedTimer activeTimeout;
  activeTimeout.start();
  while (!curator->IsAssetBeingTransformed(assets[0]) && activeTimeout.elapsed() < 5000)
  {
    QApplication::processEvents(QEventLoop::AllEvents, 20);
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  }
  XII_TEST_BOOL(curator->IsAssetBeingTransformed(assets[0]));
  processor->StopProcessTask(true);
  XII_TEST_BOOL(!processor->GetTransformProgress().m_bActive);
  for (const auto& guid : assets)
    XII_TEST_BOOL(!curator->IsAssetBeingTransformed(guid));
}
