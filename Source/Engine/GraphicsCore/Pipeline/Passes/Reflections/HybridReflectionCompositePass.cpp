/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Reflections/HybridReflectionCompositePass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

using namespace xiiRenderPassUtils;
struct xiiHybridReflectionCompositePass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hEnvironmentSpecular;    ///< ShaderResource in (Tier 2 local probe / sky BRDF response).
  xiiRenderGraphTextureHandle m_hScreenSpaceReflections; ///< ShaderResource in (Tier 1 screen-space response + confidence).
  xiiRenderGraphTextureHandle m_hRayTracedReflections;   ///< ShaderResource in (Tier 3 traced response + validity).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;          ///< UnorderedAccess in/out (authoritative scene radiance).

  xiiSizeU32 m_Size;
};
struct xiiHybridReflectionCompositePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Linear;
};
xiiHybridReflectionCompositePass::xiiHybridReflectionCompositePass()  = default;
xiiHybridReflectionCompositePass::~xiiHybridReflectionCompositePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHybridReflectionCompositePass, 1, xiiRTTIDefaultAllocator<xiiHybridReflectionCompositePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EnvironmentSpecular", m_InputEnvironmentSpecular)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SSRTerm", m_InputSSRTerm)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("RTFinalReflections", m_InputRTFinalReflections)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("HDRSceneColorInput", m_InputHDRSceneColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HDRSceneColor")),
    XII_MEMBER_PROPERTY("HDRSceneColorOutput", m_OutputHDRSceneColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("HDRSceneColor")),
    XII_MEMBER_PROPERTY("EnvironmentSpecularOutput", m_OutputEnvironmentSpecular)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("EnvironmentSpecular")),
    XII_MEMBER_PROPERTY("SSRTermOutput", m_OutputSSRTerm)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("SSRTerm")),
    XII_MEMBER_PROPERTY("RTFinalReflectionsOutput", m_OutputRTFinalReflections)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("RTFinalReflections")),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Reflections") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiHybridReflectionCompositePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/ReflectionComposite.xiiShader"));
  XII_SUCCEED_OR_RETURN(LinearClampSampler(m_pState->m_Linear));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture(m_InputHDRSceneColor), 0, 0, size));
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputEnvironmentSpecular)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputSSRTerm)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputRTFinalReflections)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputHDRSceneColor)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hEnvironmentSpecular = context.GetInputTexture(m_InputEnvironmentSpecular);
    data.m_hScreenSpaceReflections = context.GetInputTexture(m_InputSSRTerm);
    data.m_hRayTracedReflections = context.GetInputTexture(m_InputRTFinalReflections);
    data.m_hHDRSceneColor = context.GetInputTexture(m_InputHDRSceneColor);
    Setup(data, builder); }, xiiMakeDelegate(&xiiHybridReflectionCompositePass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputHDRSceneColor, emitted.first->m_hHDRSceneColor));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputEnvironmentSpecular, emitted.first->m_hEnvironmentSpecular));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputSSRTerm, emitted.first->m_hScreenSpaceReflections));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputRTFinalReflections, emitted.first->m_hRayTracedReflections));
  return XII_SUCCESS;
}
void xiiHybridReflectionCompositePass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hEnvironmentSpecular    = builder.ReadTexture(data.m_hEnvironmentSpecular, xiiGALResourceStateFlags::ShaderResource);
  data.m_hScreenSpaceReflections = builder.ReadTexture(data.m_hScreenSpaceReflections, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedReflections   = builder.ReadTexture(data.m_hRayTracedReflections, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor          = builder.WriteTexture(builder.ReadTexture(data.m_hHDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
}
void xiiHybridReflectionCompositePass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HybridReflectionComposite");
  {
    cmd.SetPipelineState(m_pState->m_Pipeline);
    cmd.ResolveAndSetSampler("LinearClampSampler", m_pState->m_Linear.Borrow());
    cmd.ResolveAndSetShaderResourceTextureView("g_EnvironmentSpecular", context.GetTexture(data.m_hEnvironmentSpecular)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ScreenSpaceReflection", context.GetTexture(data.m_hScreenSpaceReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RayTracedReflection", context.GetTexture(data.m_hRayTracedReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Reflections_HybridReflectionCompositePass);
