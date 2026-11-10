/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Math.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Pipeline/RenderConfiguration.h>

#include <bit>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderGlobalIllumination, 1)
  XII_ENUM_CONSTANTS(xiiRenderGlobalIllumination::Inherit, xiiRenderGlobalIllumination::PipelineDefault, xiiRenderGlobalIllumination::Disabled,
    xiiRenderGlobalIllumination::ScreenSpace, xiiRenderGlobalIllumination::DDGI, xiiRenderGlobalIllumination::RayTraced)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderFeaturePolicy, 1)
  XII_ENUM_CONSTANTS(xiiRenderFeaturePolicy::Inherit, xiiRenderFeaturePolicy::PipelineDefault, xiiRenderFeaturePolicy::Disabled)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDDGIProbeOverrides, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiDDGIProbeOverrides>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ProbeCountX", m_uiProbeCountX)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 256U)),
    XII_MEMBER_PROPERTY("ProbeCountY", m_uiProbeCountY)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 256U)),
    XII_MEMBER_PROPERTY("ProbeCountZ", m_uiProbeCountZ)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 256U)),
    XII_MEMBER_PROPERTY("ProbeUpdateBudget", m_uiProbeUpdateBudget)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 1048576U)),
    XII_MEMBER_PROPERTY("ProbeSpacing", m_fProbeSpacing)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("TemporalHysteresis", m_fTemporalHysteresis)->AddAttributes(new xiiDefaultValueAttribute(-1.0f), new xiiClampValueAttribute(-1.0f, 0.9999f)),
    XII_MEMBER_PROPERTY("MaximumRelocationDistance", m_fMaximumRelocationDistance)->AddAttributes(new xiiDefaultValueAttribute(-1.0f), new xiiClampValueAttribute(-1.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderConfiguration, xiiNoBase, 4, xiiRTTIDefaultAllocator<xiiRenderConfiguration>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BloomStrength", m_fBloomStrength)->AddAttributes(new xiiDefaultValueAttribute(xiiRenderConfiguration::s_fInheritBloomStrength), new xiiClampValueAttribute(-1.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("RenderScale", m_fRenderScale)->AddAttributes(new xiiDefaultValueAttribute(xiiRenderConfiguration::s_fInheritRenderScale), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ENUM_MEMBER_PROPERTY("GlobalIllumination", xiiRenderGlobalIllumination, m_GlobalIllumination),
    XII_MEMBER_PROPERTY("DDGI", m_DDGI),
    XII_ENUM_MEMBER_PROPERTY("Shadows", xiiRenderFeaturePolicy, m_Shadows),
    XII_ENUM_MEMBER_PROPERTY("ScreenSpaceReflections", xiiRenderFeaturePolicy, m_ScreenSpaceReflections),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  template <typename Settings>
  void ApplyDDGIProbeOverrides(Settings& settings, const xiiDDGIProbeOverrides& overrides)
  {
    if (overrides.m_uiProbeCountX != 0)
      settings.m_uiProbeCountX = overrides.m_uiProbeCountX;
    if (overrides.m_uiProbeCountY != 0)
      settings.m_uiProbeCountY = overrides.m_uiProbeCountY;
    if (overrides.m_uiProbeCountZ != 0)
      settings.m_uiProbeCountZ = overrides.m_uiProbeCountZ;
    if (overrides.m_uiProbeUpdateBudget != 0)
      settings.m_uiProbeUpdateBudget = overrides.m_uiProbeUpdateBudget;
    if (overrides.m_fProbeSpacing != 0.0f)
      settings.m_fProbeSpacing = overrides.m_fProbeSpacing;
    if (overrides.m_fTemporalHysteresis != -1.0f)
      settings.m_fTemporalHysteresis = overrides.m_fTemporalHysteresis;
    if (overrides.m_fMaximumRelocationDistance != -1.0f)
      settings.m_fMaximumRelocationDistance = overrides.m_fMaximumRelocationDistance;
  }
} // namespace

bool xiiDDGIProbeOverrides::IsValid() const
{
  return m_uiProbeCountX <= 256 && m_uiProbeCountY <= 256 && m_uiProbeCountZ <= 256 && m_uiProbeUpdateBudget <= 1048576 &&
    xiiMath::IsFinite(m_fProbeSpacing) && m_fProbeSpacing >= 0.0f &&
    (m_fTemporalHysteresis == -1.0f || (m_fTemporalHysteresis >= 0.0f && m_fTemporalHysteresis < 1.0f)) &&
    xiiMath::IsFinite(m_fMaximumRelocationDistance) && (m_fMaximumRelocationDistance == -1.0f || m_fMaximumRelocationDistance >= 0.0f);
}

void xiiDDGIProbeOverrides::ApplyOverrides(const xiiDDGIProbeOverrides& overrides)
{
  ApplyDDGIProbeOverrides(*this, overrides);
}

void xiiDDGIProbeOverrides::ApplyTo(xiiDDGISettings& settings) const
{
  ApplyDDGIProbeOverrides(settings, *this);
}

bool xiiRenderConfiguration::IsValid() const
{
  return xiiMath::IsFinite(m_fBloomStrength) && (m_fBloomStrength == s_fInheritBloomStrength || m_fBloomStrength >= 0.0f) &&
    (m_fRenderScale == s_fInheritRenderScale || (m_fRenderScale >= 0.1f && m_fRenderScale <= 1.0f)) &&
    m_GlobalIllumination.GetValue() < xiiRenderGlobalIllumination::ENUM_COUNT && m_DDGI.IsValid() &&
    m_Shadows.GetValue() <= xiiRenderFeaturePolicy::Disabled && m_ScreenSpaceReflections.GetValue() <= xiiRenderFeaturePolicy::Disabled;
}

xiiRenderPipelineVariant xiiRenderConfiguration::GetPipelineVariant() const
{
  xiiRenderPipelineVariant variant;
  variant.m_GlobalIllumination = static_cast<xiiRenderGlobalIllumination::Enum>(m_GlobalIllumination.GetValue());
  variant.m_Disabled.AddOrRemove(xiiOptionalRenderFeature::Shadows, m_Shadows == xiiRenderFeaturePolicy::Disabled);
  variant.m_Disabled.AddOrRemove(xiiOptionalRenderFeature::ScreenSpaceReflections, m_ScreenSpaceReflections == xiiRenderFeaturePolicy::Disabled);
  return variant;
}

void xiiRenderConfiguration::ApplyOverrides(const xiiRenderConfiguration& overrides)
{
  if (overrides.m_fBloomStrength != s_fInheritBloomStrength)
    m_fBloomStrength = overrides.m_fBloomStrength;
  if (overrides.m_fRenderScale != s_fInheritRenderScale)
    m_fRenderScale = overrides.m_fRenderScale;
  if (overrides.m_GlobalIllumination != xiiRenderGlobalIllumination::Inherit)
    m_GlobalIllumination = overrides.m_GlobalIllumination;
  m_DDGI.ApplyOverrides(overrides.m_DDGI);
  if (overrides.m_Shadows != xiiRenderFeaturePolicy::Inherit)
    m_Shadows = overrides.m_Shadows;
  if (overrides.m_ScreenSpaceReflections != xiiRenderFeaturePolicy::Inherit)
    m_ScreenSpaceReflections = overrides.m_ScreenSpaceReflections;
}

xiiResult xiiRenderConfiguration::Serialize(xiiStreamWriter& stream) const
{
  if (!IsValid())
    return XII_FAILURE;
  const auto uiBloomStrength = std::bit_cast<xiiUInt32>(m_fBloomStrength);
  const auto uiRenderScale   = std::bit_cast<xiiUInt32>(m_fRenderScale);
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&uiBloomStrength));
  XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&uiRenderScale));
  const xiiUInt8 uiMode = m_GlobalIllumination.GetValue();
  XII_SUCCEED_OR_RETURN(stream.WriteBytes(&uiMode, sizeof(uiMode)));
  const xiiUInt32 values[] = {m_DDGI.m_uiProbeCountX, m_DDGI.m_uiProbeCountY, m_DDGI.m_uiProbeCountZ, m_DDGI.m_uiProbeUpdateBudget,
                              std::bit_cast<xiiUInt32>(m_DDGI.m_fProbeSpacing), std::bit_cast<xiiUInt32>(m_DDGI.m_fTemporalHysteresis), std::bit_cast<xiiUInt32>(m_DDGI.m_fMaximumRelocationDistance)};
  for (const auto& value : values)
    XII_SUCCEED_OR_RETURN(stream.WriteDWordValue(&value));
  const xiiUInt8 policies[] = {m_Shadows.GetValue(), m_ScreenSpaceReflections.GetValue()};
  return stream.WriteBytes(policies, sizeof(policies));
}

xiiResult xiiRenderConfiguration::Deserialize(xiiStreamReader& stream, xiiUInt32 uiVersion)
{
  if (uiVersion == 0 || uiVersion > s_uiSerializationVersion)
    return XII_FAILURE;
  xiiRenderConfiguration configuration;
  xiiUInt32              uiBloomStrength = 0;
  xiiUInt32              uiRenderScale   = 0;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&uiBloomStrength));
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&uiRenderScale));
  configuration.m_fBloomStrength = std::bit_cast<float>(uiBloomStrength);
  configuration.m_fRenderScale   = std::bit_cast<float>(uiRenderScale);
  if (uiVersion >= 2)
  {
    xiiUInt8 uiMode = 0;
    if (stream.ReadBytes(&uiMode, sizeof(uiMode)) != sizeof(uiMode))
      return XII_FAILURE;
    configuration.m_GlobalIllumination = static_cast<xiiRenderGlobalIllumination::Enum>(uiMode);
  }
  if (uiVersion >= 3)
  {
    xiiUInt32 values[7] = {};
    for (auto& value : values)
      XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&value));
    configuration.m_DDGI = {values[0], values[1], values[2], values[3],
                            std::bit_cast<float>(values[4]), std::bit_cast<float>(values[5]), std::bit_cast<float>(values[6])};
  }
  if (uiVersion >= 4)
  {
    xiiUInt8 policies[2] = {};
    if (stream.ReadBytes(policies, sizeof(policies)) != sizeof(policies))
      return XII_FAILURE;
    configuration.m_Shadows                = static_cast<xiiRenderFeaturePolicy::Enum>(policies[0]);
    configuration.m_ScreenSpaceReflections = static_cast<xiiRenderFeaturePolicy::Enum>(policies[1]);
  }
  if (!configuration.IsValid())
    return XII_FAILURE;
  *this = configuration;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderConfiguration);
