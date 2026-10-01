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
      settings.m_vPlanetUpDirection.IsValid() && settings.m_vPlanetUpDirection.GetLengthSquared() > 1e-6f &&
      xiiMath::IsFinite(settings.m_fGroundAltitudeMeters) &&
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
      lhs.m_vPlanetUpDirection == rhs.m_vPlanetUpDirection && lhs.m_fGroundAltitudeMeters == rhs.m_fGroundAltitudeMeters &&
      lhs.m_fMiePhaseG == rhs.m_fMiePhaseG &&
      lhs.m_uiTransmittanceIntegrationSteps == rhs.m_uiTransmittanceIntegrationSteps &&
      lhs.m_uiMultiScatterSqrtSamples == rhs.m_uiMultiScatterSqrtSamples;
  }
} // namespace

class xiiAtmosphereManager::State
{
public:
  struct CacheEntry
  {
    xiiAtmosphereSettings       m_Settings;
    xiiSharedPtr<xiiGALTexture> m_pTransmittanceLUT;
    xiiSharedPtr<xiiGALTexture> m_pMultiScatterLUT;
    bool                        m_bGenerated = false;
  };

  CacheEntry* GetEntry(xiiAtmosphereLUTHandle handle)
  {
    return handle.IsValid() && handle.m_uiIndex < m_Entries.GetCount() ? &m_Entries[handle.m_uiIndex] : nullptr;
  }

  const CacheEntry* GetEntry(xiiAtmosphereLUTHandle handle) const
  {
    return handle.IsValid() && handle.m_uiIndex < m_Entries.GetCount() ? &m_Entries[handle.m_uiIndex] : nullptr;
  }

  xiiDynamicArray<CacheEntry> m_Entries;
  xiiAtmosphereSettings       m_Settings;
  xiiAtmosphereLUTHandle      m_hDefaultEntry;
  xiiUInt64                   m_uiConfigurationRevision = 1U;
  bool                        m_bEngineStarted          = false;
  bool                        m_bInitialized            = false;
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
    XII_MEMBER_PROPERTY("PlanetUpDirection", m_vPlanetUpDirection),
    XII_MEMBER_PROPERTY("GroundAltitudeMeters", m_fGroundAltitudeMeters)->AddAttributes(new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("MiePhaseG", m_fMiePhaseG)->AddAttributes(new xiiClampValueAttribute(-0.999f, 0.999f)),
    XII_MEMBER_PROPERTY("TransmittanceIntegrationSteps", m_uiTransmittanceIntegrationSteps)->AddAttributes(new xiiClampValueAttribute(1U, 1024U)),
    XII_MEMBER_PROPERTY("MultiScatterSqrtSamples", m_uiMultiScatterSqrtSamples)->AddAttributes(new xiiClampValueAttribute(1U, 64U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAtmosphereLUTHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAtmosphereLUTHandle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Index", m_uiIndex),
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
  s_pState                 = XII_DEFAULT_NEW(State);
  s_pState->m_bInitialized = true;
  AcquireLUTs(s_pState->m_Settings, s_pState->m_hDefaultEntry).AssertSuccess("Failed to create the default atmosphere LUT cache entry.");
}

void xiiAtmosphereManager::EngineStartup()
{
  if (s_pState == nullptr)
    return;

  s_pState->m_bEngineStarted = true;
  for (xiiUInt32 i = 0U; i < s_pState->m_Entries.GetCount(); ++i)
    EnsureGpuResources(xiiAtmosphereLUTHandle{i}).IgnoreResult();
}

void xiiAtmosphereManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  for (State::CacheEntry& entry : s_pState->m_Entries)
  {
    entry.m_pMultiScatterLUT.Clear();
    entry.m_pTransmittanceLUT.Clear();
    entry.m_bGenerated = false;
  }
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

  xiiAtmosphereLUTHandle hEntry;
  if (AcquireLUTs(settings, hEntry).Failed())
    return XII_FAILURE;

  s_pState->m_Settings      = settings;
  s_pState->m_hDefaultEntry = hEntry;
  ++s_pState->m_uiConfigurationRevision;
  return XII_SUCCESS;
}

bool xiiAtmosphereManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
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

xiiResult xiiAtmosphereManager::AcquireLUTs(const xiiAtmosphereSettings& settings, xiiAtmosphereLUTHandle& out_handle)
{
  out_handle = {};
  if (!IsInitialized() || !IsValid(settings))
    return XII_FAILURE;

  for (xiiUInt32 i = 0U; i < s_pState->m_Entries.GetCount(); ++i)
  {
    if (IsEqual(s_pState->m_Entries[i].m_Settings, settings))
    {
      out_handle.m_uiIndex = i;
      return s_pState->m_bEngineStarted ? EnsureGpuResources(out_handle) : XII_SUCCESS;
    }
  }

  State::CacheEntry& entry = s_pState->m_Entries.ExpandAndGetRef();
  entry.m_Settings         = settings;
  out_handle.m_uiIndex     = s_pState->m_Entries.GetCount() - 1U;
  return s_pState->m_bEngineStarted ? EnsureGpuResources(out_handle) : XII_SUCCESS;
}

xiiAtmosphereLUTHandle xiiAtmosphereManager::GetDefaultLUTHandle()
{
  return s_pState != nullptr ? s_pState->m_hDefaultEntry : xiiAtmosphereLUTHandle{};
}

const xiiAtmosphereSettings& xiiAtmosphereManager::GetConfiguration(xiiAtmosphereLUTHandle handle)
{
  XII_ASSERT_DEV(IsInitialized() && s_pState->GetEntry(handle) != nullptr, "Invalid atmosphere LUT cache handle.");
  return s_pState->GetEntry(handle)->m_Settings;
}

xiiResult xiiAtmosphereManager::EnsureGpuResources()
{
  return EnsureGpuResources(GetDefaultLUTHandle());
}

xiiResult xiiAtmosphereManager::EnsureGpuResources(xiiAtmosphereLUTHandle handle)
{
  if (!IsInitialized() || !s_pState->m_bEngineStarted)
    return XII_FAILURE;

  State::CacheEntry* pEntry = s_pState->GetEntry(handle);
  if (pEntry == nullptr)
    return XII_FAILURE;
  if (pEntry->m_pTransmittanceLUT != nullptr && pEntry->m_pMultiScatterLUT != nullptr)
    return XII_SUCCESS;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return XII_FAILURE;

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;

  description.m_Size.width                   = 256U;
  description.m_Size.height                  = 64U;
  xiiSharedPtr<xiiGALTexture> pTransmittance = pDevice->CreateTexture(description);
  if (pTransmittance == nullptr)
    return XII_FAILURE;
  pTransmittance->SetDebugName("Atmosphere Transmittance LUT");

  description.m_Size.width                  = 32U;
  description.m_Size.height                 = 32U;
  xiiSharedPtr<xiiGALTexture> pMultiScatter = pDevice->CreateTexture(description);
  if (pMultiScatter == nullptr)
    return XII_FAILURE;
  pMultiScatter->SetDebugName("Atmosphere Multi-Scatter LUT");

  pEntry->m_pTransmittanceLUT = std::move(pTransmittance);
  pEntry->m_pMultiScatterLUT  = std::move(pMultiScatter);
  pEntry->m_bGenerated        = false;
  return XII_SUCCESS;
}

xiiSharedPtr<xiiGALTexture> xiiAtmosphereManager::GetTransmittanceLUT()
{
  return GetTransmittanceLUT(GetDefaultLUTHandle());
}

xiiSharedPtr<xiiGALTexture> xiiAtmosphereManager::GetTransmittanceLUT(xiiAtmosphereLUTHandle handle)
{
  const State::CacheEntry* pEntry = s_pState != nullptr ? s_pState->GetEntry(handle) : nullptr;
  return pEntry != nullptr ? pEntry->m_pTransmittanceLUT : nullptr;
}

xiiSharedPtr<xiiGALTexture> xiiAtmosphereManager::GetMultiScatterLUT()
{
  return GetMultiScatterLUT(GetDefaultLUTHandle());
}

xiiSharedPtr<xiiGALTexture> xiiAtmosphereManager::GetMultiScatterLUT(xiiAtmosphereLUTHandle handle)
{
  const State::CacheEntry* pEntry = s_pState != nullptr ? s_pState->GetEntry(handle) : nullptr;
  return pEntry != nullptr ? pEntry->m_pMultiScatterLUT : nullptr;
}

xiiUInt64 xiiAtmosphereManager::GetConfigurationRevision()
{
  return s_pState != nullptr ? s_pState->m_uiConfigurationRevision : 0U;
}

bool xiiAtmosphereManager::IsGenerationPending()
{
  return IsGenerationPending(GetDefaultLUTHandle());
}

bool xiiAtmosphereManager::IsGenerationPending(xiiAtmosphereLUTHandle handle)
{
  const State::CacheEntry* pEntry = s_pState != nullptr ? s_pState->GetEntry(handle) : nullptr;
  return pEntry == nullptr || !pEntry->m_bGenerated;
}

void xiiAtmosphereManager::MarkLUTsGenerated(xiiUInt64 uiConfigurationRevision)
{
  if (s_pState != nullptr && uiConfigurationRevision == s_pState->m_uiConfigurationRevision)
    MarkLUTsGenerated(s_pState->m_hDefaultEntry);
}

void xiiAtmosphereManager::MarkLUTsGenerated(xiiAtmosphereLUTHandle handle)
{
  State::CacheEntry* pEntry = s_pState != nullptr ? s_pState->GetEntry(handle) : nullptr;
  if (pEntry != nullptr)
    pEntry->m_bGenerated = true;
}

void xiiAtmosphereManager::InvalidateLUTs()
{
  if (s_pState != nullptr)
  {
    ++s_pState->m_uiConfigurationRevision;
    if (State::CacheEntry* pEntry = s_pState->GetEntry(s_pState->m_hDefaultEntry))
      pEntry->m_bGenerated = false;
  }
}

xiiAtmosphereCacheStats xiiAtmosphereManager::GetCacheStats()
{
  xiiAtmosphereCacheStats stats;
  if (s_pState != nullptr)
  {
    stats.m_uiConfigurationRevision = s_pState->m_uiConfigurationRevision;
    const State::CacheEntry* pEntry = s_pState->GetEntry(s_pState->m_hDefaultEntry);
    stats.m_uiGeneratedRevision     = pEntry != nullptr && pEntry->m_bGenerated ? s_pState->m_uiConfigurationRevision : 0U;
    stats.m_bGpuResourcesAvailable  = pEntry != nullptr && pEntry->m_pTransmittanceLUT != nullptr && pEntry->m_pMultiScatterLUT != nullptr;
    stats.m_bGenerationPending      = pEntry == nullptr || !pEntry->m_bGenerated;
  }
  return stats;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_Atmosphere);
