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
  xiiUInt64             m_uiRevision = 0U;
  bool                  m_bValid     = false;
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
    XII_MEMBER_PROPERTY("AllowAbsentTexture", m_bAllowAbsentTexture)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("AsyncCopy", m_bAsyncCopy)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Temporal") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
void xiiTextureHistoryPass::InvalidateTemporalHistory()
{
  if (m_pState)
  {
    ++m_pState->m_uiRevision;
    m_pState->m_bValid = false;
  }
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
void xiiTextureHistoryPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  const auto first = pins.GetCount();
  xiiRenderPipelinePass::GetPins(pins);
  if (m_bAllowAbsentTexture)
    for (xiiUInt32 i = first; i < pins.GetCount(); ++i)
      if (pins[i].m_bInput && pins[i].m_sName == "Texture")
        pins[i].m_bOptional = true;
}

xiiResult xiiTextureHistoryPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto input = context.GetInputTexture(m_Input);
  if (m_bAllowAbsentTexture && !input.IsValid())
  {
    InvalidateTemporalHistory();
    return XII_SUCCESS;
  }
  const auto* description = context.m_Graph.GetTextureDescription(input);
  if (!description || !description->m_Size.width || !description->m_Size.height || description->m_uiSampleCount != 1 || !description->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || description->m_Usage == xiiGALResourceUsage::Sparse)
    return context.ReportFailure("Texture history requires a nonempty, single-sample, nonsparse shader-resource texture.");
  auto allocation             = *description;
  allocation.m_BindFlags      = xiiGALBindFlags::ShaderResource;
  allocation.m_Usage          = xiiGALResourceUsage::Default;
  allocation.m_CPUAccessFlags = xiiGALCPUAccessFlag::None;
  allocation.m_MiscFlags      = {};
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle m_Input, m_Output;
    State::Slot                 m_Next;
    xiiUInt64                   m_uiRevision = 0U;
  };
  context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCopy ? xiiGALCommandQueueFlags::Transfer : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Input = builder.ReadTexture(input, xiiGALResourceStateFlags::CopySource);
      data.m_Output = builder.WriteTexture(builder.DeclareImportedTexture("Retained Frame", allocation), xiiGALResourceStateFlags::CopyDestination);
      builder.ExportTexture(data.m_Output, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassFramePreparation([state = m_pState, allocation, &data](xiiRenderGraph& graph, const xiiView*, xiiStringBuilder& out_sError) -> xiiResult {
        const xiiUInt64 uiFrame     = graph.GetFrameIndex();
        xiiUInt64       uiCompleted = 0;
        if (xiiRenderGraphManager::GetFrameCompletion(uiFrame, uiCompleted).Failed())
        {
          out_sError = "Texture history requires an active shared GPU frame.";
          return XII_FAILURE;
        }

        State::Slot* pAvailable = nullptr;
        for (auto& slot : state->m_Slots)
          if (slot.m_Texture != state->m_Last.m_Texture && slot.m_uiLastUsedFrame <= uiCompleted)
          {
            pAvailable = &slot;
            break;
          }

        State::Slot next = pAvailable ? *pAvailable : State::Slot{};
        if (!next.m_Texture || next.m_Texture->GetDescription() != allocation)
        {
          next.m_Texture = xiiGALDevice::GetDefaultDevice()->CreateTexture(allocation);
          if (!next.m_Texture)
          {
            out_sError = "Could not allocate texture history storage.";
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
            out_sError = "Could not allocate texture history readiness.";
            return XII_FAILURE;
          }
        }
        if (graph.BindImportedTexture(data.m_Output, next.m_Texture, next.m_Texture->GetResourceState()).Failed())
        {
          out_sError = "Could not bind texture history storage.";
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
      commands.EnqueueSignal(data.m_Next.m_Fence.Borrow(), data.m_Next.m_uiValue); }, true);
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Temporal_TextureHistoryPass);
