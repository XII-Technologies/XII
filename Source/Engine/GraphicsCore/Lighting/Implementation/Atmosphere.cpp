/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>

namespace
{
  bool IsValid(const xiiAtmosphereSettings& settings)
  {
    return xiiMath::IsFinite(settings.m_fPlanetRadiusKm) && settings.m_fPlanetRadiusKm > 0.0f &&
           xiiMath::IsFinite(settings.m_fAtmosphereRadiusKm) && settings.m_fAtmosphereRadiusKm > settings.m_fPlanetRadiusKm &&
           xiiMath::IsFinite(settings.m_fRayleighScaleHeightKm) && settings.m_fRayleighScaleHeightKm > 0.0f &&
           xiiMath::IsFinite(settings.m_fMieScaleHeightKm) && settings.m_fMieScaleHeightKm > 0.0f &&
           settings.m_vRayleighScattering.IsValid() && settings.m_vMieScattering.IsValid() &&
           settings.m_vMieAbsorption.IsValid() && settings.m_vOzoneAbsorption.IsValid() &&
           settings.m_vRayleighScattering.x >= 0.0f && settings.m_vRayleighScattering.y >= 0.0f && settings.m_vRayleighScattering.z >= 0.0f &&
           settings.m_vMieScattering.x >= 0.0f && settings.m_vMieScattering.y >= 0.0f && settings.m_vMieScattering.z >= 0.0f &&
           settings.m_vMieAbsorption.x >= 0.0f && settings.m_vMieAbsorption.y >= 0.0f && settings.m_vMieAbsorption.z >= 0.0f &&
           settings.m_vOzoneAbsorption.x >= 0.0f && settings.m_vOzoneAbsorption.y >= 0.0f && settings.m_vOzoneAbsorption.z >= 0.0f &&
           xiiMath::IsFinite(settings.m_fMiePhaseG) && settings.m_fMiePhaseG > -1.0f && settings.m_fMiePhaseG < 1.0f &&
           settings.m_uiTransmittanceIntegrationSteps > 0U && settings.m_uiTransmittanceIntegrationSteps <= 1024U &&
           settings.m_uiMultiScatterSqrtSamples > 0U && settings.m_uiMultiScatterSqrtSamples <= 64U;
  }

  bool IsEqual(const xiiAtmosphereSettings& lhs, const xiiAtmosphereSettings& rhs)
  {
    return lhs.m_fPlanetRadiusKm == rhs.m_fPlanetRadiusKm && lhs.m_fAtmosphereRadiusKm == rhs.m_fAtmosphereRadiusKm &&
           lhs.m_fRayleighScaleHeightKm == rhs.m_fRayleighScaleHeightKm && lhs.m_fMieScaleHeightKm == rhs.m_fMieScaleHeightKm &&
           lhs.m_vRayleighScattering == rhs.m_vRayleighScattering && lhs.m_vMieScattering == rhs.m_vMieScattering &&
           lhs.m_vMieAbsorption == rhs.m_vMieAbsorption && lhs.m_vOzoneAbsorption == rhs.m_vOzoneAbsorption &&
           lhs.m_fMiePhaseG == rhs.m_fMiePhaseG &&
           lhs.m_uiTransmittanceIntegrationSteps == rhs.m_uiTransmittanceIntegrationSteps &&
           lhs.m_uiMultiScatterSqrtSamples == rhs.m_uiMultiScatterSqrtSamples;
  }
}

class xiiAtmosphereManager::State
{
public:
  xiiAtmosphereSettings         m_Settings;
  xiiSharedPtr<xiiGALTexture>   m_pTransmittanceLUT;
  xiiSharedPtr<xiiGALTexture>   m_pMultiScatterLUT;
  xiiUInt64                     m_uiConfigurationRevision = 1U;
  xiiUInt64                     m_uiGeneratedRevision     = 0U;
  bool                          m_bEngineStarted           = false;
  bool                          m_bInitialized             = false;
};

xiiUniquePtr<xiiAtmosphereManager::State> xiiAtmosphereManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, AtmosphereManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiAtmosphereManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiAtmosphereManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiAtmosphereManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiAtmosphereManager::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAtmosphereSettings, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAtmosphereSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PlanetRadiusKm", m_fPlanetRadiusKm)->AddAttributes(new xiiClampValueAttribute(1.0f, xiiVariant()), new xiiSuffixAttribute(" km")),
    XII_MEMBER_PROPERTY("AtmosphereRadiusKm", m_fAtmosphereRadiusKm)->AddAttributes(new xiiClampValueAttribute(1.0f, xiiVariant()), new xiiSuffixAttribute(" km")),
    XII_MEMBER_PROPERTY("RayleighScaleHeightKm", m_fRayleighScaleHeightKm)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiSuffixAttribute(" km")),
    XII_MEMBER_PROPERTY("MieScaleHeightKm", m_fMieScaleHeightKm)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiSuffixAttribute(" km")),
    XII_MEMBER_PROPERTY("RayleighScattering", m_vRayleighScattering),
    XII_MEMBER_PROPERTY("MieScattering", m_vMieScattering),
    XII_MEMBER_PROPERTY("MieAbsorption", m_vMieAbsorption),
    XII_MEMBER_PROPERTY("OzoneAbsorption", m_vOzoneAbsorption),
    XII_MEMBER_PROPERTY("MiePhaseG", m_fMiePhaseG)->AddAttributes(new xiiClampValueAttribute(-0.999f, 0.999f)),
    XII_MEMBER_PROPERTY("TransmittanceIntegrationSteps", m_uiTransmittanceIntegrationSteps)->AddAttributes(new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("MultiScatterSqrtSamples", m_uiMultiScatterSqrtSamples)->AddAttributes(new xiiClampValueAttribute(1U, 64U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAtmosphereCacheStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAtmosphereCacheStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ConfigurationRevision", m_uiConfigurationRevision),
    XII_MEMBER_PROPERTY("GeneratedRevision", m_uiGeneratedRevision),
    XII_MEMBER_PROPERTY("GpuResourcesAvailable", m_bGpuResourcesAvailable),
    XII_MEMBER_PROPERTY("GenerationPending", m_bGenerationPending),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiAtmosphereManager::Startup()
{
  s_pState = XII_DEFAULT_NEW(State);
  s_pState->m_bInitialized = true;
}

void xiiAtmosphereManager::EngineStartup()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_bEngineStarted = true;
  EnsureGpuResources().IgnoreResult();
}

void xiiAtmosphereManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_pMultiScatterLUT.Clear();
  s_pState->m_pTransmittanceLUT.Clear();
  s_pState->m_uiGeneratedRevision = 0U;
  s_pState->m_bEngineStarted = false;
}

void xiiAtmosphereManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiAtmosphereManager::Configure(const xiiAtmosphereSettings& settings)
{
  if (s_pState == nullptr || !IsValid(settings))
    return XII_FAILURE;

  if (IsEqual(s_pState->m_Settings, settings))
    return XII_SUCCESS;

  s_pState->m_Settings = settings;
  ++s_pState->m_uiConfigurationRevision;
  s_pState->m_uiGeneratedRevision = 0U;
  return XII_SUCCESS;
}

bool xiiAtmosphereManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

const xiiAtmosphereSettings& xiiAtmosphereManager::GetConfiguration()
{
  XII_ASSERT_DEV(IsInitialized(), "Atmosphere manager is not started.");
  return s_pState->m_Settings;
}

xiiResult xiiAtmosphereManager::EnsureGpuResources()
{
  if (!IsInitialized() || !s_pState->m_bEngineStarted)
    return XII_FAILURE;
  if (s_pState->m_pTransmittanceLUT != nullptr && s_pState->m_pMultiScatterLUT != nullptr)
    return XII_SUCCESS;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return XII_FAILURE;

  xiiGALTextureCreationDescription description;
  description.m_Type       = xiiGALResourceDimension::Texture2D;
  description.m_Format     = xiiGALResourceFormat::RGBA16Float;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags  = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage      = xiiGALResourceUsage::Default;

  description.m_Size.width  = 256U;
  description.m_Size.height = 64U;
  xiiSharedPtr<xiiGALTexture> pTransmittance = pDevice->CreateTexture(description);
  if (pTransmittance == nullptr)
    return XII_FAILURE;
  pTransmittance->SetDebugName("Atmosphere Transmittance LUT");

  description.m_Size.width  = 32U;
  description.m_Size.height = 32U;
  xiiSharedPtr<xiiGALTexture> pMultiScatter = pDevice->CreateTexture(description);
  if (pMultiScatter == nullptr)
    return XII_FAILURE;
  pMultiScatter->SetDebugName("Atmosphere Multi-Scatter LUT");

  s_pState->m_pTransmittanceLUT = std::move(pTransmittance);
  s_pState->m_pMultiScatterLUT  = std::move(pMultiScatter);
  s_pState->m_uiGeneratedRevision = 0U;
  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALTexture> xiiAtmosphereManager::GetTransmittanceLUT()
{
  return s_pState != nullptr ? s_pState->m_pTransmittanceLUT : nullptr;
}

xiiSharedPtr<xiiGALTexture> xiiAtmosphereManager::GetMultiScatterLUT()
{
  return s_pState != nullptr ? s_pState->m_pMultiScatterLUT : nullptr;
}

xiiUInt64 xiiAtmosphereManager::GetConfigurationRevision()
{
  return s_pState != nullptr ? s_pState->m_uiConfigurationRevision : 0U;
}

bool xiiAtmosphereManager::IsGenerationPending()
{
  return s_pState == nullptr || s_pState->m_uiGeneratedRevision != s_pState->m_uiConfigurationRevision;
}

void xiiAtmosphereManager::MarkLUTsGenerated(xiiUInt64 uiConfigurationRevision)
{
  if (s_pState != nullptr && uiConfigurationRevision == s_pState->m_uiConfigurationRevision)
    s_pState->m_uiGeneratedRevision = uiConfigurationRevision;
}

void xiiAtmosphereManager::InvalidateLUTs()
{
  if (s_pState != nullptr)
  {
    ++s_pState->m_uiConfigurationRevision;
    s_pState->m_uiGeneratedRevision = 0U;
  }
}

xiiAtmosphereCacheStats xiiAtmosphereManager::GetCacheStats()
{
  xiiAtmosphereCacheStats stats;
  if (s_pState != nullptr)
  {
    stats.m_uiConfigurationRevision = s_pState->m_uiConfigurationRevision;
    stats.m_uiGeneratedRevision = s_pState->m_uiGeneratedRevision;
    stats.m_bGpuResourcesAvailable = s_pState->m_pTransmittanceLUT != nullptr && s_pState->m_pMultiScatterLUT != nullptr;
    stats.m_bGenerationPending = stats.m_uiGeneratedRevision != stats.m_uiConfigurationRevision;
  }
  return stats;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_Atmosphere);
