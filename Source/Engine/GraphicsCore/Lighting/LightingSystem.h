/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Pipeline/FrameConstants.h>

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Lighting/IESProfileResource.h>
#include <GraphicsCore/Lighting/LocalShadow.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

class xiiExtractedRenderData;
class xiiGALBuffer;
class xiiGALCommandList;
class xiiGALDevice;
class xiiRenderGraphBlackboard;
class xiiView;
class xiiLightingManager;

/// Coherent ReSTIR DI cost/quality presets. Custom preserves the individually authored controls.
struct XII_GRAPHICSCORE_DLL xiiReSTIRDIQualityPreset
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Integrated,
    Balanced,
    High,
    Ultra,
    Custom,

    Default = High
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiReSTIRDIQualityPreset);

/// CPU-side settings for the clustered lighting data path.
struct XII_GRAPHICSCORE_DLL xiiLightingSystemSettings
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                         m_uiMaxActiveLights                 = 65536U;
  xiiUInt32                         m_uiMaxLightsPerCluster             = 128U;
  xiiUInt32                         m_uiClusterTileSize                 = 16U;
  xiiUInt32                         m_uiClusterDepthSlices              = 24U;
  xiiColor                          m_AmbientLightColor                 = xiiColor(0.03f, 0.035f, 0.04f, 1.0f);
  float                             m_fIndirectLightIntensity           = 1.0f;
  float                             m_fContactShadowLength              = 0.35f;
  float                             m_fContactShadowThickness           = 0.02f;
  xiiUInt32                         m_uiContactShadowSteps              = 16U;
  float                             m_fSSGIRayLength                    = 2.0f;
  xiiUInt32                         m_uiSSGISampleCount                 = 8U;
  float                             m_fSSGIThickness                    = 0.15f;
  float                             m_fSSGIIntensity                    = 1.0f;
  float                             m_fSSRefractionScale                = 0.05f;
  float                             m_fSSRefractionMaxDistance          = 0.1f;
  float                             m_fSSRefractionChromatic            = 0.002f;
  xiiUInt32                         m_uiLocalShadowAtlasSize            = 4096U;
  xiiUInt32                         m_uiLocalShadowTileSize             = 256U;
  xiiUInt32                         m_uiMaxLocalShadowFacesPerFrame     = 64U;
  float                             m_fVolumetricFogDensity             = 0.015f;
  float                             m_fVolumetricHeightFalloff          = 0.08f;
  float                             m_fVolumetricBaseHeight             = 0.0f;
  float                             m_fVolumetricAnisotropy             = 0.45f;
  float                             m_fDirectionalShadowMaxPenumbra     = 24.0f; ///< Maximum PCSS filter radius in shadow texels.
  xiiUInt32                         m_uiDirectionalShadowBlockerSamples = 12U;
  xiiUInt32                         m_uiDirectionalShadowFilterSamples  = 16U;
  xiiEnum<xiiReSTIRDIQualityPreset> m_ReSTIRDIQualityPreset;
  xiiUInt32                         m_uiReSTIRDIEmissiveVisibilitySamples          = 4U; ///< Rays per selected emissive triangle when visibility is refreshed.
  xiiUInt32                         m_uiReSTIRDIEmissiveBinaryRefreshPeriod        = 2U; ///< Frames between interleaved refreshes of fully lit/occluded samples.
  float                             m_fReSTIRDIEmissiveVisibilityHistoryWeight     = 0.75f;
  float                             m_fReSTIRDIEmissiveVisibilityReactiveThreshold = 0.35f;
  xiiUInt32                         m_uiReSTIRDIFreshCandidateCount                = 8U;
  xiiUInt32                         m_uiReSTIRDITemporalHistoryM                   = 20U;
  xiiUInt32                         m_uiReSTIRDISpatialNeighborCount               = 4U;
  xiiUInt32                         m_uiReSTIRDISpatialRadius                      = 2U; ///< Maximum spatial reuse radius in pixels.
  xiiUInt32                         m_uiRTGIMaxPathHits                            = 2U; ///< Maximum diffuse surface hits in a near-field path.
  xiiUInt32                         m_uiRTGILocalLightSamples                      = 2U; ///< Stratified analytical local-light samples evaluated at each hit.
  xiiUInt32                         m_uiRTGIEmissiveLightSamples                   = 1U; ///< Power-weighted emissive-triangle next-event samples evaluated at each hit.
  xiiUInt32                         m_uiRTReflectionMaxPathHits                    = 2U; ///< Maximum glossy surface hits in a ray-traced reflection path.
  float                             m_fRTReflectionHistoryWeight                   = 0.90f;
  float                             m_fRTReflectionMomentsHistoryWeight            = 0.94f;
  float                             m_fRTReflectionConfidenceDecay                 = 0.97f;
  float                             m_fRTReflectionSpatialWeight                   = 0.35f;
  float                             m_fRTReflectionDepthThreshold                  = 0.02f;
  float                             m_fRTReflectionNormalThreshold                 = 0.95f;
  float                             m_fRTReflectionHitDistanceRelativeThreshold    = 0.08f;
  float                             m_fRTReflectionHitDistanceAbsoluteThreshold    = 0.05f; ///< Minimum hit-distance tolerance in metres.
  float                             m_fRTReflectionVarianceGamma                   = 2.5f;
  xiiUInt32                         m_uiMaxIESProfiles                             = 256U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightingSystemSettings);

/// Light representation consumed by clustered lighting shaders.
///
/// The layout is intentionally 16-byte aligned and mirrors Data/Base/Shaders/Pipeline/LightingData.h.
struct XII_GRAPHICSCORE_DLL xiiGpuLightData
{
  XII_DECLARE_POD_TYPE();

  xiiVec4    m_PositionAndInvRange;    ///< xyz = world position, w = 1 / range.
  xiiVec4    m_DirectionAndType;       ///< xyz = world direction, w = xiiGpuLightType.
  xiiVec4    m_ColorAndIntensity;      ///< rgb = normalized linear color; w = cd (local), lx (directional), or nt (area).
  xiiVec4    m_AttenuationAndSize;     ///< x = range, y = source radius, z = tube length, w = reserved.
  xiiVec4    m_SpotAnglesAndRectSize;  ///< x = cos(inner half angle), y = cos(outer half angle), zw = rect extents.
  xiiVec4    m_ShadowData;             ///< x = casts shadow, y = camera fade-out distance in metres (zero is automatic/full), z = angular/source size, w = reserved.
  xiiVec4    m_BoundsCenterAndRadius;  ///< xyz = culling sphere center, w = culling sphere radius.
  xiiVec4    m_OrientationRightAndIES; ///< xyz = local right axis, w = compact IES profile index plus one (zero means none).
  xiiVec4U32 m_Metadata;               ///< x = stable light ID, y = compact frame index, z = LightType, w = 16-bit content revision.
};

static_assert(sizeof(xiiGpuLightData) == 144);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuLightData);

/// Per-view lighting system that owns extracted light data and GAL upload resources.
class XII_GRAPHICSCORE_DLL xiiLightingSystem
{
public:
  enum class LightType : xiiUInt32
  {
    Directional  = 0U,
    Point        = 1U,
    Spot         = 2U,
    Rectangle    = 3U,
    Disc         = 4U,
    Sphere       = 5U,
    Tube         = 6U,
    EmissiveMesh = 7U,
  };

  struct FrameStatistics
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiActiveLightCount       = 0U;
    xiiUInt32 m_uiDirectionalLightCount  = 0U;
    xiiUInt32 m_uiLocalLightCount        = 0U;
    xiiUInt32 m_uiSkippedLightCount      = 0U;
    xiiUInt32 m_uiActiveIESProfileCount  = 0U;
    xiiUInt32 m_uiSkippedIESProfileCount = 0U;
    xiiUInt32 m_uiEmissiveMeshLightCount = 0U;
  };

  ~xiiLightingSystem();

  void BuildFrameData(const xiiView& view, const xiiExtractedRenderData& extractedData, xiiUInt32 uiFrameIndex);
  void UploadFrameData(xiiGALCommandList& ref_commandList);

  void BindFrameConstants(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindLightData(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindIESProfiles(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindLightingResources(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;

  void WriteBlackboard(xiiRenderGraphBlackboard& ref_blackboard) const;

  const xiiLightingSystemSettings&                         GetSettings() const { return m_Settings; }
  const FrameStatistics&                                   GetFrameStatistics() const { return m_Stats; }
  const xiiLocalShadowAtlasStatistics&                     GetLocalShadowStatistics() const { return m_LocalShadowStatistics; }
  [[nodiscard]] xiiArrayPtr<const xiiLocalShadowAtlasData> GetLocalShadowData() const { return xiiArrayPtr<const xiiLocalShadowAtlasData>(m_LocalShadowData.GetData(), m_LocalShadowData.GetCount()); }
  xiiUInt32                                                GetActiveLightCount() const { return m_Stats.m_uiActiveLightCount; }

  xiiUInt32 GetClusterCountX() const { return m_uiClusterCountX; }
  xiiUInt32 GetClusterCountY() const { return m_uiClusterCountY; }
  xiiUInt32 GetClusterCountZ() const { return m_Settings.m_uiClusterDepthSlices; }
  xiiUInt32 GetTotalClusterCount() const { return m_uiTotalClusterCount; }

  xiiGALBuffer* GetLightDataBuffer() const;

  xiiDDGIContext&                      GetDDGIContext() { return m_DDGI; }
  const xiiDDGIContext&                GetDDGIContext() const { return m_DDGI; }
  xiiSparseVoxelRadianceContext&       GetSparseVoxelRadianceContext() { return m_SparseVoxelRadiance; }
  const xiiSparseVoxelRadianceContext& GetSparseVoxelRadianceContext() const { return m_SparseVoxelRadiance; }
  void                                 InvalidateGlobalIlluminationHistory();

private:
  friend class xiiLightingManager;

  xiiDDGIContext                m_DDGI;
  xiiSparseVoxelRadianceContext m_SparseVoxelRadiance;

  xiiLightingSystem();

  [[nodiscard]] xiiResult Initialize(xiiSharedPtr<xiiGALDevice> pDevice, const xiiLightingSystemSettings& settings = {});
  void                    Shutdown();

  using PerFrameCameraConstants = xiiFrameCameraConstants;
  using PerFrameLightConstants  = xiiFrameLightConstants;
  using PerFrameGlobalConstants = xiiFrameGlobalConstants;

  void EnsureGpuResources();
  void ResetFrameData();

  bool      AppendLight(xiiGpuLightData lightData, LightType type, xiiUInt32 uiStableLightId);
  xiiUInt32 ResolveIESProfile(const xiiIESProfileResourceHandle& hProfile);

  static xiiColor EvaluateTemperatureColor(xiiUInt32 uiTemperature);
  static xiiColor EvaluateLightColor(const xiiColorLinearUB& color, xiiUInt32 uiTemperature);

private:
  xiiLightingSystemSettings m_Settings;
  FrameStatistics           m_Stats;

  xiiSharedPtr<xiiGALDevice> m_pDevice;
  xiiSharedPtr<xiiGALBuffer> m_pCameraConstantsBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pLightConstantsBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pGlobalConstantsBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pLightDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pIESProfileDataBuffer;

  xiiDynamicArray<xiiGpuLightData>                     m_LightData;
  xiiLocalShadowAtlasDataArray                         m_LocalShadowData;
  xiiLocalShadowAtlasStatistics                        m_LocalShadowStatistics;
  xiiDynamicArray<float>                               m_IESProfileData;
  xiiHashTable<xiiIESProfileResourceHandle, xiiUInt32> m_IESProfileSlots;

  PerFrameCameraConstants m_CameraConstants;
  PerFrameLightConstants  m_LightConstants;
  PerFrameGlobalConstants m_GlobalConstants;

  xiiUInt32 m_uiClusterCountX     = 1U;
  xiiUInt32 m_uiClusterCountY     = 1U;
  xiiUInt32 m_uiTotalClusterCount = 1U;
};
