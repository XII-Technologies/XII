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
  xiiSharedPtr<xiiGALBuffer>          m_pIdleDestination;
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
  if (!description || description->m_Mode != xiiGALBufferMode::Structured || description->m_uiElementByteStride != sizeof(xiiGpuVirtualShadowFeedback) || description->m_uiSize % sizeof(xiiGpuVirtualShadowFeedback) || description->m_uiSize < 2 * sizeof(xiiGpuVirtualShadowFeedback) || description->m_uiSize > 1048577ULL * sizeof(xiiGpuVirtualShadowFeedback))
    return context.ReportFailure("Shadow feedback readback requires a bounded structured append stream plus header.");
  if (!m_State)
    m_State = XII_DEFAULT_NEW(State);

  xiiGALBufferCreationDescription staging;
  staging.m_uiSize         = description->m_uiSize;
  staging.m_Usage          = xiiGALResourceUsage::Staging;
  staging.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;

  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Input, m_Output;
    xiiSharedPtr<State::Slot>  m_pSlot;
    xiiUInt64                  m_uiValue = 0U;
  };
  context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Input = builder.ReadBuffer(input, xiiGALResourceStateFlags::CopySource);
    data.m_Output = builder.WriteBuffer(builder.DeclareImportedBuffer("Shadow Request Readback", staging), xiiGALResourceStateFlags::CopyDestination);
    builder.ExportBuffer(data.m_Output, xiiGALResourceStateFlags::CopyDestination);
    builder.SetPassFramePreparation([state = m_State, staging, uiSlotLimit = m_uiMaxPendingReadbacks, &data](xiiRenderGraph& ref_graph, const xiiView*, xiiStringBuilder& out_sError) -> xiiResult {
      const xiiUInt64 uiFrame = ref_graph.GetFrameIndex();
      xiiUInt64 uiCompleted = 0;
      if (!xiiVirtualShadowMapManager::IsInitialized() || !xiiGALDevice::HasDefaultDevice() || xiiRenderGraphManager::GetFrameCompletion(uiFrame, uiCompleted).Failed())
      {
        out_sError = "Shadow feedback readback requires configured device residency and an active shared GPU frame.";
        return XII_FAILURE;
      }

      const auto pDevice = xiiGALDevice::GetDefaultDevice();
      state->Consume(*pDevice, uiCompleted);
      data.m_pSlot.Clear();
      for (const auto& pSlot : state->m_Slots)
        if (!pSlot->m_bPending && pSlot->m_uiFrame <= uiCompleted)
        {
          data.m_pSlot = pSlot;
          break;
        }
      if (!data.m_pSlot && state->m_Slots.GetCount() < uiSlotLimit)
      {
        data.m_pSlot = XII_DEFAULT_NEW(State::Slot);
        state->m_Slots.PushBack(data.m_pSlot);
      }

      // Keep the declared import bound while the readback ring applies backpressure.
      // This destination receives no copy and cannot alias an in-flight readback slot.
      auto& pDestination = data.m_pSlot ? data.m_pSlot->m_Buffer : state->m_pIdleDestination;
      if (!pDestination || pDestination->GetDescription() != staging)
        pDestination = pDevice->CreateBuffer(staging);
      if (!pDestination || ref_graph.BindImportedBuffer(data.m_Output, pDestination, pDestination->GetResourceState()).Failed())
      {
        out_sError = "Could not bind the declared shadow feedback staging storage.";
        return XII_FAILURE;
      }
      if (!data.m_pSlot)
      {
        ++state->m_Stats.m_uiDroppedReadbacks;
        return XII_SUCCESS;
      }

      auto& slot = *data.m_pSlot;
      if (!slot.m_Fence)
        slot.m_Fence = pDevice->CreateFence({});
      if (!slot.m_Fence)
      {
        out_sError = "Could not allocate shadow feedback completion fence.";
        return XII_FAILURE;
      }
      data.m_uiValue = ++slot.m_uiValue;
      slot.m_uiFrame = uiFrame;
      slot.m_uiConfigurationRevision = xiiVirtualShadowMapManager::GetConfigurationRevision();
      return XII_SUCCESS;
    });
    builder.SetPassSubmissionCompletion([&data] {
      if (data.m_pSlot)
        data.m_pSlot->m_bPending = true;
    }); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    if (!data.m_pSlot)
      return;
    auto& commands = execute.GetCommandList();
    commands.CopyBuffer(execute.GetBuffer(data.m_Input), execute.GetBuffer(data.m_Output));
    commands.EnqueueSignal(data.m_pSlot->m_Fence.Borrow(), data.m_uiValue); }, true);
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_VirtualShadowFeedbackReadbackPass);
