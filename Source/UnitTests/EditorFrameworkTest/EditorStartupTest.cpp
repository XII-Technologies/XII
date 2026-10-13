/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFrameworkTest/AssetDependenciesTest.h>
#include <EditorFrameworkTest/AssetDependencyTrackingTest.h>
#include <EditorFrameworkTest/AssetProcessorTest.h>
#include <EditorFrameworkTest/AssetProfileTest.h>
#include <EditorFrameworkTest/AssetResaveTest.h>
#include <EditorFrameworkTest/DocumentSaveUiTest.h>
#include <EditorFrameworkTest/PropertyAnimationTestUtils.h>
#include <EditorFrameworkTest/SceneWindowTest.h>

#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QPointer>
#include <QTreeView>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/DocumentManager.h>
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
    XII_TEST_BOOL(xiiToolsProject::IsProjectOpen());
    TestDocumentSaveUi(*editor, project.GetFileDirectory());
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
