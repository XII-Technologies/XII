/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereMultiScatterPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereMultiScatterPass,1,xiiRTTIDefaultAllocator<xiiAtmosphereMultiScatterPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Atmosphere",m_Atmosphere)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Transmittance",m_Transmittance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MultiScatter",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Width",m_uiWidth)->AddAttributes(new xiiDefaultValueAttribute(32U),new xiiClampValueAttribute(1U,4096U)),
    XII_MEMBER_PROPERTY("Height",m_uiHeight)->AddAttributes(new xiiDefaultValueAttribute(32U),new xiiClampValueAttribute(1U,4096U)),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Atmosphere")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiAtmosphereMultiScatterPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiWidth > 0 && m_uiWidth <= 4096 && m_uiHeight > 0 && m_uiHeight <= 4096 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiAtmosphereMultiScatterPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto atmosphere = context.GetInputBuffer(m_Atmosphere);
  if (!xiiScreenSpacePassUtils::Uniform(context, atmosphere, sizeof(xiiAtmosphereConstants)))
    return context.ReportFailure("AtmosphereMultiScatter requires exact atmosphere constants.");
  const auto transmittance = context.GetInputTexture(m_Transmittance);
  if (!xiiScreenSpacePassUtils::Image(context, transmittance, 3))
    return context.ReportFailure("AtmosphereMultiScatter requires a nonempty float-readable RGB transmittance LUT.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Atmosphere/AtmosphereMultiScatter.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_Linear));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle               m_Atmosphere;
    xiiRenderGraphTextureHandle              m_Output;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiRenderGraphTextureHandle              m_Transmittance;
    xiiSharedPtr<xiiGALSampler>              m_Linear;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Atmosphere=builder.ReadBuffer(atmosphere,xiiGALResourceStateFlags::ConstantBuffer);data.m_Size={m_uiWidth,m_uiHeight};data.m_Output=xiiCompositionPassUtils::Output(builder,data.m_Size,xiiGALResourceFormat::RGBA32Float);data.m_Pipeline=m_Pipeline;
    data.m_Transmittance=builder.ReadTexture(transmittance,xiiGALResourceStateFlags::ShaderResource);data.m_Linear=m_Linear; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& cmd=execute.GetCommandList();cmd.SetPipelineState(data.m_Pipeline.Borrow());cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants",execute.GetBuffer(data.m_Atmosphere),xiiGALShaderType::Compute);
    cmd.ResolveAndSetSampler("LinearClampSampler",data.m_Linear.Borrow());cmd.ResolveAndSetShaderResourceTextureView("g_TransmittanceLUT",execute.GetTexture(data.m_Transmittance)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_MultiScatterLUT",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();cmd.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Atmosphere_AtmosphereMultiScatterPass);
