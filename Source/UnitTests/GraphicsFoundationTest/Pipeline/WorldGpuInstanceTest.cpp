/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

namespace
{
  class WorldReloadLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> m_Closed;
    void                                                  CloseDataStream(const xiiResource* resource, const xiiResourceLoadData& data) override
    {
      xiiResourceLoaderFromMemory::CloseDataStream(resource, data);
      m_Closed->m_Content.RaiseSignal();
    }
  };
} // namespace

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
  const auto mesh = xiiResourceManager::LoadResource<xiiMeshResource>("WorldGpuInstanceTest.Mesh");
  {
    xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
    descriptor.Save(stream);
    xiiResourceManager::UpdateResourceWithCustomLoader(mesh, std::move(loader));
    xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
      return;
  }
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ray tracing builds use independent mutable frame resources")
  {
    if (!xiiRayTracingResources::IsHardwareRayTracingSupported())
      xiiTestFramework::Output(xiiTestOutput::Details, "Hardware ray tracing unavailable: TLAS integration check skipped.");
    else
    {
      xiiRayTracingFrameResources resources[2];
      xiiRenderGraphContext       context;
      XII_TEST_RESULT(context.Initialize("Independent extracted TLAS"));
      xiiRenderGraphFrame gpuFrame;
      XII_TEST_RESULT(gpuFrame.Begin());
      auto& graph = context.GetGraph();
      auto& cache = context.GetResourceCache();
      cache.BeginFrame(gpuFrame.GetFrameIndex(), gpuFrame.GetCompletedFrame());
      graph.BeginSetup(gpuFrame.GetFrameIndex());
      const auto first  = xiiRayTracingResources::AddBuildPass(graph, gpuFrame.GetFrameIndex(), resources[0], *instances);
      const auto second = xiiRayTracingResources::AddBuildPass(graph, gpuFrame.GetFrameIndex(), resources[1], *instances);
      XII_TEST_INT(first.m_uiInstanceCount, 1);
      XII_TEST_INT(second.m_uiInstanceCount, 1);
      XII_TEST_BOOL(first.m_pTopLevelAS != nullptr && second.m_pTopLevelAS != nullptr && first.m_pTopLevelAS != second.m_pTopLevelAS);
      XII_TEST_BOOL(first.m_hMaterialData != second.m_hMaterialData);
      graph.EndSetup();
      if (XII_TEST_RESULT(graph.Compile()))
        XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache));
      cache.EndFrame();
      gpuFrame.End();
      environment.GetDevice()->WaitIdle();
      xiiTestFramework::Output(xiiTestOutput::Details, "Built two independent TLAS instances from one authoritative world extraction.");
    }
  }

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
      struct Export
      {
        XII_DECLARE_POD_TYPE();
      };
      graph.AddPass<Export>("World inputs", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
        builder.ExportBuffer(builder.ReadBuffer(uploaded.m_Instances, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(builder.ReadBuffer(uploaded.m_Skinning, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {});
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &graphs[camera].GetBlackboard(), &cache)))
        return nullptr;
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
      const bool  warm  = i < xiiGeometryResidencyManager::GetConfiguration().m_uiFramesInFlight;
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
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Static component packets are shared by cameras and refreshed on mesh hot reload")
  {
    xiiWorldDescription staticDescription("Shared static mesh snapshots");
    xiiWorld            staticWorld(staticDescription);
    XII_LOCK(staticWorld.GetWriteMarker());
    staticWorld.SetWorldSimulationEnabled(false);
    auto*          renderer = staticWorld.GetOrCreateModule<xiiRenderWorldModule>();
    xiiGameObject* object   = nullptr;
    staticWorld.CreateObject(xiiGameObjectDescription(), object);
    xiiMeshComponent* staticMesh = nullptr;
    xiiMeshComponent::CreateComponent(object, staticMesh);
    staticMesh->SetMesh(mesh);
    xiiVariantDictionary properties;
    properties.Insert("Categories", static_cast<xiiUInt32>(xiiRenderExtractionCategory::Geometry));
    xiiRenderPipelineResourceDescriptor pipeline;
    pipeline.AddPass("Collect", "xiiExtractionDemandPass", properties);
    const auto resource = xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>("SharedStaticMeshPipeline", std::move(pipeline));
    xiiCamera  camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    xiiView* views[2];
    for (auto& view : views)
    {
      renderer->CreateView("Shared static camera", view);
      view->SetCamera(&camera);
      view->SetViewport({0, 0, 16, 16});
      view->SetRenderPipelineResource(resource);
    }
    staticWorld.Update();
    if (!XII_TEST_INT(views[0]->GetExtractedRenderData()->GetAllRenderData().GetCount(), 1))
      return;
    XII_TEST_BOOL(views[0]->GetExtractedRenderData()->GetAllRenderData()[0] == views[1]->GetExtractedRenderData()->GetAllRenderData()[0]);
    staticWorld.Update();
    XII_TEST_INT(views[0]->GetExtractionStatistics().m_uiDispatchedComponents, 0);
    XII_TEST_INT(views[1]->GetExtractionStatistics().m_uiDispatchedComponents, 0);
    const auto  oldBuffer = static_cast<const xiiMeshRenderData*>(views[0]->GetExtractedRenderData()->GetAllRenderData()[0])->m_hMeshBuffer;
    xiiGeometry replacement;
    replacement.AddBox(xiiVec3(2), true);
    xiiMeshResourceDescriptor reloaded;
    reloaded.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(replacement, xiiGALPrimitiveTopology::TriangleList, false);
    reloaded.AddSection(12, 0, 0);
    reloaded.ComputeBounds();
    const auto                      closed = xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>>(XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>));
    xiiUniquePtr<WorldReloadLoader> loader = XII_DEFAULT_NEW(WorldReloadLoader);
    loader->m_Closed                       = closed;
    xiiMemoryStreamWriter stream(&loader->m_CustomData);
    reloaded.Save(stream);
    xiiResourceManager::UpdateResourceWithCustomLoader(mesh, std::move(loader));
    XII_TEST_BOOL(closed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(10)) == xiiThreadSignal::WaitResult::Signaled);
    {
      xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final);
    }
    staticWorld.Update();
    const auto* packet = static_cast<const xiiMeshRenderData*>(views[0]->GetExtractedRenderData()->GetAllRenderData()[0]);
    XII_TEST_BOOL(packet->m_hMeshBuffer != oldBuffer);
    XII_TEST_BOOL(packet == views[1]->GetExtractedRenderData()->GetAllRenderData()[0]);
    XII_TEST_INT(views[0]->GetExtractionStatistics().m_uiDispatchedComponents + views[1]->GetExtractionStatistics().m_uiDispatchedComponents, 1);
    XII_TEST_BOOL(renderer->GetGpuInstanceData() == nullptr);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component instances retain transforms and reject identities after index compaction")
  {
    xiiGameObject*           object = nullptr;
    xiiGameObjectDescription authored;
    authored.m_bDynamic                     = true;
    authored.m_LocalPosition                = xiiVec3(10, 0, 0);
    const auto                 objectHandle = world.CreateObject(authored, object);
    xiiInstancedMeshComponent* renderable   = nullptr;
    xiiInstancedMeshComponent::CreateComponent(object, renderable);
    renderable->SetMesh(mesh);
    renderable->SetVisibilityMask(0x42);
    renderable->AddInstance(xiiMat4::MakeTranslation(xiiVec3(1, 0, 0)));
    renderable->AddInstance(xiiMat4::MakeTranslation(xiiVec3(2, 0, 0)));
    world.Update();
    XII_TEST_BOOL(world.TryGetObject(objectHandle, object)); // Object storage may compact during Update().
    auto extractInstances = [&] {
      extracted.Clear();
      xiiMsgExtractRenderData message;
      message.m_pExtractedRenderData = &extracted;
      renderable->SendMessage(message);
      extracted.SortAndBatches();
      XII_TEST_RESULT(module->UpdateGpuInstances(extracted, ++frame));
    };
    extractInstances();
    XII_TEST_INT(instances->GetInstanceCount(), 2);
    const auto first  = instances->GetInstances()[0].m_uiObjectId;
    const auto second = instances->GetInstances()[1].m_uiObjectId;
    XII_TEST_FLOAT(instances->GetInstances()[0].m_GlobalTransform.GetTranslationVector().x, 11, 0);
    XII_TEST_FLOAT(instances->GetInstances()[1].m_GlobalTransform.GetTranslationVector().x, 12, 0);
    XII_TEST_INT(instances->GetInstances()[0].m_uiVisibilityMask, 0x42);
    XII_TEST_RESULT(instances->Resolve(second, source));
    XII_TEST_INT(source.m_uiInstanceIndex, 1);
    xiiDynamicArray<xiiUInt32> selection;
    instances->SelectInstances(extracted.GetAllRenderData(), selection);
    XII_TEST_INT(selection.GetCount(), 2);
    renderable->SetInstanceTransform(1, xiiMat4::MakeTranslation(xiiVec3(3, 0, 0)));
    extractInstances();
    XII_TEST_INT(instances->GetInstances()[1].m_uiObjectId, second);
    XII_TEST_FLOAT(instances->GetInstances()[1].m_GlobalTransform.GetTranslationVector().x, 13, 0);
    XII_TEST_FLOAT(instances->GetInstances()[1].m_PreviousGlobalTransform.GetTranslationVector().x, 12, 0);
    {
      xiiDefaultMemoryStreamStorage bytes;
      xiiMemoryStreamWriter         writer(&bytes);
      xiiWorldWriter                worldWriter;
      const xiiGameObject*          root = object;
      worldWriter.WriteObjects(writer, xiiMakeArrayPtr(&root, 1));
      xiiMemoryStreamReader reader(&bytes);
      xiiWorldReader        worldReader;
      XII_TEST_RESULT(worldReader.ReadWorldDescription(reader));
      xiiWorldDescription loadedDescription("Deserialized component instances");
      xiiWorld            loadedWorld(loadedDescription);
      XII_LOCK(loadedWorld.GetWriteMarker());
      loadedWorld.SetWorldSimulationEnabled(false);
      xiiDynamicArray<xiiGameObject*> roots;
      xiiPrefabInstantiationOptions   options;
      options.m_pCreatedRootObjectsOut = &roots;
      worldReader.InstantiatePrefab(loadedWorld, xiiTransform::MakeIdentity(), options);
      if (!XII_TEST_INT(roots.GetCount(), 1))
        return;
      const auto restoredHandle = roots[0]->GetHandle();
      loadedWorld.Update();
      xiiGameObject* restoredObject = nullptr;
      if (!XII_TEST_BOOL(loadedWorld.TryGetObject(restoredHandle, restoredObject)))
        return;
      xiiInstancedMeshComponent* restored = nullptr;
      if (XII_TEST_BOOL(restoredObject->TryGetComponentOfBaseType(restored)))
      {
        XII_TEST_BOOL(restored->GetMesh() == mesh);
        XII_TEST_INT(restored->GetVisibilityMask(), 0x42);
        XII_TEST_INT(restored->GetInstanceCount(), 2);
        XII_TEST_FLOAT(restored->GetInstanceTransforms()[1].GetTranslationVector().x, 3, 0);
      }
    }
    renderable->RemoveInstance(0);
    XII_TEST_BOOL(instances->Resolve(first, source).Failed());
    XII_TEST_BOOL(instances->Resolve(second, source).Failed());
    extractInstances();
    XII_TEST_INT(instances->GetInstanceCount(), 1);
    XII_TEST_FLOAT(instances->GetInstances()[0].m_GlobalTransform.GetTranslationVector().x, 13, 0);
    XII_TEST_BOOL(instances->GetInstances()[0].m_uiObjectId != first && instances->GetInstances()[0].m_uiObjectId != second);
    renderable->ClearInstances();
    extractInstances();
    XII_TEST_INT(instances->GetInstanceCount(), 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Packing benchmark for 1024 unchanged world components")
  {
    xiiWorldDescription benchmarkDescription("GPU extraction packing benchmark");
    xiiWorld            benchmarkWorld(benchmarkDescription);
    XII_LOCK(benchmarkWorld.GetWriteMarker());
    benchmarkWorld.SetWorldSimulationEnabled(false);
    auto*                               renderer = benchmarkWorld.GetOrCreateModule<xiiRenderWorldModule>();
    xiiDynamicArray<xiiComponentHandle> components;
    for (xiiUInt32 i = 0; i < 1024; ++i)
    {
      xiiGameObject*           object = nullptr;
      xiiGameObjectDescription authored;
      authored.m_LocalPosition = xiiVec3(float(i), 0, 0);
      benchmarkWorld.CreateObject(authored, object);
      xiiStaticMeshComponent* renderable = nullptr;
      components.PushBack(xiiStaticMeshComponent::CreateComponent(object, renderable));
      renderable->SetMesh(mesh);
    }
    benchmarkWorld.Update();
    xiiExtractedRenderData  packets;
    xiiMsgExtractRenderData request;
    request.m_pExtractedRenderData = &packets;
    for (const auto& handle : components)
    {
      const xiiComponent* renderable = nullptr;
      if (benchmarkWorld.TryGetComponent(handle, renderable))
        renderable->SendMessage(request);
    }
    packets.SortAndBatches();
    XII_TEST_RESULT(renderer->UpdateGpuInstances(packets, 1));
    XII_TEST_INT(renderer->GetGpuInstanceData()->GetInstanceCount(), 1024);
    const auto start = xiiTime::Now();
    for (xiiUInt32 i = 0; i < 20; ++i)
      XII_TEST_RESULT(renderer->UpdateGpuInstances(packets, 2 + i));
    const auto elapsed = xiiTime::Now() - start;
    XII_TEST_BOOL(renderer->GetGpuInstanceData()->GetUploadRanges().IsEmpty());
    xiiTestFramework::Output(xiiTestOutput::Details, "1024 unchanged world components: 20 packing updates = %.3f ms total, %.3f ms/update (Debug).", elapsed.GetMilliseconds(), elapsed.GetMilliseconds() / 20);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "A live world releases derived buffers across renderer shutdown and restart")
  {
    environment.GetDevice()->WaitIdle();
    extracted.Clear();
    xiiFrameAllocator::Reset();
    xiiStartup::ShutdownHighLevelSystems();
    XII_TEST_BOOL(module->GetGpuInstanceData() == nullptr);
    xiiStartup::StartupHighLevelSystems();
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized());
    xiiGameObject*           object = nullptr;
    xiiGameObjectDescription authored;
    authored.m_bDynamic = true;
    world.CreateObject(authored, object);
    xiiMeshComponent* renderable = nullptr;
    const auto        handle     = xiiMeshComponent::CreateComponent(object, renderable);
    renderable->SetMesh(mesh);
    world.Update();
    if (!XII_TEST_BOOL(world.TryGetComponent(handle, renderable)))
      return;
    xiiMsgExtractRenderData request;
    request.m_pExtractedRenderData = &extracted;
    renderable->SendMessage(request);
    extracted.SortAndBatches();
    XII_TEST_RESULT(module->UpdateGpuInstances(extracted, 1));
    XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), 1);
    XII_TEST_BOOL(module->GetGpuInstanceData()->GetInstances()[0].m_uiObjectId != identity);
    xiiRenderGraphContext context;
    XII_TEST_RESULT(context.Initialize("World upload after renderer restart"));
    xiiRenderGraphFrame gpuFrame;
    XII_TEST_RESULT(gpuFrame.Begin());
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(gpuFrame.GetFrameIndex(), gpuFrame.GetCompletedFrame());
    graph.BeginSetup(gpuFrame.GetFrameIndex());
    const auto uploaded = module->GetGpuInstanceData()->AddUploadPass(graph, gpuFrame.GetFrameIndex());
    XII_TEST_BOOL(uploaded.m_Instances.IsValid() && uploaded.m_Skinning.IsValid());
    graph.EndSetup();
    if (XII_TEST_RESULT(graph.Compile()))
      XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache));
    cache.EndFrame();
    gpuFrame.End();
    environment.GetDevice()->WaitIdle();
  }
}
