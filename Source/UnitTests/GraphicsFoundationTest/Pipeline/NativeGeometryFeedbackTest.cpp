/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackReadbackPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>

namespace
{
  template <typename T> xiiRenderGraphBufferHandle UploadFeedbackInput(xiiRenderGraph& graph, xiiStringView name, xiiArrayPtr<T> values)
  {
    struct Data
    {
      xiiRenderGraphBufferHandle m_Buffer;
      xiiDynamicArray<xiiUInt8>  m_Bytes;
    };
    const auto emitted = graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = values.GetCount() * sizeof(T); description.m_uiElementByteStride = sizeof(T);
      description.m_Mode = xiiGALBufferMode::Structured; description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_Buffer = builder.WriteBuffer(name, description, xiiGALResourceStateFlags::CopyDestination);
      data.m_Bytes = xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(values.GetPtr()), values.GetCount() * sizeof(T)); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Buffer), 0, data.m_Bytes); });
    return emitted.first->m_Buffer;
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeGeometryFeedback);
XII_CREATE_SIMPLE_TEST(NativeGeometryFeedback, AuthoredAsynchronousResidency)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshBufferResourceDescriptor mesh;
  mesh.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const auto             buffer = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Native feedback shared mesh", std::move(mesh));
  xiiGeometryDescription description;
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    auto& lod                    = description.m_Lods.ExpandAndGetRef();
    lod.m_hMeshBuffer            = buffer;
    lod.m_fMinimumScreenCoverage = i == 0 ? 16.0f : i == 1 ? 8.0f :
                                                             0.0f;
  }
  xiiGeometryHandle handles[4];
  for (auto& handle : handles)
  {
    handle = xiiGeometryResidencyManager::RegisterGeometry(description);
    xiiGeometryResidencyManager::RequestResidency(handle, 2, 0);
    if (!XII_TEST_BOOL(!handle.IsInvalidated()))
      return;
  }
  XII_SCOPE_EXIT(for (const auto handle : handles) xiiGeometryResidencyManager::UnregisterGeometry(handle, 0));
  const auto                                 asset = xiiLoadPipelineLibraryResource("GeometryLodFeedback");
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto                          authoring = loaded->GetAuthoring();
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authoring->m_Descriptor.m_Id, asset.GetResourceID()});
  xiiUuid resetId, generateId, readbackId;
  for (const auto& pass : authoring->m_Descriptor.m_Passes)
  {
    if (pass.m_sType == "xiiGeometryLodFeedbackResetPass")
      resetId = pass.m_Id;
    if (pass.m_sType == "xiiGeometryLodFeedbackPass")
      generateId = pass.m_Id;
    if (pass.m_sType == "xiiGeometryLodFeedbackReadbackPass")
      readbackId = pass.m_Id;
  }
  xiiUuid instances[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto node = root.AddSubgraph(i ? "Camera requests" : "Bounded shadow requests", authoring->m_Descriptor.m_Id);
    instances[i]    = root.m_Passes[node].m_Id;
    auto& reset     = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    reset.m_Path.PushBack(resetId);
    reset.m_Properties.Insert("Capacity", i ? 8U : 2U);
    auto& generate = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    generate.m_Path.PushBack(generateId);
    generate.m_Properties.Insert("PriorityTier", i ? 3U : 1U);
    generate.m_Properties.Insert("PurposeMask", i ? 1U : 2U);
    generate.m_Properties.Insert("AsyncCompute", i == 0);
    auto& readback = root.m_Passes[node].m_Overrides.ExpandAndGetRef();
    readback.m_Path.PushBack(readbackId);
    readback.m_Properties.Insert("MaxPendingReadbacks", 3U);
    for (const auto& mapping : authoring->m_Descriptor.m_Interface)
      root.m_Interface.PushBack({mapping.m_bInput ? xiiStringBuilder(mapping.m_sName) : xiiStringBuilder(i ? "Camera/" : "Shadow/", mapping.m_sName), mapping.m_bInput, root.m_Passes[node].m_Id, mapping.m_sName});
  }
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Native authored geometry feedback", std::move(root));
  xiiRenderPipelineInstance pipelines[2];
  xiiRenderGraphContext     contexts[2];
  for (xiiUInt32 view = 0; view < 2; ++view)
    if (!XII_TEST_RESULT(pipelines[view].Synchronize(resource)) || !XII_TEST_RESULT(contexts[view].Initialize("Native geometry streaming")))
      return;
  xiiUInt32   readbacks[2] = {xiiInvalidIndex, xiiInvalidIndex};
  const auto* compiled     = pipelines[0].GetPipeline();
  for (xiiUInt32 index = 0; index < compiled->GetProvenance().GetCount(); ++index)
  {
    const auto& origin = compiled->GetProvenance()[index];
    if (origin.m_Element == readbackId && !origin.m_InstancePath.IsEmpty())
      for (xiiUInt32 i = 0; i < 2; ++i)
        if (origin.m_InstancePath[0] == instances[i])
          readbacks[i] = index;
  }
  if (!XII_TEST_BOOL(readbacks[0] != xiiInvalidIndex && readbacks[1] != xiiInvalidIndex))
    return;
  xiiDynamicArray<xiiUInt64> recorded[2];
  bool                       sawAccepted = false, sawStale = false;
  xiiUInt64                  abandoned = 0;
  xiiGeometryHandle          retired;
  const auto                 render = [&](xiiUInt32 viewIndex, xiiUInt32 phase, bool execute = true) {
    auto&               pipeline = pipelines[viewIndex];
    auto&               context  = contexts[viewIndex];
    auto&               graph    = context.GetGraph();
    auto&               cache    = context.GetResourceCache();
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    // This fixture executes its graph directly; reproduce the existing device frame owner's streaming tick.
    xiiGeometryResidencyManager::ProcessStreaming(frame.GetFrameIndex(), frame.GetCompletedFrame(), xiiGeometryResidencyManager::GetConfiguration().m_uiUploadBudgetPerFrameBytes);
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiGpuSceneInstance records[67];
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(records); ++i)
    {
      records[i].m_uiGeometryIndex    = (phase == 4 && i % 4 == 0 ? retired : handles[i % 4]).GetInternalID().m_InstanceIndex;
      records[i].m_uiObjectId         = i + 1;
      records[i].m_uiFlags            = xiiGpuInstanceFlags::Enabled;
      records[i].m_BoundsCenterRadius = xiiVec4(1, 0, 0, 1);
    }
    records[0].m_uiForcedLod         = 1;
    records[1].m_uiForcedLod         = 2;
    records[2].m_uiForcedLod         = 7;
    records[4].m_uiLodSelectionIndex = 0;
    records[5].m_uiLodSelectionIndex = 0;
    records[61].m_uiFlags |= xiiGpuInstanceFlags::Static;
    records[62].m_uiFlags              = 0;
    records[63].m_uiVisibilityMask     = 0;
    records[64].m_BoundsCenterRadius.x = 102;
    xiiDynamicArray<xiiGpuGeometryRecord, xiiAlignedAllocatorWrapper> geometry;
    xiiUInt32                                                         count = 0;
    for (const auto handle : handles)
      count = xiiMath::Max(count, handle.GetInternalID().m_InstanceIndex + 1U);
    geometry.SetCount(count);
    for (const auto handle : handles)
    {
      auto& data                              = geometry[(phase == 4 && handle == handles[0] ? retired : handle).GetInternalID().m_InstanceIndex];
      data.m_uiGeneration                     = phase == 4 && handle == handles[0] ? retired.GetInternalID().m_Generation : handle.GetInternalID().m_Generation + (phase == 2 ? 1 : 0);
      data.m_uiLodCount                       = 3;
      data.m_uiResidentLodMask                = phase == 3 ? 0 : 4;
      data.m_Lods[0].m_fMinimumScreenCoverage = 16;
      data.m_Lods[1].m_fMinimumScreenCoverage = 8;
      data.m_Lods[2].m_fMinimumScreenCoverage = 0;
    }
    xiiGpuVisibilityView view;
    view.m_ViewProjectionMatrix          = xiiMat4::MakeIdentity();
    view.m_OcclusionViewProjectionMatrix = xiiMat4::MakeIdentity();
    for (auto& plane : view.m_FrustumPlanes)
      plane = xiiVec4::MakeZero();
    view.m_FrustumPlanes[0] = xiiVec4(1, 0, 0, -100);
    view.m_CameraPosition   = xiiVec4::MakeZero();
    view.m_ViewportAndHiZ   = xiiVec4(64, 64, 0, 0);
    view.m_LodParameters    = xiiVec4::MakeZero();
    view.m_uiInstanceCount  = phase == 1 ? 0 : 67;
    view.m_uiVisibilityMask = 1;
    view.m_uiExcludedFlags  = xiiGpuInstanceFlags::Static;
    view.m_uiUseCandidates  = 1;
    view.m_uiCandidateCount = phase == 1 ? 0 : xiiMath::MaxValue<xiiUInt32>();
    xiiUInt32 candidates[67];
    for (xiiUInt32 i = 0; i < 67; ++i)
      candidates[i] = i;
    candidates[65]                                            = xiiInvalidIndex;
    candidates[66]                                            = 0;
    const auto                                       scene    = UploadFeedbackInput(graph, "Feedback instances", xiiMakeArrayPtr(records));
    const auto                                       metadata = UploadFeedbackInput(graph, "Feedback geometry", geometry.GetArrayPtr());
    const auto                                       snapshot = UploadFeedbackInput(graph, "Feedback view", xiiMakeArrayPtr(&view, 1));
    const auto                                       query    = UploadFeedbackInput(graph, "Feedback candidates", xiiMakeArrayPtr(candidates));
    xiiHybridArray<xiiRenderPipelineInputBinding, 4> inputs;
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("SceneInstances", true), scene));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("GeometryMetadata", true), metadata));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("VisibilityView", true), snapshot));
    inputs.PushBack(xiiRenderPipelineInputBinding(graph, compiled->FindInterface("Candidates", true), query));
    if (!XII_TEST_RESULT(pipeline.Build(nullptr, graph, context.GetBlackboard(), {}, inputs)))
      return;
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      const auto* readback = static_cast<const xiiGeometryLodFeedbackReadbackPass*>(pipeline.GetPass(readbacks[i]));
      const auto  stats    = readback->GetStats();
      if (stats.m_uiSourceFrame)
      {
        XII_TEST_BOOL(recorded[viewIndex].Contains(stats.m_uiSourceFrame));
        XII_TEST_BOOL(stats.m_uiSourceFrame != abandoned);
        sawAccepted |= stats.m_uiAcceptedRequests > 0;
        sawStale |= stats.m_uiStaleRequests > 0;
      }
    }
    xiiRenderGraphBufferHandle requests[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      requests[i] = pipeline.GetInterfaceBinding(compiled->FindInterface(i ? "Camera/Requests" : "Shadow/Requests", false))->m_hBuffer;
      graph.AddPass<xiiUInt32>(i ? "Export camera demand" : "Export shadow demand", xiiGALCommandQueueFlags::Graphics, [&, i](auto&, xiiRenderGraphBuilder& builder) { builder.ExportBuffer(builder.ReadBuffer(requests[i], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    }
    XII_TEST_BOOL(requests[0].m_Id != requests[1].m_Id);
    graph.EndSetup();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()))
      return;
    if (!execute)
    {
      abandoned = frame.GetFrameIndex();
      frame.End();
      return;
    }
    if (!XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
      return;
    recorded[viewIndex].PushBack(frame.GetFrameIndex());
    frame.End();
    environment.GetDevice()->WaitIdle();
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      xiiGpuGeometryLodRequest header;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(requests[i]).Borrow(), 0, header))
        return;
      XII_TEST_INT(header.m_uiKey, phase == 1 ? 0 : phase == 3 ? 61 :
                                                                                 60);
      XII_TEST_INT(header.m_uiDesiredLod, phase == 1 ? 0 : i ? 4 :
                                                                               2);
      XII_TEST_BOOL((header.m_uiPriority != 0) == (phase != 1 && i == 0));
      xiiUInt32 unique = 0;
      for (xiiUInt32 entry = 1; entry <= (i ? 8U : 2U); ++entry)
      {
        xiiGpuGeometryLodRequest request;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(requests[i]).Borrow(), entry * sizeof(request), request))
          return;
        if (!request.m_uiKey)
        {
          XII_TEST_INT(request.m_uiDesiredLod, xiiInvalidIndex);
          continue;
        }
        ++unique;
        XII_TEST_INT(request.m_uiDesiredLod, 0);
        XII_TEST_INT(request.m_uiPurposeMask, i ? 1 : 2);
        XII_TEST_INT(request.m_uiPriority, ((i ? 3U : 1U) << 30) | 65536U);
        const auto identity = request.GetGeometry().GetInternalID();
        bool       found    = false;
        for (const auto handle : handles)
          found |= identity.m_InstanceIndex == (phase == 4 && handle == handles[0] ? retired : handle).GetInternalID().m_InstanceIndex && identity.m_Generation == (phase == 4 && handle == handles[0] ? retired.GetInternalID().m_Generation : handle.GetInternalID().m_Generation + (phase == 2 ? 1 : 0));
        XII_TEST_BOOL(found);
      }
      XII_TEST_INT(unique, header.m_uiDesiredLod);
    }
  };
  for (xiiUInt32 view = 0; view < 2; ++view)
    render(view, 0);
  render(0, 1, false);
  for (xiiUInt32 view = 0; view < 2; ++view)
    render(view, 2);
  for (xiiUInt32 view = 0; view < 2; ++view)
    render(view, 3);
  for (xiiUInt32 view = 0; view < 2; ++view)
    render(view, 1);
  for (xiiUInt32 view = 0; view < 2; ++view)
    render(view, 0);
  XII_TEST_BOOL(sawAccepted && sawStale);
  for (const auto handle : handles)
  {
    const auto* resident = xiiGeometryResidencyManager::GetGpuRecord(handle);
    XII_TEST_BOOL(resident && (resident->m_uiResidentLodMask & 1U) != 0);
  }
  retired = handles[0];
  xiiGeometryResidencyManager::UnregisterGeometry(retired, 0);
  {
    xiiRenderGraphFrame retirement;
    if (!XII_TEST_RESULT(retirement.Begin()))
      return;
    xiiGeometryResidencyManager::ProcessStreaming(retirement.GetFrameIndex(), retirement.GetCompletedFrame(), xiiGeometryResidencyManager::GetConfiguration().m_uiUploadBudgetPerFrameBytes);
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsValid(retired));
    handles[0] = xiiGeometryResidencyManager::RegisterGeometry(description);
    xiiGeometryResidencyManager::RequestResidency(handles[0], 2, 0);
    XII_TEST_BOOL(!handles[0].IsInvalidated());
    XII_TEST_BOOL(handles[0] != retired);
  }
  render(0, 4);
  render(0, 1);
  const auto stats = static_cast<const xiiGeometryLodFeedbackReadbackPass*>(pipelines[0].GetPass(readbacks[1]))->GetStats();
  XII_TEST_INT(stats.m_uiStaleRequests, 1);
  XII_TEST_INT(stats.m_uiAcceptedRequests, 3);
  const auto* replacement = xiiGeometryResidencyManager::GetGpuRecord(handles[0]);
  XII_TEST_BOOL(replacement != nullptr);
  if (replacement)
  {
    XII_TEST_INT(replacement->m_uiResidentLodMask, 4U);
  }
}
