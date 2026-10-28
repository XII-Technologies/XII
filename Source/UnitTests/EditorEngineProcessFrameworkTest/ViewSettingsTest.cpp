/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>

XII_CREATE_SIMPLE_TEST_GROUP(EditorViews);
XII_CREATE_SIMPLE_TEST(EditorViews, Settings)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Engine ray-tracing capability survives the startup handshake")
  {
    for (bool supported : {false, true})
    {
      xiiProjectReadyMsgToEditor message;
      message.m_bHardwareRayTracing = supported;
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      xiiReflectionSerializer::WriteObjectToBinary(writer, message.GetDynamicRTTI(), &message);
      xiiProjectReadyMsgToEditor copy;
      xiiMemoryStreamReader      reader(&storage);
      xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *copy.GetDynamicRTTI(), &copy);
      XII_TEST_BOOL(copy.m_bHardwareRayTracing == supported);
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Redraw camera and render mode survive IPC serialization")
  {
    xiiViewRedrawMsgToEngine message;
    message.m_CameraMode             = xiiCameraMode::OrthoFixedHeight;
    message.m_RenderMode             = xiiViewRenderMode::Wireframe;
    message.m_fFovOrDim              = 42.0f;
    message.m_fNearPlane             = -1000.0f;
    message.m_fFarPlane              = 1000.0f;
    message.m_uiHWND                 = 0;
    message.m_uiWindowWidth          = 80;
    message.m_uiWindowHeight         = 60;
    message.m_bUpdatePickingData     = false;
    message.m_bEnablePickingSelected = false;
    message.m_bEnablePickTransparent = false;
    message.m_vPosition              = xiiVec3::MakeZero();
    message.m_vDirForwards           = xiiVec3(1, 0, 0);
    message.m_vDirUp                 = xiiVec3(0, 0, 1);
    message.m_vDirRight              = xiiVec3(0, -1, 0);
    message.m_ViewMatrix             = xiiMat4::MakeIdentity();
    message.m_ProjMatrix             = xiiMat4::MakeIdentity();
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiReflectionSerializer::WriteObjectToBinary(writer, message.GetDynamicRTTI(), &message);
    xiiViewRedrawMsgToEngine copy;
    xiiMemoryStreamReader    reader(&storage);
    xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *copy.GetDynamicRTTI(), &copy);
    XII_TEST_BOOL(copy.m_CameraMode == message.m_CameraMode);
    XII_TEST_BOOL(copy.m_RenderMode == message.m_RenderMode);
    XII_TEST_FLOAT(copy.m_fFovOrDim, 42.0f, 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Preview lighting toggles and cleans up Engine objects")
  {
    xiiWorldDescription description("Preview lighting test");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    {
      xiiEngineViewLightSettings settings(false);
      settings.SetupForEngine(&world, 0);
      XII_TEST_INT(world.GetObjectCount(), 0);
      settings.SetDirectionalLight(true);
      settings.SetDirectionalLightIntensity(25.0f);
      settings.SetDirectionalLightShadows(true);
      settings.UpdateForEngine(&world);
      XII_TEST_INT(world.GetObjectCount(), 1);
      auto                          objects = world.GetObjects();
      xiiDirectionalLightComponent* light   = nullptr;
      XII_TEST_BOOL(objects->TryGetComponentOfBaseType(light));
      XII_TEST_FLOAT(light->GetIntensity(), 25.0f, 0.0f);
      XII_TEST_BOOL(light->GetCastShadows());
      const auto oldHandle = objects->GetHandle();
      settings.SetDirectionalLight(false);
      settings.UpdateForEngine(&world);
      XII_TEST_INT(world.GetObjectCount(), 0);
      settings.SetDirectionalLight(true);
      settings.UpdateForEngine(&world);
      XII_TEST_INT(world.GetObjectCount(), 1);
      xiiGameObject* oldObject = nullptr;
      XII_TEST_BOOL(!world.TryGetObject(oldHandle, oldObject));
      settings.UpdateForEngine(&world);
      XII_TEST_INT(world.GetObjectCount(), 1);
    }
    XII_TEST_INT(world.GetObjectCount(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Camera framing rejects invalid bounds and converges")
  {
    xiiCamera  camera;
    const auto direction = xiiVec3(1, 0, 0);
    XII_TEST_BOOL(!xiiEngineProcessViewContext::FocusCameraOnObject(camera, xiiBoundingBoxSphere::MakeInvalid(), 70.0f, direction));
    const auto bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 2.0f));
    XII_TEST_BOOL(xiiEngineProcessViewContext::FocusCameraOnObject(camera, bounds, 70.0f, direction));
    XII_TEST_BOOL(!xiiEngineProcessViewContext::FocusCameraOnObject(camera, bounds, 70.0f, direction));
    XII_TEST_BOOL(camera.GetCenterPosition().IsValid());
  }
}
