/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereTransmittancePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereTransmittancePass,1,xiiRTTIDefaultAllocator<xiiAtmosphereTransmittancePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Atmosphere",m_Atmosphere)->AddFlags(xiiPropertyFlags::ReadOnly),

    XII_MEMBER_PROPERTY("Transmittance",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Width",m_uiWidth)->AddAttributes(new xiiDefaultValueAttribute(256U),new xiiClampValueAttribute(1U,4096U)),
    XII_MEMBER_PROPERTY("Height",m_uiHeight)->AddAttributes(new xiiDefaultValueAttribute(64U),new xiiClampValueAttribute(1U,4096U)),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Atmosphere")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiAtmosphereTransmittancePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiWidth > 0 && m_uiWidth <= 4096 && m_uiHeight > 0 && m_uiHeight <= 4096 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiAtmosphereTransmittancePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto atmosphere = context.GetInputBuffer(m_Atmosphere);
  if (!xiiScreenSpacePassUtils::Uniform(context, atmosphere, sizeof(xiiAtmosphereConstants)))
    return context.ReportFailure("AtmosphereTransmittance requires exact atmosphere constants.");

  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Atmosphere/AtmosphereTransmittance.xiiShader"));

  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle               m_Atmosphere;
    xiiRenderGraphTextureHandle              m_Output;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Atmosphere = builder.ReadBuffer(atmosphere, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Size       = {m_uiWidth, m_uiHeight};
    data.m_Output     = xiiCompositionPassUtils::Output(builder, data.m_Size, xiiGALResourceFormat::RGBA32Float);
    data.m_Pipeline   = m_Pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& cmd=execute.GetCommandList();cmd.SetPipelineState(data.m_Pipeline.Borrow());cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants",execute.GetBuffer(data.m_Atmosphere),xiiGALShaderType::Compute);

    cmd.ResolveAndSetUnorderedAccessTextureView("g_TransmittanceLUT",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();cmd.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Atmosphere_AtmosphereTransmittancePass);
