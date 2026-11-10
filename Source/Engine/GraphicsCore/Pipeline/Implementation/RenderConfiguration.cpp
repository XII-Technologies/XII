/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Math.h>
#include <GraphicsCore/Pipeline/RenderConfiguration.h>

#include <bit>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRenderGlobalIllumination, 1)
  XII_ENUM_CONSTANTS(xiiRenderGlobalIllumination::Inherit, xiiRenderGlobalIllumination::PipelineDefault, xiiRenderGlobalIllumination::Disabled,
    xiiRenderGlobalIllumination::ScreenSpace, xiiRenderGlobalIllumination::DDGI, xiiRenderGlobalIllumination::RayTraced)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRenderConfiguration, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiRenderConfiguration>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BloomStrength", m_fBloomStrength)->AddAttributes(new xiiDefaultValueAttribute(xiiRenderConfiguration::s_fInheritBloomStrength), new xiiClampValueAttribute(-1.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("RenderScale", m_fRenderScale)->AddAttributes(new xiiDefaultValueAttribute(xiiRenderConfiguration::s_fInheritRenderScale), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ENUM_MEMBER_PROPERTY("GlobalIllumination", xiiRenderGlobalIllumination, m_GlobalIllumination),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool xiiRenderConfiguration::IsValid() const
{
  return xiiMath::IsFinite(m_fBloomStrength) && (m_fBloomStrength == s_fInheritBloomStrength || m_fBloomStrength >= 0.0f) &&
    (m_fRenderScale == s_fInheritRenderScale || (m_fRenderScale >= 0.1f && m_fRenderScale <= 1.0f)) &&
    m_GlobalIllumination.GetValue() < xiiRenderGlobalIllumination::ENUM_COUNT;
}

void xiiRenderConfiguration::ApplyOverrides(const xiiRenderConfiguration& overrides)
{
  if (overrides.m_fBloomStrength != s_fInheritBloomStrength)
    m_fBloomStrength = overrides.m_fBloomStrength;
  if (overrides.m_fRenderScale != s_fInheritRenderScale)
    m_fRenderScale = overrides.m_fRenderScale;
  if (overrides.m_GlobalIllumination != xiiRenderGlobalIllumination::Inherit)
    m_GlobalIllumination = overrides.m_GlobalIllumination;
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
  return stream.WriteBytes(&uiMode, sizeof(uiMode));
}

xiiResult xiiRenderConfiguration::Deserialize(xiiStreamReader& stream, bool bHasGlobalIllumination)
{
  xiiRenderConfiguration configuration;
  xiiUInt32              uiBloomStrength = 0;
  xiiUInt32              uiRenderScale   = 0;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&uiBloomStrength));
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&uiRenderScale));
  configuration.m_fBloomStrength = std::bit_cast<float>(uiBloomStrength);
  configuration.m_fRenderScale   = std::bit_cast<float>(uiRenderScale);
  if (bHasGlobalIllumination)
  {
    xiiUInt8 uiMode = 0;
    if (stream.ReadBytes(&uiMode, sizeof(uiMode)) != sizeof(uiMode))
      return XII_FAILURE;
    configuration.m_GlobalIllumination = static_cast<xiiRenderGlobalIllumination::Enum>(uiMode);
  }
  if (!configuration.IsValid())
    return XII_FAILURE;
  *this = configuration;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderConfiguration);
