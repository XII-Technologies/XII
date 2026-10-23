/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Composition/CompositionPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/IrradianceLayerPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIrradianceLayerPass,1,xiiRTTIDefaultAllocator<xiiIrradianceLayerPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CoarseIrradiance",m_Coarse)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DetailIrradiance",m_Detail)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Irradiance",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Global Illumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiIrradianceLayerPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  coarse            = context.GetInputTexture(this->m_Coarse);
  const auto* coarseDescription = xiiCompositionPassUtils::Image(context, coarse);
  const auto  detail            = context.GetInputTexture(this->m_Detail);
  const auto* detailDescription = xiiCompositionPassUtils::Image(context, detail);
  if (!coarseDescription || !detailDescription || coarseDescription->m_Size != detailDescription->m_Size)
    return context.ReportFailure("IrradianceLayer requires matching nonempty float-readable RGBA single-sample 2D images.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/GlobalIllumination/IrradianceLayer.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Coarse, m_Detail, m_Output;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Coarse=builder.ReadTexture(coarse,xiiGALResourceStateFlags::ShaderResource);
    data.m_Detail=builder.ReadTexture(detail,xiiGALResourceStateFlags::ShaderResource);
    data.m_Size=coarseDescription->m_Size; data.m_Output=xiiCompositionPassUtils::Output(builder,data.m_Size,xiiGALResourceFormat::RGBA32Float); data.m_Pipeline=m_Pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList(); commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetShaderResourceTextureView("g_CoarseIrradiance",execute.GetTexture(data.m_Coarse)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_DetailIrradiance",execute.GetTexture(data.m_Detail)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Radiance",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess(); commands.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_IrradianceLayerPass);
