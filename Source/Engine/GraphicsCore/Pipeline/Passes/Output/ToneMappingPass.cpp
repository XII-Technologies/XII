/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Output/ToneMappingPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/ToneMappingConstants.h>

struct xiiToneMappingPass::Data
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;

  xiiRenderGraphTextureHandle m_hHDRInput;
  xiiRenderGraphTextureHandle m_hBloom;
  xiiRenderGraphBufferHandle  m_hExposure;
  xiiRenderGraphTextureHandle m_hDisplayLinear;
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiSizeU32                  m_Size;
  xiiDisplayOutputSettings    m_DisplaySettings;
};
struct xiiToneMappingPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;

  xiiRenderPassUtils::DisplayConfiguration m_Display;
};

xiiToneMappingPass::xiiToneMappingPass()  = default;
xiiToneMappingPass::~xiiToneMappingPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiToneMappingPass, 1, xiiRTTIDefaultAllocator<xiiToneMappingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("UpscaledColor", m_InputUpscaledColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("BloomTexture", m_InputBloomTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("CurrentExposure", m_InputCurrentExposure)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DisplayLinearColor", m_OutputDisplayLinearColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseViewDefaults", m_bUseViewDefaults)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ENUM_MEMBER_PROPERTY("Operator", xiiToneMappingOperator, m_Operator),
    XII_ENUM_MEMBER_PROPERTY("OutputMode", xiiDisplayOutputMode, m_OutputMode),
    XII_MEMBER_PROPERTY("BloomStrength", m_fBloomStrength)->AddAttributes(new xiiDefaultValueAttribute(0.05f)),
    XII_MEMBER_PROPERTY("PaperWhiteNits", m_fPaperWhiteNits)->AddAttributes(new xiiDefaultValueAttribute(203.0f)),
    XII_MEMBER_PROPERTY("MaximumDisplayNits", m_fMaximumDisplayNits)->AddAttributes(new xiiDefaultValueAttribute(1000.0f)),
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


xiiResult xiiToneMappingPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  xiiDisplayOutputSettings settings;
  settings.m_ToneMappingOperator = m_Operator;
  settings.m_OutputMode          = m_OutputMode;
  settings.m_fBloomStrength      = m_fBloomStrength;
  settings.m_fPaperWhiteNits     = m_fPaperWhiteNits;
  settings.m_fMaximumDisplayNits = m_fMaximumDisplayNits;
  if (!xiiDisplayOutputManager::IsValid(settings))
    return XII_FAILURE;
  if (m_pState)
    m_pState->m_Display.m_bValid = false;
  return XII_SUCCESS;
}
xiiResult xiiToneMappingPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/ToneMapping.xiiShader"));
  xiiDisplayOutputSettings settings;
  settings.m_ToneMappingOperator = m_Operator;
  settings.m_OutputMode          = m_OutputMode;
  settings.m_fBloomStrength      = m_fBloomStrength;
  settings.m_fPaperWhiteNits     = m_fPaperWhiteNits;
  settings.m_fMaximumDisplayNits = m_fMaximumDisplayNits;
  XII_SUCCEED_OR_RETURN(m_pState->m_Display.Resolve(context, settings, m_bUseViewDefaults));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, context.GetInputTexture(m_InputUpscaledColor), 0, 0, size));
  auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size=size;
    data.m_DisplaySettings=m_pState->m_Display.m_Settings;
    data.m_hHDRInput=context.GetInputTexture(m_InputUpscaledColor);
    data.m_hBloom=context.GetInputTexture(m_InputBloomTexture);
    data.m_hExposure=context.GetInputBuffer(m_InputCurrentExposure);
    Setup(data,builder); }, [](const Data& data, xiiRenderGraphPassContext& execute) { Execute(data, execute); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputDisplayLinearColor, emitted.first->m_hDisplayLinear));
  return XII_SUCCESS;
}

void xiiToneMappingPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_Pipeline                = m_pState->m_Pipeline;
  const xiiUInt32 uiOutputWidth  = data.m_Size.width;
  const xiiUInt32 uiOutputHeight = data.m_Size.height;

  data.m_hHDRInput = builder.ReadTexture(data.m_hHDRInput, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBloom    = builder.ReadTexture(data.m_hBloom, xiiGALResourceStateFlags::ShaderResource);
  data.m_hExposure = builder.ReadBuffer(data.m_hExposure, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hDisplayLinear     = builder.WriteTexture("DisplayLinearColor", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiToneMappingConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Tone Mapping Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiToneMappingPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = data.m_Size.width;
  const xiiUInt32    uiOutputHeight = data.m_Size.height;

  cmd.BeginDebugGroup("ToneMapping");
  {
    {
      xiiGALMapHelper<xiiToneMappingConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->Operator           = data.m_DisplaySettings.m_ToneMappingOperator.GetValue();
      pConstants->OutputMode         = data.m_DisplaySettings.m_OutputMode.GetValue();
      pConstants->BloomStrength      = data.m_DisplaySettings.m_fBloomStrength;
      pConstants->PaperWhiteNits     = data.m_DisplaySettings.m_fPaperWhiteNits;
      pConstants->MaximumDisplayNits = data.m_DisplaySettings.m_fMaximumDisplayNits;
      pConstants->_Padding           = xiiVec3::MakeZero();
    }

    cmd.SetPipelineState(data.m_Pipeline.Borrow());


    cmd.ResolveAndSetConstantBuffer("xiiToneMappingConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRInput", context.GetTexture(data.m_hHDRInput)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BloomTex", context.GetTexture(data.m_hBloom)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Exposure", context.GetBuffer(data.m_hExposure)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_LDROut", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_ToneMappingPass);
