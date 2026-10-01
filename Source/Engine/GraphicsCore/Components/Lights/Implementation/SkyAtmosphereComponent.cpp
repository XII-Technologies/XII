/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLightScatteringTechnique, 1)
  XII_ENUM_CONSTANT(xiiLightScatteringTechnique::EpipolarSampling),
  XII_ENUM_CONSTANT(xiiLightScatteringTechnique::BruteForceRayMarching)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLightScatteringCascadeProcessingMode, 1)
  XII_ENUM_CONSTANT(xiiLightScatteringCascadeProcessingMode::SinglePass),
  XII_ENUM_CONSTANT(xiiLightScatteringCascadeProcessingMode::MultiPass),
  XII_ENUM_CONSTANT(xiiLightScatteringCascadeProcessingMode::MultiPassInstanced)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLightScatteringRefinementCriterion, 1)
  XII_ENUM_CONSTANT(xiiLightScatteringRefinementCriterion::Depth),
  XII_ENUM_CONSTANT(xiiLightScatteringRefinementCriterion::InscatteringChange)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLightScatteringExtinctionEvaluationMode, 1)
  XII_ENUM_CONSTANT(xiiLightScatteringExtinctionEvaluationMode::PerPixel),
  XII_ENUM_CONSTANT(xiiLightScatteringExtinctionEvaluationMode::EpipolarSlice)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLightScatteringSingleEvaluationMode, 1)
  XII_ENUM_CONSTANT(xiiLightScatteringSingleEvaluationMode::None),
  XII_ENUM_CONSTANT(xiiLightScatteringSingleEvaluationMode::Integration),
  XII_ENUM_CONSTANT(xiiLightScatteringSingleEvaluationMode::LookupTable)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLightScatteringHighOrderEvaluationMode, 1)
  XII_ENUM_CONSTANT(xiiLightScatteringHighOrderEvaluationMode::None),
  XII_ENUM_CONSTANT(xiiLightScatteringHighOrderEvaluationMode::UnoccludedOnly),
  XII_ENUM_CONSTANT(xiiLightScatteringHighOrderEvaluationMode::OccludedOnly)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkyAtmosphereRenderData, 1, xiiRTTIDefaultAllocator<xiiSkyAtmosphereRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSkyAtmosphereComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Atmosphere", GetAtmosphereSettings, SetAtmosphereSettings),
    XII_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Lighting"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSkyAtmosphereComponent::xiiSkyAtmosphereComponent()  = default;
xiiSkyAtmosphereComponent::~xiiSkyAtmosphereComponent() = default;

void xiiSkyAtmosphereComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& stream = inout_stream.GetStream();

  stream << m_AtmosphereSettings.m_fPlanetRadiusKm;
  stream << m_AtmosphereSettings.m_fAtmosphereRadiusKm;
  stream << m_AtmosphereSettings.m_fRayleighScaleHeightKm;
  stream << m_AtmosphereSettings.m_fMieScaleHeightKm;
  stream << m_AtmosphereSettings.m_vRayleighScattering;
  stream << m_AtmosphereSettings.m_vMieScattering;
  stream << m_AtmosphereSettings.m_vMieAbsorption;
  stream << m_AtmosphereSettings.m_vOzoneAbsorption;
  stream << m_AtmosphereSettings.m_vPlanetUpDirection;
  stream << m_AtmosphereSettings.m_fGroundAltitudeMeters;
  stream << m_AtmosphereSettings.m_fMiePhaseG;
  stream << m_AtmosphereSettings.m_uiTransmittanceIntegrationSteps;
  stream << m_AtmosphereSettings.m_uiMultiScatterSqrtSamples;
  stream << m_iPriority;
}

void xiiSkyAtmosphereComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& stream = inout_stream.GetStream();

  stream >> m_AtmosphereSettings.m_fPlanetRadiusKm;
  stream >> m_AtmosphereSettings.m_fAtmosphereRadiusKm;
  stream >> m_AtmosphereSettings.m_fRayleighScaleHeightKm;
  stream >> m_AtmosphereSettings.m_fMieScaleHeightKm;
  stream >> m_AtmosphereSettings.m_vRayleighScattering;
  stream >> m_AtmosphereSettings.m_vMieScattering;
  stream >> m_AtmosphereSettings.m_vMieAbsorption;
  stream >> m_AtmosphereSettings.m_vOzoneAbsorption;
  stream >> m_AtmosphereSettings.m_vPlanetUpDirection;
  stream >> m_AtmosphereSettings.m_fGroundAltitudeMeters;
  stream >> m_AtmosphereSettings.m_fMiePhaseG;
  stream >> m_AtmosphereSettings.m_uiTransmittanceIntegrationSteps;
  stream >> m_AtmosphereSettings.m_uiMultiScatterSqrtSamples;
  stream >> m_iPriority;
}

xiiResult xiiSkyAtmosphereComponent::GetLocalBounds(xiiBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(out_bounds);
  XII_IGNORE_UNUSED(ref_msg);
  out_bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiSkyAtmosphereComponent::SetAtmosphereSettings(const xiiAtmosphereSettings& settings)
{
  m_AtmosphereSettings = settings;
  InvalidateCachedRenderData();
}

const xiiAtmosphereSettings& xiiSkyAtmosphereComponent::GetAtmosphereSettings() const
{
  return m_AtmosphereSettings;
}

void xiiSkyAtmosphereComponent::SetPriority(xiiInt32 iPriority)
{
  if (m_iPriority == iPriority)
    return;

  m_iPriority = iPriority;
  InvalidateCachedRenderData();
}

xiiInt32 xiiSkyAtmosphereComponent::GetPriority() const
{
  return m_iPriority;
}

void xiiSkyAtmosphereComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  const xiiRenderWorldModule* pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiSkyAtmosphereRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiSkyAtmosphereRenderData>(this);
  pRenderData->m_AtmosphereSettings       = m_AtmosphereSettings;
  pRenderData->m_iPriority                = m_iPriority;
  pRenderData->m_uiSortingKey             = GetUniqueIdForRendering();
  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_SkyAtmosphereComponent);
