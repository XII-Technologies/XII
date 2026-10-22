/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Transparency/WeightedBlendedOITResolvePass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Texture.h>

struct xiiWeightedBlendedOITResolvePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
xiiWeightedBlendedOITResolvePass::xiiWeightedBlendedOITResolvePass()  = default;
xiiWeightedBlendedOITResolvePass::~xiiWeightedBlendedOITResolvePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWeightedBlendedOITResolvePass, 1, xiiRTTIDefaultAllocator<xiiWeightedBlendedOITResolvePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OITAccumulate", m_InputAccumulation)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("Bypass")),
    XII_MEMBER_PROPERTY("OITReveal", m_InputRevealage)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("Bypass")),
    XII_MEMBER_PROPERTY("HDRSceneColorInput", m_InputHDR)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HDRSceneColor")),
    XII_MEMBER_PROPERTY("HDRSceneColorOutput", m_OutputHDR)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HDRSceneColor")),
    XII_MEMBER_PROPERTY("Bypass", m_bBypass)->AddAttributes(new xiiDefaultValueAttribute(false)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Transparency") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiWeightedBlendedOITResolvePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto hdr = context.GetInputTexture(m_InputHDR);
  if (m_bBypass)
    return context.SetOutput(m_OutputHDR, hdr);
  const auto  accumulation            = context.GetInputTexture(m_InputAccumulation);
  const auto  revealage               = context.GetInputTexture(m_InputRevealage);
  const auto* hdrDescription          = context.m_Graph.GetTextureDescription(hdr);
  const auto* accumulationDescription = context.m_Graph.GetTextureDescription(accumulation);
  const auto* revealageDescription    = context.m_Graph.GetTextureDescription(revealage);
  if (!hdrDescription || !accumulationDescription || !revealageDescription || hdrDescription->m_Size != accumulationDescription->m_Size || hdrDescription->m_Size != revealageDescription->m_Size)
    return XII_FAILURE;
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/OITResolve.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle m_HDR, m_Accumulation, m_Revealage;
    xiiSizeU32                  m_Size;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_HDR = builder.WriteTexture(builder.ReadTexture(hdr, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Accumulation = builder.ReadTexture(accumulation, xiiGALResourceStateFlags::ShaderResource);
    data.m_Revealage = builder.ReadTexture(revealage, xiiGALResourceStateFlags::ShaderResource);
    data.m_Size = hdrDescription->m_Size; }, [this](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    commands.SetPipelineState(m_pState->m_Pipeline);
    commands.ResolveAndSetShaderResourceTextureView("g_OITAccum", execute.GetTexture(data.m_Accumulation)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_OITReveal", execute.GetTexture(data.m_Revealage)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_HDROut", execute.GetTexture(data.m_HDR)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    commands.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U}); });
  return context.SetOutput(m_OutputHDR, emitted.first->m_HDR);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Transparency_WeightedBlendedOITResolvePass);
