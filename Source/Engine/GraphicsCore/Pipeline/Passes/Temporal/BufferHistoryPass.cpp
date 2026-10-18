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
  bool                  m_bValid = false;
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
    m_pState->m_bValid = false;
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
  xiiUInt64 completed = 0;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Buffer history requires an active shared GPU frame.");
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  auto&     state    = *m_pState;
  xiiUInt32 selected = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < state.m_Slots.GetCount(); ++i)
    if (state.m_Slots[i].m_Buffer != state.m_Last.m_Buffer && state.m_Slots[i].m_uiLastUsedFrame <= completed)
    {
      selected = i;
      break;
    }
  State::Slot next;
  if (selected != xiiInvalidIndex)
    next = state.m_Slots[selected];
  if (!next.m_Buffer || next.m_Buffer->GetDescription().m_uiSize != description->m_uiSize || next.m_Buffer->GetDescription().m_uiElementByteStride != description->m_uiElementByteStride)
  {
    auto allocation             = *description;
    allocation.m_BindFlags      = xiiGALBindFlags::ShaderResource;
    allocation.m_Usage          = xiiGALResourceUsage::Mutable;
    allocation.m_CPUAccessFlags = xiiGALCPUAccessFlag::None;
    next.m_Buffer               = xiiGALDevice::GetDefaultDevice()->CreateBuffer(allocation);
    if (!next.m_Buffer)
      return context.ReportFailure("Could not allocate buffer history storage.");
  }
  if (!next.m_Fence)
  {
    xiiGALFenceCreationDescription fence;
    fence.m_Type = xiiGALFenceType::General;
    next.m_Fence = xiiGALDevice::GetDefaultDevice()->CreateFence(fence);
    if (!next.m_Fence)
      return context.ReportFailure("Could not allocate buffer history readiness.");
  }
  ++next.m_uiValue;
  next.m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  if (selected == xiiInvalidIndex)
    state.m_Slots.PushBack(next);
  else
    state.m_Slots[selected] = next;
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Input, m_Output;
    State::Slot                m_Next;
  };
  context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCopy ? xiiGALCommandQueueFlags::Transfer : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Input = builder.ReadBuffer(input, xiiGALResourceStateFlags::CopySource);
      data.m_Output = builder.WriteBuffer(builder.ImportBuffer("Retained Frame", next.m_Buffer, next.m_Buffer->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_Output, xiiGALResourceStateFlags::ShaderResource);
      data.m_Next = next; }, [state = m_pState](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.CopyBuffer(execute.GetBuffer(data.m_Input), execute.GetBuffer(data.m_Output));
      commands.EnqueueSignal(data.m_Next.m_Fence.Borrow(), data.m_Next.m_uiValue);
      state->m_Last = data.m_Next;
      state->m_bValid = true; }, true);
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Temporal_BufferHistoryPass);
