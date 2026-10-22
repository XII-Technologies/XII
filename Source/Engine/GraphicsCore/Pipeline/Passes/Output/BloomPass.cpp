/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/BloomConstants.h>

struct xiiBloomPass::Data
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Sampler;

  xiiRenderGraphTextureHandle m_hHDRIn; ///< ShaderResource in (upscaled HDR input).
  xiiRenderGraphTextureHandle m_hBloom; ///< UnorderedAccess out (bloom result).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiSizeU32                  m_Size;
  xiiDisplayOutputSettings    m_DisplaySettings;
};
struct xiiBloomPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALSampler>              m_Sampler;
  xiiRenderPassUtils::DisplayConfiguration m_Display;
};

xiiBloomPass::xiiBloomPass()  = default;
xiiBloomPass::~xiiBloomPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBloomPass, 1, xiiRTTIDefaultAllocator<xiiBloomPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("UpscaledColor", m_InputUpscaledColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("BloomTexture", m_OutputBloomTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseViewDefaults", m_bUseViewDefaults)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Knee", m_fKnee)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Output")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


xiiResult xiiBloomPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  xiiDisplayOutputSettings settings;
  settings.m_fBloomThreshold = m_fThreshold;
  settings.m_fBloomKnee      = m_fKnee;
  settings.m_fBloomRadius    = m_fRadius;
  if (!xiiDisplayOutputManager::IsValid(settings))
    return XII_FAILURE;
  if (m_pState)
    m_pState->m_Display.m_bValid = false;
  return XII_SUCCESS;
}
xiiResult xiiBloomPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/BloomChain.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_pState->m_Sampler));
  xiiDisplayOutputSettings settings;
  settings.m_fBloomThreshold = m_fThreshold;
  settings.m_fBloomKnee      = m_fKnee;
  settings.m_fBloomRadius    = m_fRadius;
  XII_SUCCEED_OR_RETURN(m_pState->m_Display.Resolve(context, settings, m_bUseViewDefaults));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, context.GetInputTexture(m_InputUpscaledColor), 0, 0, size));
  auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size=size;
    data.m_DisplaySettings=m_pState->m_Display.m_Settings;
    data.m_hHDRIn=context.GetInputTexture(m_InputUpscaledColor);
    Setup(data,builder); }, [](const Data& data, xiiRenderGraphPassContext& execute) { Execute(data, execute); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputBloomTexture, emitted.first->m_hBloom));
  return XII_SUCCESS;
}

void xiiBloomPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_Pipeline                = m_pState->m_Pipeline;
  data.m_Sampler                 = m_pState->m_Sampler;
  const xiiUInt32 uiOutputWidth  = data.m_Size.width;
  const xiiUInt32 uiOutputHeight = data.m_Size.height;

  data.m_hHDRIn = builder.ReadTexture(data.m_hHDRIn, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hBloom             = builder.WriteTexture("BloomTexture", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiBloomConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Bloom Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiBloomPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = data.m_Size.width;
  const xiiUInt32    uiOutputHeight = data.m_Size.height;

  cmd.BeginDebugGroup("Bloom");
  {
    {
      xiiGALMapHelper<xiiBloomConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->BloomParameters = xiiVec4(data.m_DisplaySettings.m_fBloomThreshold, data.m_DisplaySettings.m_fBloomKnee, data.m_DisplaySettings.m_fBloomRadius, 0.0f);
    }

    cmd.SetPipelineState(data.m_Pipeline.Borrow());

    cmd.ResolveAndSetSampler("g_sLinearClamp", data.m_Sampler.Borrow());
    cmd.ResolveAndSetConstantBuffer("xiiBloomConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRIn", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_BloomOut", context.GetTexture(data.m_hBloom)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_BloomPass);
