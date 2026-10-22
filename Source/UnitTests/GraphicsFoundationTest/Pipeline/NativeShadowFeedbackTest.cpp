/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowFeedbackReadbackPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeShadowFeedback);
XII_CREATE_SIMPLE_TEST(NativeShadowFeedback, ReusableViewsOverflowAndReconfiguration)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiVirtualShadowMapSettings settings;
  settings.m_uiVirtualResolution  = 64;
  settings.m_uiPageSize           = 16;
  settings.m_uiPhysicalPageCount  = 64;
  settings.m_uiMaxPageAllocations = 64;
  if (!XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings)))
    return;
  const auto                                 asset = xiiLoadPipelineLibraryResource("VirtualShadowFeedback");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring()))
    return;
  const auto authored = loaded->GetAuthoring();
  xiiUuid    resetId, generateId, readbackId;
  for (const auto& pass : authored->m_Descriptor.m_Passes)
  {
    if (pass.m_sType == "xiiVirtualShadowFeedbackResetPass")
      resetId = pass.m_Id;
    if (pass.m_sType == "xiiVirtualShadowFeedbackPass")
      generateId = pass.m_Id;
    if (pass.m_sType == "xiiVirtualShadowFeedbackReadbackPass")
      readbackId = pass.m_Id;
  }
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authored->m_Descriptor.m_Id, asset.GetResourceID()});
  xiiUuid occurrences[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node      = root.AddSubgraph(i ? "Bounded" : "Complete", authored->m_Descriptor.m_Id);
    occurrences[i]       = root.m_Passes[node].m_Id;
    const auto configure = [&](const xiiUuid& id, xiiStringView property, xiiVariant value) {
      for (auto& item : root.m_Passes[node].m_Overrides)
        if (item.m_Path.GetCount() == 1 && item.m_Path[0] == id)
        {
          item.m_Properties.Insert(property, value);
          return;
        }
      auto& item = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
      item.m_Path.PushBack(id);
      item.m_Properties.Insert(property, value);
    };
    configure(resetId, "Capacity", i ? 8U : 128U);
    configure(generateId, "LightId", i + 1);
    configure(generateId, "VirtualResolution", 64U);
    configure(generateId, "PageSize", 16U);
    configure(readbackId, "MaxPendingReadbacks", 1U);
    for (const auto& pin : authored->m_Descriptor.m_Interface)
      root.m_Interface.PushBack({xiiStringBuilder(i ? "Bounded/" : "Complete/", pin.m_sName), pin.m_bInput, occurrences[i], pin.m_sName});
  }
  const auto                                 resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Native shadow feedback views", std::move(root));
  xiiResourceLock<xiiRenderPipelineResource> resourceLock(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  const auto                                 compiled = resourceLock.IsValid() ? resourceLock->GetPipeline() : xiiSharedPtr<const xiiRenderPipeline>();
  if (!XII_TEST_BOOL(compiled != nullptr))
    return;
  XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet() && compiled->GetExtractionRequirements().IsNoFlagSet());
  xiiUInt32 readbacks[2] = {xiiInvalidIndex, xiiInvalidIndex};
  for (xiiUInt32 i = 0; i < compiled->GetProvenance().GetCount(); ++i)
    if (compiled->GetProvenance()[i].m_Element == readbackId)
      readbacks[compiled->GetProvenance()[i].m_InstancePath[0] == occurrences[1] ? 1 : 0] = i;
  if (!XII_TEST_BOOL(readbacks[0] != xiiInvalidIndex && readbacks[1] != xiiInvalidIndex))
    return;
  xiiRenderPipelineInstance pipelines[2];
  xiiRenderGraphTestContext contexts[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
    if (!XII_TEST_RESULT(pipelines[i].Initialize(compiled)) || !XII_TEST_RESULT(contexts[i].Initialize("Shadow demand view")))
      return;
  XII_TEST_BOOL(pipelines[0].GetPipeline() == pipelines[1].GetPipeline());
  xiiUInt64              previousFrame = 0;
  xiiVirtualShadowPageId firstPages[2][2];
  for (xiiUInt32 phase = 0; phase < 6; ++phase)
  {
    if (phase == 2)
    {
      environment.GetDevice()->WaitIdle();
      XII_TEST_RESULT(xiiVirtualShadowMapManager::Configure(settings));
    }
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto& graph = contexts[view].GetGraph();
      auto& cache = contexts[view].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      struct Source
      {
        xiiRenderGraphTextureHandle m_Depth;
        xiiRenderGraphBufferHandle  m_Cascades;
        xiiUInt32                   m_uiCascadeCount;
      };
      const auto                                       source = graph.AddPass<Source>("Synthetic receiver depth", xiiGALCommandQueueFlags::Graphics, [phase](Source& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription image;
        image.m_Type = xiiGALResourceDimension::Texture2D;
        image.m_Size = {64, 64}; image.m_Format = xiiGALResourceFormat::D32Float;
        image.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
        data.m_Depth = builder.WriteTexture("Receiver Depth", image, xiiGALResourceStateFlags::DepthWrite);
        xiiGALBufferCreationDescription constants;
        constants.m_uiSize = sizeof(xiiShadowCascadeConstants); constants.m_BindFlags = xiiGALBindFlags::UniformBuffer;
        constants.m_Usage = xiiGALResourceUsage::Dynamic; constants.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
        data.m_Cascades = builder.WriteBuffer("Cascade Snapshot", constants, xiiGALResourceStateFlags::ConstantBuffer);
        data.m_uiCascadeCount = phase >= 4 ? 0U : 1U; }, [](const Source& data, xiiRenderGraphPassContext& execute) {
        auto& cmd = execute.GetCommandList();
        auto* depth = execute.GetTexture(data.m_Depth);
        cmd.ClearDepthStencilView(depth->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0.5f, 0);
        xiiGALStateTransitionDescription restore;
        restore.m_pResource = depth; restore.m_NewState = xiiGALResourceStateFlags::DepthWrite;
        restore.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
        cmd.TransitionResourceStates(xiiMakeArrayPtr(&restore, 1));
        xiiGALMapHelper<xiiShadowCascadeConstants> constants(cmd, execute.GetBuffer(data.m_Cascades), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        for (auto& matrix : constants->CascadeViewProjection) matrix = xiiMat4::MakeIdentity();
        constants->CascadeSplitDepths = xiiVec4(100); constants->CascadeWorldRadii = xiiVec4(1);
        constants->ActiveCascadeCount = data.m_uiCascadeCount; constants->_Pad = xiiVec3::MakeZero(); });
      xiiHybridArray<xiiRenderPipelineInputBinding, 4> inputs;
      for (auto prefix : {"Complete/", "Bounded/"})
      {
        inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface(xiiStringBuilder(prefix, "SceneDepth"), true), source.first->m_Depth));
        inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface(xiiStringBuilder(prefix, "CascadeConstants"), true), source.first->m_Cascades));
      }
      xiiVariantDictionary parameters;
      parameters.Insert("ShadowFeedbackNearPlane", view ? 0.2f : 0.1f);
      parameters.Insert("ShadowFeedbackInverseViewProjection", view ? xiiMat4::MakeScaling(xiiVec3(0.5f, 0.5f, 1)) : xiiMat4::MakeIdentity());
      if (!XII_TEST_RESULT(pipelines[view].Build(nullptr, graph, contexts[view].GetBlackboard(), parameters, inputs)))
        return;
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        const auto stats = static_cast<const xiiVirtualShadowFeedbackReadbackPass*>(pipelines[view].GetPass(readbacks[i]))->GetStats();
        if (phase == 1 || phase == 4)
        {
          XII_TEST_INT(stats.m_uiSourceFrame, previousFrame);
          XII_TEST_INT(stats.m_uiRawRequests, 64);
          XII_TEST_INT(stats.m_uiSubmittedRequests, i ? 8 : 64);
          XII_TEST_INT(stats.m_uiOverflowRequests, i ? 56 : 0);
          xiiVirtualShadowPageMapping mapping;
          XII_TEST_BOOL(xiiVirtualShadowMapManager::TryGetMapping(firstPages[view][i], mapping));
        }
        if (phase == 2)
        {
          XII_TEST_INT(stats.m_uiStaleReadbacks, 1);
        }
        if (phase == 5)
        {
          XII_TEST_INT(stats.m_uiSourceFrame, previousFrame);
          XII_TEST_INT(stats.m_uiRawRequests, 0);
          XII_TEST_INT(stats.m_uiSubmittedRequests, 0);
          XII_TEST_INT(stats.m_uiOverflowRequests, 0);
          XII_TEST_INT(stats.m_uiDroppedReadbacks, 0);
        }
      }
      xiiRenderGraphBufferHandle streams[2];
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        streams[i] = pipelines[view].GetInterfaceBinding(compiled->FindInterface(i ? "Bounded/Requests" : "Complete/Requests", false))->m_hBuffer;
        graph.AddPass<xiiUInt32>(i ? "Export bounded requests" : "Export complete requests", xiiGALCommandQueueFlags::Graphics, [stream = streams[i]](auto&, xiiRenderGraphBuilder& builder) { builder.ExportBuffer(builder.ReadBuffer(stream, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
      }
      graph.EndSetup();
      if (phase == 2)
        continue; // Abandoned setup must not publish or block the next capture.
      xiiStringBuilder error;
      if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = (phase & 1U) != 0}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
        return;
      if (view == 0 && phase < 2)
      {
        const auto& statistics = graph.GetStatistics();
        xiiTestFramework::Output(xiiTestOutput::Details, "Shadow feedback (phase %u): %u logical operations, %u queue submissions, %u barriers.", phase, statistics.m_uiCompiledPassCount, statistics.m_uiQueueSubmissionCount, statistics.m_uiTotalBarrierCount);
        if (phase == 0)
        {
          XII_TEST_INT(statistics.m_uiQueueSubmissionCount, 1);
        }
      }
      environment.GetDevice()->WaitIdle();
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        const auto                  stream = graph.GetExportedBuffer(streams[i]);
        xiiGpuVirtualShadowFeedback header;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), stream.Borrow(), 0, header))
          return;
        XII_TEST_INT(header.m_uiVirtualKeyLow, phase >= 4 ? 0 : 64);
        if (phase < 4)
        {
          xiiGpuVirtualShadowFeedback request;
          if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), stream.Borrow(), sizeof(request), request))
            return;
          const auto page = xiiVirtualShadowPageId::FromPackedValue(xiiUInt64(request.m_uiVirtualKeyLow) | (xiiUInt64(request.m_uiVirtualKeyHigh) << 32));
          XII_TEST_BOOL(page.IsValid());
          XII_TEST_INT(page.m_uiLightId, i + 1);
          XII_TEST_BOOL(page.m_uiPageX < 4 && page.m_uiPageY < 4);
          if (view == 1)
          {
            XII_TEST_BOOL(page.m_uiPageX >= 1 && page.m_uiPageX <= 2 && page.m_uiPageY >= 1 && page.m_uiPageY <= 2);
          }
          XII_TEST_INT(request.m_uiFlags, 1);
          firstPages[view][i] = page;
        }
      }
    }
    if (phase == 2)
    {
      XII_TEST_INT(xiiVirtualShadowMapManager::GetStats().m_uiResidentPageCount, 0);
    }
    previousFrame = frame.GetFrameIndex();
  }
}
