/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Clock.h>
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
  xiiExposureSettings        m_Settings;
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
  xiiExposureSettings                      m_LastSettings;
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
  InvalidateTemporalHistory();
  return XII_SUCCESS;
}

xiiResult xiiAutoExposurePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  const auto getSettings = [authored = m_Settings, bUseViewDefaults = m_bUseViewDefaults](const xiiView* pView) {
    return bUseViewDefaults && pView ? pView->GetDisplayOutputSettings().m_Exposure : authored;
  };
  const auto getManualExposure = [](const xiiView* pView) {
    return pView && pView->GetCamera() ? pView->GetCamera()->GetExposure() : 1.0f;
  };

  const auto  hHistogram = context.GetInputBuffer(m_InputLuminanceHistogram);
  const auto* pHistogram = context.m_Graph.GetBufferDescription(hHistogram);
  const bool  bCanMeter  = pHistogram && pHistogram->m_Mode == xiiGALBufferMode::Structured && pHistogram->m_uiSize == 256 * sizeof(xiiUInt32) &&
    pHistogram->m_uiElementByteStride == sizeof(xiiUInt32) && pHistogram->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource);

  xiiGALBufferCreationDescription exposureDescription;
  exposureDescription.m_uiElementByteStride = sizeof(float);
  exposureDescription.m_uiSize              = sizeof(xiiVec2);
  exposureDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  exposureDescription.m_Mode                = xiiGALBufferMode::Structured;

  if (!bCanMeter)
  {
    if (getSettings(context.m_pView).m_Mode != xiiExposureMode::Manual)
      return context.ReportFailure("Auto exposure requires a readable structured histogram of 256 uint counters.");
    InvalidateTemporalHistory();
    m_pState->m_Exposure.Clear();
    struct Manual
    {
      xiiRenderGraphBufferHandle m_Output;
      xiiVec2                    m_Value;
    };
    const auto emitted = context.m_Graph.AddPass<Manual>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Manual& data, xiiRenderGraphBuilder& builder) {
      data.m_Output = builder.WriteBuffer("Manual Exposure", exposureDescription, xiiGALResourceStateFlags::CopyDestination);
      builder.SetPassFramePreparation([getSettings, getManualExposure, &data](xiiRenderGraph&, const xiiView* pView, xiiStringBuilder& out_sError) -> xiiResult {
        const auto settings = getSettings(pView);
        if (settings.m_Mode != xiiExposureMode::Manual)
        {
          out_sError = "Automatic exposure requires a connected luminance histogram.";
          return XII_FAILURE;
        }
        const float fExposure = getManualExposure(pView) * xiiMath::Pow(2.0f, settings.m_fExposureCompensation);
        if (!xiiMath::IsFinite(fExposure) || fExposure <= 0.0f)
        {
          out_sError = "Manual exposure requires a finite positive compensated camera exposure.";
          return XII_FAILURE;
        }
        data.m_Value.Set(fExposure, 0.0f);
        return XII_SUCCESS;
      }); }, [](const Manual& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Output), 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_Value), sizeof(data.m_Value))); });
    return context.SetOutput(m_OutputCurrentExposure, emitted.first->m_Output);
  }

  XII_SUCCEED_OR_RETURN(LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/ExposureAdaptation.xiiShader"));
  exposureDescription.m_BindFlags.Add(xiiGALBindFlags::UnorderedAccess);
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_State = m_pState;
    data.m_Pipeline = m_pState->m_Pipeline;
    data.m_hHistogram = builder.ReadBuffer(hHistogram, xiiGALResourceStateFlags::ShaderResource);
    data.m_hExposure = builder.ReadWriteBuffer(builder.DeclareImportedBuffer("CurrentExposure", exposureDescription), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_hConstants = Constants(builder, "Exposure Adaptation Constants", sizeof(xiiExposureAdaptationConstants));
    builder.SetPassFramePreparation([getSettings, getManualExposure, exposureDescription, &data](xiiRenderGraph& ref_graph, const xiiView* pView, xiiStringBuilder& out_sError) -> xiiResult {
      data.m_Settings = getSettings(pView);
      data.m_fManualExposure = getManualExposure(pView);
      const float fCompensated = data.m_fManualExposure * xiiMath::Pow(2.0f, data.m_Settings.m_fExposureCompensation);
      if (!xiiDisplayOutputManager::IsValid(data.m_Settings) ||
          (data.m_Settings.m_Mode == xiiExposureMode::Manual && (!xiiMath::IsFinite(fCompensated) || fCompensated <= 0.0f)))
      {
        out_sError = "Invalid exposure settings or compensated camera exposure.";
        return XII_FAILURE;
      }
      data.m_fDeltaTime = xiiMath::Clamp(static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()), 0.0f, 0.25f);
      auto& state = *data.m_State;
      XII_LOCK(state.m_Mutex);
      if (!state.m_Exposure)
      {
        state.m_Exposure = xiiGALDevice::GetDefaultDevice()->CreateBuffer(exposureDescription);
        state.m_bHistoryValid = false;
      }
      if (!state.m_Exposure || ref_graph.BindImportedBuffer(data.m_hExposure, state.m_Exposure, state.m_Exposure->GetResourceState()).Failed())
      {
        out_sError = "Could not allocate or bind the declared exposure history buffer.";
        return XII_FAILURE;
      }
      data.m_uiHistoryRevision = state.m_uiHistoryRevision;
      data.m_bHistoryValid = state.m_bHistoryValid && state.m_LastSettings == data.m_Settings;
      return XII_SUCCESS;
    });
    builder.SetPassSubmissionCompletion([&data] {
      auto& state = *data.m_State;
      XII_LOCK(state.m_Mutex);
      if (state.m_uiHistoryRevision == data.m_uiHistoryRevision)
      {
        state.m_LastSettings = data.m_Settings;
        state.m_bHistoryValid = data.m_Settings.m_Mode == xiiExposureMode::Automatic;
      }
    }); }, [](const Data& data, xiiRenderGraphPassContext& execute) { Execute(data, execute); });
  return context.SetOutput(m_OutputCurrentExposure, emitted.first->m_hExposure);
}

void xiiAutoExposurePass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AutoExposure");
  {
    const xiiExposureSettings& settings = data.m_Settings;
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
    if (cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      context.ReportFailure("Could not bind auto exposure resources.");
      cmd.EndDebugGroup();
      return;
    }
    cmd.DispatchCompute({1U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Exposure_AutoExposurePass);
