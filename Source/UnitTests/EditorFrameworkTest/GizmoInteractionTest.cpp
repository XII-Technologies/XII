/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QPointer>
#include <QThread>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  class PlacementDocumentWindow : public xiiQtGameObjectDocumentWindow, public xiiGameObjectGizmoInterface
  {
  public:
    explicit PlacementDocumentWindow(xiiSceneDocument* document) : xiiQtGameObjectDocumentWindow(document) { FinishWindowCreation(); }
    xiiStringView          GetWindowLayoutGroupName() const override { return "PlacementDocumentTest"; }
    xiiObjectAccessorBase* GetObjectAccessor() override { return GetGameObjectDocument()->GetObjectAccessor(); }
    bool                   CanDuplicateSelection() const override { return false; }
    void                   DuplicateSelection() override { XII_TEST_BOOL_MSG(false, "This test does not request duplication."); }
  };

  class PlacementTestView : public xiiQtEngineViewWidget
  {
  public:
    using xiiQtEngineViewWidget::xiiQtEngineViewWidget;
    ~PlacementTestView()
    {
      if (s_InteractionContext.m_pLastHoveredViewWidget == this)
        s_InteractionContext = {};
    }
    void SetSurfaceSize() { m_pViewportWidget->setFixedSize(256, 256); }
    void Move(QMouseEvent* event) { mouseMoveEvent(event); }
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorViewportInteraction);
XII_CREATE_SIMPLE_TEST(EditorViewportInteraction, HandleFeedback)
{
  xiiTranslateGizmo    tool;
  xiiEngineGizmoHandle handle;
  handle.ConfigureHandle(&tool, xiiEngineGizmoHandleType::TranslationAxis, xiiColor::Red, xiiGizmoFlags::Pickable);
  const auto* type = handle.GetDynamicRTTI();
  const auto  read = [&](const char* name) {
    return xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(type->FindPropertyByName(name)), &handle);
  };
  auto transform        = xiiTransform::MakeIdentity();
  transform.m_vPosition = xiiVec3(2, 3, 4);
  tool.SetTransformation(transform);
  tool.ConfigureInteraction(&handle, nullptr, transform.m_vPosition, xiiVec2I32(800, 600));
  for (auto end : {xiiGizmoEvent::Type::EndInteractions, xiiGizmoEvent::Type::CancelInteractions})
  {
    tool.m_GizmoEvents.Broadcast({&tool, xiiGizmoEvent::Type::BeginInteractions});
    XII_TEST_INT(read("VisualState").Get<xiiInt32>(), static_cast<xiiInt32>(xiiGizmoVisualState::Active));
    XII_TEST_VEC3(read("DragOrigin").Get<xiiVec3>(), transform.m_vPosition, 0);
    handle.SetVisualParameters(1, 2);
    handle.SetInteractionText("12.50 units");
    XII_TEST_STRING(read("InteractionText").Get<xiiString>(), "12.50 units");
    tool.m_GizmoEvents.Broadcast({&tool, end});
    XII_TEST_INT(read("VisualState").Get<xiiInt32>(), static_cast<xiiInt32>(xiiGizmoVisualState::Normal));
    XII_TEST_VEC2(read("RotationArc").Get<xiiVec2>(), xiiVec2::MakeZero(), 0);
    XII_TEST_BOOL(read("InteractionText").Get<xiiString>().IsEmpty());
  }
}

XII_CREATE_SIMPLE_TEST(EditorViewportInteraction, PlacementDocument)
{
  // The command/undo checks run without a render surface. On Windows a hidden
  // Vulkan viewport also exercises real surface queries and mouse-move routing.
  // Neither case injects desktop input, presents a window or moves the cursor.
  auto* editor = xiiQtEditorApp::GetSingleton();
  xiiStartup::StartupCoreSystems();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  xiiStringBuilder directory = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  directory.AppendPath("PlacementDocument");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(directory));
  editor->StartupEditor(xiiQtEditorApp::StartupFlags::Headless | xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, directory);
  XII_SCOPE_EXIT(editor->ShutdownEditor());
  xiiQtContainerWindow container;
  container.SetWindowPresentationEnabled(false);
  xiiStringBuilder project = directory;
  project.AppendPath("Project", "xiiProject");
  if (!XII_TEST_RESULT(editor->CreateOrOpenProject(true, project)))
    return;
  xiiStringBuilder path = project.GetFileDirectory();
  path.AppendPath("Placement.xiiScene");
  auto* scene = xiiDynamicCast<xiiSceneDocument*>(editor->CreateDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(scene != nullptr))
    return;
  QPointer<PlacementDocumentWindow> window = new PlacementDocumentWindow(scene);
  XII_SCOPE_EXIT({
    scene->GetDocumentManager()->CloseDocument(scene);
    QElapsedTimer elapsed;
    elapsed.start();
    while (!window.isNull() && elapsed.elapsed() < 2000)
    {
      QApplication::processEvents(QEventLoop::AllEvents, 20);
      QThread::msleep(1);
    }
    XII_TEST_BOOL(window.isNull());
  });
  scene->SetEditToolConfigDelegate([&](xiiGameObjectEditTool* tool) { tool->ConfigureTool(scene, window, window); });
  auto* accessor = scene->GetObjectAccessor();
  accessor->StartTransaction("Placement source");
  xiiUuid source;
  XII_TEST_STATUS(accessor->AddObjectByName(scene->GetObjectManager()->GetRootObject(), "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), source));
  const auto* object = accessor->GetObject(source);
  XII_TEST_STATUS(accessor->SetValueByName(object, "LocalPosition", xiiVec3(2, 3, 4)));
  accessor->FinishTransaction();
  scene->GetSelectionManager()->SetSelection(object);
  scene->SetActiveEditTool(xiiGetStaticRTTI<xiiDragToPositionGizmoEditTool>());
  auto* handle = xiiDynamicCast<xiiEngineGizmoHandle*>(scene->FindSyncObject(xiiGetStaticRTTI<xiiEngineGizmoHandle>()));
  auto* gizmo  = handle ? xiiDynamicCast<xiiDragToPositionGizmo*>(handle->GetOwnerGizmo()) : nullptr;
  if (!XII_TEST_BOOL(gizmo != nullptr))
    return;
  auto*      history   = scene->GetCommandHistory();
  const auto undoCount = history->GetUndoStackSize();
  const auto original  = scene->GetGlobalTransform(object);
  gizmo->ConfigureInteraction(handle, nullptr, original.m_vPosition, xiiVec2I32(800, 600));
  QMouseEvent press(QEvent::MouseButtonPress, QPointF(128, 128), QPointF(500, 400), Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
  XII_TEST_BOOL(gizmo->MousePressEvent(&press) == xiiEditorInput::WasExclusivelyHandled);
  XII_SCOPE_EXIT(if (gizmo->IsActiveInputContext()) { gizmo->FocusLost(true); xiiEditorInputContext::SetActiveInputContext(nullptr); });
  for (const xiiVec3 target : {xiiVec3(5, 7, 9), xiiVec3(-2, 1, 6)})
  {
    auto placement        = gizmo->GetTransformation();
    placement.m_vPosition = target;
    gizmo->SetTransformation(placement);
    gizmo->m_GizmoEvents.Broadcast({gizmo, xiiGizmoEvent::Type::Interaction});
    XII_TEST_VEC3(scene->GetGlobalTransform(object).m_vPosition, target, 0.001f);
    XII_TEST_VEC3(accessor->GetByName<xiiVec3>(object, "LocalPosition"), target, 0.001f);
  }
  gizmo->FocusLost(false);
  xiiEditorInputContext::SetActiveInputContext(nullptr);
  XII_TEST_INT(history->GetUndoStackSize(), undoCount + 1);
  XII_TEST_STATUS(history->Undo());
  XII_TEST_VEC3(scene->GetGlobalTransform(object).m_vPosition, original.m_vPosition, 0.001f);
  XII_TEST_STATUS(history->Redo());
  XII_TEST_VEC3(scene->GetGlobalTransform(object).m_vPosition, xiiVec3(-2, 1, 6), 0.001f);

  if (QGuiApplication::platformName() == "windows")
  {
    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Hidden viewport moves selected mesh onto another mesh")
    {
      XII_TEST_STATUS(history->Undo());
      history->ClearRedoHistory();
      accessor->StartTransaction("Placement geometry");
      const xiiDocumentObject* target = nullptr;
      for (const auto* child : scene->GetObjectManager()->GetRootObject()->GetChildren())
        if (child->GetType()->IsDerivedFrom<xiiGameObject>() && accessor->GetByName<xiiString>(child, "Name") == "Sphere")
          target = child;
      if (!XII_TEST_BOOL(target != nullptr))
        return;
      XII_TEST_STATUS(accessor->SetValueByName(target, "LocalPosition", xiiVec3(6, 3, 4)));
      xiiUuid mesh;
      XII_TEST_STATUS(accessor->AddObjectByName(object, "Components", -1, xiiGetStaticRTTI<xiiMeshComponent>(), mesh));
      XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(mesh), "Mesh", "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"));
      accessor->FinishTransaction();
      const auto asset = xiiConversionUtils::ConvertStringToUuid("{ 618ee743-ed04-4fac-bf5f-572939db2f1d }");
      XII_TEST_BOOL(xiiAssetCurator::GetSingleton()->TransformAsset(asset, xiiTransformFlags::TriggeredManually).Succeeded());
      XII_TEST_RESULT(xiiAssetCurator::GetSingleton()->WriteAssetTables(nullptr, true));
      auto* connection = xiiEditorEngineProcessConnection::GetSingleton();
      XII_TEST_RESULT(connection->RestartProcess());
      XII_TEST_STATUS(scene->WaitForEngineStatusLoaded(xiiTime::MakeFromSeconds(10)));
      xiiEngineViewConfig config;
      config.m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 70, 0.1f, 100);
      config.m_Camera.LookAt(xiiVec3(0, 3, 4), xiiVec3(6, 3, 4), xiiVec3::MakeAxisZ());
      auto* view = new PlacementTestView(window, window, &config);
      window->AddViewWidget(view);
      view->SetSurfaceSize();
      XII_SCOPE_EXIT(delete view);
      scene->SyncObjectsToEngine();
      scene->SendObjectSelection();
      const int   center = int(128 / view->devicePixelRatio());
      QMouseEvent hover(QEvent::MouseMove, QPointF(center, center), QPointF(500, 400), Qt::NoButton, Qt::NoButton, Qt::ShiftModifier);
      view->Move(&hover);
      gizmo->ConfigureInteraction(handle, &config.m_Camera, original.m_vPosition, xiiVec2I32(256, 256));
      gizmo->MousePressEvent(&press);
      xiiUInt32     frame = 0;
      QElapsedTimer elapsed;
      elapsed.start();
      bool moved = false;
      while (!moved && elapsed.elapsed() < 10000)
      {
        // Captured drags still need surface updates when hover-based picking is off.
        view->SetEnablePicking(false);
        view->SyncToEngine();
        xiiQtUiServices::TickEvent tick;
        tick.m_Type    = xiiQtUiServices::TickEvent::Type::EndFrame;
        tick.m_uiFrame = frame++;
        tick.m_Time    = xiiTime::Now();
        xiiQtUiServices::s_TickEvent.Broadcast(tick);
        const auto  hit = view->PickObject(center, center, true);
        QMouseEvent move(QEvent::MouseMove, QPointF(center, center), QPointF(500, 400), Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
        view->Move(&move);
        moved = !scene->GetGlobalTransform(object).m_vPosition.IsEqual(original.m_vPosition, 0.001f);
        if (moved)
        {
          XII_TEST_BOOL(hit.m_PickedObject == target->GetGuid());
          XII_TEST_VEC3(scene->GetGlobalTransform(object).m_vPosition, hit.m_vPickedPosition, 0.001f);
        }
      }
      XII_TEST_BOOL_MSG(moved, "Dragging the selected mesh must change its document position, not just its handle.");
      gizmo->FocusLost(false);
      xiiEditorInputContext::SetActiveInputContext(nullptr);
    }
  }
}
