/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Transparency/WeightedBlendedOITResolvePass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Texture.h>

struct xiiWeightedBlendedOITResolvePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipelines[2];
};
xiiWeightedBlendedOITResolvePass::xiiWeightedBlendedOITResolvePass() { m_OutputHDR.SetResolution({xiiRenderPipelineTextureResolutionMode::Input}); }
xiiWeightedBlendedOITResolvePass::~xiiWeightedBlendedOITResolvePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWeightedBlendedOITResolvePass, 1, xiiRTTIDefaultAllocator<xiiWeightedBlendedOITResolvePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("OITAccumulate", m_InputAccumulation)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("Bypass")),
    XII_MEMBER_PROPERTY("OITReveal", m_InputRevealage)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinOptionalAttribute("Bypass")),
    XII_MEMBER_PROPERTY("HDRSceneColorInput", m_InputHDR)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HDRSceneColor")),
    XII_MEMBER_PROPERTY("HDRSceneColorOutput", m_OutputHDR)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HDRSceneColor")),
    XII_MEMBER_PROPERTY("AllowAbsentAccumulation", m_bAllowAbsentAccumulation)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Bypass", m_bBypass)->AddAttributes(new xiiDefaultValueAttribute(false)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Transparency") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
void xiiWeightedBlendedOITResolvePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
    if (out_pins[i].m_bInput && (out_pins[i].m_sName == "OITAccumulate" || out_pins[i].m_sName == "OITReveal"))
      out_pins[i].m_bOptional = m_bBypass || m_bAllowAbsentAccumulation;
}

xiiResult xiiWeightedBlendedOITResolvePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  hdr            = context.GetInputTexture(m_InputHDR);
  const auto* hdrDescription = xiiScreenSpacePassUtils::Image(context, hdr, 4);
  if (!hdrDescription || (hdrDescription->m_Format != xiiGALResourceFormat::RGBA16Float && hdrDescription->m_Format != xiiGALResourceFormat::RGBA32Float))
    return context.ReportFailure("Weighted transparency requires readable single-sample RGBA16Float or RGBA32Float HDR.");
  if (m_bBypass)
    return context.SetOutput(m_OutputHDR, hdr);
  const auto accumulation = context.GetInputTexture(m_InputAccumulation), revealage = context.GetInputTexture(m_InputRevealage);
  if (m_bAllowAbsentAccumulation && !accumulation.IsValid() && !revealage.IsValid())
    return context.SetOutput(m_OutputHDR, hdr);
  const auto* accumulationDescription = xiiScreenSpacePassUtils::Image(context, accumulation, 4, &hdrDescription->m_Size);
  const auto* revealageDescription    = xiiScreenSpacePassUtils::Image(context, revealage, 1, &hdrDescription->m_Size);
  if (!accumulationDescription || (accumulationDescription->m_Format != xiiGALResourceFormat::RGBA16Float && accumulationDescription->m_Format != xiiGALResourceFormat::RGBA32Float) ||
      !revealageDescription || (revealageDescription->m_Format != xiiGALResourceFormat::R8UNormalized && revealageDescription->m_Format != xiiGALResourceFormat::R16Float && revealageDescription->m_Format != xiiGALResourceFormat::R32Float))
    return context.ReportFailure("Weighted transparency requires matching readable floating RGBA accumulation and R8UNormalized, R16Float or R32Float revealage; an incomplete pair is invalid.");
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_OutputHDR, hdr, size));
  if (size != hdrDescription->m_Size)
    return context.ReportFailure("Weighted transparency requires input-sized output.");
  const bool fullPrecision = hdrDescription->m_Format == xiiGALResourceFormat::RGBA32Float;
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  auto& pipeline = m_pState->m_Pipelines[fullPrecision ? 1 : 0];
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(pipeline, fullPrecision ? "Shaders/Pipeline/OITResolve32.xiiShader" : "Shaders/Pipeline/OITResolve.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_HDR, m_Accumulation, m_Revealage, m_Output;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_HDR=builder.ReadTexture(hdr,xiiGALResourceStateFlags::ShaderResource);
    data.m_Accumulation=builder.ReadTexture(accumulation,xiiGALResourceStateFlags::ShaderResource);
    data.m_Revealage=builder.ReadTexture(revealage,xiiGALResourceStateFlags::ShaderResource);
    xiiGALTextureCreationDescription output;output.m_Type=xiiGALResourceDimension::Texture2D;output.m_Size=size;output.m_Format=hdrDescription->m_Format;output.m_BindFlags=xiiGALBindFlags::ShaderResource|xiiGALBindFlags::UnorderedAccess;
    data.m_Output=builder.WriteTexture("Composed transparency",output,xiiGALResourceStateFlags::UnorderedAccess);data.m_Size=size;data.m_Pipeline=pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetShaderResourceTextureView("g_HDRBase",execute.GetTexture(data.m_HDR)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_OITAccum",execute.GetTexture(data.m_Accumulation)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_OITReveal",execute.GetTexture(data.m_Revealage)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_HDROut",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    if(commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()){execute.ReportFailure("Could not bind weighted transparency resolve resources.");return;}
    commands.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_OutputHDR, emitted.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Transparency_WeightedBlendedOITResolvePass);
