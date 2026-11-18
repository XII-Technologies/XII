/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/BufferHistoryPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/Device/Device.h>

struct xiiBufferHistoryPass::State : public xiiRefCounted
{
  struct Slot
  {
    xiiSharedPtr<xiiGALBuffer> m_Buffer;
    xiiSharedPtr<xiiGALFence>  m_Fence;
    xiiUInt64                  m_uiValue = 0, m_uiLastUsedFrame = 0;
  };
  xiiDynamicArray<Slot> m_Slots;
  Slot                  m_Last;
  xiiUInt64             m_uiRevision = 0U;
  bool                  m_bValid     = false;
};
xiiBufferHistoryPass::xiiBufferHistoryPass()  = default;
xiiBufferHistoryPass::~xiiBufferHistoryPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBufferHistoryPass, 1, xiiRTTIDefaultAllocator<xiiBufferHistoryPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Buffer", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousBuffer", m_Previous)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCopy", m_bAsyncCopy)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Temporal") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
void xiiBufferHistoryPass::InvalidateTemporalHistory()
{
  if (m_pState)
  {
    ++m_pState->m_uiRevision;
    m_pState->m_bValid = false;
  }
}
xiiResult xiiBufferHistoryPass::BuildTemporalHistory(xiiRenderPipelinePassContext& context)
{
  if (!m_pState || !m_pState->m_bValid)
    return XII_SUCCESS;
  const auto last = m_pState->m_Last;
  for (auto& slot : m_pState->m_Slots)
    if (slot.m_Buffer == last.m_Buffer)
      slot.m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope scope(context);
  const auto                 emitted = context.m_Graph.AddPass<xiiRenderGraphBufferHandle>("Publish Buffer History", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
      builder.WaitForFence(last.m_Fence, last.m_uiValue).AssertSuccess();
      data = builder.ReadWriteBuffer(builder.ImportBuffer("Previous Frame", last.m_Buffer, last.m_Buffer->GetResourceState()), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {});
  return context.SetOutput(m_Previous, *emitted.first);
}
xiiResult xiiBufferHistoryPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  input       = context.GetInputBuffer(m_Input);
  const auto* description = context.m_Graph.GetBufferDescription(input);
  if (!description || description->m_uiSize == 0 || description->m_Mode != xiiGALBufferMode::Structured || description->m_uiElementByteStride == 0 || description->m_uiSize % description->m_uiElementByteStride != 0)
    return context.ReportFailure("Buffer history requires a nonempty structured shader-resource buffer.");
  auto allocation             = *description;
  allocation.m_BindFlags      = xiiGALBindFlags::ShaderResource;
  allocation.m_Usage          = xiiGALResourceUsage::Mutable;
  allocation.m_CPUAccessFlags = xiiGALCPUAccessFlag::None;
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Input, m_Output;
    State::Slot                m_Next;
    xiiUInt64                  m_uiRevision = 0U;
  };
  context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCopy ? xiiGALCommandQueueFlags::Transfer : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Input = builder.ReadBuffer(input, xiiGALResourceStateFlags::CopySource);
      data.m_Output = builder.WriteBuffer(builder.DeclareImportedBuffer("Retained Frame", allocation), xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_Output, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassFramePreparation([state = m_pState, allocation, &data](xiiRenderGraph& graph, const xiiView*, xiiStringBuilder& out_sError) -> xiiResult {
        const xiiUInt64 uiFrame     = graph.GetFrameIndex();
        xiiUInt64       uiCompleted = 0;
        if (xiiRenderGraphManager::GetFrameCompletion(uiFrame, uiCompleted).Failed())
        {
          out_sError = "Buffer history requires an active shared GPU frame.";
          return XII_FAILURE;
        }

        State::Slot* pAvailable = nullptr;
        for (auto& slot : state->m_Slots)
          if (slot.m_Buffer != state->m_Last.m_Buffer && slot.m_uiLastUsedFrame <= uiCompleted)
          {
            pAvailable = &slot;
            break;
          }

        State::Slot next = pAvailable ? *pAvailable : State::Slot{};
        if (!next.m_Buffer || next.m_Buffer->GetDescription() != allocation)
        {
          next.m_Buffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(allocation);
          if (!next.m_Buffer)
          {
            out_sError = "Could not allocate buffer history storage.";
            return XII_FAILURE;
          }
        }
        if (!next.m_Fence)
        {
          xiiGALFenceCreationDescription fence;
          fence.m_Type = xiiGALFenceType::General;
          next.m_Fence = xiiGALDevice::GetDefaultDevice()->CreateFence(fence);
          if (!next.m_Fence)
          {
            out_sError = "Could not allocate buffer history readiness.";
            return XII_FAILURE;
          }
        }
        if (graph.BindImportedBuffer(data.m_Output, next.m_Buffer, next.m_Buffer->GetResourceState()).Failed())
        {
          out_sError = "Could not bind buffer history storage.";
          return XII_FAILURE;
        }

        ++next.m_uiValue;
        next.m_uiLastUsedFrame = uiFrame;
        if (pAvailable)
          *pAvailable = next;
        else
          state->m_Slots.PushBack(next);
        data.m_Next       = std::move(next);
        data.m_uiRevision = state->m_uiRevision;
        return XII_SUCCESS;
      });
      builder.SetPassSubmissionCompletion([state = m_pState, &data] {
        if (state->m_uiRevision == data.m_uiRevision && (!state->m_bValid || state->m_Last.m_uiLastUsedFrame < data.m_Next.m_uiLastUsedFrame))
        {
          state->m_Last = data.m_Next;
          state->m_bValid = true;
        }
      }); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.CopyBuffer(execute.GetBuffer(data.m_Input), execute.GetBuffer(data.m_Output));
      commands.EnqueueSignal(data.m_Next.m_Fence.Borrow(), data.m_Next.m_uiValue); }, true);
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Temporal_BufferHistoryPass);
