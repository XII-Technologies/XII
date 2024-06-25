#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/Extractor.h>

struct xiiPerLightData;
struct xiiPerDecalData;
struct xiiPerReflectionProbeData;
struct xiiPerClusterData;

class xiiClusteredDataCPU : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusteredDataCPU, xiiRenderData);

public:
  xiiClusteredDataCPU();
  ~xiiClusteredDataCPU();

  static constexpr xiiUInt32 MAX_LIGHT_DATA            = 1024U;
  static constexpr xiiUInt32 MAX_DECAL_DATA            = 1024U;
  static constexpr xiiUInt32 MAX_REFLECTION_PROBE_DATA = 1024U;
  static constexpr xiiUInt32 MAX_ITEMS_PER_CLUSTER     = 256U;

  xiiArrayPtr<xiiPerLightData>           m_LightData;
  xiiArrayPtr<xiiPerDecalData>           m_DecalData;
  xiiArrayPtr<xiiPerReflectionProbeData> m_ReflectionProbeData;
  xiiArrayPtr<xiiPerClusterData>         m_ClusterData;
  xiiArrayPtr<xiiUInt32>                 m_ClusterItemList;

  xiiUInt32                   m_uiSkyIrradianceIndex = 0;
  xiiEnum<xiiCameraUsageHint> m_cameraUsageHint      = xiiCameraUsageHint::Default;

  float    m_fFogHeight             = 0.0f;
  float    m_fFogHeightFalloff      = 0.0f;
  float    m_fFogDensityAtCameraPos = 0.0f;
  float    m_fFogDensity            = 0.0f;
  float    m_fFogInvSkyDistance     = 0.0f;
  xiiColor m_FogColor               = xiiColor::Black;
};

class XII_GRAPHICSCORE_DLL xiiClusteredDataExtractor : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusteredDataExtractor, xiiExtractor);

public:
  xiiClusteredDataExtractor(xiiStringView sName = "ClusteredDataExtractor");
  ~xiiClusteredDataExtractor();

  virtual void      PostSortAndBatch(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

private:
  void FillItemListAndClusterData(xiiClusteredDataCPU* pData);

  template <xiiUInt32 MaxData>
  struct TempCluster
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_BitMask[MaxData / 32];
  };

  xiiDynamicArray<xiiPerLightData, xiiAlignedAllocatorWrapper>                 m_TempLightData;
  xiiDynamicArray<xiiPerDecalData, xiiAlignedAllocatorWrapper>                 m_TempDecalData;
  xiiDynamicArray<xiiPerReflectionProbeData, xiiAlignedAllocatorWrapper>       m_TempReflectionProbeData;
  xiiDynamicArray<TempCluster<xiiClusteredDataCPU::MAX_LIGHT_DATA>>            m_TempLightsClusters;
  xiiDynamicArray<TempCluster<xiiClusteredDataCPU::MAX_DECAL_DATA>>            m_TempDecalsClusters;
  xiiDynamicArray<TempCluster<xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA>> m_TempReflectionProbeClusters;
  xiiDynamicArray<xiiUInt32>                                                   m_TempClusterItemList;

  xiiDynamicArray<xiiSimdBSphere, xiiAlignedAllocatorWrapper> m_ClusterBoundingSpheres;
};
