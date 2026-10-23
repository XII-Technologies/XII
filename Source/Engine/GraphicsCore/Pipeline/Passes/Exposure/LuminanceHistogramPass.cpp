/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Exposure/LuminanceHistogramPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Exposure/ExposureHistogramConstants.h>

using namespace xiiRenderPassUtils;
struct xiiLuminanceHistogramPass::Data
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;

  xiiRenderGraphTextureHandle m_hHDRIn;     ///< ShaderResource in (current HDR scene color).
  xiiRenderGraphBufferHandle  m_hHistogram; ///< UnorderedAccess out (256-bin luminance histogram).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiSizeU32                  m_Size;
  xiiDisplayOutputSettings    m_DisplaySettings;
};

struct xiiLuminanceHistogramPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  DisplayConfiguration                     m_Display;
};

xiiLuminanceHistogramPass::xiiLuminanceHistogramPass()  = default;
xiiLuminanceHistogramPass::~xiiLuminanceHistogramPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLuminanceHistogramPass, 1, xiiRTTIDefaultAllocator<xiiLuminanceHistogramPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("HDRSceneColor", m_InputHDRSceneColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("LuminanceHistogram", m_OutputLuminanceHistogram)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseViewDefaults", m_bUseViewDefaults)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("MinimumLogLuminance", m_fMinimumLogLuminance)->AddAttributes(new xiiDefaultValueAttribute(-12.0f)),
    XII_MEMBER_PROPERTY("MaximumLogLuminance", m_fMaximumLogLuminance)->AddAttributes(new xiiDefaultValueAttribute(20.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Exposure")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiLuminanceHistogramPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (!xiiMath::IsFinite(m_fMinimumLogLuminance) || !xiiMath::IsFinite(m_fMaximumLogLuminance) || m_fMinimumLogLuminance >= m_fMaximumLogLuminance)
    return XII_FAILURE;
  if (m_pState)
    m_pState->m_Display.m_bValid = false;
  return XII_SUCCESS;
}

xiiResult xiiLuminanceHistogramPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/ExposureHistogram.xiiShader"));
  xiiDisplayOutputSettings authored;
  authored.m_Exposure.m_fMinimumLogLuminance = m_fMinimumLogLuminance;
  authored.m_Exposure.m_fMaximumLogLuminance = m_fMaximumLogLuminance;
  XII_SUCCEED_OR_RETURN(m_pState->m_Display.Resolve(context, authored, m_bUseViewDefaults));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(OutputSize(context, context.GetInputTexture(m_InputHDRSceneColor), size));
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Size = size;
    data.m_DisplaySettings = m_pState->m_Display.m_Settings;
    data.m_hHDRIn = context.GetInputTexture(m_InputHDRSceneColor);
    Setup(data, builder); }, [](const Data& data, xiiRenderGraphPassContext& execute) { Execute(data, execute); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputLuminanceHistogram, emitted.first->m_hHistogram));
  return XII_SUCCESS;
}

void xiiLuminanceHistogramPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_Pipeline = m_pState->m_Pipeline;
  data.m_hHDRIn   = builder.ReadTexture(data.m_hHDRIn, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U;
  description.m_uiSize              = description.m_uiElementByteStride * 256U;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hHistogram                 = builder.WriteBuffer("LuminanceHistogram", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiExposureHistogramConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Exposure Histogram Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiLuminanceHistogramPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = data.m_Size.width;
  const xiiUInt32    uiRenderHeight = data.m_Size.height;

  cmd.BeginDebugGroup("LuminanceHistogram");
  {
    const xiiExposureSettings& settings = data.m_DisplaySettings.m_Exposure;
    const float                fRange   = settings.m_fMaximumLogLuminance - settings.m_fMinimumLogLuminance;
    {
      xiiGALMapHelper<xiiExposureHistogramConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->LogLuminanceRange = xiiVec4(settings.m_fMinimumLogLuminance, settings.m_fMaximumLogLuminance, 1.0f / fRange, fRange);
      pConstants->InputResolution   = xiiVec2U32(uiRenderWidth, uiRenderHeight);
      pConstants->_Padding          = xiiVec2::MakeZero();
    }

    xiiUInt32 zeroHistogram[256] = {};
    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hHistogram), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroHistogram), sizeof(zeroHistogram)));
    cmd.SetPipelineState(data.m_Pipeline.Borrow());
    cmd.ResolveAndSetConstantBuffer("xiiExposureHistogramConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRInput", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Histogram", context.GetBuffer(data.m_hHistogram)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 15U) / 16U, (uiRenderHeight + 15U) / 16U, 1U});
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Exposure_LuminanceHistogramPass);
