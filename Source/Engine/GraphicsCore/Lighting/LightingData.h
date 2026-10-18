/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Foundation/Math/Vec4.h>
#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

enum class xiiGpuLightType : xiiUInt32
{
  Directional  = 0U,
  Point        = 1U,
  Spot         = 2U,
  Rectangle    = 3U,
  Disc         = 4U,
  Sphere       = 5U,
  Tube         = 6U,
  EmissiveMesh = 7U,
};

/// Light representation consumed by clustered lighting shaders.
///
/// The layout is intentionally 16-byte aligned and mirrors Data/Base/Shaders/Pipeline/LightingData.h.
struct XII_GRAPHICSCORE_DLL xiiGpuLightData
{
  XII_DECLARE_POD_TYPE();

  xiiVec4    m_PositionAndInvRange;    ///< xyz = world position, w = 1 / range.
  xiiVec4    m_DirectionAndType;       ///< xyz = world direction, w = xiiGpuLightType.
  xiiVec4    m_ColorAndIntensity;      ///< rgb = normalized linear color; w = cd (local), lx (directional), or nt (area).
  xiiVec4    m_AttenuationAndSize;     ///< x = range, y = source radius, z = tube length, w = reserved.
  xiiVec4    m_SpotAnglesAndRectSize;  ///< x = cos(inner half angle), y = cos(outer half angle), zw = rect extents.
  xiiVec4    m_ShadowData;             ///< x = casts shadow, y = camera fade-out distance in metres (zero is automatic/full), z = angular/source size, w = reserved.
  xiiVec4    m_BoundsCenterAndRadius;  ///< xyz = culling sphere center, w = culling sphere radius.
  xiiVec4    m_OrientationRightAndIES; ///< xyz = local right axis, w = compact IES profile index plus one (zero means none).
  xiiVec4U32 m_Metadata;               ///< x = stable light ID, y = compact frame index, z = LightType, w = 16-bit content revision.
};

static_assert(sizeof(xiiGpuLightData) == 144);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuLightData);
