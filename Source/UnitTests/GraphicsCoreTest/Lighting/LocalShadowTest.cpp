/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/LightingSystem.h>
#include <GraphicsCore/Lighting/LocalShadow.h>

XII_CREATE_SIMPLE_TEST(Lighting, LocalShadows)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Collision-free spot and point allocations")
  {
    xiiDynamicArray<xiiGpuLightData> lights;
    lights.SetCount(3U);
    xiiMemoryUtils::ZeroFill(lights.GetData(), lights.GetCount());

    lights[0].m_Metadata.z                   = static_cast<xiiUInt32>(xiiLightingSystem::LightType::Spot);
    lights[0].m_DirectionAndType             = xiiVec4(1.0f, 0.0f, 0.0f, static_cast<float>(xiiLightingSystem::LightType::Spot));
    lights[0].m_OrientationRightAndIES       = xiiVec4(0.0f, 1.0f, 0.0f, 0.0f);
    lights[0].m_AttenuationAndSize           = xiiVec4(20.0f, 0.1f, 0.0f, 0.0f);
    lights[0].m_SpotAnglesAndRectSize.y      = xiiMath::Cos(xiiAngle::MakeFromDegree(25.0f));
    lights[0].m_ShadowData.x                 = 1.0f;

    for (xiiUInt32 i = 1U; i < lights.GetCount(); ++i)
    {
      lights[i].m_Metadata.z         = static_cast<xiiUInt32>(xiiLightingSystem::LightType::Point);
      lights[i].m_DirectionAndType.w = static_cast<float>(xiiLightingSystem::LightType::Point);
      lights[i].m_PositionAndInvRange = xiiVec4(static_cast<float>(i) * 4.0f, 0.0f, 1.0f, 0.05f);
      lights[i].m_AttenuationAndSize = xiiVec4(20.0f, 0.1f, 0.0f, 0.0f);
      lights[i].m_ShadowData.x       = 1.0f;
    }

    xiiLocalShadowAtlasSettings settings;
    settings.m_uiAtlasSize = 1024U;
    settings.m_uiTileSize  = 256U;

    xiiLocalShadowAtlasDataArray shadowData;
    xiiLocalShadowAtlasStatistics statistics;
    xiiLocalShadowAtlasBuilder::Build(settings, lights, shadowData, statistics);

    XII_TEST_INT(statistics.m_uiRequestedLights, 3U);
    XII_TEST_INT(statistics.m_uiAllocatedLights, 3U);
    XII_TEST_INT(statistics.m_uiDroppedLights, 0U);
    XII_TEST_INT(statistics.m_uiAllocatedFaces, 13U);
    XII_TEST_INT(statistics.m_uiTileCapacity, 16U);
    XII_TEST_INT(shadowData[0].m_Metadata.x, 1U);
    XII_TEST_INT(shadowData[1].m_Metadata.x, 6U);
    XII_TEST_INT(shadowData[2].m_Metadata.x, 6U);

    xiiHashSet<xiiUInt32> allocatedTiles;
    for (const xiiLocalShadowAtlasData& shadow : shadowData)
    {
      for (xiiUInt32 uiFace = 0U; uiFace < shadow.m_Metadata.x; ++uiFace)
      {
        const xiiVec4 scaleBias = shadow.m_AtlasScaleBias[uiFace];
        const xiiUInt32 uiTileX = static_cast<xiiUInt32>(scaleBias.z / scaleBias.x + 0.5f);
        const xiiUInt32 uiTileY = static_cast<xiiUInt32>(scaleBias.w / scaleBias.y + 0.5f);
        const xiiUInt32 uiTile = uiTileY * 4U + uiTileX;
        XII_TEST_BOOL(!allocatedTiles.Contains(uiTile));
        allocatedTiles.Insert(uiTile);
        XII_TEST_BOOL(shadow.m_ViewProjection[uiFace].IsValid());
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cubemaps are admitted atomically")
  {
    xiiGpuLightData lights[2];
    xiiMemoryUtils::ZeroFill(lights, 2U);
    lights[0].m_Metadata.z = static_cast<xiiUInt32>(xiiLightingSystem::LightType::Spot);
    lights[0].m_DirectionAndType = xiiVec4(1.0f, 0.0f, 0.0f, static_cast<float>(xiiLightingSystem::LightType::Spot));
    lights[0].m_OrientationRightAndIES = xiiVec4(0.0f, 1.0f, 0.0f, 0.0f);
    lights[0].m_AttenuationAndSize = xiiVec4(10.0f, 0.1f, 0.0f, 0.0f);
    lights[0].m_SpotAnglesAndRectSize.y = xiiMath::Cos(xiiAngle::MakeFromDegree(30.0f));
    lights[0].m_ShadowData.x = 1.0f;
    lights[1].m_Metadata.z = static_cast<xiiUInt32>(xiiLightingSystem::LightType::Point);
    lights[1].m_AttenuationAndSize = xiiVec4(10.0f, 0.1f, 0.0f, 0.0f);
    lights[1].m_ShadowData.x = 1.0f;

    xiiLocalShadowAtlasSettings settings;
    settings.m_uiAtlasSize = 512U;
    settings.m_uiTileSize  = 256U;

    xiiLocalShadowAtlasDataArray shadowData;
    xiiLocalShadowAtlasStatistics statistics;
    xiiLocalShadowAtlasBuilder::Build(settings, lights, shadowData, statistics);

    XII_TEST_INT(statistics.m_uiAllocatedLights, 1U);
    XII_TEST_INT(statistics.m_uiDroppedLights, 1U);
    XII_TEST_INT(statistics.m_uiAllocatedFaces, 1U);
    XII_TEST_INT(shadowData[0].m_Metadata.z, 1U);
    XII_TEST_INT(shadowData[1].m_Metadata.z, 0U);
  }
}
