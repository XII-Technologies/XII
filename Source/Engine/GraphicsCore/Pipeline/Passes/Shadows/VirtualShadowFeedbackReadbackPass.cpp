/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowFeedbackReadbackPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/Device/Device.h>

struct xiiVirtualShadowFeedbackReadbackPass::State : xiiRefCounted
{
  struct Slot : xiiRefCounted
  {
    xiiSharedPtr<xiiGALBuffer> m_Buffer;
    xiiSharedPtr<xiiGALFence>  m_Fence;
    xiiUInt64                  m_uiValue = 0, m_uiFrame = 0, m_uiConfigurationRevision = 0;
    bool                       m_bPending = false;
  };
  xiiDynamicArray<xiiSharedPtr<Slot>> m_Slots;
  xiiSharedPtr<xiiGALCommandList>     m_MapCommands;
  xiiVirtualShadowFeedbackStats       m_Stats;
  void                                Consume(xiiGALDevice& device, xiiUInt64 completed)
  {
    for (auto& slot : m_Slots)
    {
      if (!slot->m_bPending)
        continue;
      if (slot->m_Fence->GetCompletedValue() < slot->m_uiValue)
      {
        // An abandoned recording never signals. Shared frame completion makes it retryable.
        if (slot->m_uiFrame <= completed)
        {
          slot->m_bPending = false;
          ++m_Stats.m_uiDroppedReadbacks;
        }
        continue;
      }
      if (slot->m_uiConfigurationRevision != xiiVirtualShadowMapManager::GetConfigurationRevision())
      {
        slot->m_bPending = false;
        ++m_Stats.m_uiStaleReadbacks;
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
          const auto*                                  stream   = static_cast<const xiiGpuVirtualShadowFeedback*>(mapped);
          const auto                                   capacity = static_cast<xiiUInt32>(slot->m_Buffer->GetDescription().m_uiSize / sizeof(xiiGpuVirtualShadowFeedback) - 1);
          const auto                                   count    = xiiMath::Min(stream[0].m_uiVirtualKeyLow, capacity);
          xiiDynamicArray<xiiVirtualShadowPageRequest> requests;
          requests.Reserve(count);
          for (xiiUInt32 i = 1; i <= count; ++i)
          {
            const auto& source   = stream[i];
            auto&       request  = requests.ExpandAndGetRef();
            request.m_Page       = xiiVirtualShadowPageId::FromPackedValue(xiiUInt64(source.m_uiVirtualKeyLow) | (xiiUInt64(source.m_uiVirtualKeyHigh) << 32));
            request.m_uiPriority = source.m_uiPriority;
            request.m_bPinned    = (source.m_uiFlags & 1U) != 0;
          }
          xiiVirtualShadowMapManager::SubmitFeedback(requests);
          if (slot->m_uiFrame >= m_Stats.m_uiSourceFrame)
          {
            m_Stats.m_uiSourceFrame       = slot->m_uiFrame;
            m_Stats.m_uiRawRequests       = stream[0].m_uiVirtualKeyLow;
            m_Stats.m_uiSubmittedRequests = count;
            m_Stats.m_uiOverflowRequests  = stream[0].m_uiVirtualKeyLow - count;
          }
          slot->m_bPending = false;
        }
        m_MapCommands->UnmapBuffer(slot->m_Buffer.Borrow(), xiiGALMapType::Read).AssertSuccess();
      }
      m_MapCommands->End();
      m_MapCommands->Reset();
    }
  }
};
xiiVirtualShadowFeedbackReadbackPass::xiiVirtualShadowFeedbackReadbackPass()  = default;
xiiVirtualShadowFeedbackReadbackPass::~xiiVirtualShadowFeedbackReadbackPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualShadowFeedbackReadbackPass, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowFeedbackReadbackPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Requests", m_Requests)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MaxPendingReadbacks", m_uiMaxPendingReadbacks)->AddAttributes(new xiiDefaultValueAttribute(4U), new xiiClampValueAttribute(1U, 16U)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Shadows") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiVirtualShadowFeedbackReadbackPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiMaxPendingReadbacks >= 1 && m_uiMaxPendingReadbacks <= 16 ? XII_SUCCESS : XII_FAILURE;
}
xiiVirtualShadowFeedbackStats xiiVirtualShadowFeedbackReadbackPass::GetStats() const { return m_State ? m_State->m_Stats : xiiVirtualShadowFeedbackStats(); }
xiiResult                     xiiVirtualShadowFeedbackReadbackPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  input       = context.GetInputBuffer(m_Requests);
  const auto* description = context.m_Graph.GetBufferDescription(input);
  if (!description || description->m_Mode != xiiGALBufferMode::Structured || description->m_uiElementByteStride != sizeof(xiiGpuVirtualShadowFeedback) ||
      description->m_uiSize % sizeof(xiiGpuVirtualShadowFeedback) || description->m_uiSize < 2 * sizeof(xiiGpuVirtualShadowFeedback) || description->m_uiSize > 1048577ULL * sizeof(xiiGpuVirtualShadowFeedback))
    return context.ReportFailure("Shadow feedback readback requires a bounded structured append stream plus header.");
  xiiUInt64 completed = 0;
  if (!xiiVirtualShadowMapManager::IsInitialized() || !xiiGALDevice::HasDefaultDevice() ||
      xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Shadow feedback readback requires configured device residency and an active shared GPU frame.");
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
      return context.ReportFailure("Could not allocate shadow feedback staging storage.");
  }
  if (!slot->m_Fence)
    slot->m_Fence = device->CreateFence({});
  if (!slot->m_Fence)
    return context.ReportFailure("Could not allocate shadow feedback completion fence.");
  ++slot->m_uiValue;
  slot->m_uiFrame                 = context.m_Graph.GetFrameIndex();
  slot->m_uiConfigurationRevision = xiiVirtualShadowMapManager::GetConfigurationRevision();
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Input, m_Output;
    xiiSharedPtr<State::Slot>  m_Slot;
    xiiUInt64                  m_uiValue;
  };
  context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Input = builder.ReadBuffer(input, xiiGALResourceStateFlags::CopySource);
    data.m_Output = builder.WriteBuffer(builder.ImportBuffer("Shadow Request Readback", slot->m_Buffer, slot->m_Buffer->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
    builder.ExportBuffer(data.m_Output, xiiGALResourceStateFlags::CopyDestination);
    data.m_Slot = slot; data.m_uiValue = slot->m_uiValue; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& cmd = execute.GetCommandList();
    cmd.CopyBuffer(execute.GetBuffer(data.m_Input), execute.GetBuffer(data.m_Output));
    cmd.EnqueueSignal(data.m_Slot->m_Fence.Borrow(), data.m_uiValue);
    data.m_Slot->m_bPending = true; }, true);
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_VirtualShadowFeedbackReadbackPass);
