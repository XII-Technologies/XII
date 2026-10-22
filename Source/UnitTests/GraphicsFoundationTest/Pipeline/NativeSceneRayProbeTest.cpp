/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsCore/Pipeline/Passes/RayTracing/SceneRayProbePass.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/Passes/RayTracing/RayTracingScenePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <Shaders/Pipeline/Passes/RayTracing/SceneRayProbeConstants.h>
XII_DEFINE_AS_POD_TYPE(xiiSceneRayProbeResult);
XII_CREATE_SIMPLE_TEST_GROUP(NativeSceneRayProbe);
XII_CREATE_SIMPLE_TEST(NativeSceneRayProbe, WorldChangesRepeatedInstancesAndOwnerRetirement)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!xiiRayTracingResources::IsHardwareRayTracingSupported())
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Scene probe requires hardware ray tracing.");
    return;
  }
  const auto                                 asset = xiiLoadPipelineLibraryResource("WorldRayProbe");
  xiiResourceLock<xiiRenderPipelineResource> source(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(source.IsValid() && source->GetAuthoring() && source->GetPipeline()))
    return;
  const auto authored = source->GetAuthoring();
  xiiUuid    probeId;
  for (const auto& pass : authored->m_Descriptor.m_Passes)
    if (pass.m_pType == xiiSceneRayProbePass().GetDynamicRTTI())
      probeId = pass.m_Id;
  if (!XII_TEST_BOOL(probeId.IsValid()))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node       = root.AddSubgraph(i ? "Offset probe" : "Origin probe", authored->m_Descriptor.m_Id);
    auto&      properties = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    properties.m_Path.PushBack(probeId);
    properties.m_Properties.Insert("Origin", xiiVec3(i ? 5.0f : 0.0f, 0, 3));
    properties.m_Properties.Insert("Direction", xiiVec3(0, 0, -3));
    properties.m_Properties.Insert("MaximumDistance", 10.0f);
    root.m_Interface.PushBack({i ? "OffsetResult" : "OriginResult", false, root.m_Passes[node].m_Id, "Result"});
  }
  const auto  resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Repeated world ray probes", std::move(root));
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
  meshDescription.AddSection(12, 0, 0);
  meshDescription.ComputeBounds();
  const auto             mesh = xiiResourceManager::CreateResource<xiiMeshResource>("Ray probe world cube", std::move(meshDescription));
  xiiUniquePtr<xiiWorld> world;
  xiiView*               view   = nullptr;
  xiiRenderWorldModule*  module = nullptr;
  xiiComponentHandle     componentHandle;
  xiiGameObjectHandle    objectHandle;
  const auto             createWorld = [&] {
    xiiWorldDescription description("Ray probe authoritative world");
    world = XII_DEFAULT_NEW(xiiWorld, description);
    XII_LOCK(world->GetWriteMarker());
    world->SetWorldSimulationEnabled(false);
    module = world->GetOrCreateModule<xiiRenderWorldModule>();
    module->CreateView("World ray source", view);
    view->SetRenderEnabled(false);
  };
  const auto createObject = [&] {
    xiiGameObjectDescription description;
    description.m_bDynamic            = true;
    xiiGameObject* object             = nullptr;
    objectHandle                      = world->CreateObject(description, object);
    xiiStaticMeshComponent* component = nullptr;
    componentHandle                   = xiiStaticMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
  };
  createWorld();
  {
    XII_LOCK(world->GetWriteMarker());
    createObject();
  }
  xiiRenderGraphTestContext context;
  xiiRenderPipelineInstance pipeline;
  if (!XII_TEST_RESULT(context.Initialize("Repeated owned ray probes")))
    return;
  xiiUInt32 stableId = 0;
  for (xiiUInt32 phase = 0; phase < 7; ++phase)
  {
    if (phase == 4)
      createWorld();
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    if (!XII_TEST_RESULT(pipeline.Synchronize(resource)))
      return;
    xiiUInt32 identity = 0;
    {
      XII_LOCK(world->GetWriteMarker());
      if (phase == 5)
        createObject();
      if (phase == 2)
      {
        xiiGameObject* object = nullptr;
        if (!XII_TEST_BOOL(world->TryGetObject(objectHandle, object)))
          return;
        object->SetLocalPosition(xiiVec3(5, 0, 0));
      }
      world->Update();
      xiiExtractedRenderData  packets;
      xiiMsgExtractRenderData request;
      request.m_pExtractedRenderData = &packets;
      const xiiComponent* component  = nullptr;
      if (phase != 4 && static_cast<const xiiWorld&>(*world).TryGetComponent(componentHandle, component))
        component->SendMessage(request);
      packets.SortAndBatches();
      if (!XII_TEST_RESULT(module->UpdateGpuInstances(packets, frame.GetFrameIndex())))
        return;
      const auto* instances = module->GetGpuInstanceData();
      if (!XII_TEST_BOOL(instances != nullptr) || !XII_TEST_INT(instances->GetInstanceCount(), phase == 4 ? 0 : 1))
        return;
      if (phase != 4)
      {
        identity = instances->GetInstances()[0].m_uiObjectId;
        xiiGpuInstanceSource owner;
        XII_TEST_RESULT(instances->Resolve(identity, owner));
        XII_TEST_BOOL(owner.m_hOwnerObject == objectHandle && owner.m_hOwnerComponent == componentHandle);
        if (phase == 0)
          stableId = identity;
        if (phase == 1 || phase == 2)
          XII_TEST_INT(identity, stableId);
        if (phase == 5)
        {
          XII_TEST_BOOL(identity != stableId);
          XII_TEST_BOOL(instances->Resolve(stableId, owner).Failed());
        }
      }
    }
    {
      // Graph setup borrows immutable extraction with read access, never mutable component storage.
      XII_LOCK(world->GetReadMarker());
      xiiVariantDictionary parameters;
      if (phase == 1)
        parameters.Insert("RayProbeInstanceMask", 0U);
      if (phase == 2)
        parameters.Insert("RayProbeMaximumDistance", 1.0f);
      if (phase == 6)
        parameters.Insert("RayProbeMinimumDistance", -1.0f);
      const auto built = pipeline.Build(view, graph, context.GetBlackboard(), parameters);
      if (phase == 6)
      {
        XII_TEST_BOOL(built.Failed());
        XII_TEST_BOOL(pipeline.GetLastBuildError().FindSubString("Invalid scene ray") != nullptr);
        const auto* stale = pipeline.GetInterfaceBinding(pipeline.GetPipeline()->FindInterface("OriginResult", false));
        XII_TEST_BOOL(!stale || !stale->m_hBuffer.IsValid());
        graph.EndSetup();
        frame.End();
        continue;
      }
      if (!XII_TEST_RESULT_MSG(built, "%s", pipeline.GetLastBuildError().GetStartPointer()))
        return;
    }
    xiiRenderGraphBufferHandle outputs[2];
    outputs[0] = pipeline.GetInterfaceBinding(pipeline.GetPipeline()->FindInterface("OriginResult", false))->m_hBuffer;
    outputs[1] = pipeline.GetInterfaceBinding(pipeline.GetPipeline()->FindInterface("OffsetResult", false))->m_hBuffer;
    XII_TEST_BOOL(outputs[0] != outputs[1]);
    xiiGALTopLevelAS* structures[2] = {};
    xiiUInt32         sceneCount    = 0;
    for (xiiUInt32 i = 0; i < pipeline.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
      if (const auto* scene = xiiDynamicCast<const xiiRayTracingScenePass*>(pipeline.GetPass(i)))
      {
        XII_TEST_INT(scene->GetLastBuildStatistics().m_uiInstanceCount, phase == 4 ? 0 : 1);
        structures[sceneCount++] = pipeline.GetOutputBinding(i, "Scene")->m_AccelerationStructure.m_pStructure.Borrow();
      }
    XII_TEST_INT(sceneCount, 2);
    if (phase != 4)
      XII_TEST_BOOL(structures[0] && structures[1] && structures[0] != structures[1]);
    if (phase == 3)
    {
      pipeline.Clear();
      world.Clear();
      module = nullptr;
      view   = nullptr;
      // Neither the world, occurrence TLAS owner nor probe pass survives recording.
    }
    graph.EndSetup();
    xiiStringBuilder error;
    const auto       compiled = graph.Compile({.m_bEnableAsyncQueues = false}, &error);
    if (!XII_TEST_RESULT_MSG(compiled, "%s", error.GetData()))
      return;
    const auto executed = graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error);
    if (!XII_TEST_RESULT_MSG(executed, "%s", error.GetData()))
      return;
    XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
    frame.End();
    environment.GetDevice()->WaitIdle();
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      xiiSceneRayProbeResult result;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(outputs[i]).Borrow(), 0, result))
        return;
      const bool hit = phase != 1 && phase != 2 && phase != 4 && i == (phase == 3 ? 1U : 0U);
      XII_TEST_INT(result.Identity.x, hit ? 1 : 0);
      if (hit)
      {
        XII_TEST_INT(result.Identity.y, identity);
        XII_TEST_BOOL(result.Identity.z < 12);
        XII_TEST_FLOAT(result.Intersection.x, 2.5f, 0.0001f);
        XII_TEST_BOOL(result.Intersection.y >= 0 && result.Intersection.z >= 0 && result.Intersection.y + result.Intersection.z <= 1.0001f);
      }
      else
      {
        XII_TEST_INT(result.Identity.y, xiiInvalidIndex);
        XII_TEST_INT(result.Identity.z, xiiInvalidIndex);
        XII_TEST_FLOAT(result.Intersection.x, phase == 2 ? 1.0f : 10.0f, 0.0001f);
      }
    }
    xiiTestFramework::Output(xiiTestOutput::Details, "Ray probe phase %u: %u compiled operations, one submission, %u barriers.", phase, graph.GetStatistics().m_uiCompiledPassCount, graph.GetStatistics().m_uiTotalBarrierCount);
  }
}
