/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <cmath>
namespace
{
  xiiColor EvaluateTemperatureColor(xiiUInt32 uiTemperature)
  {
    const float fTemperature = static_cast<float>(xiiMath::Clamp(uiTemperature, 1000U, 50000U)) / 100.0f;

    float fRed   = 1.0f;
    float fGreen = 1.0f;
    float fBlue  = 1.0f;

    if (fTemperature <= 66.0f)
    {
      fRed   = 1.0f;
      fGreen = xiiMath::Clamp(0.39008158f * static_cast<float>(std::log(fTemperature)) - 0.63184144f, 0.0f, 1.0f);
      fBlue  = fTemperature <= 19.0f ? 0.0f : xiiMath::Clamp(0.5432068f * static_cast<float>(std::log(fTemperature - 10.0f)) - 1.1962541f, 0.0f, 1.0f);
    }
    else
    {
      fRed   = xiiMath::Clamp(1.2929362f * static_cast<float>(std::pow(fTemperature - 60.0f, -0.13320476f)), 0.0f, 1.0f);
      fGreen = xiiMath::Clamp(1.1298909f * static_cast<float>(std::pow(fTemperature - 60.0f, -0.07551485f)), 0.0f, 1.0f);
      fBlue  = 1.0f;
    }

    return xiiColor(fRed, fGreen, fBlue, 1.0f);
  }
} // namespace
xiiColor xiiEvaluateLightColor(const xiiColorLinearUB& color, xiiUInt32 uiTemperature)
{
  xiiColor result = color.ToLinearFloat() * EvaluateTemperatureColor(uiTemperature);
  result.a        = 1.0f;
  return result;
}

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuLightData, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuLightData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PositionAndInvRange", m_PositionAndInvRange),
    XII_MEMBER_PROPERTY("DirectionAndType", m_DirectionAndType),
    XII_MEMBER_PROPERTY("ColorAndIntensity", m_ColorAndIntensity),
    XII_MEMBER_PROPERTY("AttenuationAndSize", m_AttenuationAndSize),
    XII_MEMBER_PROPERTY("SpotAnglesAndRectSize", m_SpotAnglesAndRectSize),
    XII_MEMBER_PROPERTY("ShadowData", m_ShadowData),
    XII_MEMBER_PROPERTY("BoundsCenterAndRadius", m_BoundsCenterAndRadius),
    XII_MEMBER_PROPERTY("OrientationRightAndIES", m_OrientationRightAndIES),
    XII_MEMBER_PROPERTY("Metadata", m_Metadata),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_LightingData);
