/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Exposure/AutoExposurePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Exposure/ExposureAdaptationConstants.h>

using namespace xiiRenderPassUtils;
struct xiiAutoExposurePass::Data
{
  xiiSharedPtr<State>                      m_State;
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiUInt64                                m_uiHistoryRevision = 0;

  xiiRenderGraphBufferHandle m_hHistogram; ///< ShaderResource in (luminance histogram).
  xiiRenderGraphBufferHandle m_hExposure;  ///< UnorderedAccess in/out (persistent exposure value).
  xiiRenderGraphBufferHandle m_hConstants;
  bool                       m_bHistoryValid = false;
  xiiDisplayOutputSettings   m_DisplaySettings;
  float                      m_fDeltaTime      = 0.0f;
  float                      m_fManualExposure = 1.0f;
};

struct xiiAutoExposurePass::State : public xiiRefCounted
{
  xiiMutex                                 m_Mutex;
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiSharedPtr<xiiGALBuffer>               m_Exposure;
  bool                                     m_bHistoryValid     = false;
  xiiUInt64                                m_uiHistoryRevision = 0;
  DisplayConfiguration                     m_Display;
};

xiiAutoExposurePass::xiiAutoExposurePass()  = default;
xiiAutoExposurePass::~xiiAutoExposurePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAutoExposurePass, 1, xiiRTTIDefaultAllocator<xiiAutoExposurePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LuminanceHistogram", m_InputLuminanceHistogram)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("CurrentExposure", m_OutputCurrentExposure)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseViewDefaults", m_bUseViewDefaults)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Settings", m_Settings),
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

void xiiAutoExposurePass::InvalidateTemporalHistory()
{
  if (m_pState)
  {
    XII_LOCK(m_pState->m_Mutex);
    m_pState->m_bHistoryValid = false;
    ++m_pState->m_uiHistoryRevision;
  }
}
xiiResult xiiAutoExposurePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (!xiiDisplayOutputManager::IsValid(m_Settings))
    return XII_FAILURE;
  if (m_pState)
    m_pState->m_Display.m_bValid = false;
  InvalidateTemporalHistory();
  return XII_SUCCESS;
}

xiiResult xiiAutoExposurePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/ExposureAdaptation.xiiShader"));
  xiiDisplayOutputSettings authored;
  authored.m_Exposure         = m_Settings;
  const auto previousRevision = m_pState->m_Display.m_uiViewRevision;
  const auto previousOverride = m_pState->m_Display.m_Override;
  const bool wasValid         = m_pState->m_Display.m_bValid;
  XII_SUCCEED_OR_RETURN(m_pState->m_Display.Resolve(context, authored, m_bUseViewDefaults));
  if (!wasValid || previousRevision != m_pState->m_Display.m_uiViewRevision || previousOverride != m_pState->m_Display.m_Override)
    InvalidateTemporalHistory();
  if (!m_pState->m_Exposure)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(float);
    description.m_uiSize              = sizeof(float) * 2;
    description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    m_pState->m_Exposure              = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
    if (!m_pState->m_Exposure)
      return XII_FAILURE;
  }
  if (!context.GetInputBuffer(m_InputLuminanceHistogram).IsValid())
    return XII_FAILURE;
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_hHistogram = context.GetInputBuffer(m_InputLuminanceHistogram);
    data.m_DisplaySettings = m_pState->m_Display.m_Settings;
    data.m_fDeltaTime = xiiMath::Clamp(static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()), 0.0f, 0.25f);
    data.m_fManualExposure = context.m_pView && context.m_pView->GetCamera() ? context.m_pView->GetCamera()->GetExposure() : 1.0f;
    Setup(data, builder); }, [](const Data& data, xiiRenderGraphPassContext& execute) { Execute(data, execute); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputCurrentExposure, emitted.first->m_hExposure));
  return XII_SUCCESS;
}

void xiiAutoExposurePass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_State    = m_pState;
  data.m_Pipeline = m_pState->m_Pipeline;
  {
    XII_LOCK(m_pState->m_Mutex);
    data.m_uiHistoryRevision = m_pState->m_uiHistoryRevision;
    data.m_bHistoryValid     = m_pState->m_bHistoryValid;
  }
  data.m_hHistogram = builder.ReadBuffer(data.m_hHistogram, xiiGALResourceStateFlags::ShaderResource);

  data.m_hExposure = builder.ImportBuffer("CurrentExposure", m_pState->m_Exposure, m_pState->m_Exposure->GetResourceState());
  data.m_hExposure = builder.WriteBuffer(data.m_hExposure, xiiGALResourceStateFlags::UnorderedAccess);


  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiExposureAdaptationConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Exposure Adaptation Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiAutoExposurePass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AutoExposure");
  {
    const xiiExposureSettings& settings = data.m_DisplaySettings.m_Exposure;
    const float                fRange   = settings.m_fMaximumLogLuminance - settings.m_fMinimumLogLuminance;
    {
      xiiGALMapHelper<xiiExposureAdaptationConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->LogLuminanceRange = xiiVec4(settings.m_fMinimumLogLuminance, settings.m_fMaximumLogLuminance, fRange, 0.0f);
      pConstants->Metering          = xiiVec4(settings.m_fLowPercentile, settings.m_fHighPercentile, settings.m_fAdaptationSpeedBright, settings.m_fAdaptationSpeedDark);
      pConstants->Exposure          = xiiVec4(data.m_fDeltaTime, settings.m_fMinimumEV100, settings.m_fMaximumEV100, settings.m_fExposureCompensation);
      pConstants->ManualExposure    = data.m_fManualExposure;
      pConstants->AutomaticExposure = settings.m_Mode == xiiExposureMode::Automatic ? 1U : 0U;
      pConstants->HistoryValid      = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding          = 0.0f;
    }

    cmd.SetPipelineState(data.m_Pipeline.Borrow());

    cmd.ResolveAndSetConstantBuffer("xiiExposureAdaptationConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Histogram", context.GetBuffer(data.m_hHistogram)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Exposure", context.GetBuffer(data.m_hExposure)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({1U, 1U, 1U});
    {
      XII_LOCK(data.m_State->m_Mutex);
      if (data.m_uiHistoryRevision == data.m_State->m_uiHistoryRevision)
        data.m_State->m_bHistoryValid = true;
    }
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Exposure_AutoExposurePass);
