/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowTablesPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeShadowTables);
XII_CREATE_SIMPLE_TEST(NativeShadowTables, FrozenViewsIncrementalUploadsAndRetirement)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiVirtualShadowMapSettings settings;
  settings.m_uiVirtualResolution = 64;
  settings.m_uiPageSize          = 16;
  settings.m_uiPhysicalPageCount = settings.m_uiMaxPageAllocations = 8;
  settings.m_uiFramesInFlight                                      = 2;
  if (!XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings)))
    return;
  const auto                                 asset = xiiCreateVirtualShadowTablesTestResource();
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto                          authoring = loaded->GetAuthoring();
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authoring->m_Descriptor.m_Id, asset.GetResourceID()});
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto instance = root.AddSubgraph(i ? "Repeated residency consumer" : "First residency consumer", authoring->m_Descriptor.m_Id);
    for (const auto& pin : authoring->m_Descriptor.m_Interface)
      root.m_Interface.PushBack({xiiStringBuilder(i ? "Repeated/" : "First/", pin.m_sName), false, root.m_Passes[instance].m_Id, pin.m_sName});
  }
  const auto                                 resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Repeated native table definitions", std::move(root));
  xiiResourceLock<xiiRenderPipelineResource> resolved(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  const auto                                 compiled = resolved->GetPipeline();
  if (!XII_TEST_BOOL(compiled != nullptr))
    return;
  XII_TEST_BOOL(compiled->GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(compiled->GetRuntimeFeatures() == xiiRenderPipelineFeature::VirtualShadows);
  xiiRenderPipelineInstance pipelines[2];
  xiiRenderGraphTestContext contexts[2];
  for (xiiUInt32 v = 0; v < 2; ++v)
    if (!XII_TEST_RESULT(pipelines[v].Initialize(compiled)) || !XII_TEST_RESULT(contexts[v].Initialize("Native shadow table view")))
      return;
  const auto submit = [](xiiUInt32 light, bool pinned = false) {
    xiiVirtualShadowPageRequest request;
    request.m_Page.m_uiLightId = light;
    request.m_bPinned          = pinned;
    xiiVirtualShadowMapManager::SubmitFeedback(xiiMakeArrayPtr(&request, 1));
  };
  xiiSharedPtr<const xiiVirtualShadowTableUpload> oldCapture;
  for (xiiUInt32 phase = 0; phase < 10; ++phase)
  {
    if (phase == 9)
    {
      environment.GetDevice()->WaitIdle();
      if (!XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings)))
        return;
      XII_TEST_BOOL(!xiiVirtualShadowMapManager::IsTableUploadRecorded(*oldCapture));
    }
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    if (phase == 0)
    {
      submit(1);
      submit(2);
    }
    if (phase == 5)
    {
      submit(1, true);
      submit(3, true);
    }
    if (phase == 6)
    {
      xiiVirtualShadowPageId page;
      page.m_uiLightId = 2;
      XII_TEST_BOOL(xiiVirtualShadowMapManager::InvalidatePage(page));
    }
    xiiRenderGraphBufferHandle framePhysical[2], frameTables[2];
    xiiVirtualShadowGpuLayout  frameLayouts[2];
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      auto& graph = contexts[v].GetGraph();
      auto& cache = contexts[v].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      const auto* producer   = static_cast<const xiiVirtualShadowTablesPass*>(pipelines[v].GetPass(0));
      const auto* oldBinding = pipelines[v].GetOutputBinding(0, "Residency");
      XII_TEST_BOOL(!oldBinding->m_pFrameData || oldBinding->m_pFrameData->GetSetupSerial() != graph.GetSetupSerial());
      if (!XII_TEST_RESULT(pipelines[v].Build(nullptr, graph, contexts[v].GetBlackboard())))
        return;
      const auto  residency = pipelines[v].GetOutputBinding(0, "Residency")->m_pFrameData.Downcast<const xiiVirtualShadowResidencySnapshot>();
      const auto* snapshot  = residency ? &residency->m_Bindings : nullptr;
      if (!XII_TEST_BOOL(snapshot != nullptr))
        return;
      const auto repeated = static_cast<const xiiVirtualShadowTablesPass*>(pipelines[v].GetPass(1));
      XII_TEST_BOOL(producer != repeated);
      XII_TEST_BOOL(pipelines[v].GetOutputBinding(1, "Residency")->m_pFrameData.Downcast<const xiiVirtualShadowResidencySnapshot>()->m_Bindings.m_hPhysicalPageTable == snapshot->m_hPhysicalPageTable);
      XII_TEST_BOOL(pipelines[v].GetOutputBinding(1, "Residency")->m_pFrameData.Downcast<const xiiVirtualShadowResidencySnapshot>()->m_Bindings.m_hVirtualPageTable == snapshot->m_hVirtualPageTable);
      XII_TEST_INT(repeated->GetUploadBytes(), 0);
      const auto capture = xiiVirtualShadowMapManager::CaptureTableUpload(frame.GetFrameIndex());
      if (phase == 4 && v == 0)
      {
        const auto epoch              = xiiVirtualShadowMapManager::GetConfigurationRevision();
        auto       invalid            = settings;
        invalid.m_uiVirtualResolution = invalid.m_uiPageSize = 1U << 30;
        XII_TEST_BOOL(xiiVirtualShadowMapManager::Configure(invalid).Failed());
        XII_TEST_INT(xiiVirtualShadowMapManager::GetConfigurationRevision(), epoch);
        XII_TEST_INT(xiiVirtualShadowMapManager::GetConfiguration().m_uiPageSize, settings.m_uiPageSize);
        XII_TEST_BOOL(xiiVirtualShadowMapManager::CaptureTableUpload(frame.GetFrameIndex()) == capture);
        XII_TEST_BOOL(residency->m_Upload->m_uiConfigurationRevision == xiiVirtualShadowMapManager::GetConfigurationRevision());
      }

      if (phase == 0 && v == 0)
      {
        oldCapture = capture;
        XII_TEST_INT(producer->GetUploadBytes(), (8 + 16) * sizeof(xiiGpuVirtualShadowPage));
        XII_TEST_INT(producer->GetUploadRanges(), 2);
        submit(3); // Late demand must not rewrite tables already captured for another view.
        XII_TEST_BOOL(capture == xiiVirtualShadowMapManager::CaptureTableUpload(frame.GetFrameIndex()));
      }
      if (phase == 2 && v == 0)
      {
        XII_TEST_INT(capture->m_PhysicalRecords.GetCount(), 1);
        XII_TEST_INT(producer->GetUploadBytes(), (1 + 16) * sizeof(xiiGpuVirtualShadowPage));
      }
      if ((phase == 3 || phase == 4) && v == 0)
        XII_TEST_INT(producer->GetUploadBytes(), 0);
      if (phase == 5 && v == 0)
      {
        XII_TEST_INT(capture->m_PhysicalRanges.GetCount(), 2);
        XII_TEST_INT(producer->GetUploadRanges(), 3);
        XII_TEST_INT(producer->GetUploadBytes(), (2 + 16) * sizeof(xiiGpuVirtualShadowPage));
      }
      if (phase == 8 && v == 0)
        XII_TEST_BOOL(producer->GetUploadBytes() > 0); // abandoned slice retries
      if (phase == 9)
      {
        XII_TEST_BOOL(capture->m_uiConfigurationRevision != oldCapture->m_uiConfigurationRevision);
        XII_TEST_BOOL(capture->m_PhysicalBuffer != oldCapture->m_PhysicalBuffer);
      }
      struct Outputs
      {
        xiiRenderGraphBufferHandle m_SourcePhysical, m_SourceVirtual, m_Physical, m_Virtual;
      };
      const auto consumerQueue = (phase & 1U) ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics;
      const auto exported      = graph.AddPass<Outputs>("Capture explicit residency outputs", consumerQueue, [&](Outputs& data, xiiRenderGraphBuilder& builder) {
        data.m_SourcePhysical = builder.ReadBuffer(snapshot->m_hPhysicalPageTable, xiiGALResourceStateFlags::CopySource);
        data.m_SourceVirtual = builder.ReadBuffer(snapshot->m_hVirtualPageTable, xiiGALResourceStateFlags::CopySource);
        data.m_Physical = builder.WriteBuffer("Physical table capture", *graph.GetBufferDescription(data.m_SourcePhysical), xiiGALResourceStateFlags::CopyDestination);
        data.m_Virtual = builder.WriteBuffer("Virtual table capture", *graph.GetBufferDescription(data.m_SourceVirtual), xiiGALResourceStateFlags::CopyDestination);
        builder.ExportBuffer(data.m_Physical, xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(data.m_Virtual, xiiGALResourceStateFlags::ShaderResource); }, [](const Outputs& data, xiiRenderGraphPassContext& execute) {
        auto* physicalSource = execute.GetBuffer(data.m_SourcePhysical);
        auto* virtualSource = execute.GetBuffer(data.m_SourceVirtual);
        execute.GetCommandList().CopyBufferRegion(physicalSource, 0, execute.GetBuffer(data.m_Physical), 0, physicalSource->GetDescription().m_uiSize);
        execute.GetCommandList().CopyBufferRegion(virtualSource, 0, execute.GetBuffer(data.m_Virtual), 0, virtualSource->GetDescription().m_uiSize); }, true);
      const auto physical = exported.first->m_Physical, table = exported.first->m_Virtual;
      const auto layout = static_cast<const xiiVirtualShadowGpuLayout&>(*snapshot);
      graph.EndSetup();
      if (phase == 6)
      {
        // Model a discarded recording with a never-signaled real fence. Recording
        // alone must not acknowledge dirty bytes, even after the shared frame retires.
        if (v == 0)
          xiiVirtualShadowMapManager::RecordTableUpload(capture, environment.GetDevice()->CreateFence({}));
        continue;
      }
      xiiStringBuilder error;
      if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = (phase & 1U) != 0}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &contexts[v].GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
        return;
      if (phase & 1U)
      {
        bool crossQueueWait = false;
        for (const auto& submission : graph.GetQueueSubmissions())
          crossQueueWait |= !submission.m_WaitSubmissionIndices.IsEmpty();
        XII_TEST_BOOL(crossQueueWait);
      }
      if (v == 0 && (phase == 0 || phase == 2 || phase == 3 || phase == 5))
        xiiTestFramework::Output(xiiTestOutput::Details, "Table phase %u: %llu upload bytes, %u upload ranges, %u submissions.", phase, producer->GetUploadBytes(), producer->GetUploadRanges(), graph.GetStatistics().m_uiQueueSubmissionCount);
      if (v == 1)
        XII_TEST_INT(producer->GetUploadBytes(), 0);
      frameTables[v]   = table;
      framePhysical[v] = physical;
      frameLayouts[v]  = layout;
    }
    if (phase == 6)
      continue;
    // Both view graphs have been submitted without a host wait between them.
    // Odd phases copy the captures on compute, whereas ReadBufferRecord submits
    // its staging copy on graphics. Complete that producer queue before host
    // readback; the readback's own graphics fence cannot order compute writes.
    if (phase & 1U)
    {
      auto* queue = environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Compute);
      queue->WaitForFenceValue(queue->GetNextFenceValue() - 1);
    }
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      auto&       graph       = contexts[v].GetGraph();
      const auto& layout      = frameLayouts[v];
      const auto  gpuTable    = graph.GetExportedBuffer(frameTables[v]);
      const auto  gpuPhysical = graph.GetExportedBuffer(framePhysical[v]);
      xiiUInt32   resident    = 0;
      for (xiiUInt32 i = 0; i < layout.m_uiVirtualTableCapacity; ++i)
      {
        xiiGpuVirtualShadowPage record;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), gpuTable.Borrow(), (layout.m_uiVirtualTableBaseIndex + i) * sizeof(record), record))
          return;
        if (!(record.m_uiFlags & 1U))
          continue;
        ++resident;
        const auto id = xiiVirtualShadowPageId::FromPackedValue(xiiUInt64(record.m_uiVirtualKeyLow) | (xiiUInt64(record.m_uiVirtualKeyHigh) << 32));
        XII_TEST_BOOL(id.IsValid() && id.m_uiLightId >= 1 && id.m_uiLightId <= (phase == 0 ? 2U : 3U));
        xiiGpuVirtualShadowPage slot;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), gpuPhysical.Borrow(), (layout.m_uiFrameBaseIndex + record.m_uiPhysicalPage) * sizeof(slot), slot))
          return;
        XII_TEST_INT(slot.m_uiVirtualKeyLow, record.m_uiVirtualKeyLow);
        XII_TEST_INT(slot.m_uiVirtualKeyHigh, record.m_uiVirtualKeyHigh);
        XII_TEST_INT(slot.m_uiFlags, record.m_uiFlags);
      }
      XII_TEST_INT(resident, phase == 9 ? 0 : phase == 0 ? 2 :
                                                           3);
      XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    }
  }
}

XII_CREATE_SIMPLE_TEST(NativeShadowTables, FrozenPageMappingsRetireBeforePhysicalReuse)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiVirtualShadowMapSettings settings;
  settings.m_uiVirtualResolution = 64;
  settings.m_uiPageSize          = 16;
  settings.m_uiPhysicalPageCount = settings.m_uiMaxPageAllocations = 2;
  settings.m_uiFramesInFlight                                      = 2;
  if (!XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings)))
    return;
  const auto                                 asset = xiiCreateVirtualShadowTablesTestResource();
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetPipeline()))
    return;
  xiiRenderPipelineInstance pipelines[2];
  xiiRenderGraphTestContext contexts[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
    if (!XII_TEST_RESULT(pipelines[i].Initialize(loaded->GetPipeline())) || !XII_TEST_RESULT(contexts[i].Initialize("Frozen page lifetime view")))
      return;
  const auto request = [](xiiUInt32 light) {
    xiiVirtualShadowPageRequest page;
    page.m_Page.m_uiLightId = light;
    xiiVirtualShadowMapManager::SubmitFeedback(xiiMakeArrayPtr(&page, 1));
  };
  xiiVirtualShadowPageId first, replacement;
  first.m_uiLightId                        = 1;
  replacement.m_uiLightId                  = 3;
  xiiUInt32                  firstPhysical = xiiInvalidIndex;
  xiiVirtualShadowRasterPage staleWork;
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    if (phase == 0)
    {
      request(1);
      request(2);
      xiiVirtualShadowPageMapping mapping;
      if (!XII_TEST_BOOL(xiiVirtualShadowMapManager::TryGetMapping(first, mapping)))
        return;
      firstPhysical                                          = mapping.m_uiPhysicalPage;
      const auto                                  projection = xiiMat4::MakeIdentity();
      xiiDynamicArray<xiiVirtualShadowRasterPage> work;
      xiiVirtualShadowMapManager::GatherRasterPages(xiiMakeArrayPtr(&projection, 1), 1, work);
      if (!XII_TEST_INT(work.GetCount(), 1))
        return;
      staleWork = work[0];
    }
    if (phase == 2)
    {
      XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiRetiredPageCount, 0);
      request(3);
      xiiVirtualShadowPageMapping mapping;
      XII_TEST_BOOL(xiiVirtualShadowMapManager::TryGetMapping(replacement, mapping));
      XII_TEST_INT(mapping.m_uiPhysicalPage, firstPhysical);
      XII_TEST_BOOL(!xiiVirtualShadowMapManager::MarkPageRendered(staleWork));
    }
    xiiRenderGraphBufferHandle tables[2];
    xiiVirtualShadowGpuLayout  layouts[2];
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      auto& graph = contexts[v].GetGraph();
      auto& cache = contexts[v].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      if (!XII_TEST_RESULT(pipelines[v].Build(nullptr, graph, contexts[v].GetBlackboard())))
        return;
      const auto  residency = pipelines[v].GetOutputBinding(0, "Residency")->m_pFrameData.Downcast<const xiiVirtualShadowResidencySnapshot>();
      const auto* snapshot  = residency ? &residency->m_Bindings : nullptr;
      if (!XII_TEST_BOOL(snapshot != nullptr))
        return;
      layouts[v] = *snapshot;
      if (phase == 1 && v == 0)
      {
        // Old demand is retired, but this frame's immutable table still maps it.
        XII_TEST_BOOL(frame.GetCompletedFrame() >= staleWork.m_uiFrame);
        const auto frozen = xiiVirtualShadowMapManager::CaptureTableUpload(frame.GetFrameIndex());
        request(3);
        xiiVirtualShadowPageMapping mapping;
        XII_TEST_BOOL(!xiiVirtualShadowMapManager::TryGetMapping(first, mapping));
        XII_TEST_BOOL(!xiiVirtualShadowMapManager::TryGetMapping(replacement, mapping));
        XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiRetiredPageCount, 1);
        XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiFreePageCount, 0);
        XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiResidentPageCount, 1);
        XII_TEST_BOOL(xiiVirtualShadowMapManager::CaptureTableUpload(frame.GetFrameIndex()) == frozen);
      }
      const auto exported = graph.AddPass<xiiRenderGraphBufferHandle>("Read frozen virtual mappings", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
        data = builder.ReadBuffer(snapshot->m_hVirtualPageTable, xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(data, xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
      tables[v]           = *exported.first;
      graph.EndSetup();
      xiiStringBuilder error;
      if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &contexts[v].GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
        return;
    }
    // Neither view waits for the host before both have submitted their graph.
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      const auto table    = contexts[v].GetGraph().GetExportedBuffer(tables[v]);
      xiiUInt32  count    = 0;
      bool       hasFirst = false, hasReplacement = false;
      for (xiiUInt32 i = 0; i < layouts[v].m_uiVirtualTableCapacity; ++i)
      {
        xiiGpuVirtualShadowPage record;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), table.Borrow(), (layouts[v].m_uiVirtualTableBaseIndex + i) * sizeof(record), record))
          return;
        if (!(record.m_uiFlags & 1U))
          continue;
        ++count;
        const auto page = xiiVirtualShadowPageId::FromPackedValue(xiiUInt64(record.m_uiVirtualKeyLow) | (xiiUInt64(record.m_uiVirtualKeyHigh) << 32));
        hasFirst |= page.m_uiLightId == 1;
        hasReplacement |= page.m_uiLightId == 3;
      }
      XII_TEST_INT(count, 2);
      XII_TEST_BOOL(hasFirst == (phase < 2));
      XII_TEST_BOOL(hasReplacement == (phase == 2));
    }
  }
}
