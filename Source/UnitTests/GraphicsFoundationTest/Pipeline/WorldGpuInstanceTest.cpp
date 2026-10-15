/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
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
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Two cameras share sparse uploads; warmed unchanged frames upload zero bytes")
  {
    xiiRenderGraphContext graphs[2];
    XII_TEST_RESULT(graphs[0].Initialize("World upload camera A"));
    XII_TEST_RESULT(graphs[1].Initialize("World upload camera B"));
    auto render = [&](xiiUInt32 camera, const xiiRenderGraphFrame& gpuFrame) -> xiiSharedPtr<xiiGALBuffer> {
      auto& graph = graphs[camera].GetGraph();
      auto& cache = graphs[camera].GetResourceCache();
      cache.BeginFrame(gpuFrame.GetFrameIndex(), gpuFrame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(gpuFrame.GetFrameIndex());
      const auto uploaded = instances->AddUploadPass(graph, gpuFrame.GetFrameIndex());
      const auto repeated = instances->AddUploadPass(graph, gpuFrame.GetFrameIndex());
      XII_TEST_BOOL(uploaded.m_Instances == repeated.m_Instances && uploaded.m_Skinning == repeated.m_Skinning);
      struct Export { XII_DECLARE_POD_TYPE(); };
      graph.AddPass<Export>("World inputs", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
        builder.ExportBuffer(builder.ReadBuffer(uploaded.m_Instances, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(builder.ReadBuffer(uploaded.m_Skinning, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
      }, [](const Export&, xiiRenderGraphPassContext&) {});
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &graphs[camera].GetBlackboard(), &cache))) return nullptr;
      return graph.GetExportedBuffer(uploaded.m_Instances);
    };
    for (xiiUInt32 i = 0; i < xiiGeometryResidencyManager::GetConfiguration().m_uiFramesInFlight + 1; ++i)
    {
      xiiRenderGraphFrame gpuFrame;
      XII_TEST_RESULT(gpuFrame.Begin());
      frame = gpuFrame.GetFrameIndex();
      XII_TEST_RESULT(module->UpdateGpuInstances(extracted, frame));
      const auto first = render(0, gpuFrame), second = render(1, gpuFrame);
      XII_TEST_BOOL(first != nullptr && first == second);
      const auto& stats = instances->GetUploadStatistics();
      const bool warm = i < xiiGeometryResidencyManager::GetConfiguration().m_uiFramesInFlight;
      XII_TEST_INT(stats.m_uiUploadPasses, warm ? 1 : 0);
      XII_TEST_INT(stats.m_uiBytesUploaded, warm ? sizeof(xiiGpuSceneInstance) + sizeof(xiiMat4) : 0);
      gpuFrame.End();
      environment.GetDevice()->WaitIdle();
    }
    parent->SetLocalPosition(xiiVec3(7, 0, 0));
    world.Update();
    XII_TEST_RESULT(extract());
    xiiRenderGraphFrame gpuFrame;
    XII_TEST_RESULT(gpuFrame.Begin());
    const auto first = render(0, gpuFrame), second = render(1, gpuFrame);
    XII_TEST_BOOL(first == second);
    XII_TEST_INT(instances->GetUploadStatistics().m_uiUploadPasses, 1);
    XII_TEST_INT(instances->GetUploadStatistics().m_uiUploadRanges, 1);
    XII_TEST_INT(instances->GetUploadStatistics().m_uiBytesUploaded, sizeof(xiiGpuSceneInstance));
    gpuFrame.End();
    environment.GetDevice()->WaitIdle();
    xiiTestFramework::Output(xiiTestOutput::Details, "Two-camera upload: one changed instance = %u bytes once; warmed static frame = 0 bytes.", xiiUInt32(sizeof(xiiGpuSceneInstance)));
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
