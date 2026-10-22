/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <EditorEngineProcessFrameworkTest/GpuBufferReadback.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

static void RunSceneLodReload(bool skinned)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto mesh    = xiiResourceManager::LoadResource<xiiMeshResource>("NativeSectionReload");
  auto       publish = [&](bool replacement) -> bool {
    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(replacement ? 2.0f : 1.0f), true);
    xiiMeshResourceDescriptor description;
    description.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
    description.AddLOD(0);
    if (replacement)
    {
      description.AddSection(12, 0, 3, 0);
      description.AddSection(12, 0, 4, 1);
      description.AddLOD(0);
      description.AddSection(12, 0, 5, 2);
    }
    else
    {
      description.AddSection(0, 0, 0, 0); // Empty fine LOD, followed by nonempty coarse sections.
      description.AddSection(6, 0, 1, 1);
      description.AddSection(6, 6, 2, 1);
    }
    description.ComputeBounds();
    xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
    description.Save(stream);
    xiiResourceManager::UpdateResourceWithCustomLoader(mesh, std::move(loader));
    xiiResourceLock<xiiMeshResource> resource(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    return XII_TEST_BOOL(resource.GetAcquireResult() == xiiResourceAcquireResult::Final);
  };
  if (!publish(false))
    return;
  xiiWorldDescription worldDescription("Authored section reload");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*          renderer = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiGameObject* object   = nullptr;
  world.CreateObject(xiiGameObjectDescription(), object);
  xiiMeshComponentBase*    component        = nullptr;
  xiiLODMeshComponent*     lodComponent     = nullptr;
  xiiSkinnedMeshComponent* skinnedComponent = nullptr;
  xiiMat4                  palette          = xiiMat4::MakeTranslation(xiiVec3(0, 1, 0));
  if (skinned)
  {
    object->MakeDynamic();
    xiiSkinnedMeshComponent::CreateComponent(object, skinnedComponent);
    XII_TEST_RESULT(skinnedComponent->SetSkinningMatrices(xiiMakeArrayPtr(&palette, 1)));
    component = skinnedComponent;
  }
  else
  {
    xiiLODMeshComponent::CreateComponent(object, lodComponent);
    component = lodComponent;
  }
  component->SetMesh(mesh);
  world.Update();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*   view       = nullptr;
  const auto viewHandle = renderer->CreateView("Reload extraction", view);
  XII_SCOPE_EXIT(renderer->DestroyView(viewHandle));
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 32, 32});
  xiiGpuInstanceData     primary(world), delayed(world);
  xiiExtractedRenderData extracted;
  auto                   extract = [&] {
    extracted.Clear();
    xiiMsgExtractRenderData message;
    message.m_pView                = view;
    message.m_pExtractedRenderData = &extracted;
    XII_TEST_BOOL(component->SendMessage(message));
    extracted.SortAndBatches();
  };
  extract();
  if (!XII_TEST_INT(extracted.GetAllRenderData().GetCount(), 1))
    return;
  XII_TEST_INT(xiiDynamicCast<const xiiMeshRenderData*>(extracted.GetAllRenderData()[0])->m_uiPrimitiveCount, 0);
  XII_TEST_RESULT(primary.UpdateFromExtraction(extracted, 1));
  XII_TEST_RESULT(delayed.UpdateFromExtraction(extracted, 1));
  XII_TEST_INT(primary.GetInstanceCount(), 2);
  XII_TEST_INT(delayed.GetInstanceCount(), 2);
  auto checkPose = [&](const xiiGpuInstanceData& database, float offset) {
    if (!skinned)
      return;
    // All sections/LODs share one current palette. Reload resets history in each
    // context when that context adopts the new geometry, without changing its peers.
    XII_TEST_INT(database.GetSkinningMatrices().GetCount(), 1);
    for (const auto& instance : database.GetInstances())
    {
      XII_TEST_INT(instance.m_uiSkinningCount, 1);
      XII_TEST_INT(instance.m_uiPreviousSkinningCount, 1);
      XII_TEST_INT(instance.m_uiSkinningOffset, instance.m_uiPreviousSkinningOffset);
      XII_TEST_FLOAT(database.GetSkinningMatrices()[instance.m_uiSkinningOffset].GetTranslationVector().y, offset, 0);
    }
  };
  checkPose(primary, 1);
  checkPose(delayed, 1);
  xiiUInt32 retainedId = 0, retiredId = 0;
  for (const auto& instance : primary.GetInstances())
  {
    xiiGpuInstanceSource source;
    XII_TEST_RESULT(primary.Resolve(instance.m_uiObjectId, source));
    XII_TEST_INT(source.m_uiLodIndex, 1);
    XII_TEST_INT(source.m_uiMaterialIndex, source.m_uiSectionIndex + 1);
    XII_TEST_INT(instance.m_uiFirstPrimitive, source.m_uiSectionIndex * 6);
    XII_TEST_INT(instance.m_uiPrimitiveCount, 6);
    (source.m_uiSectionIndex == 0 ? retainedId : retiredId) = instance.m_uiObjectId;
  }
  const auto oldGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(mesh);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::ReleaseMeshGeometry(oldGeometry, 3));
  xiiGeometryResidencyManager::ProcessStreaming(1, 0, xiiMath::MaxValue<xiiUInt64>());
  const auto* oldRecord = xiiGeometryResidencyManager::GetGpuRecord(oldGeometry);
  if (!XII_TEST_BOOL(oldRecord != nullptr))
    return;
  XII_TEST_INT(oldRecord->m_Lods[0].m_uiMeshletCount, 0);
  XII_TEST_INT(oldRecord->m_Lods[1].m_uiMeshletCount, 2);

  // Upload and read the actual geometry table: an authored empty LOD must not borrow
  // the following LOD's meshlets through the source API's zero-means-to-end convention.
  xiiRenderGraphTestContext uploadContext;
  if (!XII_TEST_RESULT(uploadContext.Initialize("Empty LOD upload")))
    return;
  auto& graph = uploadContext.GetGraph();
  auto& cache = uploadContext.GetResourceCache();
  cache.BeginFrame(1, 0);
  graph.BeginSetup(1);
  const auto                   uploaded = xiiGeometryResidencyManager::AddUploadPass(graph, 1);
  xiiGpuFrameCompletionTracker completion;
  completion.Initialize(environment.GetDevice());
  struct Export
  {
    XII_DECLARE_POD_TYPE();
  };
  graph.AddPass<Export>("Empty LOD metadata export", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) { builder.ExportBuffer(uploaded.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {}, true);
  graph.EndSetup();
  if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), view, &uploadContext.GetBlackboard(), &cache)))
    return;
  completion.CaptureSubmittedFrame(1);
  completion.WaitForFrame(1); // The upload can run on a transfer queue; wait for all submitted queues.
  xiiGpuGeometryRecord gpuRecord;
  const auto           index = uploaded.m_uiGeometryBaseIndex + oldGeometry.GetInternalID().m_InstanceIndex;
  if (xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(uploaded.m_hGeometryMetadata).Borrow(), index * sizeof(gpuRecord), gpuRecord))
  {
    XII_TEST_INT(gpuRecord.m_Lods[0].m_uiMeshletCount, 0);
    XII_TEST_INT(gpuRecord.m_Lods[1].m_uiMeshletCount, 2);
  }
  if (!publish(true))
    return;
  if (lodComponent != nullptr)
    lodComponent->SetLodBias(2);
  if (skinnedComponent != nullptr)
  {
    palette = xiiMat4::MakeTranslation(xiiVec3(0, 3, 0));
    XII_TEST_RESULT(skinnedComponent->SetSkinningMatrices(xiiMakeArrayPtr(&palette, 1)));
  }
  extract();
  XII_TEST_RESULT(primary.UpdateFromExtraction(extracted, 2));
  XII_TEST_INT(primary.GetInstanceCount(), 3);
  const auto newGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(mesh);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::ReleaseMeshGeometry(newGeometry, 3));
  XII_TEST_BOOL(newGeometry != oldGeometry);
  xiiGpuInstanceSource source;
  XII_TEST_BOOL(primary.Resolve(retiredId, source).Failed());
  XII_TEST_RESULT(primary.Resolve(retainedId, source));
  XII_TEST_INT(source.m_uiLodIndex, 1);
  XII_TEST_INT(source.m_uiMaterialIndex, 4);
  for (const auto& instance : primary.GetInstances())
  {
    XII_TEST_INT(instance.m_uiGeometryIndex, newGeometry.GetInternalID().m_InstanceIndex);
    XII_TEST_FLOAT(instance.m_fLodBias, skinned ? 0 : 2, 0);
  }
  for (const auto& instance : delayed.GetInstances())
  {
    XII_TEST_INT(instance.m_uiGeometryIndex, oldGeometry.GetInternalID().m_InstanceIndex);
    XII_TEST_FLOAT(instance.m_fLodBias, 0, 0);
    XII_TEST_RESULT(delayed.Resolve(instance.m_uiObjectId, source));
    XII_TEST_INT(source.m_uiMaterialIndex, source.m_uiSectionIndex + 1);
  }
  checkPose(primary, 3);
  checkPose(delayed, 1);
  XII_TEST_RESULT(delayed.UpdateFromExtraction(extracted, 3));
  XII_TEST_INT(delayed.GetInstanceCount(), 3);
  for (const auto& instance : delayed.GetInstances())
    XII_TEST_INT(instance.m_uiGeometryIndex, newGeometry.GetInternalID().m_InstanceIndex);
  checkPose(delayed, 3);
}

XII_CREATE_SIMPLE_TEST(NativeSceneLod, Reload)
{
  RunSceneLodReload(false);
}

XII_CREATE_SIMPLE_TEST(SkinningRaster, ReloadAndContextIsolation)
{
  RunSceneLodReload(true);
}
