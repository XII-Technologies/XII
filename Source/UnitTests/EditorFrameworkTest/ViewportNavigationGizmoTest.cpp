/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QThread>
#include <QWheelEvent>
#include <Texture/Image/Image.h>

namespace
{
  class NavigationDocumentWindow : public xiiQtGameObjectDocumentWindow
  {
  public:
    explicit NavigationDocumentWindow(xiiSceneDocument* document) : xiiQtGameObjectDocumentWindow(document) { FinishWindowCreation(); }
    xiiStringView GetWindowLayoutGroupName() const override { return "NavigationGizmoTest"; }
  };

  class NavigationView : public xiiQtGameObjectViewWidget
  {
  public:
    using xiiQtGameObjectViewWidget::xiiQtGameObjectViewWidget;
    void SetSurfaceSize(int width, int height)
    {
      resize(width, height);
      m_pViewportWidget->setFixedSize(width, height);
    }
    void AdvanceInterpolation(float seconds)
    {
      m_LastCameraUpdate -= xiiTime::MakeFromSeconds(seconds);
      UpdateCameraInterpolation();
    }
    void Mouse(QEvent::Type type, const xiiVec2& position, Qt::MouseButton button, Qt::MouseButtons buttons)
    {
      const QPointF local(position.x, position.y);
      QMouseEvent   event(type, local, mapToGlobal(local.toPoint()), button, buttons, Qt::NoModifier);
      QApplication::sendEvent(this, &event);
    }
  };

  class NavigationInputProbe : public xiiEditorInputContext
  {
  public:
    xiiUInt32      m_uiPresses = 0;
    xiiUInt32      m_uiWheels  = 0;
    void           OnSetOwner(xiiQtEngineDocumentWindow*, xiiQtEngineViewWidget*) override {}
    xiiEditorInput DoMousePressEvent(QMouseEvent*) override
    {
      ++m_uiPresses;
      return xiiEditorInput::WasExclusivelyHandled;
    }
    xiiEditorInput DoWheelEvent(QWheelEvent*) override
    {
      ++m_uiWheels;
      return xiiEditorInput::WasExclusivelyHandled;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorViewportNavigation);
XII_CREATE_SIMPLE_TEST(EditorViewportNavigation, NavigationGeometry)
{
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 70, 0.1f, 1000);
  camera.LookAt(xiiVec3(6, -7, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
  for (xiiUInt32 rotation = 0; rotation < 40; ++rotation)
  {
    const xiiViewportNavigationGizmo geometry(camera, xiiVec2(800, 600));
    XII_TEST_VEC2(geometry.m_vOrigin, xiiVec2(736, 64), 0);
    for (xiiUInt32 i = 0; i < 6; ++i)
    {
      const auto& marker = geometry.m_Markers[i];
      const auto  axis   = xiiViewportNavigationGizmo::GetAxisDirection(marker.m_uiAxis);
      const auto  screen = geometry.m_vOrigin + xiiVec2(axis.Dot(camera.GetCenterDirRight()), -axis.Dot(camera.GetCenterDirUp())) * 38.0f;
      XII_TEST_VEC2(marker.m_vPosition, screen, 0.001f);
      XII_TEST_BOOL(geometry.Contains(marker.m_vPosition));
      if (i > 0)
        XII_TEST_BOOL(geometry.m_Markers[i - 1].m_fDepth <= marker.m_fDepth);
    }
    XII_TEST_INT(geometry.HitTest(geometry.m_Markers[5].m_vPosition), geometry.m_Markers[5].m_uiAxis);
    XII_TEST_INT(geometry.HitTest(xiiVec2::MakeZero()), -1);
    XII_TEST_BOOL(!geometry.Contains(xiiVec2(800, 600)));
    const xiiViewportNavigationGizmo resized(camera, xiiVec2(1000, 300));
    XII_TEST_VEC2(resized.m_vOrigin - geometry.m_vOrigin, xiiVec2(200, 0), 0);
    for (xiiUInt32 i = 0; i < 6; ++i)
      XII_TEST_VEC2(resized.m_Markers[i].m_vPosition - geometry.m_Markers[i].m_vPosition, xiiVec2(200, 0), 0.001f);
    camera.RotateLocally(xiiAngle::MakeFromDegree(13), xiiAngle::MakeFromDegree(7), xiiAngle::MakeFromDegree(19));
  }

  for (xiiUInt32 axis = 0; axis < 6; ++axis)
  {
    const xiiVec3 direction = -xiiViewportNavigationGizmo::GetAxisDirection(axis);
    camera.LookAt(-direction * 10.0f, xiiVec3::MakeZero(), axis >= 4 ? xiiVec3::MakeAxisX() : xiiVec3::MakeAxisZ());
    const xiiViewportNavigationGizmo geometry(camera, xiiVec2(800, 600));
    XII_TEST_INT(geometry.HitTest(geometry.m_vOrigin), axis); // The front marker wins an exact overlap.
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Configured navigation basis and continuous pole orbit")
  {
    camera.SetCoordinateSystem(xiiBasisAxis::PositiveX, xiiBasisAxis::NegativeZ, xiiBasisAxis::PositiveY);
    XII_TEST_VEC3(camera.GetCoordinateSystem().m_vUpDir, xiiVec3::MakeAxisY(), 0);
    const xiiVec3 target(2, 3, 4);
    camera.LookAt(target + xiiVec3(0, 10, 0), target, xiiVec3::MakeAxisX());
    const auto before = camera.GetCenterDirForwards();
    xiiCameraMoveContext::OrbitCameraAround(camera, target, xiiVec2(0, 0.0001f));
    XII_TEST_BOOL(camera.GetCenterDirForwards().Dot(before) > 0.99999f);
    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      xiiCameraMoveContext::OrbitCameraAround(camera, target, xiiVec2(0.01f, 0.02f));
      XII_TEST_FLOAT((camera.GetCenterPosition() - target).GetLength(), 10, 0.002f);
      XII_TEST_VEC3(camera.GetCenterPosition() + camera.GetCenterDirForwards() * 10.0f, target, 0.003f);
      XII_TEST_BOOL(camera.GetCenterDirUp().IsValid());
    }
  }
}

XII_CREATE_SIMPLE_TEST(EditorViewportNavigation, NavigationInput)
{
  auto* editor = xiiQtEditorApp::GetSingleton();
  xiiStartup::StartupCoreSystems();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  xiiStringBuilder directory = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  directory.AppendPath("NavigationInput");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(directory));
  editor->StartupEditor(xiiQtEditorApp::StartupFlags::Headless | xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, directory);
  XII_SCOPE_EXIT(editor->ShutdownEditor());
  xiiQtContainerWindow container;
  container.SetWindowPresentationEnabled(false);
  xiiStringBuilder path = directory;
  path.AppendPath("Project", "xiiProject");
  if (!XII_TEST_RESULT(editor->CreateOrOpenProject(true, path)))
    return;
  path = path.GetFileDirectory();
  path.AppendPath("Navigation.xiiScene");
  auto* scene = xiiDynamicCast<xiiSceneDocument*>(editor->CreateDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(scene != nullptr))
    return;
  QPointer<NavigationDocumentWindow> window = new NavigationDocumentWindow(scene);
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

  xiiEngineViewConfig config;
  config.ApplyPerspectiveSetting();
  auto* view = new NavigationView(window, window, &config);
  window->AddViewWidget(view);
  view->SetSurfaceSize(800, 600);
  XII_SCOPE_EXIT(delete view);
  NavigationInputProbe probe;
  probe.SetOwner(window, view);
  view->m_InputContexts.InsertAt(0, &probe);
  const xiiVec3   target(2, 3, 4);
  constexpr float distance = 12;
  auto            reset    = [&] {
    config.m_Camera.LookAt(target + xiiVec3(6, -8, 4).GetNormalized() * distance, target, xiiVec3::MakeAxisZ());
    view->m_pCameraMoveContext->SetOrbitDistance(distance);
  };
  auto click = [&](xiiUInt32 axis, QEvent::Type pressType = QEvent::MouseButtonPress) {
    const auto geometry = view->GetNavigationGizmoGeometry();
    for (const auto& marker : geometry.m_Markers)
    {
      if (marker.m_uiAxis == axis)
      {
        view->Mouse(pressType, marker.m_vPosition, Qt::LeftButton, Qt::LeftButton);
        view->Mouse(QEvent::MouseButtonRelease, marker.m_vPosition, Qt::LeftButton, Qt::NoButton);
        break;
      }
    }
  };
  auto checkOrbit = [&] {
    XII_TEST_VEC3(view->GetCameraOrbitPoint(), target, 0.001f);
    XII_TEST_FLOAT((config.m_Camera.GetCenterPosition() - target).GetLength(), distance, 0.001f);
    XII_TEST_FLOAT(config.m_Camera.GetCenterDirUp().Dot(config.m_Camera.GetCenterDirForwards()), 0, 0.0001f);
  };
  for (bool orthographic : {false, true})
  {
    config.m_Camera.SetCameraMode(orthographic ? xiiCameraMode::OrthoFixedHeight : xiiCameraMode::PerspectiveFixedFovY, orthographic ? 24 : 70, 0.1f, 1000);
    for (xiiUInt32 axis = 0; axis < 6; ++axis)
    {
      reset();
      click(axis);
      for (xiiUInt32 step = 0; step < 8; ++step)
      {
        view->AdvanceInterpolation(0.05f);
        checkOrbit();
      }
      XII_TEST_VEC3(config.m_Camera.GetCenterDirForwards(), -xiiViewportNavigationGizmo::GetAxisDirection(axis), 0.001f);
      XII_TEST_BOOL(config.m_Camera.IsOrthographic() == orthographic);
      XII_TEST_FLOAT(config.m_Camera.GetFovOrDim(), orthographic ? 24 : 70, 0);
      click(axis, QEvent::MouseButtonDblClick);
      view->AdvanceInterpolation(1);
      XII_TEST_VEC3(config.m_Camera.GetCenterDirForwards(), xiiViewportNavigationGizmo::GetAxisDirection(axis), 0.001f);
      checkOrbit();
      click(axis);
      view->AdvanceInterpolation(1);
      XII_TEST_VEC3(config.m_Camera.GetCenterDirForwards(), -xiiViewportNavigationGizmo::GetAxisDirection(axis), 0.001f);
      checkOrbit();
    }
  }
  XII_TEST_INT(probe.m_uiPresses, 0);
  reset();
  const auto center = view->GetNavigationGizmoGeometry().m_vOrigin;
  for (const auto& position : {QPointF(center.x, center.y), QPointF(200, 200)})
  {
    QWheelEvent wheel(position, view->mapToGlobal(position.toPoint()), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(view, &wheel);
  }
  XII_TEST_INT(probe.m_uiWheels, 1);
  const auto beforeDrag = config.m_Camera.GetCenterDirForwards();
  view->Mouse(QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
  view->Mouse(QEvent::MouseMove, center + xiiVec2(30, 15), Qt::NoButton, Qt::LeftButton);
  view->Mouse(QEvent::MouseMove, xiiVec2(300, 300), Qt::NoButton, Qt::LeftButton);
  checkOrbit();
  XII_TEST_BOOL(!config.m_Camera.GetCenterDirForwards().IsEqual(beforeDrag, 0.01f));
  view->Mouse(QEvent::MouseButtonRelease, xiiVec2(300, 300), Qt::LeftButton, Qt::NoButton);
  XII_TEST_BOOL(!xiiEditorInputContext::IsAnyInputContextActive());
  view->AdvanceInterpolation(1);
  checkOrbit();

  // Release away from the marker must not start a snap.
  reset();
  const auto unchanged = config.m_Camera.GetCenterPosition();
  click(0);
  view->CancelCameraInterpolation();
  view->Mouse(QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
  view->Mouse(QEvent::MouseButtonRelease, xiiVec2(200, 200), Qt::LeftButton, Qt::NoButton);
  view->AdvanceInterpolation(1);
  XII_TEST_VEC3(config.m_Camera.GetCenterPosition(), unchanged, 0.001f);

  for (bool escape : {true, false})
  {
    view->Mouse(QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
    if (escape)
    {
      QKeyEvent event(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
      QApplication::sendEvent(view, &event);
    }
    else
    {
      QFocusEvent event(QEvent::FocusOut);
      QApplication::sendEvent(view, &event);
    }
    XII_TEST_BOOL(!xiiEditorInputContext::IsAnyInputContextActive());
  }

  config.m_Camera.SetCoordinateSystem(xiiBasisAxis::PositiveX, xiiBasisAxis::NegativeZ, xiiBasisAxis::PositiveY);
  reset();
  view->InterpolateCameraOrbitTo(xiiVec3::MakeAxisY());
  view->AdvanceInterpolation(1);
  XII_TEST_VEC3(config.m_Camera.GetCenterDirForwards(), xiiVec3::MakeAxisY(), 0.001f);
  XII_TEST_VEC3(config.m_Camera.GetCenterDirUp(), xiiVec3::MakeAxisX(), 0.001f);
  checkOrbit();

  view->Mouse(QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
  view->SetNavigationGizmoEnabled(false);
  XII_TEST_BOOL(!xiiEditorInputContext::IsAnyInputContextActive());
  view->Mouse(QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
  XII_TEST_INT(probe.m_uiPresses, 1);
  view->SetNavigationGizmoEnabled(true);
  view->Mouse(QEvent::MouseButtonPress, xiiVec2(200, 200), Qt::LeftButton, Qt::LeftButton);
  XII_TEST_INT(probe.m_uiPresses, 2);
  view->SetSurfaceSize(1000, 400);
  XII_TEST_VEC2(view->GetNavigationGizmoGeometry().m_vOrigin, xiiVec2(936, 64), 0);

  xiiEngineViewConfig previewConfig;
  previewConfig.ApplyPerspectiveSetting();
  auto* preview = new xiiQtOrbitCamViewWidget(window, &previewConfig);
  window->AddViewWidget(preview);
  XII_SCOPE_EXIT(delete preview);
  preview->ConfigureFixed(target, xiiVec3(2), target + xiiVec3(0, -10, 0));
  const auto scenePosition = config.m_Camera.GetCenterPosition();
  preview->OrbitCamera(xiiVec2(0.2f, 0.1f));
  XII_TEST_VEC3(preview->GetCameraOrbitPoint(), target, 0);
  XII_TEST_FLOAT((previewConfig.m_Camera.GetCenterPosition() - target).GetLength(), 10, 0.001f);
  XII_TEST_VEC3(config.m_Camera.GetCenterPosition(), scenePosition, 0);
  preview->SetNavigationGizmoEnabled(false);
  XII_TEST_BOOL(view->IsNavigationGizmoEnabled());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Asset preview clipping follows navigation and bounds without hiding the surrounding scene")
  {
    auto& camera       = previewConfig.m_Camera;
    auto  checkVisible = [&](const xiiVec3& point) {
      const auto frustum = xiiFrustum::MakeFromFOV(camera.GetCenterPosition(), camera.GetCenterDirForwards(), camera.GetCenterDirUp(), camera.GetFovX(1), camera.GetFovY(1), camera.GetNearPlane(), camera.GetFarPlane());
      XII_TEST_INT(frustum.GetObjectPosition(&point, 1), xiiVolumePosition::Inside);
    };
    for (float extent : {0.1f, 1.0f, 1000.0f})
    {
      preview->ConfigureRelative(target, xiiVec3(extent), xiiVec3(5, -2, 3), 2);
      preview->SetOrbitVolume(target, xiiVec3(extent)); // Consume the initial bounds reply before navigating.
      XII_TEST_BOOL(camera.GetFarPlane() >= 1000);
      XII_TEST_BOOL(camera.GetNearPlane() < extent * 0.01f);
      checkVisible(camera.GetCenterPosition() + camera.GetCenterDirForwards() * 500.0f);

      const xiiVec3 position = target + xiiVec3(0, -20000, 0);
      camera.LookAt(position, target, xiiVec3::MakeAxisZ());
      preview->SyncToEngine();
      XII_TEST_VEC3(camera.GetCenterPosition(), position, 0);
      checkVisible(target);
      checkVisible(target + xiiVec3(0, extent, 0));

      QWheelEvent wheel(QPointF(20, 20), preview->mapToGlobal(QPoint(20, 20)), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
      QApplication::sendEvent(preview, &wheel);
      preview->SyncToEngine();
      const auto zoomedPosition = camera.GetCenterPosition();
      XII_TEST_BOOL((zoomedPosition - target).GetLength() > (position - target).GetLength());
      checkVisible(target);

      // A later bounds response must expand clipping without resetting navigation.
      preview->SetOrbitVolume(target, xiiVec3(extent * 10));
      preview->SyncToEngine();
      XII_TEST_VEC3(camera.GetCenterPosition(), zoomedPosition, 0);
      checkVisible(target + xiiVec3(0, extent * 10, 0));
    }

    // Fixed texture/material previews have no asset bounds and keep their authored planes.
    previewConfig.ApplyPerspectiveSetting(90, 0.01f, 100);
    preview->ConfigureFixed(target, xiiVec3::MakeZero(), target + xiiVec3(2, 0, 0));
    preview->SyncToEngine();
    XII_TEST_FLOAT(camera.GetNearPlane(), 0.01f, 0);
    XII_TEST_FLOAT(camera.GetFarPlane(), 100, 0);
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 12, -1000, 1000);
    preview->ConfigureRelative(target, xiiVec3(10), xiiVec3(5, -2, 3), 2);
    preview->SyncToEngine();
    XII_TEST_FLOAT(camera.GetNearPlane(), -1000, 0);
    XII_TEST_FLOAT(camera.GetFarPlane(), 1000, 0);
    XII_TEST_VEC3(config.m_Camera.GetCenterPosition(), scenePosition, 0);
  }

  // Navigation in another viewport must not cancel this view's animation.
  NavigationInputProbe otherViewInput;
  otherViewInput.SetOwner(window, preview);
  view->InterpolateCameraOrbitTo(-xiiVec3::MakeAxisX());
  otherViewInput.MakeActiveInputContext();
  view->AdvanceInterpolation(1);
  otherViewInput.MakeActiveInputContext(false);
  XII_TEST_VEC3(config.m_Camera.GetCenterDirForwards(), -xiiVec3::MakeAxisX(), 0.001f);
  checkOrbit();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Hidden native viewport receives navigation through production IPC and rendering")
  {
    auto* connection = xiiEditorEngineProcessConnection::GetSingleton();
    if (!XII_TEST_RESULT(connection->RestartProcess()))
      return;
    const auto loaded = scene->WaitForEngineStatusLoaded(xiiTime::MakeFromSeconds(10));
    XII_TEST_STATUS(loaded);
    if (loaded.Failed())
      return;
    config.m_Camera.SetCoordinateSystem(xiiBasisAxis::PositiveX, xiiBasisAxis::PositiveY, xiiBasisAxis::PositiveZ);
    config.m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 70, 0.1f, 1000);
    reset();
    view->SetSurfaceSize(320, 240);
    xiiUInt32 frame = 0;
    auto      tick  = [&] {
      view->SyncToEngine();
      xiiQtUiServices::TickEvent event;
      event.m_Type    = xiiQtUiServices::TickEvent::Type::EndFrame;
      event.m_uiFrame = frame++;
      event.m_Time    = xiiTime::Now();
      xiiQtUiServices::s_TickEvent.Broadcast(event);
      QApplication::processEvents(QEventLoop::AllEvents, 10);
      QThread::msleep(5);
    };
    auto capture = [&](bool enabled, xiiImage& image) {
      view->SetNavigationGizmoEnabled(enabled);
      xiiStringBuilder output = directory;
      output.AppendPath(enabled ? "ViewportEnabled.png" : "ViewportDisabled.png");
      if (xiiOSFile::ExistsFile(output))
        XII_TEST_RESULT(xiiOSFile::DeleteFile(output));
      tick();
      view->TakeScreenshot(output);
      QElapsedTimer elapsed;
      elapsed.start();
      while (!xiiOSFile::ExistsFile(output) && elapsed.elapsed() < 15000)
        tick();
      return image.LoadFrom(output);
    };
    xiiImage enabled;
    xiiImage disabled;
    if (!XII_TEST_RESULT(capture(true, enabled)) || !XII_TEST_RESULT(capture(false, disabled)))
      return;
    XII_TEST_INT(enabled.GetWidth(), disabled.GetWidth());
    XII_TEST_INT(enabled.GetHeight(), disabled.GetHeight());
    const float ratio          = static_cast<float>(view->devicePixelRatio());
    const auto  geometry       = view->GetNavigationGizmoGeometry();
    xiiUInt32   changedInside  = 0;
    xiiUInt32   changedOutside = 0;
    for (xiiUInt32 y = 0; y < enabled.GetHeight(); ++y)
      for (xiiUInt32 x = 0; x < enabled.GetWidth(); ++x)
      {
        const auto* a = enabled.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y);
        const auto* b = disabled.GetPixelPointer<xiiUInt8>(0, 0, 0, x, y);
        if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2])
          continue;
        if (geometry.Contains(xiiVec2(static_cast<float>(x) / ratio, static_cast<float>(y) / ratio)))
          ++changedInside;
        else
          ++changedOutside;
      }
    XII_TEST_BOOL(changedInside > 200);
    XII_TEST_INT(changedOutside, 0);
    view->SetNavigationGizmoEnabled(true);
  }
#endif
}
