#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Declarations.h>

struct xiiPerLightData;
struct xiiPerDecalData;
struct xiiPerReflectionProbeData;
struct xiiPerClusterData;

class XII_GRAPHICSCORE_DLL xiiClusteredDataCPU : public xiiRenderData
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

struct XII_GRAPHICSCORE_DLL xiiClusteredDataGPU
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiClusteredDataGPU);

public:
  xiiClusteredDataGPU();
  ~xiiClusteredDataGPU();

  xiiUInt32                   m_uiSkyIrradianceIndex = 0;
  xiiEnum<xiiCameraUsageHint> m_CameraUsageHint      = xiiCameraUsageHint::Default;

  xiiSharedPtr<xiiGALBuffer> m_pLightDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pDecalDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pReflectionProbeDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pClusterDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pClusterItemBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pClusterDataConstantBuffer;

  xiiSharedPtr<xiiGALSampler> m_pShadowSampler;

  xiiDecalAtlasResourceHandle m_hDecalAtlas;
  xiiSharedPtr<xiiGALSampler> m_pDecalAtlasSampler;

  void BindResources(xiiRenderContext* pRenderContext);
};

class XII_GRAPHICSCORE_DLL xiiClusteredDataProvider : public xiiFrameDataProvider<xiiClusteredDataGPU>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusteredDataProvider, xiiFrameDataProviderBase);

public:
  xiiClusteredDataProvider();
  ~xiiClusteredDataProvider();

private:
  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) override;

  xiiClusteredDataGPU m_Data;
};
