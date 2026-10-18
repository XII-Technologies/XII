/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/LightingData.h>
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
