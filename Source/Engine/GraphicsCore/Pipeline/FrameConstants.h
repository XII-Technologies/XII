/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiView;

/// Shared shader layouts for immutable frame snapshots. These never own scene state.
struct XII_GRAPHICSCORE_DLL xiiFrameCameraConstants
{
  XII_DECLARE_POD_TYPE();

  xiiMat4 m_ViewProjectionMatrix        = xiiMat4::MakeIdentity();
  xiiMat4 m_InverseViewProjectionMatrix = xiiMat4::MakeIdentity();
  xiiVec4 m_CameraPositionAndNearPlane  = xiiVec4::MakeZero();
  xiiVec4 m_CameraForwardAndFarPlane    = xiiVec4(1.0f, 0.0f, 0.0f, 1000.0f);
};

struct XII_GRAPHICSCORE_DLL xiiFrameLightConstants
{
  XII_DECLARE_POD_TYPE();

  xiiVec4   m_MainLightDirectionAndIntensity    = xiiVec4(0.0f, 0.0f, -1.0f, 0.0f);
  xiiVec4   m_MainLightColor                    = xiiVec4(1.0f, 1.0f, 1.0f, 0.0f); ///< rgb = primary color, w = directional-light prefix count.
  xiiVec4   m_AmbientLightColor                 = xiiVec4(0.03f, 0.035f, 0.04f, 1.0f);
  xiiUInt32 m_uiActiveLightCount                = 0U;
  xiiUInt32 m_uiClusterCountX                   = 1U;
  xiiUInt32 m_uiClusterCountY                   = 1U;
  xiiUInt32 m_uiClusterCountZ                   = 1U;
  xiiUInt32 m_uiMaxLightsPerCluster             = 128U;
  float     m_fClusterNearPlane                 = 0.1f;
  float     m_fClusterLogFarOverNear            = 1.0f;
  float     m_fIndirectLightIntensity           = 1.0f;
  float     m_fContactShadowLength              = 0.35f;
  float     m_fContactShadowThickness           = 0.02f;
  xiiUInt32 m_uiContactShadowSteps              = 16U;
  xiiUInt32 m_uiLocalShadowTileSize             = 256U;
  float     m_fVolumetricFogDensity             = 0.015f;
  float     m_fVolumetricHeightFalloff          = 0.08f;
  float     m_fVolumetricBaseHeight             = 0.0f;
  float     m_fVolumetricAnisotropy             = 0.45f;
  float     m_fDirectionalShadowSourceRadius    = 0.0f;
  float     m_fDirectionalShadowMaxPenumbra     = 24.0f;
  xiiUInt32 m_uiDirectionalShadowBlockerSamples = 12U;
  xiiUInt32 m_uiDirectionalShadowFilterSamples  = 16U;
};
static_assert(sizeof(xiiFrameLightConstants) == 128U, "PerFrameLight must remain byte-compatible with PipelineCommon.xiiShader.");

struct XII_GRAPHICSCORE_DLL xiiFrameGlobalConstants
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32  m_uiFrameIndex              = 0U;
  float      m_fDeltaTimeMs              = 0.0f;
  float      m_fGlobalTime               = 0.0f;
  float      m_fWorldTime                = 0.0f;
  xiiVec4    m_RenderScaleJitter         = xiiVec4(1.0f, 0.0f, 0.0f, 0.0f);
  xiiVec4U32 m_ReSTIRDISettings          = xiiVec4U32(4U, 2U, 0U, 0U);
  xiiVec4U32 m_ReSTIRDIReuseSettings     = xiiVec4U32(8U, 20U, 4U, 2U);
  xiiVec4    m_ReSTIRDIStabilitySettings = xiiVec4(0.75f, 0.35f, 0.0f, 0.0f);
  xiiVec4U32 m_RTGISettings              = xiiVec4U32(2U, 0U, 0U, 0U);
  xiiVec4U32 m_RTReflectionSettings      = xiiVec4U32(2U, 0U, 0U, 0U);
};


static_assert(sizeof(xiiFrameCameraConstants) == 160);
static_assert(sizeof(xiiFrameGlobalConstants) == 112);
XII_GRAPHICSCORE_DLL xiiFrameCameraConstants xiiSnapshotCameraConstants(const xiiView& view);
XII_GRAPHICSCORE_DLL xiiFrameGlobalConstants xiiSnapshotGlobalConstants(const xiiView& view, xiiUInt32 frameIndex);
