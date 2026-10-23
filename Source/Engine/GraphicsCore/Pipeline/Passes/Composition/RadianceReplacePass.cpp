/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Composition/CompositionPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Composition/RadianceReplacePass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRadianceReplacePass,1,xiiRTTIDefaultAllocator<xiiRadianceReplacePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Scene",m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousContribution",m_PreviousContribution)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Replacement",m_Replacement)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Radiance",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Composition")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiRadianceReplacePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  scene                           = context.GetInputTexture(m_Scene);
  const auto* sceneDescription                = xiiCompositionPassUtils::Image(context, scene);
  const auto  previousContribution            = context.GetInputTexture(m_PreviousContribution);
  const auto* previousContributionDescription = xiiCompositionPassUtils::Image(context, previousContribution);
  const auto  replacement                     = context.GetInputTexture(m_Replacement);
  const auto* replacementDescription          = xiiCompositionPassUtils::Image(context, replacement);
  if (!sceneDescription || !previousContributionDescription || !replacementDescription || previousContributionDescription->m_Size != sceneDescription->m_Size || replacementDescription->m_Size != sceneDescription->m_Size)
    return context.ReportFailure("RadianceReplace requires matching nonempty float-readable RGBA single-sample 2D images.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Composition/RadianceReplace.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Scene, m_PreviousContribution, m_Replacement, m_Output;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Scene=builder.ReadTexture(scene,xiiGALResourceStateFlags::ShaderResource);
    data.m_PreviousContribution=builder.ReadTexture(previousContribution,xiiGALResourceStateFlags::ShaderResource);
    data.m_Replacement=builder.ReadTexture(replacement,xiiGALResourceStateFlags::ShaderResource);
    data.m_Size=sceneDescription->m_Size;data.m_Output=xiiCompositionPassUtils::Output(builder,data.m_Size,xiiGALResourceFormat::RGBA32Float);data.m_Pipeline=m_Pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetShaderResourceTextureView("g_Scene",execute.GetTexture(data.m_Scene)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_PreviousContribution",execute.GetTexture(data.m_PreviousContribution)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Replacement",execute.GetTexture(data.m_Replacement)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Radiance",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();commands.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Composition_RadianceReplacePass);
