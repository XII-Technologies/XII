/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Output/ColorGradingPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/ColorGradingConstants.h>

struct xiiColorGradingPass::Data
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;

  xiiRenderGraphTextureHandle m_hDisplayLinear; ///< ShaderResource in (tone-mapped display-linear input).
  xiiRenderGraphTextureHandle m_hGraded;        ///< UnorderedAccess out (graded display-linear output).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiSizeU32                  m_Size;
  xiiDisplayOutputSettings    m_DisplaySettings;
  bool                        m_bDefaultViewMode = true;
};
struct xiiColorGradingPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;

  xiiRenderPassUtils::DisplayConfiguration m_Display;
};

xiiColorGradingPass::xiiColorGradingPass()
{
  m_OutputGradedColor.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiColorGradingPass::~xiiColorGradingPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiColorGradingPass, 1, xiiRTTIDefaultAllocator<xiiColorGradingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DisplayLinearColorInput", m_InputDisplayLinearColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("DisplayLinearColor")),
    XII_MEMBER_PROPERTY("GradedColor", m_OutputGradedColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ColorGradingConstants", m_OutputColorGradingConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Color Grading Constants")),
    XII_MEMBER_PROPERTY("DisplayLinearColorOutput", m_OutputDisplayLinearColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("DisplayLinearColor")),
    XII_MEMBER_PROPERTY("UseViewDefaults", m_bUseViewDefaults)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Settings", m_Settings),
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


xiiResult xiiColorGradingPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  xiiDisplayOutputSettings settings;
  settings.m_ColorGrading = m_Settings;
  if (!xiiDisplayOutputManager::IsValid(settings))
    return XII_FAILURE;
  if (m_pState)
    m_pState->m_Display.m_bValid = false;
  return XII_SUCCESS;
}
xiiResult xiiColorGradingPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/ColorGrading.xiiShader"));
  xiiDisplayOutputSettings settings;
  settings.m_ColorGrading = m_Settings;
  XII_SUCCEED_OR_RETURN(m_pState->m_Display.Resolve(context, settings, m_bUseViewDefaults));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_OutputGradedColor, context.GetInputTexture(m_InputDisplayLinearColor), size));
  const auto* sizingInput = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputDisplayLinearColor));
  if (!sizingInput || sizingInput->m_Size != size)
    return context.ReportFailure("This operation requires input-sized output; resample its inputs before processing at another resolution.");
  auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size=size;
    data.m_DisplaySettings=m_pState->m_Display.m_Settings;
    data.m_bDefaultViewMode=!context.m_pView || context.m_pView->GetViewRenderMode()==xiiViewRenderMode::Default;
    data.m_hDisplayLinear=context.GetInputTexture(m_InputDisplayLinearColor);
    Setup(data,builder); }, [](const Data& data, xiiRenderGraphPassContext& execute) { Execute(data, execute); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputGradedColor, emitted.first->m_hGraded));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputColorGradingConstants, emitted.first->m_hConstants));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputDisplayLinearColor, emitted.first->m_hDisplayLinear));
  return XII_SUCCESS;
}

void xiiColorGradingPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_Pipeline                = m_pState->m_Pipeline;
  const xiiUInt32 uiOutputWidth  = data.m_Size.width;
  const xiiUInt32 uiOutputHeight = data.m_Size.height;

  data.m_hDisplayLinear = builder.ReadTexture(data.m_hDisplayLinear, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hGraded            = builder.WriteTexture("GradedColor", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiColorGradingConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Color Grading Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiColorGradingPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = data.m_Size.width;
  const xiiUInt32    uiOutputHeight = data.m_Size.height;

  cmd.BeginDebugGroup("ColorGrading");
  {
    const xiiColorGradingSettings settings = data.m_bDefaultViewMode ? data.m_DisplaySettings.m_ColorGrading : xiiColorGradingSettings{};
    {
      xiiGALMapHelper<xiiColorGradingConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->ColorAdjustments = xiiVec4(settings.m_fSaturation, settings.m_fContrast, settings.m_fVignetteStrength, settings.m_fVignetteRoundness);
      pConstants->FilmGrain        = xiiVec4(settings.m_fFilmGrainStrength, static_cast<float>(context.GetFrameIndex() & 0x00FFFFFFU), 0.0f, 0.0f);
    }

    cmd.SetPipelineState(data.m_Pipeline.Borrow());


    cmd.ResolveAndSetConstantBuffer("xiiColorGradingConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DisplayLinear", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GradedOut", context.GetTexture(data.m_hGraded)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_ColorGradingPass);
