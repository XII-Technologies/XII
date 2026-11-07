/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsCore/Pipeline/Passes/RayTracing/SceneRayProbePass.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Lighting/EmissiveLightManager.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Material/MaterialResource.h>
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
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingMaterialData.h>
#include <Shaders/Pipeline/Passes/RayTracing/SceneRayProbeConstants.h>
XII_DEFINE_AS_POD_TYPE(xiiSceneRayProbeResult);
XII_DEFINE_AS_POD_TYPE(xiiRayTracingGeometryData);
XII_CREATE_SIMPLE_TEST_GROUP(NativeSceneRayProbe);
static void RunWorldRaySceneTest(bool emissive)
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
  const auto                                 asset = xiiCreateWorldRayProbeTestResource();
  xiiResourceLock<xiiRenderPipelineResource> source(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(source.IsValid() && source->GetAuthoring() && source->GetPipeline()))
    return;
  const auto authored = source->GetAuthoring();
  xiiUuid    probeId, sceneId;
  for (const auto& pass : authored->m_Descriptor.m_Passes)
  {
    if (pass.m_pType == xiiGetStaticRTTI<xiiSceneRayProbePass>())
      probeId = pass.m_Id;
    if (pass.m_pType == xiiGetStaticRTTI<xiiRayTracingScenePass>())
      sceneId = pass.m_Id;
  }
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
    if (emissive)
    {
      auto& build = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      build.m_Path.PushBack(sceneId);
      build.m_Properties.Insert("BuildEmissiveLights", true);
    }
    root.m_Interface.PushBack({i ? "OffsetResult" : "OriginResult", false, root.m_Passes[node].m_Id, "Result"});
  }
  const auto      resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(emissive ? "Emissive world ray probes" : "Repeated world ray probes", std::move(root));
  xiiGeometry     box;
  const xiiUInt32 triangleCount = emissive ? 288 : 12;
  for (xiiUInt32 index = 0; index < triangleCount / 12 + (emissive ? 1 : 0); ++index)
  {
    xiiGeometry::GeoOptions options;
    options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0, (float(index) - (emissive ? 1 : 0)) * 4, 0));
    box.AddBox(xiiVec3(1), true, options);
  }
  xiiMeshResourceDescriptor meshDescription;
  meshDescription.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, false);
  meshDescription.AddSection(triangleCount, emissive ? 12 : 0, 0);
  meshDescription.ComputeBounds();
  const auto                mesh = xiiResourceManager::CreateResource<xiiMeshResource>(emissive ? "Ray emissive boxes" : "Ray probe world cube", std::move(meshDescription));
  xiiMaterialResourceHandle bright, black;
  if (emissive)
  {
    xiiMaterialResourceDescriptor material;
    material.m_EmissiveColor = xiiColor(4, 2, 1);
    bright                   = xiiResourceManager::CreateResource<xiiMaterialResource>("Ray emissive cube", std::move(material));
    black                    = xiiResourceManager::CreateResource<xiiMaterialResource>("Ray nonemissive cube", xiiMaterialResourceDescriptor{});
  }
  const auto settings = xiiEmissiveLightManager::GetConfiguration();
  XII_SCOPE_EXIT(xiiEmissiveLightManager::Configure(settings).AssertSuccess());
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
    if (emissive)
      component->SetMaterialOverride(0, bright);
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
    if (emissive)
    {
      auto configuration = settings;
      if (phase == 2)
        configuration.m_uiMaxTriangles = 5;
      XII_TEST_RESULT(xiiEmissiveLightManager::Configure(configuration));
    }
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
        if (emissive)
          object->SetLocalScaling(xiiVec3(2, 3, 1));
      }
      if (emissive && (phase == 1 || phase == 2))
      {
        xiiStaticMeshComponent* component = nullptr;
        if (!XII_TEST_BOOL(world->TryGetComponent(componentHandle, component)))
          return;
        component->SetMaterialOverride(0, phase == 1 ? black : bright);
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
    xiiGALTopLevelAS*          structures[2] = {};
    xiiRenderGraphBufferHandle geometryRecords;
    xiiRenderGraphBufferHandle emissionTables[2][5];
    const xiiStringView        emissionPins[] = {"EmissiveTriangles", "EmissiveHeader", "TriangleAliases", "BlockAliases", "AliasHeader"};
    xiiUInt32                  sceneCount     = 0;
    for (xiiUInt32 i = 0; i < pipeline.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
      if (const auto* scene = xiiDynamicCast<const xiiRayTracingScenePass*>(pipeline.GetPass(i)))
      {
        XII_TEST_INT(scene->GetLastBuildStatistics().m_uiInstanceCount, phase == 4 ? 0 : 1);
        if (emissive && phase != 4)
          for (xiiUInt32 pin = 0; pin < 5; ++pin)
            emissionTables[sceneCount][pin] = pipeline.GetOutputBinding(i, emissionPins[pin])->m_hBuffer;
        structures[sceneCount++] = pipeline.GetOutputBinding(i, "Scene")->m_AccelerationStructure.m_pStructure.Borrow();
        if (phase != 4)
          geometryRecords = pipeline.GetOutputBinding(i, "Geometry")->m_hBuffer;
      }
    XII_TEST_INT(sceneCount, 2);
    if (phase != 4)
    {
      XII_TEST_BOOL(structures[0] && structures[1] && structures[0] != structures[1]);
      if (emissive)
      {
        for (xiiUInt32 pin = 0; pin < 5; ++pin)
        {
          XII_TEST_BOOL(emissionTables[0][pin].IsValid() && emissionTables[0][pin] != emissionTables[1][pin]);
          for (xiiUInt32 occurrence = 0; occurrence < 2; ++occurrence)
          {
            const auto setup = [&](auto& data, xiiRenderGraphBuilder& builder) {
              data = builder.ReadBuffer(emissionTables[occurrence][pin], xiiGALResourceStateFlags::ShaderResource);
              builder.ExportBuffer(data, xiiGALResourceStateFlags::ShaderResource);
            };
            graph.AddPass<xiiRenderGraphBufferHandle>("Export ray emissive table", xiiGALCommandQueueFlags::Graphics, setup, [](const auto&, xiiRenderGraphPassContext&) {}, true);
          }
        }
      }
      graph.AddPass<xiiRenderGraphBufferHandle>("Export indexed ray geometry", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
        data = builder.ReadBuffer(geometryRecords, xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(data, xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    }
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
    if (phase != 4)
    {
      xiiRayTracingGeometryData geometry;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(geometryRecords).Borrow(), 0, geometry))
        return;
      // Closest-hit shading must use the same indexed addressing as BLAS traversal.
      XII_TEST_INT(geometry.BufferIndices.z, sizeof(xiiUInt16));
      XII_TEST_BOOL(geometry.BufferIndices.y != xiiInvalidIndex);
      XII_TEST_INT(geometry.PrimitiveRange.x, triangleCount);
      XII_TEST_INT(geometry.VertexAttributes.w, emissive ? 12 : 0);
      if (emissive)
      {
        XII_TEST_INT(geometry.PrimitiveRange.y, phase == 1 ? xiiInvalidIndex : 0);
        for (xiiUInt32 occurrence = 0; occurrence < 2; ++occurrence)
        {
          const auto read = [&](xiiUInt32 pin, xiiUInt32 offset, auto& value) {
            return xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(emissionTables[occurrence][pin]).Borrow(), offset, value);
          };
          xiiGpuEmissiveLightHeader header;
          xiiGpuEmissiveAliasHeader alias;
          if (!read(1, 0, header) || !read(4, 0, alias))
            return;
          XII_TEST_INT(header.FrameIndex, xiiUInt32(graph.GetFrameIndex()));
          XII_TEST_INT(alias.FrameIndex, header.FrameIndex);
          XII_TEST_INT(header.CandidateCount, phase == 1 ? 0 : triangleCount);
          XII_TEST_INT(header.TriangleCount, phase == 1 ? 0 : phase == 2 ? 5 :
                                                                           triangleCount);
          XII_TEST_INT(header.OverflowCount, phase == 2 ? triangleCount - 5 : 0);
          XII_TEST_INT(alias.BlockCount, phase == 1 ? 0 : phase == 2 ? 1 :
                                                                       2);
          float total = 0, area = 0;
          float weights[288] = {}, probabilities[288] = {}, blockProbabilities[2] = {}, storedBlockProbabilities[2] = {}, storedTriangleProbabilities[288] = {};
          for (xiiUInt32 block = 0; block < alias.BlockCount; ++block)
          {
            xiiGpuEmissiveAliasEntry entry;
            if (!read(3, block * sizeof(entry), entry))
              return;
            if (!XII_TEST_BOOL(entry.AliasIndex < alias.BlockCount))
              return;
            storedBlockProbabilities[block] = entry.SelectionProbability;
            blockProbabilities[block] += entry.Probability / alias.BlockCount;
            blockProbabilities[entry.AliasIndex] += (1 - entry.Probability) / alias.BlockCount;
          }
          for (xiiUInt32 block = 0; block < alias.BlockCount; ++block)
            XII_TEST_FLOAT(storedBlockProbabilities[block], blockProbabilities[block], 1e-6f);
          struct TriangleRecords
          {
            XII_DECLARE_POD_TYPE();
            xiiGpuEmissiveTriangle m_Records[288];
          } triangles;
          struct AliasRecords
          {
            XII_DECLARE_POD_TYPE();
            xiiGpuEmissiveAliasEntry m_Records[288];
          } aliases;
          if (header.TriangleCount == 288 && (!read(0, 0, triangles) || !read(2, 0, aliases)))
            return;
          for (xiiUInt32 triangleIndex = 0; triangleIndex < header.TriangleCount; ++triangleIndex)
          {
            xiiGpuEmissiveTriangle   triangle;
            xiiGpuEmissiveAliasEntry entry;
            if (header.TriangleCount == 288)
            {
              triangle = triangles.m_Records[triangleIndex];
              entry    = aliases.m_Records[triangleIndex];
            }
            else if (!read(0, triangleIndex * sizeof(triangle), triangle) || !read(2, triangleIndex * sizeof(entry), entry))
              return;
            XII_TEST_INT(triangle.Metadata.x, identity);
            XII_TEST_INT(triangle.Metadata.y, 0);
            XII_TEST_INT(triangle.Metadata.z, triangleIndex);
            const auto  p0 = triangle.Position0AndArea.GetAsVec3(), p1 = triangle.Position1AndPowerWeight.GetAsVec3(), p2 = triangle.Position2AndInvArea.GetAsVec3();
            const float expectedArea = (p1 - p0).CrossRH(p2 - p0).GetLength() * 0.5f;
            XII_TEST_FLOAT(triangle.Position0AndArea.w, expectedArea, 1e-5f);
            XII_TEST_FLOAT(triangle.Position2AndInvArea.w * expectedArea, 1, 1e-5f);
            XII_TEST_FLOAT(triangle.NormalAndTwoSided.GetAsVec3().GetLength(), 1, 1e-5f);
            for (auto position : {p0, p1, p2})
            {
              XII_TEST_FLOAT(xiiMath::Abs(position.z), 0.5f, 1e-5f);
              XII_TEST_BOOL(xiiMath::Abs(position.x - (phase == 2 || phase == 3 ? 5.0f : 0.0f)) <= (phase == 2 || phase == 3 ? 1.00001f : 0.50001f));
              XII_TEST_BOOL(xiiMath::Abs(position.y - float(triangleIndex / 12) * 4 * (phase == 2 || phase == 3 ? 3 : 1)) <= (phase == 2 || phase == 3 ? 1.50001f : 0.50001f));
            }
            const float power = expectedArea * (4 * 0.2126f + 2 * 0.7152f + 0.0722f);
            XII_TEST_FLOAT(triangle.Position1AndPowerWeight.w, power, 1e-4f);
            storedTriangleProbabilities[triangleIndex] = entry.SelectionProbability;
            weights[triangleIndex] = power;
            total += power;
            area += expectedArea;
            XII_TEST_BOOL(xiiMath::IsFinite(entry.Probability) && entry.Probability >= 0 && entry.Probability <= 1);
            if (!XII_TEST_BOOL(entry.AliasIndex < header.TriangleCount))
              return;
            const xiiUInt32 block   = triangleIndex / 256;
            const xiiUInt32 columns = xiiMath::Min(256U, header.TriangleCount - block * 256);
            XII_TEST_INT(entry.AliasIndex / 256, block);
            probabilities[triangleIndex] += entry.Probability / columns * blockProbabilities[block];
            probabilities[entry.AliasIndex] += (1 - entry.Probability) / columns * blockProbabilities[block];
          }
          XII_TEST_FLOAT(alias.TotalPower, total, xiiMath::Max(1e-4f, total * 1e-5f));
          if (header.TriangleCount)
          {
            for (xiiUInt32 index = 0; index < header.TriangleCount; ++index)
            {
              XII_TEST_FLOAT(probabilities[index], weights[index] / total, 1e-6f);
              XII_TEST_FLOAT(storedTriangleProbabilities[index] * storedBlockProbabilities[index / 256], probabilities[index], 1e-6f);
            }
            if (phase != 2)
              XII_TEST_FLOAT(area, (phase == 3 ? 22.0f : 6.0f) * (triangleCount / 12), 1e-5f);
          }
        }
      }
    }
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
        XII_TEST_BOOL(result.Identity.z < triangleCount);
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

XII_CREATE_SIMPLE_TEST(NativeSceneRayProbe, WorldChangesRepeatedInstancesAndOwnerRetirement)
{
  RunWorldRaySceneTest(false);
}

XII_CREATE_SIMPLE_TEST(NativeSceneRayProbe, EmissiveProposalsTransformOverflowAndOwnerRetirement)
{
  RunWorldRaySceneTest(true);
}
