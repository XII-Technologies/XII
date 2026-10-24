/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
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
    explicit GizmoTestDocument(xiiWorld& world) : xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::None)
    {
      m_pWorld = &world;
      XII_TEST_STATUS(Initialize(xiiUuid::MakeUuid(), {}, nullptr, "Gizmo test"));
    }
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
  class GizmoTestView : public xiiEngineProcessViewContext
  {
  public:
    GizmoTestView(GizmoTestDocument& document, xiiViewHandle view) : xiiEngineProcessViewContext(&document) { m_hView = view; }
    using xiiEngineProcessViewContext::PickGizmoAtPixel;

  protected:
    xiiViewHandle CreateView() override { return {}; }
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
    view->SetPipelineParameter("Editor.DevicePixelRatio", 2.0f);
    CheckSize(256);
    view->RemovePipelineParameter("Editor.DevicePixelRatio");
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shader descriptions and picking share ring, plane and DPI transforms")
  {
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
    camera.LookAt(xiiVec3(0, 0, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
    handle.SetTransformation(xiiTransform::MakeIdentity());
    handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::RotationRing, xiiColor::Red, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::ShowInOrtho);
    xiiMat4 transform;
    float   pixel, time = -1;
    XII_TEST_RESULT(handle.ComputeViewTransform(*view, transform, pixel));
    const auto edge = transform.TransformPosition(xiiVec3::MakeAxisX());
    XII_TEST_RESULT(handle.PickRay(*view, edge + xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), time));
    XII_TEST_BOOL(handle.PickRay(*view, xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), time).Failed());
    handle.SetVisualState(xiiGizmoVisualState::Active);
    handle.SetVisualParameters(0.2f, 1.3f);
    handle.SetDragOrigin(xiiVec3(1, 2, 0));
    xiiDynamicArray<xiiEditorGizmoInstance> instances;
    xiiVariantArray                         files;
    XII_TEST_RESULT(handle.AppendRenderData(*view, 0, instances, files));
    XII_TEST_INT(instances.GetCount(), 1);
    XII_TEST_INT(instances[0].Identity.x, static_cast<xiiUInt32>(xiiEngineGizmoHandleType::RotationRing));
    XII_TEST_BOOL((instances[0].Identity.y & XII_EDITOR_GIZMO_ACTIVE) != 0);
    XII_TEST_FLOAT(instances[0].Parameters.y, 0.2f, 0);
    XII_TEST_FLOAT(instances[0].Parameters.z, 1.3f, 0);
    XII_TEST_VEC4(instances[0].GuideStart, xiiVec4(1, 2, -5, 0), 0);
    handle.SetVisualState(xiiGizmoVisualState::Disabled);
    XII_TEST_BOOL(handle.PickRay(*view, edge + xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), time).Failed());
    handle.SetVisualState(xiiGizmoVisualState::Normal);
    handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::TranslationPlane, xiiColor::Blue, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::ShowInOrtho);
    const auto center = transform.TransformPosition(xiiVec3(0.36f, 0.36f, 0));
    XII_TEST_RESULT(handle.PickRay(*view, center + xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), time));
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
    {
      xiiEngineGizmoHandle uniform;
      uniform.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::ScaleUniform, xiiColor::LightGray, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
      uniform.SetVisible(true);
      document.SendHandle(uniform);
      XII_TEST_RESULT(document.PickGizmo(*view, 0.5f, 0.5f, result));
      XII_TEST_BOOL(result.m_HandleGuid == uniform.GetGuid());
      document.RemoveHandle(uniform);
    }
    farHandle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Sphere, xiiColor::Red, xiiGizmoFlags::Pickable);
    document.SendHandle(farHandle);
    XII_TEST_INT(document.GetContext().m_OtherPickingMap.GetHandle(farHandle.GetGuid()), farID);
    farHandle.SetVisualState(xiiGizmoVisualState::Active);
    farHandle.SetVisualParameters(0.5f, 1.0f);
    document.SendHandle(farHandle);
    XII_TEST_RESULT(document.DrawGizmos(*view));
    const auto* snapshot = view->GetPipelineParameters().GetValue("Editor.Gizmos");
    XII_TEST_BOOL(snapshot && snapshot->IsA<xiiDataBuffer>());
    if (snapshot && snapshot->IsA<xiiDataBuffer>())
    {
      const auto& bytes  = snapshot->Get<xiiDataBuffer>();
      bool        active = false;
      for (xiiUInt32 offset = 0; offset < bytes.GetCount(); offset += sizeof(xiiEditorGizmoInstance))
      {
        xiiEditorGizmoInstance instance;
        xiiMemoryUtils::Copy(reinterpret_cast<xiiUInt8*>(&instance), bytes.GetData() + offset, sizeof(instance));
        if (instance.Identity.z == farID)
        {
          active = (instance.Identity.y & XII_EDITOR_GIZMO_ACTIVE) != 0;
          XII_TEST_FLOAT(instance.Parameters.y, 0.5f, 0);
          XII_TEST_FLOAT(instance.Parameters.z, 1.0f, 0);
        }
      }
      XII_TEST_BOOL(active);
    }
    farHandle.SetVisualState(xiiGizmoVisualState::Normal);
    document.SendHandle(farHandle);
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
    // The view context owns this view and destroys it when the block ends.
    GizmoTestView pickingView(document, view->GetHandle());
    nearHandle.SetVisible(true);
    document.SendHandle(nearHandle);
    view->SetViewport({11, 27, 511, 511});
    xiiViewPickingResultMsgToEditor reply;
    reply.m_DocumentGuid    = xiiUuid::MakeUuid();
    reply.m_uiViewID        = 19;
    const auto documentGuid = reply.m_DocumentGuid;
    XII_TEST_RESULT(pickingView.PickGizmoAtPixel(266, 282, reply));
    XII_TEST_BOOL(reply.m_OtherGuid == nearHandle.GetGuid());
    XII_TEST_BOOL(!reply.m_ObjectGuid.IsValid() && !reply.m_ComponentGuid.IsValid());
    XII_TEST_VEC3(reply.m_vPickedPosition, xiiVec3(0, 0, 1.5f), 0.001f);
    xiiVec3 rayStart, rayDirection;
    XII_TEST_RESULT(view->ComputePickingRay(0.5f, 0.5f, rayStart, rayDirection));
    XII_TEST_VEC3(reply.m_vPickingRayStartPosition, rayStart, 0.001f);
    XII_TEST_VEC3(reply.m_vPickedNormal, xiiVec3::MakeZero(), 0);
    XII_TEST_BOOL(reply.m_DocumentGuid == documentGuid);
    XII_TEST_INT(reply.m_uiViewID, 19);
    reply.m_ObjectGuid = reply.m_ComponentGuid = xiiUuid::MakeUuid();
    reply.m_uiPartIndex                        = 42;
    reply.m_vPickedNormal.Set(1);
    XII_TEST_BOOL(pickingView.PickGizmoAtPixel(0, 0, reply).Failed());
    XII_TEST_BOOL(!reply.m_OtherGuid.IsValid() && !reply.m_ObjectGuid.IsValid() && !reply.m_ComponentGuid.IsValid());
    XII_TEST_INT(reply.m_uiPartIndex, 0);
    XII_TEST_VEC3(reply.m_vPickedPosition, xiiVec3::MakeZero(), 0);
    XII_TEST_VEC3(reply.m_vPickingRayStartPosition, xiiVec3::MakeZero(), 0);
    XII_TEST_VEC3(reply.m_vPickedNormal, xiiVec3::MakeZero(), 0);
    XII_TEST_BOOL(reply.m_DocumentGuid == documentGuid);
    XII_TEST_INT(reply.m_uiViewID, 19);
    view->SetViewport({0, 0, 0, 0});
    XII_TEST_BOOL(pickingView.PickGizmoAtPixel(0, 0, reply).Failed());
    view->SetViewport({0, 0, 512, 512});
    xiiCamera uninitializedCamera;
    view->SetCamera(&uninitializedCamera);
    XII_TEST_BOOL(pickingView.PickGizmoAtPixel(256, 256, reply).Failed());
    view->SetCamera(&camera);
    document.RemoveHandle(nearHandle);
    XII_TEST_BOOL(document.GetContext().m_OtherPickingMap.GetHandleToGuidMap().IsEmpty());
  }
}
