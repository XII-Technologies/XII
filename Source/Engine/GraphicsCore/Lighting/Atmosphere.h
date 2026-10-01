/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiGALTexture;

/// Stable process-local handle to one immutable atmosphere LUT cache entry.
struct XII_GRAPHICSCORE_DLL xiiAtmosphereLUTHandle
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndex = xiiInvalidIndex;

  [[nodiscard]] bool IsValid() const { return m_uiIndex != xiiInvalidIndex; }
  [[nodiscard]] bool operator==(const xiiAtmosphereLUTHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAtmosphereLUTHandle);

/// Physical parameters used to generate the shared atmosphere lookup tables.
/// Distances and extinction coefficients are expressed in kilometres because
/// that keeps the values numerically well-conditioned in the GPU integration.
struct XII_GRAPHICSCORE_DLL xiiAtmosphereSettings
{
  float     m_fPlanetRadiusKm                 = 6360.0f;
  float     m_fAtmosphereRadiusKm             = 6460.0f;
  float     m_fRayleighScaleHeightKm          = 8.0f;
  float     m_fMieScaleHeightKm               = 1.2f;
  xiiVec3   m_vRayleighScattering             = xiiVec3(0.005802f, 0.013558f, 0.033100f);
  xiiVec3   m_vMieScattering                  = xiiVec3(0.003996f);
  xiiVec3   m_vMieAbsorption                  = xiiVec3(0.004400f);
  xiiVec3   m_vOzoneAbsorption                = xiiVec3(0.000650f, 0.001881f, 0.000085f);
  xiiVec3   m_vPlanetUpDirection              = xiiVec3(0.0f, 0.0f, 1.0f);
  float     m_fGroundAltitudeMeters           = 0.0f;
  float     m_fMiePhaseG                      = 0.8f;
  xiiUInt32 m_uiTransmittanceIntegrationSteps = 40U;
  xiiUInt32 m_uiMultiScatterSqrtSamples       = 8U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAtmosphereSettings);

/// Tool-facing state of the shared atmosphere LUT cache.
struct XII_GRAPHICSCORE_DLL xiiAtmosphereCacheStats
{
  xiiUInt64 m_uiConfigurationRevision = 0U;
  xiiUInt64 m_uiGeneratedRevision     = 0U;
  bool      m_bGpuResourcesAvailable  = false;
  bool      m_bGenerationPending      = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAtmosphereCacheStats);

/// Process-wide owner of atmosphere lookup resources.
///
/// CPU state is allocated during core-system startup, after Foundation's
/// allocator exists. GPU resources are created during high-level startup and
/// released during high-level shutdown, before the graphics device disappears.
/// Views only import these textures into their render graphs; they never own or
/// destroy them.
class XII_GRAPHICSCORE_DLL xiiAtmosphereManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAtmosphereManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, AtmosphereManager);

public:
  xiiAtmosphereManager() = delete;

  [[nodiscard]] static xiiResult                    Configure(const xiiAtmosphereSettings& settings);
  [[nodiscard]] static bool                         IsSubsystemInitialized();
  [[nodiscard]] static bool                         IsInitialized();
  [[nodiscard]] static const xiiAtmosphereSettings& GetConfiguration();

  /// Finds or creates an immutable cache entry for a view/world atmosphere.
  /// Multiple views can therefore render different planets without rewriting
  /// another graph's persistent LUTs.
  [[nodiscard]] static xiiResult                    AcquireLUTs(const xiiAtmosphereSettings& settings, xiiAtmosphereLUTHandle& out_handle);
  [[nodiscard]] static xiiAtmosphereLUTHandle       GetDefaultLUTHandle();
  [[nodiscard]] static const xiiAtmosphereSettings& GetConfiguration(xiiAtmosphereLUTHandle handle);

  /// Ensures textures exist. This permits recovery when the default GAL device
  /// is installed after high-level subsystem startup.
  [[nodiscard]] static xiiResult                   EnsureGpuResources();
  [[nodiscard]] static xiiResult                   EnsureGpuResources(xiiAtmosphereLUTHandle handle);
  [[nodiscard]] static xiiSharedPtr<xiiGALTexture> GetTransmittanceLUT();
  [[nodiscard]] static xiiSharedPtr<xiiGALTexture> GetTransmittanceLUT(xiiAtmosphereLUTHandle handle);
  [[nodiscard]] static xiiSharedPtr<xiiGALTexture> GetMultiScatterLUT();
  [[nodiscard]] static xiiSharedPtr<xiiGALTexture> GetMultiScatterLUT(xiiAtmosphereLUTHandle handle);

  [[nodiscard]] static xiiUInt64               GetConfigurationRevision();
  [[nodiscard]] static bool                    IsGenerationPending();
  [[nodiscard]] static bool                    IsGenerationPending(xiiAtmosphereLUTHandle handle);
  static void                                  MarkLUTsGenerated(xiiUInt64 uiConfigurationRevision);
  static void                                  MarkLUTsGenerated(xiiAtmosphereLUTHandle handle);
  static void                                  InvalidateLUTs();
  [[nodiscard]] static xiiAtmosphereCacheStats GetCacheStats();

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  class State;
  static xiiUniquePtr<State> s_pState;
};
