/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeSchedulePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

class xiiDDGIProbeSchedulerState : public xiiRefCounted
{
public:
  xiiMutex       m_Mutex;
  xiiDDGIContext m_Context;
  xiiUInt64      m_Frame = 0;
};
xiiDDGIProbeFrameData::xiiDDGIProbeFrameData(xiiUInt64 setup, xiiUInt64 frame, xiiSharedPtr<xiiDDGIProbeSchedulerState> scheduler) : xiiRenderPipelineFrameData(setup), m_Frame(frame), m_Scheduler(std::move(scheduler)) {}
xiiDDGIProbeFrameData::~xiiDDGIProbeFrameData() = default;
void xiiDDGIProbeFrameData::CommitUpdates() const
{
  XII_LOCK(m_Scheduler->m_Mutex);
  if (m_Scheduler->m_Frame != m_Frame)
    return;
  for (const auto& update : m_Updates)
    m_Scheduler->m_Context.CommitProbeUpdate(update.m_vMetadata.x, xiiVec3::MakeZero(), true);
}
xiiDDGIProbeSchedulePass::xiiDDGIProbeSchedulePass()  = default;
xiiDDGIProbeSchedulePass::~xiiDDGIProbeSchedulePass() = default;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGIProbeFrameData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGIProbeSchedulePass,1,xiiRTTIDefaultAllocator<xiiDDGIProbeSchedulePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PreviousIrradiance",m_PreviousIrradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PreviousDistance",m_PreviousDistance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Activation", m_Activation)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RequireActivation", m_bRequireActivation)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("SkipOnRayHardware", m_bSkipOnRayHardware)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("ForwardOffset", m_fForwardOffset)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("Schedule",m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeCountX",m_CountX)->AddAttributes(new xiiDefaultValueAttribute(24U),new xiiClampValueAttribute(1U,256U)),
    XII_MEMBER_PROPERTY("ProbeCountY",m_CountY)->AddAttributes(new xiiDefaultValueAttribute(12U),new xiiClampValueAttribute(1U,256U)),
    XII_MEMBER_PROPERTY("ProbeCountZ",m_CountZ)->AddAttributes(new xiiDefaultValueAttribute(24U),new xiiClampValueAttribute(1U,256U)),
    XII_MEMBER_PROPERTY("UpdateBudget",m_Budget)->AddAttributes(new xiiDefaultValueAttribute(128U),new xiiClampValueAttribute(1U,1048576U)),
    XII_MEMBER_PROPERTY("Spacing",m_fSpacing)->AddAttributes(new xiiDefaultValueAttribute(4.0f),new xiiClampValueAttribute(0.001f,xiiVariant()),new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("Hysteresis",m_fHysteresis)->AddAttributes(new xiiDefaultValueAttribute(0.97f),new xiiClampValueAttribute(0.0f,0.9999f)),
    XII_MEMBER_PROPERTY("MaximumRelocationDistance",m_fRelocation)->AddAttributes(new xiiDefaultValueAttribute(1.8f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("TraceDistanceInProbeSpacings", m_fTraceDistanceInProbeSpacings)->AddAttributes(new xiiClampValueAttribute(0.1f, 64.0f)),
    XII_MEMBER_PROPERTY("DirectionalAtlas", m_bDirectional)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("UseViewCamera",m_bUseViewCamera)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("CameraPosition",m_CameraPosition),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDDGIProbeSchedulePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  const xiiUInt64 count  = xiiUInt64(m_CountX) * m_CountY * m_CountZ;
  const xiiResult result = m_CountX && m_CountY && m_CountZ && m_CountX <= 256 && m_CountY <= 256 && m_CountZ <= 256 && count <= 1048576 && m_Budget && m_Budget <= 1048576 &&
      xiiMath::IsFinite(m_fSpacing) && m_fSpacing > 0 && xiiMath::IsFinite(m_fSpacing * m_fSpacing * 16.0f) &&
      xiiMath::IsFinite(m_fHysteresis) && m_fHysteresis >= 0 && m_fHysteresis < 1 && xiiMath::IsFinite(m_fRelocation) && m_fRelocation >= 0 && m_CameraPosition.IsValid() && xiiMath::IsFinite(m_fForwardOffset) && m_fForwardOffset >= 0 ?
    XII_SUCCESS :
    XII_FAILURE;
  if (result.Succeeded())
    m_State.Clear();
  return result;
}

xiiDDGISettings xiiDDGIProbeSchedulePass::GetProbeSettings() const
{
  xiiDDGISettings settings;
  settings.m_uiProbeCountX              = m_CountX;
  settings.m_uiProbeCountY              = m_CountY;
  settings.m_uiProbeCountZ              = m_CountZ;
  settings.m_uiProbeUpdateBudget        = m_Budget;
  settings.m_fProbeSpacing              = m_fSpacing;
  settings.m_fTemporalHysteresis        = m_fHysteresis;
  settings.m_fMaximumRelocationDistance = m_fRelocation;
  return settings;
}

xiiResult xiiDDGIProbeSchedulePass::ValidateRenderConfiguration(const xiiRenderConfiguration& configuration, xiiStringBuilder& out_error) const
{
  const auto settings = GetProbeSettings();
  const auto uiCount  = xiiUInt64(settings.m_uiProbeCountX) * settings.m_uiProbeCountY * settings.m_uiProbeCountZ;
  if (uiCount > 1048576)
  {
    out_error = "The combined DDGI probe dimensions exceed 1,048,576 probes.";
    return XII_FAILURE;
  }
  if (!xiiMath::IsFinite(xiiMath::Square(settings.m_fProbeSpacing * m_fTraceDistanceInProbeSpacings)))
  {
    out_error = "DDGI probe spacing exceeds the supported world-space distance.";
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
void xiiDDGIProbeSchedulePass::InvalidateTemporalHistory()
{
  if (m_State)
  {
    XII_LOCK(m_State->m_Mutex);
    m_State->m_Frame = 0;
    m_State->m_Context.InvalidateHistory();
  }
}
xiiResult xiiDDGIProbeSchedulePass::Build(xiiRenderPipelinePassContext& context)
{
  if (m_bRequireActivation && !context.GetInputTexture(m_Activation).IsValid())
    return XII_SUCCESS;
  if (m_bSkipOnRayHardware && xiiRayTracingResources::IsHardwareRayTracingSupported())
    return XII_SUCCESS;
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (m_bRequireActivation && m_bDirectional && (!device || device->GetFeatures().m_TextureUAVExtendedFormats != xiiGALDeviceFeatureState::Enabled))
    return XII_SUCCESS;
  auto camera = m_CameraPosition;
  if (m_bUseViewCamera)
  {
    if (!context.m_pView || !context.m_pView->GetCamera())
      return context.ReportFailure("DDGI scheduling requires a view camera or an authored position.");
    camera = context.m_pView->GetCamera()->GetPosition() + context.m_pView->GetCamera()->GetCenterDirForwards() * m_fForwardOffset;
  }
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "DDGICameraPosition", camera));
  const auto settings   = GetProbeSettings();
  bool       bNewVolume = !m_State;
  if (m_State)
  {
    // Graph callbacks may read the scheduling policy. Only this pass changes it.
    const auto& current = m_State->m_Context.GetConfiguration();
    bNewVolume          = settings.m_uiProbeCountX != current.m_uiProbeCountX || settings.m_uiProbeCountY != current.m_uiProbeCountY ||
      settings.m_uiProbeCountZ != current.m_uiProbeCountZ || settings.m_fProbeSpacing != current.m_fProbeSpacing ||
      settings.m_fMaximumRelocationDistance != current.m_fMaximumRelocationDistance;
  }
  if (bNewVolume)
  {
    xiiSharedPtr<xiiDDGIProbeSchedulerState> state = XII_DEFAULT_NEW(xiiDDGIProbeSchedulerState);
    XII_SUCCEED_OR_RETURN(state->m_Context.Initialize(settings));
    m_State = std::move(state);
  }
  XII_LOCK(m_State->m_Mutex);
  const auto& current = m_State->m_Context.GetConfiguration();
  if (settings.m_uiProbeUpdateBudget != current.m_uiProbeUpdateBudget || settings.m_fTemporalHysteresis != current.m_fTemporalHysteresis)
    XII_SUCCEED_OR_RETURN(m_State->m_Context.SetUpdatePolicy(settings.m_uiProbeUpdateBudget, settings.m_fTemporalHysteresis));
  const auto       count = settings.m_uiProbeCountX * settings.m_uiProbeCountY * settings.m_uiProbeCountZ;
  const xiiUInt32  width = xiiUInt32(xiiMath::Ceil(xiiMath::Sqrt(float(count))));
  const xiiSizeU32 size(width * (m_bDirectional ? XII_DDGI_DIRECTION_COUNT : 1U), (count + width - 1) / width);
  const auto*      irradiance = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_PreviousIrradiance));
  const auto*      distance   = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_PreviousDistance));
  const bool       history    = !bNewVolume && irradiance && distance && irradiance->m_Size == size && distance->m_Size == size;
  if (!history)
    m_State->m_Context.InvalidateHistory();
  if (m_State->m_Context.BeginFrame(camera, context.m_Graph.GetFrameIndex()).Failed())
    return context.ReportFailure("DDGI camera/frame exceeds the CPU scheduling domain or repeats a frame.");
  m_State->m_Frame                                = context.m_Graph.GetFrameIndex();
  xiiSharedPtr<xiiDDGIProbeFrameData> snapshot    = XII_NEW(xiiFoundation::GetAlignedAllocator(), xiiDDGIProbeFrameData, context.m_Graph.GetSetupSerial(), m_State->m_Frame, m_State);
  snapshot->m_AtlasSize                           = size;
  snapshot->m_bDirectional                        = m_bDirectional;
  snapshot->m_bHistoryValid                       = history;
  const auto minimum                              = m_State->m_Context.GetMinimumCell();
  snapshot->m_Constants.MinimumCellAndAtlasWidth  = xiiVec4I32(minimum.x, minimum.y, minimum.z, xiiInt32(width));
  snapshot->m_Constants.ProbeCountsAndUpdateCount = xiiVec4U32(settings.m_uiProbeCountX, settings.m_uiProbeCountY, settings.m_uiProbeCountZ, m_State->m_Context.GetScheduledUpdates().GetCount());
  snapshot->m_Constants.SpacingHysteresisDistance = xiiVec4(settings.m_fProbeSpacing, settings.m_fTemporalHysteresis, settings.m_fProbeSpacing * m_fTraceDistanceInProbeSpacings, float(m_State->m_Frame));
  for (const auto& probe : m_State->m_Context.GetProbes())
  {
    auto& record                    = snapshot->m_Probes.ExpandAndGetRef();
    record.m_vPositionAndLastUpdate = xiiVec4(probe.m_vWorldPosition + probe.m_vRelocationOffset, float(probe.m_uiLastUpdatedFrame));
    record.m_vCellAndFlags          = xiiVec4I32(probe.m_iCellX, probe.m_iCellY, probe.m_iCellZ, xiiInt32(probe.m_Flags.GetValue()));
  }
  for (const auto& update : m_State->m_Context.GetScheduledUpdates())
  {
    auto& record                       = snapshot->m_Updates.ExpandAndGetRef();
    record.m_vPositionAndHistoryWeight = xiiVec4(update.m_vWorldPosition, update.m_fHistoryWeight);
    record.m_vMetadata                 = xiiVec4U32(update.m_uiPhysicalProbe, 0, 0, 0);
  }
  return context.SetOutput(m_Schedule, snapshot);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_DDGIProbeSchedulePass);
