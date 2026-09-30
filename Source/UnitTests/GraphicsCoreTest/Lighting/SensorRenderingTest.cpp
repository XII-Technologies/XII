/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Lighting/SensorRendering.h>

XII_CREATE_SIMPLE_TEST(Lighting, SensorRendering)
{
  XII_TEST_BOOL(xiiSensorRenderingManager::IsInitialized());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reflection")
  {
    const xiiRTTI* pProfileType = xiiGetStaticRTTI<xiiSensorProfile>();
    XII_TEST_BOOL(pProfileType->FindPropertyByName("Type") != nullptr);
    XII_TEST_BOOL(pProfileType->FindPropertyByName("FocalLengthXPixels") != nullptr);
    XII_TEST_BOOL(pProfileType->FindPropertyByName("SpectralSensitivity") != nullptr);
    XII_TEST_BOOL(pProfileType->FindPropertyByName("NoiseModel") != nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Stable deduplicated profiles")
  {
    xiiSensorProfile profile;
    profile.m_Type = xiiSensorType::InfraredCamera;
    profile.m_fWavelengthNanometers = 850.0f;
    profile.m_vSpectralSensitivity = xiiVec3(0.02f, 0.18f, 0.80f);

    xiiSensorProfileHandle hFirst;
    xiiSensorProfileHandle hSecond;
    XII_TEST_BOOL(xiiSensorRenderingManager::AcquireProfile(profile, hFirst).Succeeded());
    XII_TEST_BOOL(xiiSensorRenderingManager::AcquireProfile(profile, hSecond).Succeeded());
    XII_TEST_BOOL(hFirst.IsValid());
    XII_TEST_BOOL(hFirst == hSecond);

    xiiSensorProfile resolved;
    XII_TEST_BOOL(xiiSensorRenderingManager::GetProfile(hFirst, resolved).Succeeded());
    XII_TEST_INT(resolved.m_Type.GetValue(), xiiSensorType::InfraredCamera);
    XII_TEST_FLOAT(resolved.m_fWavelengthNanometers, 850.0f, 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Physical validation")
  {
    xiiSensorProfile invalid;
    invalid.m_fFarPlaneMeters = invalid.m_fNearPlaneMeters;
    XII_TEST_BOOL(!xiiSensorRenderingManager::IsValidProfile(invalid));

    invalid = {};
    invalid.m_vSpectralSensitivity = xiiVec3::MakeZero();
    XII_TEST_BOOL(!xiiSensorRenderingManager::IsValidProfile(invalid));

    invalid = {};
    invalid.m_Shutter = xiiSensorShutterType::Global;
    invalid.m_fRollingShutterSeconds = 0.01f;
    XII_TEST_BOOL(!xiiSensorRenderingManager::IsValidProfile(invalid));

    xiiSensorProfileHandle hInvalid;
    XII_TEST_BOOL(xiiSensorRenderingManager::AcquireProfile(invalid, hInvalid).Failed());
    XII_TEST_BOOL(!hInvalid.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Registry statistics")
  {
    const xiiSensorProfileRegistryStats stats = xiiSensorRenderingManager::GetRegistryStats();
    XII_TEST_BOOL(stats.m_uiProfileCount >= 2U);
    XII_TEST_BOOL(stats.m_uiRGBProfileCount >= 1U);
    XII_TEST_BOOL(stats.m_uiInfraredProfileCount >= 1U);
    XII_TEST_BOOL(stats.m_uiDepthProfileCount >= 1U);
    XII_TEST_BOOL(stats.m_uiLiDARProfileCount >= 1U);
    XII_TEST_BOOL(xiiSensorRenderingManager::GetDefaultProfileHandle(xiiSensorType::LiDAR).IsValid());
  }
}

