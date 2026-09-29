/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsCore/Lighting/IESProfileResource.h>

class xiiExtractedRenderData;
class xiiGALBuffer;
class xiiGALCommandList;
class xiiGALDevice;
class xiiRenderGraphBlackboard;
class xiiView;

/// CPU-side settings for the clustered lighting data path.
struct XII_GRAPHICSCORE_DLL xiiLightingSystemSettings
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiMaxActiveLights        = 65536U;
  xiiUInt32 m_uiMaxLightsPerCluster    = 128U;
  xiiUInt32 m_uiClusterTileSize        = 16U;
  xiiUInt32 m_uiClusterDepthSlices     = 24U;
  xiiColor  m_AmbientLightColor        = xiiColor(0.03f, 0.035f, 0.04f, 1.0f);
  float     m_fIndirectLightIntensity  = 1.0f;
  float     m_fContactShadowLength     = 0.35f;
  float     m_fContactShadowThickness  = 0.02f;
  xiiUInt32 m_uiContactShadowSteps     = 16U;
  xiiUInt32 m_uiLocalShadowTileSize    = 256U;
  float     m_fVolumetricFogDensity    = 0.015f;
  float     m_fVolumetricHeightFalloff = 0.08f;
  float     m_fVolumetricBaseHeight    = 0.0f;
  float     m_fVolumetricAnisotropy    = 0.45f;
  float     m_fDirectionalShadowMaxPenumbra = 24.0f; ///< Maximum PCSS filter radius in shadow texels.
  xiiUInt32 m_uiDirectionalShadowBlockerSamples = 12U;
  xiiUInt32 m_uiDirectionalShadowFilterSamples  = 16U;
  xiiUInt32 m_uiMaxIESProfiles                  = 256U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightingSystemSettings);

/// Light representation consumed by clustered lighting shaders.
///
/// The layout is intentionally 16-byte aligned and mirrors Data/Base/Shaders/Pipeline/LightingData.h.
struct XII_GRAPHICSCORE_DLL xiiGpuLightData
{
  XII_DECLARE_POD_TYPE();

  xiiVec4 m_PositionAndInvRange;   ///< xyz = world position, w = 1 / range.
  xiiVec4 m_DirectionAndType;      ///< xyz = world direction, w = xiiGpuLightType.
  xiiVec4 m_ColorAndIntensity;     ///< rgb = normalized linear color; w = cd (local), lx (directional), or nt (area).
  xiiVec4 m_AttenuationAndSize;    ///< x = range, y = source radius, z = tube length, w = reserved.
  xiiVec4 m_SpotAnglesAndRectSize; ///< x = cos(inner half angle), y = cos(outer half angle), zw = rect extents.
  xiiVec4 m_ShadowData;            ///< x = casts shadow, y = shadow fade range, z = angular/source size, w = reserved.
  xiiVec4 m_BoundsCenterAndRadius; ///< xyz = culling sphere center, w = culling sphere radius.
  xiiVec4 m_OrientationRightAndIES; ///< xyz = local right axis, w = compact IES profile index plus one (zero means none).
  xiiVec4U32 m_Metadata;           ///< x = stable light ID, y = compact frame index, z = LightType, w = reserved flags.
};

static_assert(sizeof(xiiGpuLightData) == 144);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuLightData);

/// Per-view lighting system that owns extracted light data and GAL upload resources.
class XII_GRAPHICSCORE_DLL xiiLightingSystem
{
public:
  enum class LightType : xiiUInt32
  {
    Directional = 0U,
    Point       = 1U,
    Spot        = 2U,
    Rectangle   = 3U,
    Disc        = 4U,
    Sphere      = 5U,
    Tube        = 6U,
    EmissiveMesh = 7U,
  };

  struct FrameStatistics
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiActiveLightCount      = 0U;
    xiiUInt32 m_uiDirectionalLightCount = 0U;
    xiiUInt32 m_uiLocalLightCount       = 0U;
    xiiUInt32 m_uiSkippedLightCount     = 0U;
    xiiUInt32 m_uiActiveIESProfileCount = 0U;
    xiiUInt32 m_uiSkippedIESProfileCount = 0U;
    xiiUInt32 m_uiEmissiveMeshLightCount = 0U;
  };

  xiiLightingSystem();
  ~xiiLightingSystem();

  void Initialize(xiiSharedPtr<xiiGALDevice> pDevice);
  void Shutdown();

  void BuildFrameData(const xiiView& view, const xiiExtractedRenderData& extractedData, xiiUInt32 uiFrameIndex);
  void UploadFrameData(xiiGALCommandList& ref_commandList);

  void BindFrameConstants(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindLightData(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindIESProfiles(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindLightingResources(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;

  void WriteBlackboard(xiiRenderGraphBlackboard& ref_blackboard) const;

  const xiiLightingSystemSettings& GetSettings() const { return m_Settings; }
  const FrameStatistics&           GetFrameStatistics() const { return m_Stats; }
  xiiUInt32                        GetActiveLightCount() const { return m_Stats.m_uiActiveLightCount; }

  xiiUInt32 GetClusterCountX() const { return m_uiClusterCountX; }
  xiiUInt32 GetClusterCountY() const { return m_uiClusterCountY; }
  xiiUInt32 GetClusterCountZ() const { return m_Settings.m_uiClusterDepthSlices; }
  xiiUInt32 GetTotalClusterCount() const { return m_uiTotalClusterCount; }

  xiiGALBuffer* GetLightDataBuffer() const;

private:
  struct PerFrameCameraConstants
  {
    XII_DECLARE_POD_TYPE();

    xiiMat4 m_ViewProjectionMatrix        = xiiMat4::MakeIdentity();
    xiiMat4 m_InverseViewProjectionMatrix = xiiMat4::MakeIdentity();
    xiiVec4 m_CameraPositionAndNearPlane  = xiiVec4::MakeZero();
    xiiVec4 m_CameraForwardAndFarPlane    = xiiVec4(1.0f, 0.0f, 0.0f, 1000.0f);
  };

  struct PerFrameLightConstants
  {
    XII_DECLARE_POD_TYPE();

    xiiVec4   m_MainLightDirectionAndIntensity = xiiVec4(0.0f, 0.0f, -1.0f, 0.0f);
    xiiVec4   m_MainLightColor                 = xiiVec4(1.0f, 1.0f, 1.0f, 1.0f);
    xiiVec4   m_AmbientLightColor              = xiiVec4(0.03f, 0.035f, 0.04f, 1.0f);
    xiiUInt32 m_uiActiveLightCount             = 0U;
    xiiUInt32 m_uiClusterCountX                = 1U;
    xiiUInt32 m_uiClusterCountY                = 1U;
    xiiUInt32 m_uiClusterCountZ                = 1U;
    xiiUInt32 m_uiMaxLightsPerCluster          = 128U;
    float     m_fClusterNearPlane              = 0.1f;
    float     m_fClusterLogFarOverNear         = 1.0f;
    float     m_fIndirectLightIntensity        = 1.0f;
    float     m_fContactShadowLength           = 0.35f;
    float     m_fContactShadowThickness        = 0.02f;
    xiiUInt32 m_uiContactShadowSteps           = 16U;
    xiiUInt32 m_uiLocalShadowTileSize          = 256U;
    float     m_fVolumetricFogDensity          = 0.015f;
    float     m_fVolumetricHeightFalloff       = 0.08f;
    float     m_fVolumetricBaseHeight          = 0.0f;
    float     m_fVolumetricAnisotropy          = 0.45f;
    float     m_fDirectionalShadowSourceRadius = 0.0f;
    float     m_fDirectionalShadowMaxPenumbra  = 24.0f;
    xiiUInt32 m_uiDirectionalShadowBlockerSamples = 12U;
    xiiUInt32 m_uiDirectionalShadowFilterSamples  = 16U;
  };
  static_assert(sizeof(PerFrameLightConstants) == 128U, "PerFrameLight must remain byte-compatible with PipelineCommon.xiiShader.");

  struct PerFrameGlobalConstants
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiFrameIndex      = 0U;
    float     m_fDeltaTimeMs      = 0.0f;
    float     m_fGlobalTime       = 0.0f;
    float     m_fWorldTime        = 0.0f;
    xiiVec4   m_RenderScaleJitter = xiiVec4(1.0f, 0.0f, 0.0f, 0.0f);
  };

  void EnsureGpuResources();
  void ResetFrameData();

  bool AppendLight(xiiGpuLightData lightData, LightType type, xiiUInt32 uiStableLightId);
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

  xiiDynamicArray<xiiGpuLightData> m_LightData;
  xiiDynamicArray<float>           m_IESProfileData;
  xiiHashTable<xiiIESProfileResourceHandle, xiiUInt32> m_IESProfileSlots;

  PerFrameCameraConstants m_CameraConstants;
  PerFrameLightConstants  m_LightConstants;
  PerFrameGlobalConstants m_GlobalConstants;

  xiiUInt32 m_uiClusterCountX     = 1U;
  xiiUInt32 m_uiClusterCountY     = 1U;
  xiiUInt32 m_uiTotalClusterCount = 1U;
};
