/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

XII_CREATE_SIMPLE_TEST(EditorViews, ViewProjection)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription worldDescription("Native view projection");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  xiiCamera camera;
  camera.LookAt(xiiVec3(-3, 2, 4), xiiVec3(0, 2, 4), xiiVec3::MakeAxisZ());
  auto*      renderer   = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView*   view       = nullptr;
  const auto viewHandle = renderer->CreateView("Projection", view);
  XII_SCOPE_EXIT(renderer->DestroyView(viewHandle));
  view->SetCamera(&camera);
  view->SetViewport({11, 27, 640, 320});

  const auto checkRay = [&](float x, float y, xiiCameraEye eye) {
    const auto& data = view->GetData();
    xiiVec3     start, direction, nearPosition, screen, furtherScreen;
    if (!XII_TEST_RESULT(data.ComputePickingRay(x, y, start, direction, eye)))
      return;
    XII_TEST_FLOAT(direction.GetLength(), 1, 0.0001f);
    XII_TEST_BOOL(direction.Dot(camera.GetDirForwards(eye)) > 0);
    XII_TEST_RESULT(data.ComputeWorldSpacePos(x, y, nearPosition, eye));
    XII_TEST_VEC3(start, nearPosition, 0.0001f);
    XII_TEST_RESULT(data.ComputeScreenSpacePos(start, screen, eye));
    XII_TEST_VEC3(screen, xiiVec3(x, y, 1), 0.0001f);
    XII_TEST_RESULT(data.ComputeScreenSpacePos(start + direction, furtherScreen, eye));
    XII_TEST_FLOAT(furtherScreen.x, x, 0.0001f);
    XII_TEST_FLOAT(furtherScreen.y, y, 0.0001f);
    XII_TEST_BOOL(furtherScreen.z < screen.z);
    const auto eyePosition = camera.GetPosition(eye);
    XII_TEST_FLOAT((start - eyePosition).Dot(camera.GetDirForwards(eye)), camera.GetNearPlane(), 0.0001f);
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Perspective and orthographic view depth matches native rasterization")
  {
    const xiiCameraMode::Enum modes[] = {xiiCameraMode::PerspectiveFixedFovX, xiiCameraMode::PerspectiveFixedFovY,
                                         xiiCameraMode::OrthoFixedWidth, xiiCameraMode::OrthoFixedHeight};
    for (auto mode : modes)
    {
      const bool orthographic = mode == xiiCameraMode::OrthoFixedWidth || mode == xiiCameraMode::OrthoFixedHeight;
      camera.SetCameraMode(mode, orthographic ? 10.0f : 60.0f, 0.1f, 100);
      xiiMat4 coreProjection;
      camera.GetProjectionMatrix(2, coreProjection, xiiCameraEye::Left, xiiClipSpaceDepthRange::ZeroToOne);
      const auto coreNear = coreProjection * xiiVec4(0, 0, 0.1f, 1);
      const auto coreFar  = coreProjection * xiiVec4(0, 0, 100, 1);
      XII_TEST_FLOAT(coreNear.z / coreNear.w, 0, 0.0001f);
      XII_TEST_FLOAT(coreFar.z / coreFar.w, 1, 0.0001f);
      for (auto eye : {xiiCameraEye::Left, xiiCameraEye::Right})
      {
        const auto& projection = view->GetProjectionMatrix(eye);
        const auto  nearClip   = projection * xiiVec4(0, 0, 0.1f, 1);
        const auto  farClip    = projection * xiiVec4(0, 0, 100, 1);
        XII_TEST_FLOAT(nearClip.z / nearClip.w, 1, 0.0001f);
        XII_TEST_FLOAT(farClip.z / farClip.w, 0, 0.0001f);
        XII_TEST_BOOL((projection * view->GetInverseProjectionMatrix(eye)).IsIdentity(0.0001f));
        checkRay(0.5f, 0.5f, eye);
        checkRay(0.2f, 0.75f, eye);
      }
      xiiVec3 start, direction;
      XII_TEST_RESULT(view->ComputePickingRay(0.5f, 0.5f, start, direction));
      XII_TEST_VEC3(direction, camera.GetCenterDirForwards(), 0.0001f);
      XII_TEST_VEC3(start, camera.GetCenterPosition() + direction * 0.1f, 0.0001f);
    }
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 10, -1000, 1000);
    xiiVec3 start, direction;
    XII_TEST_RESULT(view->ComputePickingRay(0.5f, 0.5f, start, direction));
    XII_TEST_VEC3(start, camera.GetCenterPosition() - camera.GetCenterDirForwards() * 1000.0f, 0.001f);
    XII_TEST_VEC3(direction, camera.GetCenterDirForwards(), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Stereo asymmetry and infinite far planes preserve forward picking rays")
  {
    camera.SetCameraMode(xiiCameraMode::Stereo, 60, 0.1f, 100);
    const auto left  = xiiGraphicsUtils::CreatePerspectiveProjectionMatrix(-0.08f, 0.12f, -0.05f, 0.05f, 0.1f, 100,
                                                                           xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    const auto right = xiiGraphicsUtils::CreatePerspectiveProjectionMatrix(-0.12f, 0.08f, -0.05f, 0.05f, 0.1f, 100,
                                                                           xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
    camera.SetStereoProjection(left, right, 2);
    for (auto eye : {xiiCameraEye::Left, xiiCameraEye::Right})
    {
      camera.SetViewMatrix(xiiGraphicsUtils::CreateLookAtViewMatrix(xiiVec3(-3, eye == xiiCameraEye::Left ? 1.9f : 2.1f, 4),
                                                                    xiiVec3(0, 2, 4), xiiVec3::MakeAxisZ(), xiiHandedness::LeftHanded),
                           eye);
      checkRay(0.5f, 0.5f, eye);
      checkRay(0.1f, 0.9f, eye);
    }
    XII_TEST_BOOL(!view->GetProjectionMatrix(xiiCameraEye::Left).IsEqual(view->GetProjectionMatrix(xiiCameraEye::Right), 0.0001f));
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, xiiMath::Infinity<float>());
    checkRay(0.5f, 0.5f, xiiCameraEye::Left);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Replacing cameras invalidates matrices even when their counters match")
  {
    xiiCamera first, second;
    first.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    second.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 80, 0.2f, 200);
    first.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    second.LookAt(xiiVec3(0, -3, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    XII_TEST_INT(first.GetSettingsModificationCounter(), second.GetSettingsModificationCounter());
    XII_TEST_INT(first.GetOrientationModificationCounter(), second.GetOrientationModificationCounter());
    view->SetCamera(&first);
    const auto oldProjection = view->GetProjectionMatrix(xiiCameraEye::Left);
    view->SetCamera(&second);
    XII_TEST_BOOL(!oldProjection.IsEqual(view->GetProjectionMatrix(xiiCameraEye::Left), 0.0001f));
    xiiVec3 start, direction;
    XII_TEST_RESULT(view->ComputePickingRay(0.5f, 0.5f, start, direction));
    XII_TEST_VEC3(direction, second.GetCenterDirForwards(), 0.0001f);
    XII_TEST_VEC3(start, second.GetCenterPosition() + direction * 0.2f, 0.0001f);
    view->SetCamera(&camera);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid projection data fails without changing query outputs")
  {
    auto data = view->GetData();
    data.m_InverseViewProjectionMatrix[0].SetZero();
    xiiVec3 start(1), direction(2), position(3);
    XII_TEST_BOOL(data.ComputePickingRay(0.5f, 0.5f, start, direction).Failed());
    XII_TEST_VEC3(start, xiiVec3(1), 0);
    XII_TEST_VEC3(direction, xiiVec3(2), 0);
    XII_TEST_BOOL(data.ComputeWorldSpacePos(0.5f, 0.5f, position).Failed());
    XII_TEST_VEC3(position, xiiVec3(3), 0);
    XII_TEST_BOOL(data.ComputePickingRay(xiiMath::Infinity<float>(), 0.5f, start, direction).Failed());
    XII_TEST_BOOL(data.ComputeWorldSpacePos(0.5f, 0.5f, position, static_cast<xiiCameraEye>(2)).Failed());
    view->SetCamera(nullptr);
    XII_TEST_BOOL(view->ComputePickingRay(0.5f, 0.5f, start, direction).Failed());
    XII_TEST_BOOL(view->ComputeWorldSpacePos(0.5f, 0.5f, position).Failed());
  }
}
