/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TextureHistoryPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

struct xiiTextureHistoryPass::State : public xiiRefCounted
{
  struct Slot
  {
    xiiSharedPtr<xiiGALTexture> m_Texture;
    xiiSharedPtr<xiiGALFence>   m_Fence;
    xiiUInt64                   m_uiValue = 0, m_uiLastUsedFrame = 0;
  };
  xiiDynamicArray<Slot> m_Slots;
  Slot                  m_Last;
  bool                  m_bValid = false;
};
xiiTextureHistoryPass::xiiTextureHistoryPass()  = default;
xiiTextureHistoryPass::~xiiTextureHistoryPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureHistoryPass, 1, xiiRTTIDefaultAllocator<xiiTextureHistoryPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Texture", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousTexture", m_Previous)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Temporal") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
void xiiTextureHistoryPass::InvalidateTemporalHistory()
{
  if (m_pState)
    m_pState->m_bValid = false;
}
xiiResult xiiTextureHistoryPass::BuildTemporalHistory(xiiRenderPipelinePassContext& context)
{
  if (!m_pState || !m_pState->m_bValid || context.m_Graph.GetFrameIndex() <= m_pState->m_Last.m_uiLastUsedFrame)
    return XII_SUCCESS;
  const auto last = m_pState->m_Last;
  for (auto& slot : m_pState->m_Slots)
    if (slot.m_Texture == last.m_Texture)
      slot.m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope scope(context);
  const auto                 emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>("Publish Texture History", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
      builder.WaitForFence(last.m_Fence, last.m_uiValue).AssertSuccess();
      data = builder.ReadWriteTexture(builder.ImportTexture("Previous Frame", last.m_Texture, last.m_Texture->GetResourceState()), xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {});
  return context.SetOutput(m_Previous, *emitted.first);
}
xiiResult xiiTextureHistoryPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  input       = context.GetInputTexture(m_Input);
  const auto* description = context.m_Graph.GetTextureDescription(input);
  if (!description || !description->m_Size.width || !description->m_Size.height || description->m_uiSampleCount != 1 ||
      !description->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || description->m_Usage == xiiGALResourceUsage::Sparse)
    return context.ReportFailure("Texture history requires a nonempty, single-sample, nonsparse shader-resource texture.");
  auto allocation             = *description;
  allocation.m_BindFlags      = xiiGALBindFlags::ShaderResource;
  allocation.m_Usage          = xiiGALResourceUsage::Default;
  allocation.m_CPUAccessFlags = xiiGALCPUAccessFlag::None;
  allocation.m_MiscFlags      = {};
  xiiUInt64 completed         = 0;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Texture history requires an active shared GPU frame.");
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  auto&     state    = *m_pState;
  xiiUInt32 selected = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < state.m_Slots.GetCount(); ++i)
    if (state.m_Slots[i].m_Texture != state.m_Last.m_Texture && state.m_Slots[i].m_uiLastUsedFrame <= completed)
    {
      selected = i;
      break;
    }
  State::Slot next;
  if (selected != xiiInvalidIndex)
    next = state.m_Slots[selected];
  if (!next.m_Texture || next.m_Texture->GetDescription() != allocation)
  {
    next.m_Texture = xiiGALDevice::GetDefaultDevice()->CreateTexture(allocation);
    if (!next.m_Texture)
      return context.ReportFailure("Could not allocate texture history storage.");
  }
  if (!next.m_Fence)
  {
    xiiGALFenceCreationDescription fence;
    fence.m_Type = xiiGALFenceType::General;
    next.m_Fence = xiiGALDevice::GetDefaultDevice()->CreateFence(fence);
    if (!next.m_Fence)
      return context.ReportFailure("Could not allocate texture history readiness.");
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
    xiiRenderGraphTextureHandle m_Input, m_Output;
    State::Slot                 m_Next;
  };
  context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Input = builder.ReadTexture(input, xiiGALResourceStateFlags::CopySource);
      data.m_Output = builder.WriteTexture(builder.ImportTexture("Retained Frame", next.m_Texture, next.m_Texture->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
      builder.ExportTexture(data.m_Output, xiiGALResourceStateFlags::ShaderResource);
      data.m_Next = next; }, [state = m_pState](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      auto* source = execute.GetTexture(data.m_Input);
      auto* destination = execute.GetTexture(data.m_Output);
      const auto& description = source->GetDescription();
      // CopyTexture does not cover every mip/slice on every backend. Name each subresource explicitly.
      for (xiiUInt32 slice = 0; slice < (description.IsArray() ? description.m_uiArraySizeOrDepth : 1U); ++slice)
        for (xiiUInt32 mip = 0; mip < description.m_uiMipLevels; ++mip)
        {
          const auto size = xiiGALTextureUtilities::GetMipLevelProperties(description, mip);
          xiiGALTextureMipLevelData subresource;
          subresource.m_uiMipLevel = mip;
          subresource.m_uiArraySlice = slice;
          const auto box = xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), {size.m_LogicalSize.width, xiiMath::Max(size.m_LogicalSize.height, 1U), xiiMath::Max(size.m_uiDepth, 1U)});
          commands.CopyTextureRegion(source, subresource, box, destination, subresource, xiiVec3U32::MakeZero());
        }
      commands.EnqueueSignal(data.m_Next.m_Fence.Borrow(), data.m_Next.m_uiValue);
      state->m_Last = data.m_Next;
      state->m_bValid = true; }, true);
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Temporal_TextureHistoryPass);
