/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Composition/OpaqueLightingCompositePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

using namespace xiiRenderPassUtils;
struct xiiOpaqueLightingCompositePass::Data
{
  XII_DECLARE_POD_TYPE();

  xiiRenderGraphTextureHandle m_hDirectLighting;       ///< ShaderResource in (direct lighting texture).
  xiiRenderGraphTextureHandle m_hIndirectLighting;     ///< ShaderResource in (indirect lighting including probe/sky specular).
  xiiRenderGraphTextureHandle m_hGBufferEmissive;      ///< ShaderResource in (surface emissive radiance).
  xiiRenderGraphTextureHandle m_hRayTracedFinalGI;     ///< ShaderResource in (optional near-field traced GI).
  xiiRenderGraphTextureHandle m_hVolumetricScattering; ///< ShaderResource in (integrated scattering and transmittance).
  xiiRenderGraphTextureHandle m_hHDRSceneColor;        ///< UnorderedAccess out (base scene radiance before reflection tier replacement).

  xiiSizeU32 m_Size;
};
struct xiiOpaqueLightingCompositePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Linear;
};
xiiOpaqueLightingCompositePass::xiiOpaqueLightingCompositePass()  = default;
xiiOpaqueLightingCompositePass::~xiiOpaqueLightingCompositePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOpaqueLightingCompositePass, 1, xiiRTTIDefaultAllocator<xiiOpaqueLightingCompositePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DirectLighting", m_InputDirectLighting)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("IndirectLighting", m_InputIndirectLighting)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("GBufferEmissive", m_InputGBufferEmissive)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("RTFinalGI", m_InputRTFinalGI)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("VolumetricScattering", m_InputVolumetricScattering)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("HDRSceneColor", m_OutputHDRSceneColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DirectLightingOutput", m_OutputDirectLighting)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("DirectLighting")),
    XII_MEMBER_PROPERTY("IndirectLightingOutput", m_OutputIndirectLighting)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("IndirectLighting")),
    XII_MEMBER_PROPERTY("GBufferEmissiveOutput", m_OutputGBufferEmissive)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("GBufferEmissive")),
    XII_MEMBER_PROPERTY("RTFinalGIOutput", m_OutputRTFinalGI)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("RTFinalGI")),
    XII_MEMBER_PROPERTY("VolumetricScatteringOutput", m_OutputVolumetricScattering)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("VolumetricScattering")),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Composition") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiOpaqueLightingCompositePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/OpaqueComposite.xiiShader"));
  XII_SUCCEED_OR_RETURN(LinearClampSampler(m_pState->m_Linear));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture(m_InputDirectLighting), 0, 0, size));
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputDirectLighting)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputIndirectLighting)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputGBufferEmissive)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputRTFinalGI)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  if (const auto* desc = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputVolumetricScattering)); !desc || desc->m_Size != size)
    return XII_FAILURE;
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_hDirectLighting = context.GetInputTexture(m_InputDirectLighting);
    data.m_hIndirectLighting = context.GetInputTexture(m_InputIndirectLighting);
    data.m_hGBufferEmissive = context.GetInputTexture(m_InputGBufferEmissive);
    data.m_hRayTracedFinalGI = context.GetInputTexture(m_InputRTFinalGI);
    data.m_hVolumetricScattering = context.GetInputTexture(m_InputVolumetricScattering);
    Setup(data, builder); }, xiiMakeDelegate(&xiiOpaqueLightingCompositePass::Execute, this));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputHDRSceneColor, emitted.first->m_hHDRSceneColor));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputDirectLighting, emitted.first->m_hDirectLighting));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputIndirectLighting, emitted.first->m_hIndirectLighting));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputGBufferEmissive, emitted.first->m_hGBufferEmissive));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputRTFinalGI, emitted.first->m_hRayTracedFinalGI));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputVolumetricScattering, emitted.first->m_hVolumetricScattering));
  return XII_SUCCESS;
}
void xiiOpaqueLightingCompositePass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_hDirectLighting       = builder.ReadTexture(data.m_hDirectLighting, xiiGALResourceStateFlags::ShaderResource);
  data.m_hIndirectLighting     = builder.ReadTexture(data.m_hIndirectLighting, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferEmissive      = builder.ReadTexture(data.m_hGBufferEmissive, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalGI     = builder.ReadTexture(data.m_hRayTracedFinalGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering = builder.ReadTexture(data.m_hVolumetricScattering, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = data.m_Size.width;
  description.m_Size.height = data.m_Size.height;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHDRSceneColor     = builder.WriteTexture("HDRSceneColor", description, xiiGALResourceStateFlags::UnorderedAccess);

  builder.SetPassAllowMerge(false);
}
void xiiOpaqueLightingCompositePass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("OpaqueLightingComposite");
  {
    cmd.SetPipelineState(m_pState->m_Pipeline);
    cmd.ResolveAndSetSampler("LinearClampSampler", m_pState->m_Linear.Borrow());
    cmd.ResolveAndSetShaderResourceTextureView("g_DirectLight", context.GetTexture(data.m_hDirectLighting)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_IndirectLight", context.GetTexture(data.m_hIndirectLighting)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Emissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Volumetric", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RayTracedGI", context.GetTexture(data.m_hRayTracedFinalGI)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDROut", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_Size.width + 7U) / 8U, (data.m_Size.height + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Composition_OpaqueLightingCompositePass);
