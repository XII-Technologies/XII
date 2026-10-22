/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>

XII_CREATE_SIMPLE_TEST_GROUP(RenderingBenchmarks);
XII_CREATE_SIMPLE_TEST(RenderingBenchmarks, WorldExtraction)
{
  if (!xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-benchmark-render-extraction", false))
  {
    xiiTestFramework::Output(xiiTestOutput::Details, "Opt in with -benchmark-render-extraction for controlled timing runs.");
    return;
  }
  constexpr xiiUInt32      count = 4096, samples = 11, iterations = 32;
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiTestFramework::Output(xiiTestOutput::Details, "Measured adapter: %s.", environment.GetDevice()->GetGraphicsDeviceAdapterProperties().m_sAdapterName.GetData());
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  descriptor.AddSection(12, 0, 0);
  descriptor.ComputeBounds();
  const auto          mesh = xiiResourceManager::CreateResource<xiiMeshResource>("WorldExtractionBenchmark.Mesh", std::move(descriptor));
  xiiWorldDescription worldDescription("Measured authoritative world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto                                 module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiDynamicArray<xiiComponentHandle>  components;
  xiiDynamicArray<xiiGameObjectHandle> owners;
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    xiiGameObjectDescription objectDescription;
    objectDescription.m_bDynamic      = true;
    objectDescription.m_LocalPosition = xiiVec3(float(i), 0, 0);
    xiiGameObject* object             = nullptr;
    owners.PushBack(world.CreateObject(objectDescription, object));
    xiiStaticMeshComponent* component = nullptr;
    components.PushBack(xiiStaticMeshComponent::CreateComponent(object, component));
    component->SetMesh(mesh);
  }
  world.Update();
  xiiExtractedRenderData  extracted;
  xiiMsgExtractRenderData request;
  request.m_pExtractedRenderData = &extracted;
  for (const auto handle : components)
  {
    const xiiComponent* component = nullptr;
    if (static_cast<const xiiWorld&>(world).TryGetComponent(handle, component))
      component->SendMessage(request);
  }
  extracted.SortAndBatches();
  xiiDynamicArray<xiiRenderData*> packets;
  packets.PushBackRange(extracted.GetAllRenderData());
  xiiHashTable<xiiComponentHandle, xiiUInt32> packetIndices;
  for (xiiUInt32 i = 0; i < packets.GetCount(); ++i)
    packetIndices.Insert(packets[i]->m_hOwnerComponent, i);
  xiiUInt64 epoch = 0;
  for (xiiUInt32 i = 0; i < 16; ++i)
    XII_TEST_RESULT(module->UpdateGpuInstances(packets, ++epoch));
  if (!XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), count))
    return;
  auto report = [](const char* label, xiiDynamicArray<double> values) {
    values.Sort();
    xiiTestFramework::Output(xiiTestOutput::Details, "%s: median %.6f ms, p95 %.6f ms, samples %u.", label, values[values.GetCount() / 2], values[xiiMath::Min(values.GetCount() - 1, xiiUInt32(values.GetCount() * 0.95))], values.GetCount());
  };
  xiiDynamicArray<double> times;
  for (xiiUInt32 sample = 0; sample < samples; ++sample)
  {
    bool       success = true;
    const auto start   = xiiTime::Now();
    for (xiiUInt32 i = 0; i < iterations; ++i)
      success &= module->UpdateGpuInstances(packets, ++epoch).Succeeded();
    const auto elapsed = xiiTime::Now() - start;
    XII_TEST_BOOL(success);
    times.PushBack(elapsed.GetMilliseconds() / iterations);
  }
  XII_TEST_BOOL(module->GetGpuInstanceData()->GetUploadRanges().IsEmpty());
  report("4096 unchanged candidates, CPU packing", times);
  extracted.Clear(); // Borrowed packet pointers remain valid until the fixture's frame allocator reset.

  xiiRenderGraphTestContext       contexts[2];
  xiiRenderGraphTimestampProfiler profiler(environment.GetDeviceShared());
  for (auto& context : contexts)
    XII_TEST_RESULT(context.Initialize("Measured two-camera upload"));
  auto execute = [&](xiiUInt32 camera, const xiiRenderGraphFrame& frame) {
    auto& context = contexts[camera];
    auto& graph   = context.GetGraph();
    auto& cache   = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    const auto uploaded = module->GetGpuInstanceData()->AddUploadPass(graph, frame.GetFrameIndex());
    struct Consumer
    {
      XII_DECLARE_POD_TYPE();
    };
    graph.AddPass<Consumer>("World upload consumer", xiiGALCommandQueueFlags::Compute, [&](Consumer&, xiiRenderGraphBuilder& builder) {
      builder.ExportBuffer(builder.ReadBuffer(uploaded.m_Instances, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource);
      builder.ExportBuffer(builder.ReadBuffer(uploaded.m_Skinning, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const Consumer&, xiiRenderGraphPassContext&) {});
    graph.EndSetup();
    xiiRenderGraphCompileSettings settings;
    settings.m_bEnableGPUProfiling = true;
    if (!XII_TEST_RESULT(graph.Compile(settings)))
      return;
    XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, camera == 0 ? &profiler : nullptr));
  };
  const auto framesInFlight = xiiGeometryResidencyManager::GetConfiguration().m_uiFramesInFlight;
  for (xiiUInt32 i = 0; i < framesInFlight + 1; ++i)
  {
    xiiRenderGraphFrame frame;
    XII_TEST_RESULT(frame.Begin());
    XII_TEST_RESULT(module->UpdateGpuInstances(packets, frame.GetFrameIndex()));
    execute(0, frame);
    execute(1, frame);
    if (i == framesInFlight)
    {
      XII_TEST_INT(module->GetGpuInstanceData()->GetUploadStatistics().m_uiBytesUploaded, 0);
      XII_TEST_INT(module->GetGpuInstanceData()->GetUploadStatistics().m_uiUploadPasses, 0);
      XII_TEST_INT(module->GetGpuInstanceData()->GetUploadStatistics().m_uiUploadRanges, 0);
    }
    frame.End();
    environment.GetDevice()->WaitIdle();
  }
  xiiTestFramework::Output(xiiTestOutput::Details, "4096 warmed unchanged candidates, two cameras: 0 bytes, 0 upload passes.");
  for (xiiUInt32 mode = 0; mode < 2; ++mode)
  {
    times.Clear();
    xiiDynamicArray<double> gpuTimes, worldTimes;
    const auto              changed = mode == 0 ? 1U : count;
    for (xiiUInt32 iteration = 0; iteration < 40; ++iteration)
    {
      xiiRenderGraphFrame frame;
      XII_TEST_RESULT(frame.Begin());
      const auto worldStart = xiiTime::Now();
      for (xiiUInt32 i = 0; i < changed; ++i)
      {
        xiiGameObject* object = nullptr;
        if (world.TryGetObject(owners[i], object))
          object->SetLocalPosition(xiiVec3(float(i), float(mode * 100 + iteration + 1), 0));
      }
      world.Update();
      bool validPackets = true;
      for (xiiUInt32 i = 0; i < changed; ++i)
      {
        extracted.Clear();
        const xiiComponent* component = nullptr;
        if (static_cast<const xiiWorld&>(world).TryGetComponent(components[i], component))
          component->SendMessage(request);
        extracted.SortAndBatches();
        validPackets &= extracted.GetAllRenderData().GetCount() == 1;
        if (extracted.GetAllRenderData().GetCount() == 1)
          packets[packetIndices[components[i]]] = extracted.GetAllRenderData()[0];
      }
      const auto worldElapsed = xiiTime::Now() - worldStart;
      XII_TEST_BOOL(validPackets);
      const auto start   = xiiTime::Now();
      const auto packed  = module->UpdateGpuInstances(packets, frame.GetFrameIndex());
      const auto elapsed = xiiTime::Now() - start;
      XII_TEST_RESULT(packed);
      execute(0, frame);
      execute(1, frame);
      const auto& stats = module->GetGpuInstanceData()->GetUploadStatistics();
      if (iteration > framesInFlight + 1)
      {
        XII_TEST_INT(stats.m_uiBytesUploaded, changed * sizeof(xiiGpuSceneInstance));
        XII_TEST_INT(stats.m_uiUploadPasses, 1);
        XII_TEST_INT(stats.m_uiUploadRanges, 1);
        times.PushBack(elapsed.GetMilliseconds());
        worldTimes.PushBack(worldElapsed.GetMilliseconds());
        const auto duration = profiler.GetPassDurationMs("World GPU Instance Upload");
        if (duration > 0)
          gpuTimes.PushBack(duration);
      }
      frame.End();
      environment.GetDevice()->WaitIdle();
    }
    report(mode == 0 ? "1 changed / 4096 candidates, CPU packing" : "4096 changed candidates, CPU packing", times);
    report(mode == 0 ? "1 changed / 4096 objects, world update and packet refresh" : "4096 changed objects, world update and packet refresh", worldTimes);
    if (!gpuTimes.IsEmpty())
      report(mode == 0 ? "288-byte shared upload, GPU duration" : "1179648-byte shared upload, GPU duration", gpuTimes);
    else
      xiiTestFramework::Output(xiiTestOutput::Details, "Transfer-queue GPU duration queries unavailable.");
  }
}
