/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Components/Fog/VolumetricCloudComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Lighting/Atmosphere.h>

XII_CREATE_SIMPLE_TEST(Lighting, Atmosphere)
{
  XII_TEST_BOOL(xiiAtmosphereManager::IsInitialized());

  const xiiRTTI* pComponentType = xiiGetStaticRTTI<xiiSkyAtmosphereComponent>();
  XII_TEST_BOOL(pComponentType->FindPropertyByName("Atmosphere") != nullptr);
  XII_TEST_BOOL(pComponentType->FindPropertyByName("Priority") != nullptr);

  const xiiRTTI* pCloudComponentType = xiiGetStaticRTTI<xiiVolumetricCloudComponent>();
  XII_TEST_BOOL(pCloudComponentType->FindPropertyByName("Cloud") != nullptr);
  XII_TEST_BOOL(pCloudComponentType->FindPropertyByName("Priority") != nullptr);

  const xiiAtmosphereSettings original = xiiAtmosphereManager::GetConfiguration();
  const xiiUInt64 uiOriginalRevision = xiiAtmosphereManager::GetConfigurationRevision();

  xiiAtmosphereSettings modified = original;
  modified.m_fMiePhaseG = 0.72f;
  XII_TEST_BOOL(xiiAtmosphereManager::Configure(modified).Succeeded());
  XII_TEST_BOOL(xiiAtmosphereManager::GetConfigurationRevision() > uiOriginalRevision);
  XII_TEST_BOOL(xiiAtmosphereManager::IsGenerationPending());

  const xiiUInt64 uiModifiedRevision = xiiAtmosphereManager::GetConfigurationRevision();
  xiiAtmosphereManager::MarkLUTsGenerated(uiModifiedRevision - 1U);
  XII_TEST_BOOL(xiiAtmosphereManager::IsGenerationPending());
  xiiAtmosphereManager::MarkLUTsGenerated(uiModifiedRevision);
  XII_TEST_BOOL(!xiiAtmosphereManager::IsGenerationPending());

  xiiAtmosphereSettings invalid = modified;
  invalid.m_fAtmosphereRadiusKm = invalid.m_fPlanetRadiusKm;
  XII_TEST_BOOL(xiiAtmosphereManager::Configure(invalid).Failed());
  XII_TEST_INT(xiiAtmosphereManager::GetConfigurationRevision(), uiModifiedRevision);

  invalid = modified;
  invalid.m_vPlanetUpDirection = xiiVec3::MakeZero();
  XII_TEST_BOOL(xiiAtmosphereManager::Configure(invalid).Failed());
  XII_TEST_INT(xiiAtmosphereManager::GetConfigurationRevision(), uiModifiedRevision);

  xiiAtmosphereSettings second = modified;
  second.m_fMiePhaseG = 0.71f;
  xiiAtmosphereLUTHandle hModified;
  xiiAtmosphereLUTHandle hSecond;
  XII_TEST_BOOL(xiiAtmosphereManager::AcquireLUTs(modified, hModified).Succeeded());
  XII_TEST_BOOL(xiiAtmosphereManager::AcquireLUTs(second, hSecond).Succeeded());
  XII_TEST_BOOL(hModified.IsValid());
  XII_TEST_BOOL(hSecond.IsValid());
  XII_TEST_BOOL(!(hModified == hSecond));
  xiiAtmosphereManager::MarkLUTsGenerated(hModified);
  XII_TEST_BOOL(!xiiAtmosphereManager::IsGenerationPending(hModified));
  XII_TEST_BOOL(xiiAtmosphereManager::IsGenerationPending(hSecond));

  XII_TEST_BOOL(xiiAtmosphereManager::Configure(original).Succeeded());
}
