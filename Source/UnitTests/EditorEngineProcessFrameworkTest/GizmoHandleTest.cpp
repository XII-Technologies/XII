/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  class GizmoTestDocument : public xiiEngineProcessDocumentContext
  {
  public:
    explicit GizmoTestDocument(xiiWorld& world) : xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::None) { m_pWorld = &world; }
    ~GizmoTestDocument() { Deinitialize(); }
    using xiiEngineProcessDocumentContext::UpdateSyncObjects;

    void SendHandle(const xiiEngineGizmoHandle& handle)
    {
      xiiEditorEngineSyncObjectMsg message;
      message.m_ObjectGuid  = handle.GetGuid();
      message.m_sObjectType = handle.GetDynamicRTTI()->GetTypeName();
      xiiContiguousMemoryStreamStorage storage;
      xiiMemoryStreamWriter            writer(&storage);
      xiiReflectionSerializer::WriteObjectToBinary(writer, handle.GetDynamicRTTI(), &handle);
      message.m_ObjectData = xiiArrayPtr<const xiiUInt8>(storage.GetData(), storage.GetStorageSize32());
      ProcessEditorEngineSyncObjectMsg(message);
      UpdateSyncObjects();
    }

    void RemoveHandle(const xiiEngineGizmoHandle& handle)
    {
      xiiEditorEngineSyncObjectMsg message;
      message.m_ObjectGuid = handle.GetGuid();
      ProcessEditorEngineSyncObjectMsg(message);
    }

  protected:
    xiiEngineProcessViewContext* CreateViewContext() override { return nullptr; }
    void                         DestroyViewContext(xiiEngineProcessViewContext*) override {}
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, GizmoHandles)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription description("Synchronized gizmo test");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Gizmo test", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt(xiiVec3(0, 0, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 512, 512});

  xiiEngineGizmoHandle handle;
  handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Box, xiiColor::Red, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::ShowInOrtho);
  handle.SetVisible(true);
  XII_SCOPE_EXIT(xiiEngineGizmoHandle::SetGizmoScale(1).IgnoreResult());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Screen size follows projection, distance, viewport, and preference")
  {
    auto CheckSize = [&](float fExpectedPixels) {
      xiiMat4 transform;
      float   fPixelSize = -1;
      XII_TEST_RESULT(handle.ComputeViewTransform(*view, transform, fPixelSize));
      xiiVec3 center, top;
      XII_TEST_RESULT(view->ComputeScreenSpacePos(transform.TransformPosition(xiiVec3::MakeZero()), center));
      XII_TEST_RESULT(view->ComputeScreenSpacePos(transform.TransformPosition(xiiVec3::MakeAxisY()), top));
      XII_TEST_FLOAT(xiiMath::Abs(top.y - center.y) * view->GetViewport().height, fExpectedPixels, 0.01f);
      XII_TEST_BOOL(fPixelSize > 0);
    };
    CheckSize(128);
    camera.LookAt(xiiVec3(0, 0, 10), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
    CheckSize(128);
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 90, 0.1f, 100);
    CheckSize(128);
    view->SetViewport({11, 27, 1024, 768});
    CheckSize(128);
    XII_TEST_RESULT(xiiEngineGizmoHandle::SetGizmoScale(1.5f));
    CheckSize(192);
    XII_TEST_BOOL(xiiEngineGizmoHandle::SetGizmoScale(0).Failed());
    XII_TEST_BOOL(xiiEngineGizmoHandle::SetGizmoScale(xiiMath::Infinity<float>()).Failed());
    XII_TEST_FLOAT(xiiEngineGizmoHandle::GetGizmoScale(), 1.5f, 0);
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, -100, 100);
    CheckSize(192);
    camera.SetCameraMode(xiiCameraMode::OrthoFixedWidth, 8, -100, 100);
    CheckSize(192);
    XII_TEST_RESULT(xiiEngineGizmoHandle::SetGizmoScale(1));
    view->SetViewport({0, 0, 512, 512});
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Visibility, orthographic filtering, small nobs, and camera-facing axes")
  {
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    camera.LookAt(xiiVec3(0, 0, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
    handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Box, xiiColor::Red, xiiGizmoFlags::Pickable);
    float fTime = -1;
    XII_TEST_RESULT(handle.PickRay(*view, xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), fTime));
    XII_TEST_FLOAT(fTime, 4.5f, 0.001f);
    auto transform = xiiTransform::MakeIdentity();
    transform.m_vScale.Set(0.001f);
    handle.SetTransformation(transform);
    XII_TEST_RESULT(handle.PickRay(*view, xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), fTime));
    XII_TEST_FLOAT(fTime, 4.9995f, 0.0001f);
    handle.SetVisible(false);
    fTime = -1;
    XII_TEST_BOOL(handle.PickRay(*view, xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), fTime).Failed());
    XII_TEST_FLOAT(fTime, -1, 0);
    handle.SetVisible(true);
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, -100, 100);
    XII_TEST_BOOL(handle.PickRay(*view, xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), fTime).Failed());
    handle.SetTransformation(xiiTransform::MakeIdentity());
    handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Box, xiiColor::Red, xiiGizmoFlags::FaceCamera | xiiGizmoFlags::ShowInOrtho);
    camera.LookAt(xiiVec3(-5, 5, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
    xiiMat4 matrix;
    float   fPixelSize;
    XII_TEST_RESULT(handle.ComputeViewTransform(*view, matrix, fPixelSize));
    XII_TEST_BOOL(matrix.TransformDirection(xiiVec3::MakeAxisX()).x < 0);
    XII_TEST_BOOL(matrix.TransformDirection(xiiVec3::MakeAxisY()).y > 0);
    XII_TEST_BOOL(matrix.TransformDirection(xiiVec3::MakeAxisZ()).z > 0);
    transform.m_vScale.SetZero();
    handle.SetTransformation(transform);
    XII_TEST_BOOL(handle.ComputeViewTransform(*view, matrix, fPixelSize).Failed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IPC updates preserve identity, change topology, and remove picking registrations")
  {
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    camera.LookAt(xiiVec3(0, 0, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
    GizmoTestDocument document(world);
    XII_TEST_INT(document.GetContext().m_uiHighlightID, 0);
    xiiEngineGizmoHandle nearHandle, farHandle;
    nearHandle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Box, xiiColor::Green, xiiGizmoFlags::Pickable);
    nearHandle.SetVisible(true);
    auto transform          = xiiTransform::MakeIdentity();
    transform.m_vPosition.z = 1;
    nearHandle.SetTransformation(transform);
    farHandle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Box, xiiColor::Red, xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop);
    farHandle.SetVisible(true);
    document.SendHandle(nearHandle);
    document.SendHandle(farHandle);
    XII_TEST_INT(world.GetObjectCount(), 0);
    const auto farID = document.GetContext().m_OtherPickingMap.GetHandle(farHandle.GetGuid());
    XII_TEST_BOOL(farID != 0);
    xiiEditorGizmoPickResult result;
    XII_TEST_RESULT(document.PickGizmo(*view, 0.5f, 0.5f, result));
    XII_TEST_BOOL(result.m_HandleGuid == farHandle.GetGuid());
    XII_TEST_BOOL(result.m_bAlwaysOnTop);
    farHandle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Sphere, xiiColor::Red, xiiGizmoFlags::Pickable);
    document.SendHandle(farHandle);
    XII_TEST_INT(document.GetContext().m_OtherPickingMap.GetHandle(farHandle.GetGuid()), farID);
    XII_TEST_RESULT(document.PickGizmo(*view, 0.5f, 0.5f, result));
    XII_TEST_BOOL(result.m_HandleGuid == nearHandle.GetGuid());
    nearHandle.SetVisible(false);
    document.SendHandle(nearHandle);
    XII_TEST_RESULT(document.PickGizmo(*view, 0.5f, 0.5f, result));
    XII_TEST_BOOL(result.m_HandleGuid == farHandle.GetGuid());
    XII_TEST_FLOAT(result.m_vPosition.z, 1.0f, 0.001f); // Box -> sphere geometry was rebuilt after IPC.
    document.GetContext().m_uiHighlightID = farID;
    document.RemoveHandle(farHandle);
    XII_TEST_INT(document.GetContext().m_OtherPickingMap.GetHandle(farHandle.GetGuid()), 0);
    XII_TEST_BOOL(!document.GetContext().m_OtherPickingMap.GetGuid(farID).IsValid());
    XII_TEST_INT(document.GetContext().m_uiHighlightID, 0);
    XII_TEST_BOOL(document.PickGizmo(*view, 0.5f, 0.5f, result).Failed());
    XII_TEST_BOOL(result.m_HandleGuid == farHandle.GetGuid()); // Failure leaves the output unchanged.
    XII_TEST_BOOL(document.PickGizmo(*view, -0.1f, 0.5f, result).Failed());
    document.RemoveHandle(nearHandle);
    XII_TEST_BOOL(document.GetContext().m_OtherPickingMap.GetHandleToGuidMap().IsEmpty());
  }
}
