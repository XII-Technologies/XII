/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionTemporalPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Denoising/TemporalDenoiseConstants.h>

struct xiiAmbientOcclusionTemporalPass::State
{
  struct Slot
  {
    xiiSharedPtr<xiiGALTexture> m_Mask, m_Surface;
    xiiSharedPtr<xiiGALFence>   m_ReadyFence;
    xiiUInt64                   m_uiReadyValue    = 0;
    xiiUInt64                   m_uiLastUsedFrame = 0;
  };
  xiiDynamicArray<Slot>                    m_Slots;
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALTexture>              m_LastMask, m_LastSurface;
  xiiSharedPtr<xiiGALFence>                m_LastFence;
  xiiUInt64                                m_uiLastReadyValue = 0;
  bool                                     m_bHistoryValid    = false;
};
xiiAmbientOcclusionTemporalPass::xiiAmbientOcclusionTemporalPass()  = default;
xiiAmbientOcclusionTemporalPass::~xiiAmbientOcclusionTemporalPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAmbientOcclusionTemporalPass, 1, xiiRTTIDefaultAllocator<xiiAmbientOcclusionTemporalPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RawAO", m_Raw)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferNormal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DilatedVelocityBuffer", m_Velocity)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameCamera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PerFrameGlobal", m_Global)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("StableAO", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PreviousAO", m_Previous)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Surface", m_Surface)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("HistoryWeight", m_fHistoryWeight)->AddAttributes(new xiiDefaultValueAttribute(0.92f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("DepthThreshold", m_fDepthThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.08f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("NormalThreshold", m_fNormalThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.82f), new xiiClampValueAttribute(-1.0f, 1.0f)),
    XII_MEMBER_PROPERTY("SpatialWeight", m_fSpatialWeight)->AddAttributes(new xiiDefaultValueAttribute(0.35f), new xiiClampValueAttribute(0.0f, 1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/AmbientOcclusion") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiAmbientOcclusionTemporalPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (!xiiMath::IsFinite(m_fHistoryWeight) || m_fHistoryWeight < 0 || m_fHistoryWeight > 1 || !xiiMath::IsFinite(m_fDepthThreshold) || m_fDepthThreshold < 0 || m_fDepthThreshold > 1 || !xiiMath::IsFinite(m_fNormalThreshold) || m_fNormalThreshold < -1 || m_fNormalThreshold > 1 || !xiiMath::IsFinite(m_fSpatialWeight) || m_fSpatialWeight < 0 || m_fSpatialWeight > 1)
    return XII_FAILURE;
  return XII_SUCCESS;
}
void xiiAmbientOcclusionTemporalPass::InvalidateTemporalHistory()
{
  if (m_pState)
    m_pState->m_bHistoryValid = false;
}
xiiResult xiiAmbientOcclusionTemporalPass::BuildTemporalHistory(xiiRenderPipelinePassContext& context)
{
  if (!m_pState || !m_pState->m_bHistoryValid)
    return XII_SUCCESS;
  xiiUInt64 completed;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("AO history requires an active shared GPU frame.");
  // Temporal outputs may be read by arbitrary downstream queues, even during resize.
  for (auto& slot : m_pState->m_Slots)
    if (slot.m_Mask == m_pState->m_LastMask)
      slot.m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope  scope(context);
  xiiRenderGraphTextureHandle previous;
  context.m_Graph.AddPass<xiiRenderGraphTextureHandle>("Publish AO History", xiiGALCommandQueueFlags::Compute, [&](auto& data, xiiRenderGraphBuilder& builder) {
    builder.WaitForFence(m_pState->m_LastFence, m_pState->m_uiLastReadyValue).AssertSuccess();
    // Publish a preserved version after external readiness so every consumer queue
    // depends on this operation even when no layout transition is necessary.
    data = builder.ReadWriteTexture(builder.ImportTexture("Previous AO", m_pState->m_LastMask, m_pState->m_LastMask->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
    previous = data; }, [](const auto&, xiiRenderGraphPassContext&) {});
  return context.SetOutput(m_Previous, previous);
}
xiiResult xiiAmbientOcclusionTemporalPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto raw = context.GetInputTexture(m_Raw), depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal), velocity = context.GetInputTexture(m_Velocity);
  const auto camera = context.GetInputBuffer(m_Camera), global = context.GetInputBuffer(m_Global);
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_Output, raw, size));
  const auto* sizingInput = context.m_Graph.GetTextureDescription(raw);
  if (!sizingInput || sizingInput->m_Size != size)
    return context.ReportFailure("This operation requires input-sized output; resample its inputs before processing at another resolution.");
  for (auto handle : {depth, normal, velocity})
  {
    const auto* description = context.m_Graph.GetTextureDescription(handle);
    if (!description || description->m_Size != size)
      return context.ReportFailure("AO temporal inputs must have matching extents.");
  }
  const auto* cameraDescription = context.m_Graph.GetBufferDescription(camera);
  const auto* globalDescription = context.m_Graph.GetBufferDescription(global);
  if (!cameraDescription || cameraDescription->m_uiSize < sizeof(xiiFrameCameraConstants) || !globalDescription || globalDescription->m_uiSize < sizeof(xiiFrameGlobalConstants))
    return context.ReportFailure("AO temporal frame constant layouts are invalid.");
  xiiUInt64 completed;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("AO temporal filtering requires an active shared GPU frame.");
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  auto& state = *m_pState;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(state.m_Pipeline, "Shaders/Pipeline/AmbientOcclusionTemporal.xiiShader"));
  const bool historyValid = state.m_bHistoryValid && state.m_LastMask && state.m_LastMask->GetDescription().m_Size == size;
  xiiUInt32  selected     = xiiInvalidIndex;
  for (xiiUInt32 i = 0; i < state.m_Slots.GetCount(); ++i)
    if (state.m_Slots[i].m_Mask != state.m_LastMask && state.m_Slots[i].m_uiLastUsedFrame <= completed)
    {
      selected = i;
      break;
    }
  if (selected == xiiInvalidIndex)
    selected = state.m_Slots.GetCount();
  State::Slot next;
  if (selected < state.m_Slots.GetCount())
    next = state.m_Slots[selected];
  if (!next.m_Mask || next.m_Mask->GetDescription().m_Size != size)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = size;
    description.m_Format    = xiiGALResourceFormat::R8UNormalized;
    description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    next.m_Mask             = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    description.m_Format    = xiiGALResourceFormat::RGBA16Float;
    next.m_Surface          = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    if (!next.m_Mask || !next.m_Surface)
      return context.ReportFailure("Could not allocate AO history images.");
  }
  if (!next.m_ReadyFence)
  {
    xiiGALFenceCreationDescription description;
    description.m_Type = xiiGALFenceType::General;
    next.m_ReadyFence  = xiiGALDevice::GetDefaultDevice()->CreateFence(description);
    if (!next.m_ReadyFence)
      return context.ReportFailure("Could not allocate AO history readiness fence.");
  }
  ++next.m_uiReadyValue;
  next.m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  if (selected == state.m_Slots.GetCount())
    state.m_Slots.PushBack(next);
  else
    state.m_Slots[selected] = next;
  if (historyValid)
    for (auto& slot : state.m_Slots)
      if (slot.m_Mask == state.m_LastMask)
        slot.m_uiLastUsedFrame = context.m_Graph.GetFrameIndex();
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle m_Raw, m_Depth, m_Normal, m_Velocity, m_Previous, m_PreviousSurface, m_Output, m_Surface;
    xiiRenderGraphBufferHandle  m_Camera, m_Global, m_Constants;
    xiiSharedPtr<xiiGALTexture> m_MaskImage, m_SurfaceImage;
    xiiSharedPtr<xiiGALFence>   m_ReadyFence;
    xiiUInt64                   m_uiReadyValue;
    xiiTemporalDenoiseConstants m_Parameters;
    xiiSizeU32                  m_Size;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_ReadyFence = next.m_ReadyFence;
    data.m_uiReadyValue = next.m_uiReadyValue;
    if (historyValid) builder.WaitForFence(state.m_LastFence, state.m_uiLastReadyValue).AssertSuccess();
    data.m_Raw = builder.ReadTexture(raw, xiiGALResourceStateFlags::ShaderResource);
    data.m_Depth = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Velocity = builder.ReadTexture(velocity, xiiGALResourceStateFlags::ShaderResource);
    // HistoryValid == 0 exits the shader before history reads. Bind initialized
    // current inputs for the first frame and after camera cuts or size changes.
    data.m_Previous = historyValid ? builder.ReadTexture(builder.ImportTexture("AO Previous Mask", state.m_LastMask, state.m_LastMask->GetResourceState()), xiiGALResourceStateFlags::ShaderResource) : data.m_Raw;
    data.m_PreviousSurface = historyValid ? builder.ReadTexture(builder.ImportTexture("AO Previous Surface", state.m_LastSurface, state.m_LastSurface->GetResourceState()), xiiGALResourceStateFlags::ShaderResource) : data.m_Normal;
    data.m_MaskImage = next.m_Mask;
    data.m_SurfaceImage = next.m_Surface;
    data.m_Output = builder.WriteTexture(builder.ImportTexture("AO Filtered", next.m_Mask, next.m_Mask->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Surface = builder.WriteTexture(builder.ImportTexture("AO Surface", next.m_Surface, next.m_Surface->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Camera = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Global = builder.ReadBuffer(global, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "AO Temporal Constants", sizeof(data.m_Parameters));
    data.m_Parameters.HistoryWeight = m_fHistoryWeight;
    data.m_Parameters.DepthThreshold = m_fDepthThreshold;
    data.m_Parameters.NormalThreshold = m_fNormalThreshold;
    data.m_Parameters.SpatialWeight = m_fSpatialWeight;
    data.m_Parameters.HistoryValid = historyValid ? 1U : 0U;
    data.m_Parameters.SignalMode = 0U;
    data.m_Parameters._Padding = xiiVec2::MakeZero(); }, [this](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> constants(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Parameters;
    }
    commands.SetPipelineState(m_pState->m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("PerFrameCamera", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("PerFrameGlobal", execute.GetBuffer(data.m_Global), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", execute.GetTexture(data.m_Raw)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", execute.GetTexture(data.m_Previous)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SceneDepth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Velocity", execute.GetTexture(data.m_Velocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", execute.GetTexture(data.m_PreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_CurrentSurface", execute.GetTexture(data.m_Surface)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    commands.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1});
    commands.EnqueueSignal(data.m_ReadyFence.Borrow(), data.m_uiReadyValue);
    m_pState->m_LastFence = data.m_ReadyFence;
    m_pState->m_uiLastReadyValue = data.m_uiReadyValue;
    m_pState->m_LastMask = data.m_MaskImage;
    m_pState->m_LastSurface = data.m_SurfaceImage;
    m_pState->m_bHistoryValid = true; });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Surface, emitted.first->m_Surface));
  return context.SetOutput(m_Output, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_AmbientOcclusion_AmbientOcclusionTemporalPass);
