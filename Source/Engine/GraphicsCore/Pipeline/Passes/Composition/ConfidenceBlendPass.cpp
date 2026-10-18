/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Composition/CompositionPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Composition/ConfidenceBlendPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Composition/ConfidenceBlendConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConfidenceBlendPass,1,xiiRTTIDefaultAllocator<xiiConfidenceBlendPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Background",m_Background)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Layer",m_Layer)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Radiance",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
    XII_MEMBER_PROPERTY("Opacity",m_fOpacity)->AddAttributes(new xiiDefaultValueAttribute(1.0f),new xiiClampValueAttribute(0.0f,1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Composition")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiConfidenceBlendPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fOpacity) && m_fOpacity >= 0 && m_fOpacity <= 1 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiConfidenceBlendPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  background            = context.GetInputTexture(m_Background);
  const auto* backgroundDescription = xiiCompositionPassUtils::Image(context, background);
  const auto  layer                 = context.GetInputTexture(m_Layer);
  const auto* layerDescription      = xiiCompositionPassUtils::Image(context, layer);
  if (!backgroundDescription || (layer.IsValid() && (!layerDescription || layerDescription->m_Size != backgroundDescription->m_Size)))
    return context.ReportFailure("ConfidenceBlend requires matching nonempty float-readable RGBA single-sample 2D images.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Composition/ConfidenceBlend.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Background, m_Layer, m_Output;
    xiiSizeU32                               m_Size;
    xiiRenderGraphBufferHandle               m_Constants;
    xiiConfidenceBlendConstants              m_Snapshot;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Background=builder.ReadTexture(background,xiiGALResourceStateFlags::ShaderResource);
    data.m_Layer=builder.ReadTexture(layer.IsValid()?layer:background,xiiGALResourceStateFlags::ShaderResource);
    data.m_Snapshot.ConfidenceOptions=xiiVec4(layer.IsValid()?m_fOpacity:0.0f,0,0,0);data.m_Constants=xiiRenderPassUtils::Constants(builder,"Confidence options",sizeof(xiiConfidenceBlendConstants));
    data.m_Size=backgroundDescription->m_Size;data.m_Output=xiiCompositionPassUtils::Output(builder,data.m_Size,xiiGALResourceFormat::RGBA32Float);data.m_Pipeline=m_Pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();commands.SetPipelineState(data.m_Pipeline.Borrow());
    {xiiGALMapHelper<xiiConfidenceBlendConstants> mapped(commands,execute.GetBuffer(data.m_Constants),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_Snapshot;}
    xiiGALStateTransitionDescription transition;transition.m_pResource=execute.GetBuffer(data.m_Constants);transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer;transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));
    commands.ResolveAndSetConstantBuffer("xiiConfidenceBlendConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Background",execute.GetTexture(data.m_Background)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Layer",execute.GetTexture(data.m_Layer)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Radiance",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();commands.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Composition_ConfidenceBlendPass);
