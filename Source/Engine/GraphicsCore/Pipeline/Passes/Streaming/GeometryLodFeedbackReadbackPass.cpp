/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Streaming/GeometryLodFeedbackReadbackPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/Device/Device.h>

struct xiiGeometryLodFeedbackReadbackPass::State : xiiRefCounted
{
  struct Slot : xiiRefCounted
  {
    xiiSharedPtr<xiiGALBuffer> m_Buffer;
    xiiSharedPtr<xiiGALFence>  m_Fence;
    xiiUInt64                  m_uiValue = 0, m_uiFrame = 0;
    bool                       m_bPending = false;
  };
  xiiDynamicArray<xiiSharedPtr<Slot>> m_Slots;
  xiiSharedPtr<xiiGALCommandList>     m_MapCommands;
  xiiGeometryLodFeedbackStats         m_Stats;
  void                                Consume(xiiGALDevice& device, xiiUInt64 completed)
  {
    for (auto& slot : m_Slots)
    {
      if (!slot->m_bPending)
        continue;
      if (slot->m_Fence->GetCompletedValue() < slot->m_uiValue)
      {
        // Frame completion proves no queued copy remains. Recording may have failed before submission.
        if (slot->m_uiFrame <= completed)
        {
          slot->m_bPending = false;
          ++m_Stats.m_uiDroppedReadbacks;
        }
        continue;
      }
      if (!m_MapCommands)
        m_MapCommands = device.CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Transfer});
      if (!m_MapCommands)
        return;
      m_MapCommands->Begin();
      void*      mapped = nullptr;
      const auto result = m_MapCommands->MapBuffer(slot->m_Buffer.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped);
      if (result.Succeeded())
      {
        if (mapped)
        {
          const auto*                 requests = static_cast<const xiiGpuGeometryLodRequest*>(mapped);
          const auto                  capacity = static_cast<xiiUInt32>(slot->m_Buffer->GetDescription().m_uiSize / sizeof(xiiGpuGeometryLodRequest) - 1);
          xiiGeometryLodFeedbackStats stats;
          stats.m_uiRawRequests      = requests[0].m_uiKey;
          stats.m_uiUniqueRequests   = xiiMath::Min(requests[0].m_uiDesiredLod, capacity);
          stats.m_uiOverflowRequests = requests[0].m_uiPriority;
          stats.m_uiSourceFrame      = slot->m_uiFrame;
          stats.m_uiDroppedReadbacks = m_Stats.m_uiDroppedReadbacks;
          for (xiiUInt32 i = 1; i <= capacity; ++i)
          {
            const auto& request = requests[i];
            if (!request.m_uiKey)
              continue;
            const auto  geometry = request.GetGeometry();
            const auto* record   = xiiGeometryResidencyManager::GetGpuRecord(geometry);
            if (!record || xiiGeometryResidencyManager::GetState(geometry) == xiiGeometryResidencyState::EvictPending || request.m_uiDesiredLod >= xiiMath::Min(record->m_uiLodCount, 8U) || !request.m_uiPurposeMask)
            {
              ++stats.m_uiStaleRequests;
              continue;
            }
            xiiGeometryResidencyManager::RequestResidency(geometry, request.m_uiDesiredLod, slot->m_uiFrame, request.m_uiPriority);
            ++stats.m_uiAcceptedRequests;
          }
          if (stats.m_uiSourceFrame >= m_Stats.m_uiSourceFrame)
            m_Stats = stats;
          slot->m_bPending = false;
        }
        m_MapCommands->UnmapBuffer(slot->m_Buffer.Borrow(), xiiGALMapType::Read).AssertSuccess();
      }
      m_MapCommands->End();
      m_MapCommands->Reset();
    }
  }
};
xiiGeometryLodFeedbackReadbackPass::xiiGeometryLodFeedbackReadbackPass()  = default;
xiiGeometryLodFeedbackReadbackPass::~xiiGeometryLodFeedbackReadbackPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGeometryLodFeedbackReadbackPass, 1, xiiRTTIDefaultAllocator<xiiGeometryLodFeedbackReadbackPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Requests", m_Requests)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MaxPendingReadbacks", m_uiMaxPendingReadbacks)->AddAttributes(new xiiDefaultValueAttribute(4U), new xiiClampValueAttribute(1U, 16U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Streaming") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiGeometryLodFeedbackStats xiiGeometryLodFeedbackReadbackPass::GetStats() const { return m_State ? m_State->m_Stats : xiiGeometryLodFeedbackStats(); }
xiiResult                   xiiGeometryLodFeedbackReadbackPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiMaxPendingReadbacks >= 1 && m_uiMaxPendingReadbacks <= 16 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiGeometryLodFeedbackReadbackPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  input       = context.GetInputBuffer(m_Requests);
  const auto* description = context.m_Graph.GetBufferDescription(input);
  if (!description || description->m_Mode != xiiGALBufferMode::Structured || description->m_uiElementByteStride != sizeof(xiiGpuGeometryLodRequest) || description->m_uiSize % sizeof(xiiGpuGeometryLodRequest))
    return context.ReportFailure("Geometry feedback readback requires the native structured request ABI.");
  const auto entries = description->m_uiSize / sizeof(xiiGpuGeometryLodRequest);
  if (entries < 2 || entries > 65537 || !xiiMath::IsPowerOf2(static_cast<xiiUInt32>(entries - 1)))
    return context.ReportFailure("Geometry feedback readback requires a bounded power-of-two request table plus header.");
  xiiUInt64 completed = 0;
  if (!xiiGALDevice::HasDefaultDevice() || xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Geometry feedback readback requires an active shared GPU frame.");
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (!m_State)
    m_State = XII_DEFAULT_NEW(State);
  m_State->Consume(*device, completed);
  xiiSharedPtr<State::Slot> slot;
  for (auto& candidate : m_State->m_Slots)
    if (!candidate->m_bPending && candidate->m_uiFrame <= completed)
    {
      slot = candidate;
      break;
    }
  if (!slot)
  {
    // Backpressure is bounded. A later frame can retry demand; expose every skipped capture.
    if (m_State->m_Slots.GetCount() >= m_uiMaxPendingReadbacks)
    {
      ++m_State->m_Stats.m_uiDroppedReadbacks;
      return XII_SUCCESS;
    }
    slot = XII_DEFAULT_NEW(State::Slot);
    m_State->m_Slots.PushBack(slot);
  }
  if (!slot->m_Buffer || slot->m_Buffer->GetDescription().m_uiSize != description->m_uiSize)
  {
    xiiGALBufferCreationDescription staging;
    staging.m_uiSize         = description->m_uiSize;
    staging.m_Usage          = xiiGALResourceUsage::Staging;
    staging.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
    slot->m_Buffer           = device->CreateBuffer(staging);
    if (!slot->m_Buffer)
      return context.ReportFailure("Could not allocate geometry feedback staging storage.");
  }
  if (!slot->m_Fence)
    slot->m_Fence = device->CreateFence({});
  if (!slot->m_Fence)
    return context.ReportFailure("Could not allocate geometry feedback completion fence.");
  ++slot->m_uiValue;
  slot->m_uiFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Input, m_Output;
    xiiSharedPtr<State::Slot>  m_Slot;
    xiiUInt64                  m_uiValue;
  };
  context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Input = builder.ReadBuffer(input, xiiGALResourceStateFlags::CopySource);
    data.m_Output = builder.WriteBuffer(builder.ImportBuffer("Geometry Request Readback", slot->m_Buffer, slot->m_Buffer->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
    builder.ExportBuffer(data.m_Output, xiiGALResourceStateFlags::CopyDestination);
    data.m_Slot = slot; data.m_uiValue = slot->m_uiValue; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList(); commands.CopyBuffer(execute.GetBuffer(data.m_Input), execute.GetBuffer(data.m_Output));
    commands.EnqueueSignal(data.m_Slot->m_Fence.Borrow(), data.m_uiValue); data.m_Slot->m_bPending = true; }, true);
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Streaming_GeometryLodFeedbackReadbackPass);
