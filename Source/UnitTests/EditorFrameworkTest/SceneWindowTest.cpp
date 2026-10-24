/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/SceneDuplicationTest.h>
#include <EditorFrameworkTest/SceneWindowTest.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
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
        // A cold first frame can spend several seconds compiling native pipeline state. A short
        // IPC observation timeout does not mean that this outstanding request has failed. Poll it
        // against the existing overall readiness deadline before submitting another frame/request.
        xiiResult reply      = XII_FAILURE;
        auto*     connection = xiiEditorEngineProcessConnection::GetSingleton();
        while (reply.Failed() && elapsed.elapsed() < 10000)
        {
          const auto timeout = xiiTime::MakeFromMilliseconds(xiiMath::Min<xiiInt64>(2000, 10000 - elapsed.elapsed()));
          reply              = connection->WaitForDocumentMessage(sceneID, xiiGetStaticRTTI<xiiViewPickingResultMsgToEditor>(), timeout, &callback);
          if (reply.Failed())
          {
            connection->Update();
            if (connection->IsProcessCrashed())
              break;
          }
        }
        if (!XII_TEST_RESULT(reply))
          return;
        QThread::msleep(1);
      }
      if (!XII_TEST_BOOL(pickedObject == sphereID))
        return;

      // A click must consume its own response, even if an earlier hover hit is
      // still queued. Ordinary hover remains asynchronous.
      const auto pixelRatio = view->devicePixelRatio();
      XII_TEST_BOOL(view->PickObject(xiiUInt16(128 / pixelRatio), xiiUInt16(128 / pixelRatio), true).m_PickedObject == sphereID);
      XII_TEST_BOOL(view->PickObject(0, 0, true).m_PickedObject != sphereID);
      XII_TEST_BOOL(view->PickObject(xiiUInt16(128 / pixelRatio), xiiUInt16(128 / pixelRatio), true).m_PickedObject == sphereID);

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

      // Exercise the editor screenshot API against distinguishable visual results, rather
      // than accepting any non-uniform startup image as evidence of functioning rendering.
      auto captureMode = [&](xiiViewRenderMode::Enum mode, const char* filename) {
        view->m_pViewConfig->m_RenderMode = mode;
        // Queue document settings (including grid visibility) before the view and
        // screenshot messages. A timer delay does not establish this IPC ordering.
        window->TriggerRedraw();
        view->SyncToEngine();
        xiiStringBuilder output = sProjectDirectory;
        output.AppendPath(filename);
        if (xiiOSFile::ExistsFile(output))
          XII_TEST_RESULT(xiiOSFile::DeleteFile(output));
        view->TakeScreenshot(output);
        QImage result;
        XII_TEST_BOOL(PumpUntil([&] { return result.load(QString::fromUtf8(output.GetData())); }, 10000));
        XII_TEST_INT(result.width(), 256);
        XII_TEST_INT(result.height(), 256);
        return result;
      };
      const auto depthImage     = captureMode(xiiViewRenderMode::DepthNonLinear, "ViewportDepth.png");
      const auto normalImage    = captureMode(xiiViewRenderMode::NormalMap, "ViewportNormals.png");
      const auto wireframeImage = captureMode(xiiViewRenderMode::Wireframe, "ViewportWireframe.png");
      XII_TEST_BOOL(depthImage != normalImage);
      if (!depthImage.isNull() && !normalImage.isNull())
      {
        XII_TEST_BOOL(qRed(depthImage.pixel(128, 128)) > qRed(depthImage.pixel(0, 0)));
        const auto normal = normalImage.pixel(128, 128);
        XII_TEST_BOOL(qRed(normal) != qGreen(normal) || qGreen(normal) != qBlue(normal));
      }
      if (!wireframeImage.isNull() && !depthImage.isNull())
      {
        XII_TEST_INT(wireframeImage.pixel(0, 0), qRgb(0, 0, 0));
        xiiUInt32 wirePixels = 0, solidPixels = 0;
        for (int y = 0; y < wireframeImage.height(); ++y)
          for (int x = 0; x < wireframeImage.width(); ++x)
          {
            wirePixels += qRed(wireframeImage.pixel(x, y)) > 160;
            solidPixels += qRed(depthImage.pixel(x, y)) > 0;
          }
        XII_TEST_BOOL(wirePixels > 100 && wirePixels < solidPixels);
      }
      const xiiViewRenderMode::Enum additionalModes[] = {xiiViewRenderMode::DepthLinear, xiiViewRenderMode::BaseColor, xiiViewRenderMode::Metallic,
                                                         xiiViewRenderMode::Roughness, xiiViewRenderMode::Specular, xiiViewRenderMode::Emissive, xiiViewRenderMode::AmbientOcclusion,
                                                         xiiViewRenderMode::MotionVectors, xiiViewRenderMode::InstanceID, xiiViewRenderMode::IndirectLighting, xiiViewRenderMode::ReflectionContribution, xiiViewRenderMode::Bloom};
      for (const auto mode : additionalModes)
      {
        xiiStringBuilder name;
        name.SetFormat("ViewportMode{}.png", static_cast<xiiUInt32>(mode));
        captureMode(mode, name.GetData());
      }
      view->InterpolateCameraTo(xiiVec3(0, -4, 1.5f), xiiVec3(4, 4, -1.5f).GetNormalized(), 60, nullptr, true);
      auto*      scenePreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(scene);
      const bool showGrid         = scenePreferences->GetShowGrid();
      scenePreferences->SetShowGrid(false);
      const auto skyImage    = captureMode(xiiViewRenderMode::Default, "ViewportAtmosphere.png");
      const auto noGridImage = captureMode(xiiViewRenderMode::DepthNonLinear, "ViewportNoGrid.png");
      scenePreferences->SetShowGrid(true);
      const auto gridImage = captureMode(xiiViewRenderMode::DepthNonLinear, "ViewportGrid.png");
      scenePreferences->SetShowGrid(showGrid);
      xiiUInt32 gridPixels = 0;
      if (!gridImage.isNull() && !noGridImage.isNull())
        for (int y = gridImage.height() / 2; y < gridImage.height(); ++y)
          for (int x = 0; x < gridImage.width(); ++x)
            if (qRed(gridImage.pixel(x, y)) > qRed(noGridImage.pixel(x, y)) + 20)
              ++gridPixels;
      XII_TEST_BOOL_MSG(gridPixels > 100, "Enabling the floor grid must draw visible lines without selecting a gizmo.");
      if (!skyImage.isNull())
      {
        xiiUInt32 blueSkyPixels = 0;
        for (int y = 0; y < skyImage.height() / 3; ++y)
          for (int x = 0; x < skyImage.width(); ++x)
          {
            const auto pixel = skyImage.pixel(x, y);
            if (qBlue(pixel) > qRed(pixel) + 8 && qBlue(pixel) > qGreen(pixel))
              ++blueSkyPixels;
          }
        XII_TEST_BOOL_MSG(blueSkyPixels > 100, "The default daylight scene must render a blue atmosphere, not a grey background.");
      }
      view->InterpolateCameraTo(xiiVec3(0, 0, 0), xiiVec3(1, 0, 0), 60, nullptr, true);
      view->m_pViewConfig->m_RenderMode = xiiViewRenderMode::Default;

      // Exercise production-size targets and continuous resize through the real Editor IPC and
      // built-in pipeline. The small fixed startup target alone cannot expose retained VRAM growth.
      QWidget* surface = nullptr;
      for (QWidget* child : view->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
        if (child->testAttribute(Qt::WA_PaintOnScreen) && child->testAttribute(Qt::WA_NativeWindow))
          surface = child;
      if (!XII_TEST_BOOL(surface != nullptr))
        return;
      for (xiiUInt32 iteration = 0; iteration < 160; ++iteration)
      {
        const xiiSizeU32 resolution              = iteration < 96 ? xiiSizeU32(768 + iteration * 8, 512 + (iteration % 24) * 8) : xiiSizeU32(1280, 720);
        xiiQtEngineViewWidget::s_FixedResolution = resolution;
        const qreal pixelRatio                   = view->devicePixelRatio();
        surface->setFixedSize(static_cast<int>(xiiMath::Ceil(resolution.width / pixelRatio)), static_cast<int>(xiiMath::Ceil(resolution.height / pixelRatio)));
        view->SetEnablePicking(true);
        view->SyncToEngine();
        xiiQtUiServices::TickEvent tick;
        tick.m_Type    = xiiQtUiServices::TickEvent::Type::EndFrame;
        tick.m_uiFrame = frame++;
        tick.m_Time    = xiiTime::Now();
        xiiQtUiServices::s_TickEvent.Broadcast(tick);
        pick.m_uiPickPosX = resolution.width / 2;
        pick.m_uiPickPosY = resolution.height / 2;
        if (!XII_TEST_BOOL(scene->SendMessageToEngine(&pick)) || !XII_TEST_RESULT(xiiEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(sceneID, xiiGetStaticRTTI<xiiViewPickingResultMsgToEditor>(), xiiTime::MakeFromSeconds(2), &callback)))
          return;
        if (!XII_TEST_BOOL(!xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed()))
          return;
      }
      capture = sProjectDirectory;
      capture.AppendPath("NativeViewportResized.png");
      if (xiiOSFile::ExistsFile(capture))
        XII_TEST_RESULT(xiiOSFile::DeleteFile(capture));
      view->TakeScreenshot(capture);
      image = QImage();
      XII_TEST_BOOL(PumpUntil([&] { return image.load(QString::fromUtf8(capture.GetData())); }, 10000));
      XII_TEST_INT(image.width(), 1280);
      XII_TEST_INT(image.height(), 720);

      TestSceneDuplicationAction(*scene, sphereID);
      view->SyncToEngine();
      PumpUntil([] { return false; }, 200);
      XII_TEST_BOOL(!xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

      // Closing the document can precede processing the asynchronous request to stop Play.
      // The engine context must release the game state's world references before world destruction.
      scene->TriggerGameModePlay(false);
      if (!XII_TEST_BOOL(PumpUntil([&] { return scene->GetGameMode() == GameMode::Play; }, 10000)))
        return;
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
  PumpUntil([] { return false; }, 500);
  XII_TEST_BOOL(!xiiEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());
  if (reopenedSphere != nullptr)
  {
    // Require a fresh reply after the game state has ticked. A crash handler may still be
    // resolving symbols, so the child merely being alive is insufficient evidence.
    scene->GetSelectionManager()->SetSelection(reopenedSphere);
    scene->SendObjectSelection();
    xiiQuerySelectionBBoxMsgToEngine query;
    query.m_uiViewID = xiiInvalidIndex;
    query.m_iPurpose = 2;
    XII_TEST_BOOL(scene->SendMessageToEngine(&query));
    XII_TEST_RESULT(xiiEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(sceneID, xiiGetStaticRTTI<xiiQuerySelectionBBoxResultMsgToEditor>(), xiiTime::MakeFromSeconds(5)));
  }
}
