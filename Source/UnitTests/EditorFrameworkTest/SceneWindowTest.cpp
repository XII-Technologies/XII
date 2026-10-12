/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/SceneWindowTest.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QPointer>
#include <QThread>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  template <typename Predicate>
  bool PumpUntil(Predicate predicate, int iMilliseconds)
  {
    QElapsedTimer elapsed;
    elapsed.start();
    while (!predicate() && elapsed.elapsed() < iMilliseconds)
    {
      QApplication::processEvents(QEventLoop::AllEvents, 20);
      QThread::msleep(1);
    }
    return predicate();
  }
} // namespace

void TestSceneWindow(xiiQtEditorApp& editor, xiiStringView sProjectDirectory)
{
  const bool bNativeView                   = QGuiApplication::platformName() == "windows" || QGuiApplication::platformName() == "xcb";
  const auto previousResolution            = xiiQtEngineViewWidget::s_FixedResolution;
  xiiQtEngineViewWidget::s_FixedResolution = xiiSizeU32(256, 256);
  XII_SCOPE_EXIT(xiiQtEngineViewWidget::s_FixedResolution = previousResolution);
  xiiStringBuilder path = sProjectDirectory;
  path.AppendPath("Native.xiiScene");
  auto* scene = xiiDynamicCast<xiiSceneDocument*>(editor.CreateDocument(path, bNativeView ? xiiDocumentFlags::RequestWindow : xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(scene != nullptr))
    return;
  XII_SCOPE_EXIT(if (scene != nullptr) scene->GetDocumentManager()->CloseDocument(scene));
  XII_TEST_INT(scene->GetUnknownObjectTypeInstances(), 0);
  XII_TEST_STATUS(scene->WaitForEngineStatusLoaded(xiiTime::MakeFromSeconds(10)));
  XII_TEST_BOOL(scene->GetEditorEngineConnection() != nullptr);

  auto*                    accessor = scene->GetObjectAccessor();
  const xiiDocumentObject* sphere   = nullptr;
  for (auto* child : scene->GetObjectManager()->GetRootObject()->GetChildren())
  {
    xiiVariant name;
    if (accessor->GetValueByName(child, "Name", name).Succeeded() && name == xiiVariant("Sphere"))
      sphere = child;
  }
  if (!XII_TEST_BOOL(sphere != nullptr))
    return;
  const xiiUuid sphereID = sphere->GetGuid();
  const xiiUuid sceneID  = scene->GetGuid();

  accessor->StartTransaction("Move native scene object");
  XII_TEST_STATUS(accessor->SetValueByName(sphere, "LocalPosition", xiiVec3(4, 0, 0)));
  accessor->FinishTransaction();
  XII_TEST_STATUS(scene->GetCommandHistory()->Undo());
  xiiVariant position;
  XII_TEST_STATUS(accessor->GetValueByName(sphere, "LocalPosition", position));
  XII_TEST_VEC3(position.Get<xiiVec3>(), xiiVec3(3, 0, 0), 0.001f);
  XII_TEST_STATUS(scene->GetCommandHistory()->Redo());
  XII_TEST_STATUS(scene->SaveDocument());

  if (bNativeView)
  {
    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native scene viewport, resource transformation, capture and picking")
    {
      // Linked test plugins start before StartupEditor mounts tool data. Refresh their data as a loaded editor plugin would.
      xiiVisualShaderTypeRegistry::GetSingleton()->UpdateNodeData();
      const xiiUuid sphereAsset = xiiConversionUtils::ConvertStringToUuid("{ 618ee743-ed04-4fac-bf5f-572939db2f1d }");
      const auto    transformed = xiiAssetCurator::GetSingleton()->TransformAsset(sphereAsset, xiiTransformFlags::TriggeredManually);
      if (!XII_TEST_BOOL_MSG(transformed.Succeeded(), "%s", transformed.m_sMessage.GetData()))
        return;
      XII_TEST_RESULT(xiiAssetCurator::GetSingleton()->WriteAssetTables(nullptr, true));

      auto* window = qobject_cast<xiiQtEngineDocumentWindow*>(xiiQtDocumentWindow::FindWindowByDocument(scene));
      if (!XII_TEST_BOOL(window != nullptr && !window->GetViewWidgets().IsEmpty()))
        return;
      auto* view = window->GetViewWidgets()[0];
      view->m_pViewConfig->m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 1000.0f);
      view->InterpolateCameraTo(xiiVec3(0, 0, 0), xiiVec3(1, 0, 0), 60, nullptr, true);
      view->SyncToEngine();
      PumpUntil([] { return false; }, 1000);
      XII_TEST_BOOL(!xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

      xiiViewPickingMsgToEngine pick;
      pick.m_uiViewID   = view->GetViewID();
      pick.m_uiPickPosX = 128;
      pick.m_uiPickPosY = 128;
      xiiUuid pickedObject;
      auto    received = [&](xiiProcessMessage* message) {
        const auto* result = static_cast<const xiiViewPickingResultMsgToEditor*>(message);
        if (result->m_uiViewID != view->GetViewID())
          return false;
        pickedObject = result->m_ObjectGuid;
        return true;
      };
      xiiProcessCommunicationChannel::WaitForMessageCallback callback = received;
      // Picking is asynchronous and startup resource updates invalidate captured images.
      // Drive complete lockstep frames and poll the IPC result until the expected owner is ready.
      // Do not pump Qt's normal redraw here: it disables picking for non-hovered automated windows.
      QElapsedTimer elapsed;
      elapsed.start();
      xiiUInt32 frame = 0U;
      while (pickedObject != sphereID && elapsed.elapsed() < 10000)
      {
        view->SetEnablePicking(true);
        view->SyncToEngine();
        xiiQtUiServices::TickEvent tick;
        tick.m_Type    = xiiQtUiServices::TickEvent::Type::EndFrame;
        tick.m_uiFrame = frame++;
        tick.m_Time    = xiiTime::Now();
        xiiQtUiServices::s_TickEvent.Broadcast(tick);
        if (!XII_TEST_BOOL(scene->SendMessageToEngine(&pick)))
          return;
        if (!XII_TEST_RESULT(xiiEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(sceneID,
                                                                                                      xiiGetStaticRTTI<xiiViewPickingResultMsgToEditor>(), xiiTime::MakeFromSeconds(2), &callback)))
          return;
        QThread::msleep(1);
      }
      if (!XII_TEST_BOOL(pickedObject == sphereID))
        return;

      // A completed native pick confirms that the transformed scene has reached the GPU.
      // Capture only after that readiness boundary; a startup delay cannot establish it.
      xiiStringBuilder capture = sProjectDirectory;
      capture.AppendPath("NativeViewport.png");
      if (xiiOSFile::ExistsFile(capture))
        XII_TEST_RESULT(xiiOSFile::DeleteFile(capture));
      view->TakeScreenshot(capture);
      QImage image;
      XII_TEST_BOOL(PumpUntil([&] { return image.load(QString::fromUtf8(capture.GetData())); }, 10000));
      XII_TEST_INT(image.width(), 256);
      XII_TEST_INT(image.height(), 256);
      if (!image.isNull())
      {
        bool bVaried = false;
        for (int y = 0; y < image.height() && !bVaried; ++y)
          for (int x = 0; x < image.width() && !bVaried; ++x)
            bVaried = image.pixel(x, y) != image.pixel(0, 0);
        XII_TEST_BOOL(bVaried);
      }
    }
  }
  else
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Native scene viewport requires Qt windows/xcb; document and IPC checks still run.");
  }

  QPointer<xiiQtDocumentWindow> closedWindow = xiiQtDocumentWindow::FindWindowByDocument(scene);
  scene->GetDocumentManager()->CloseDocument(scene);
  scene = nullptr;
  PumpUntil([&] { return closedWindow.isNull(); }, 1000);
  XII_TEST_BOOL(closedWindow.isNull());
  scene = xiiDynamicCast<xiiSceneDocument*>(editor.OpenDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(scene != nullptr))
    return;
  XII_TEST_BOOL(scene->GetGuid() == sceneID);
  XII_TEST_INT(scene->GetUnknownObjectTypeInstances(), 0);
  const auto* reopenedSphere = scene->GetObjectManager()->GetObject(sphereID);
  if (XII_TEST_BOOL(reopenedSphere != nullptr))
  {
    XII_TEST_STATUS(scene->GetObjectAccessor()->GetValueByName(reopenedSphere, "LocalPosition", position));
    XII_TEST_VEC3(position.Get<xiiVec3>(), xiiVec3(4, 0, 0), 0.001f);
  }
  XII_TEST_STATUS(scene->WaitForEngineStatusLoaded(xiiTime::MakeFromSeconds(10)));
}
