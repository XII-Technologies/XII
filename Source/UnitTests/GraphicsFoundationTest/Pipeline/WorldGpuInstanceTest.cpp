/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

XII_CREATE_SIMPLE_TEST(PipelineExtraction, WorldGpuInstances)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
  descriptor.AddSection(12, 0, 0);
  descriptor.ComputeBounds();
  const auto          mesh = xiiResourceManager::CreateResource<xiiMeshResource>("WorldGpuInstanceTest.Mesh", std::move(descriptor));
  xiiWorldDescription description("Authoritative GPU extraction world");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*                    module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic      = true;
  xiiGameObject* parent             = nullptr;
  objectDescription.m_LocalPosition = xiiVec3(1, 0, 0);
  const auto parentHandle           = world.CreateObject(objectDescription, parent);
  objectDescription.m_hParent       = parentHandle;
  objectDescription.m_LocalPosition = xiiVec3(2, 0, 0);
  xiiGameObject*    child           = nullptr;
  const auto        childHandle     = world.CreateObject(objectDescription, child);
  xiiMeshComponent* component       = nullptr;
  xiiMeshComponent::CreateComponent(child, component);
  component->SetMesh(mesh);
  world.Update();
  xiiExtractedRenderData extracted;
  xiiUInt64              frame   = 0;
  auto                   extract = [&] {
    extracted.Clear();
    xiiMsgExtractRenderData message;
    message.m_pExtractedRenderData = &extracted;
    if (component && component->IsActiveAndInitialized())
      component->SendMessage(message);
    extracted.SortAndBatches();
    return module->UpdateGpuInstances(extracted, ++frame);
  };
  XII_TEST_RESULT(extract());
  XII_TEST_BOOL(component->IsActiveAndInitialized());
  XII_TEST_INT(extracted.GetAllRenderData().GetCount(), 1);
  if (!extracted.GetAllRenderData().IsEmpty())
  {
    const auto* packet = xiiDynamicCast<const xiiMeshRenderData*>(extracted.GetAllRenderData()[0]);
    XII_TEST_BOOL(packet != nullptr && packet->m_hMesh == mesh && packet->m_hOwnerObject == childHandle);
    const auto geometry = xiiGeometryResidencyManager::AcquireMeshGeometry(mesh);
    XII_TEST_BOOL(!geometry.IsInvalidated());
    xiiGeometryResidencyManager::ReleaseMeshGeometry(geometry, frame);
  }
  const auto* instances = module->GetGpuInstanceData();
  if (!XII_TEST_INT(instances->GetInstanceCount(), 1))
    return;
  const auto identity = instances->GetInstances()[0].m_uiObjectId;
  XII_TEST_FLOAT(instances->GetInstances()[0].m_GlobalTransform.GetTranslationVector().x, 3, 0);
  xiiGpuInstanceSource source;
  XII_TEST_RESULT(instances->Resolve(identity, source));
  XII_TEST_BOOL(source.m_hOwnerObject == childHandle);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "World hierarchy propagates transforms; stopped motion settles once")
  {
    XII_TEST_RESULT(extract());
    XII_TEST_BOOL(instances->GetUploadRanges().IsEmpty());
    parent->SetLocalPosition(xiiVec3(4, 0, 0));
    world.Update();
    XII_TEST_RESULT(extract());
    XII_TEST_FLOAT(instances->GetInstances()[0].m_GlobalTransform.GetTranslationVector().x, 6, 0);
    XII_TEST_FLOAT(instances->GetInstances()[0].m_PreviousGlobalTransform.GetTranslationVector().x, 3, 0);
    XII_TEST_INT(instances->GetUploadRanges().GetCount(), 1);
    XII_TEST_RESULT(extract());
    XII_TEST_FLOAT(instances->GetInstances()[0].m_PreviousGlobalTransform.GetTranslationVector().x, 6, 0);
    XII_TEST_INT(instances->GetUploadRanges().GetCount(), 1);
    XII_TEST_RESULT(extract());
    XII_TEST_BOOL(instances->GetUploadRanges().IsEmpty());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Foreign worlds, deactivation and recycled component storage cannot resolve stale GPU IDs")
  {
    xiiWorldDescription otherDescription("Independent world");
    xiiWorld            other(otherDescription);
    XII_LOCK(other.GetWriteMarker());
    auto* otherModule = other.GetOrCreateModule<xiiRenderWorldModule>();
    XII_TEST_RESULT(otherModule->UpdateGpuInstances(extracted, frame));
    XII_TEST_INT(otherModule->GetGpuInstanceData()->GetInstanceCount(), 0);
    XII_TEST_BOOL(otherModule->GetGpuInstanceData()->Resolve(identity, source).Failed());
    component->SetActiveFlag(false);
    XII_TEST_BOOL(instances->Resolve(identity, source).Failed());
    XII_TEST_RESULT(extract());
    XII_TEST_INT(instances->GetInstanceCount(), 0);
    component->SetActiveFlag(true);
    world.Update();
    XII_TEST_RESULT(extract());
    XII_TEST_INT(instances->GetInstanceCount(), 1);
    const auto replacement = instances->GetInstances()[0].m_uiObjectId;
    XII_TEST_BOOL(replacement != identity);
    world.DeleteObjectNow(childHandle);
    component = nullptr;
    XII_TEST_BOOL(instances->Resolve(replacement, source).Failed());
    XII_TEST_RESULT(extract());
    XII_TEST_INT(instances->GetInstanceCount(), 0);
  }
}
