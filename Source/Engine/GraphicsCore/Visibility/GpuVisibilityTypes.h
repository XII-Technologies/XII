/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/Meshes/GpuInstanceTypes.h>

/// Immutable shader-facing view snapshot; no renderer, scene registry or execution ownership.
struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuVisibilityView
{
  XII_DECLARE_POD_TYPE();

  xiiMat4 m_ViewProjectionMatrix;
  /// Matrix that produced an explicitly supplied depth image. Its temporal policy belongs
  /// to the consuming graph; it need not be the current culling projection.
  xiiMat4   m_OcclusionViewProjectionMatrix;
  xiiVec4   m_FrustumPlanes[6];
  xiiVec4   m_CameraPosition;
  xiiVec4   m_ViewportAndHiZ; // width, height, Hi-Z mip count, occlusion bias
  xiiVec4   m_LodParameters;  // hysteresis, current history token, previous history token, reserved
  xiiUInt32 m_uiInstanceCount      = 0U;
  xiiUInt32 m_uiVisibilityMask     = 0xFFFFFFFFU;
  xiiUInt32 m_uiRequiredFlags      = xiiGpuInstanceFlags::Enabled;
  xiiUInt32 m_uiExcludedFlags      = 0U;
  xiiUInt32 m_uiGeometryBaseIndex  = 0U;
  xiiUInt32 m_uiMaxVisibleMeshlets = 0U;
  xiiUInt32 m_uiCandidateCount     = 0U;
  xiiUInt32 m_uiUseCandidates      = 0U;

  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetFrustumPlaneCount() const { return 6U; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiVec4   GetFrustumPlane(xiiUInt32 uiIndex) const { return m_FrustumPlanes[uiIndex]; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityView);

static_assert(sizeof(xiiGpuVisibilityView) == 304 && offsetof(xiiGpuVisibilityView, m_FrustumPlanes) == 128 &&
                offsetof(xiiGpuVisibilityView, m_uiInstanceCount) == 272,
              "Visibility snapshots must match GpuSceneCommon.h.");
