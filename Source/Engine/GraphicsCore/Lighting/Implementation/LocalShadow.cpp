/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Lighting/LightingSystem.h>
#include <GraphicsCore/Lighting/LocalShadow.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiLocalShadowAtlasData, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("ViewProjection", GetReflectedFaceCount, GetReflectedViewProjection),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AtlasScaleBias", GetReflectedFaceCount, GetReflectedAtlasScaleBias),
    XII_MEMBER_PROPERTY("LightPositionAndInvRange", m_LightPositionAndInvRange)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("Metadata", m_Metadata)->AddAttributes(new xiiReadOnlyAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiLocalShadowAtlasSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiLocalShadowAtlasSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("AtlasSize", m_uiAtlasSize)->AddAttributes(new xiiClampValueAttribute(64U, 16384U), new xiiDefaultValueAttribute(4096U)),
    XII_MEMBER_PROPERTY("TileSize", m_uiTileSize)->AddAttributes(new xiiClampValueAttribute(64U, 4096U), new xiiDefaultValueAttribute(256U)),
    XII_MEMBER_PROPERTY("MinimumNearPlane", m_fMinimumNearPlane)->AddAttributes(new xiiClampValueAttribute(0.001f, 10.0f), new xiiDefaultValueAttribute(0.01f), new xiiSuffixAttribute(" m")),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiLocalShadowAtlasStatistics, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiLocalShadowAtlasStatistics>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RequestedLights", m_uiRequestedLights)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("AllocatedLights", m_uiAllocatedLights)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("DroppedLights", m_uiDroppedLights)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("AllocatedFaces", m_uiAllocatedFaces)->AddAttributes(new xiiReadOnlyAttribute()),
    XII_MEMBER_PROPERTY("TileCapacity", m_uiTileCapacity)->AddAttributes(new xiiReadOnlyAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  static const xiiVec3 s_FaceDirections[6] = {
    xiiVec3(1.0f, 0.0f, 0.0f), xiiVec3(-1.0f, 0.0f, 0.0f),
    xiiVec3(0.0f, 1.0f, 0.0f), xiiVec3(0.0f, -1.0f, 0.0f),
    xiiVec3(0.0f, 0.0f, 1.0f), xiiVec3(0.0f, 0.0f, -1.0f)};

  static const xiiVec3 s_FaceUp[6] = {
    xiiVec3(0.0f, 1.0f, 0.0f), xiiVec3(0.0f, 1.0f, 0.0f),
    xiiVec3(0.0f, 0.0f, -1.0f), xiiVec3(0.0f, 0.0f, 1.0f),
    xiiVec3(0.0f, 1.0f, 0.0f), xiiVec3(0.0f, 1.0f, 0.0f)};

  xiiUInt32 GetFaceCount(xiiLightingSystem::LightType type)
  {
    switch (type)
    {
      case xiiLightingSystem::LightType::Point:
      case xiiLightingSystem::LightType::Sphere:
      case xiiLightingSystem::LightType::Tube:
        return 6U;
      case xiiLightingSystem::LightType::Spot:
      case xiiLightingSystem::LightType::Rectangle:
      case xiiLightingSystem::LightType::Disc:
        return 1U;
      default:
        return 0U;
    }
  }

  xiiAngle GetProjectionFieldOfView(const xiiGpuLightData& light, xiiLightingSystem::LightType type)
  {
    if (type == xiiLightingSystem::LightType::Spot)
    {
      const xiiAngle outerHalfAngle = xiiMath::ACos(xiiMath::Clamp(light.m_SpotAnglesAndRectSize.y, -1.0f, 1.0f));
      return xiiAngle::MakeFromRadian(xiiMath::Clamp(outerHalfAngle.GetRadian() * 2.0f, xiiAngle::MakeFromDegree(1.0f).GetRadian(), xiiAngle::MakeFromDegree(175.0f).GetRadian()));
    }

    return GetFaceCount(type) == 6U ? xiiAngle::MakeFromDegree(90.0f) : xiiAngle::MakeFromDegree(120.0f);
  }

  xiiVec3 GetProjectedLightUp(const xiiGpuLightData& light, const xiiVec3& vDirection)
  {
    xiiVec3 vUp = light.m_OrientationRightAndIES.GetAsVec3().CrossRH(vDirection);
    if (vUp.NormalizeIfNotZero(xiiVec3(0.0f, 0.0f, 1.0f)).Failed() || xiiMath::Abs(vUp.Dot(vDirection)) > 0.99f)
      vUp = xiiMath::Abs(vDirection.z) < 0.99f ? xiiVec3(0.0f, 0.0f, 1.0f) : xiiVec3(0.0f, 1.0f, 0.0f);
    return vUp;
  }
} // namespace

void xiiLocalShadowAtlasBuilder::Build(const xiiLocalShadowAtlasSettings& settings, xiiArrayPtr<const xiiGpuLightData> lights,
  xiiLocalShadowAtlasDataArray& out_shadowData, xiiLocalShadowAtlasStatistics& out_statistics)
{
  out_statistics = {};
  out_shadowData.SetCount(lights.GetCount());
  xiiMemoryUtils::ZeroFill(out_shadowData.GetData(), out_shadowData.GetCount());

  const xiiUInt32 uiAtlasSize = xiiMath::Max(settings.m_uiAtlasSize, 1U);
  const xiiUInt32 uiTileSize  = xiiMath::Clamp(xiiMath::PowerOfTwo_Floor(xiiMath::Max(settings.m_uiTileSize, 1U)), 1U, uiAtlasSize);
  const xiiUInt32 uiTilesPerAxis = xiiMath::Max(uiAtlasSize / uiTileSize, 1U);
  const xiiUInt32 uiTileCapacity = uiTilesPerAxis * uiTilesPerAxis;
  const float fTileScale = static_cast<float>(uiTileSize) / static_cast<float>(uiAtlasSize);
  out_statistics.m_uiTileCapacity = uiTileCapacity;

  xiiUInt32 uiNextTile = 0U;
  for (xiiUInt32 uiLight = 0U; uiLight < lights.GetCount(); ++uiLight)
  {
    const xiiGpuLightData& light = lights[uiLight];
    if (light.m_ShadowData.x <= 0.5f)
      continue;

    const auto type = static_cast<xiiLightingSystem::LightType>(light.m_Metadata.z);
    const xiiUInt32 uiFaceCount = GetFaceCount(type);
    if (uiFaceCount == 0U)
      continue;

    ++out_statistics.m_uiRequestedLights;
    if (uiFaceCount > uiTileCapacity - xiiMath::Min(uiNextTile, uiTileCapacity))
    {
      ++out_statistics.m_uiDroppedLights;
      continue;
    }

    const xiiVec3 vPosition = light.m_PositionAndInvRange.GetAsVec3();
    const float fRange      = xiiMath::Max(light.m_AttenuationAndSize.x, settings.m_fMinimumNearPlane * 2.0f);
    const float fNearPlane  = xiiMath::Min(xiiMath::Max(settings.m_fMinimumNearPlane, light.m_AttenuationAndSize.y * 0.05f), fRange * 0.5f);
    const xiiAngle fieldOfView = GetProjectionFieldOfView(light, type);
    const xiiMat4 projection = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(fieldOfView, 1.0f, fRange, fNearPlane,
      xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);

    xiiLocalShadowAtlasData& shadow = out_shadowData[uiLight];
    shadow.m_LightPositionAndInvRange = xiiVec4(vPosition, 1.0f / fRange);
    shadow.m_Metadata = xiiVec4U32(uiFaceCount, uiTileSize, 1U, static_cast<xiiUInt32>(type));

    for (xiiUInt32 uiFace = 0U; uiFace < uiFaceCount; ++uiFace)
    {
      xiiVec3 vDirection;
      xiiVec3 vUp;
      if (uiFaceCount == 6U)
      {
        vDirection = s_FaceDirections[uiFace];
        vUp        = s_FaceUp[uiFace];
      }
      else
      {
        vDirection = light.m_DirectionAndType.GetAsVec3();
        if (vDirection.NormalizeIfNotZero(xiiVec3(1.0f, 0.0f, 0.0f)).Failed())
          vDirection = xiiVec3(1.0f, 0.0f, 0.0f);
        vUp = GetProjectedLightUp(light, vDirection);
      }

      const xiiMat4 view = xiiGraphicsUtils::CreateLookAtViewMatrix(vPosition, vPosition + vDirection, vUp, xiiHandedness::LeftHanded);
      shadow.m_ViewProjection[uiFace] = projection * view;

      const xiiUInt32 uiTile = uiNextTile + uiFace;
      const xiiUInt32 uiTileX = uiTile % uiTilesPerAxis;
      const xiiUInt32 uiTileY = uiTile / uiTilesPerAxis;
      shadow.m_AtlasScaleBias[uiFace] = xiiVec4(fTileScale, fTileScale, static_cast<float>(uiTileX) * fTileScale, static_cast<float>(uiTileY) * fTileScale);
    }

    uiNextTile += uiFaceCount;
    ++out_statistics.m_uiAllocatedLights;
    out_statistics.m_uiAllocatedFaces += uiFaceCount;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_LocalShadow);
