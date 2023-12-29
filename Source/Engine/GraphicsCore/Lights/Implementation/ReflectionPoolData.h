#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Lights/Implementation/ReflectionProbeData.h>
#include <GraphicsCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <GraphicsCore/Lights/Implementation/ReflectionProbeUpdater.h>
#include <GraphicsCore/Pipeline/View.h>

class xiiSkyLightComponent;
class xiiSphereReflectionProbeComponent;
class xiiBoxReflectionProbeComponent;

static const xiiUInt32 s_uiReflectionCubeMapSize      = 128;
static const xiiUInt32 s_uiNumReflectionProbeCubeMaps = 32;
static const float     s_fDebugSphereRadius           = 0.3f;

#if XII_RENDERER_ENABLE

inline xiiUInt32 GetMipLevels()
{
  return xiiMath::Log2i(s_uiReflectionCubeMapSize) - 1; // only down to 4x4
}

//////////////////////////////////////////////////////////////////////////
/// xiiReflectionPool::Data

struct xiiReflectionPool::Data
{
  Data();
  ~Data();

  struct ProbeData
  {
    xiiReflectionProbeDesc       m_desc;
    xiiTransform                 m_GlobalTransform;
    xiiBitflags<xiiProbeFlags>   m_Flags;
    xiiTextureCubeResourceHandle m_hCubeMap; // static data or empty for dynamic.
  };

  struct WorldReflectionData
  {
    WorldReflectionData() :
      m_mapping(s_uiNumReflectionProbeCubeMaps)
    {
    }
    XII_DISALLOW_COPY_AND_ASSIGN(WorldReflectionData);

    xiiIdTable<xiiReflectionProbeId, ProbeData> m_Probes;
    xiiReflectionProbeId                        m_SkyLight; // SkyLight is always fixed at reflectionIndex 0.
    xiiEventSubscriptionID                      m_mappingSubscriptionId = 0;
    xiiReflectionProbeMapping                   m_mapping;
  };

  // WorldReflectionData management
  xiiReflectionProbeId                          AddProbe(const xiiWorld* pWorld, ProbeData&& probeData);
  xiiReflectionPool::Data::WorldReflectionData& GetWorldData(const xiiWorld* pWorld);
  void                                          RemoveProbe(const xiiWorld* pWorld, xiiReflectionProbeId id);
  void                                          UpdateProbeData(ProbeData& ref_probeData, const xiiReflectionProbeDesc& desc, const xiiReflectionProbeComponentBase* pComponent);
  bool                                          UpdateSkyLightData(ProbeData& ref_probeData, const xiiReflectionProbeDesc& desc, const xiiSkyLightComponent* pComponent);
  void                                          OnReflectionProbeMappingEvent(const xiiUInt32 uiWorldIndex, const xiiReflectionProbeMappingEvent& e);

  void PreExtraction();
  void PostExtraction();

  // Dynamic Update Queue (all worlds combined)
  xiiHashSet<xiiReflectionProbeRef> m_PendingDynamicUpdate;
  xiiDeque<xiiReflectionProbeRef>   m_DynamicUpdateQueue;

  xiiHashSet<xiiReflectionProbeRef> m_ActiveDynamicUpdate;
  xiiReflectionProbeUpdater         m_ReflectionProbeUpdater;

  void CreateReflectionViewsAndResources();
  void CreateSkyIrradianceTexture();

  xiiMutex                                             m_Mutex;
  xiiUInt64                                            m_uiWorldHasSkyLight     = 0;
  xiiUInt64                                            m_uiSkyIrradianceChanged = 0;
  xiiHybridArray<xiiUniquePtr<WorldReflectionData>, 2> m_WorldReflectionData;

  // GPU storage
  xiiGALTextureHandle                                   m_hFallbackReflectionSpecularTexture;
  xiiGALTextureHandle                                   m_hSkyIrradianceTexture;
  xiiHybridArray<xiiAmbientCube<xiiColorLinear16f>, 64> m_SkyIrradianceStorage;

  // Debug data
  xiiMeshResourceHandle                                                         m_hDebugSphere;
  xiiHybridArray<xiiMaterialResourceHandle, 6 * s_uiNumReflectionProbeCubeMaps> m_hDebugMaterial;
};

#endif
