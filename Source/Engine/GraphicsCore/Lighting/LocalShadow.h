/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/ArrayPtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

struct xiiGpuLightData;

/// CPU/GPU ABI for one local-light shadow allocation.
///
/// Point-like lights use all six faces in +X, -X, +Y, -Y, +Z, -Z order.
/// Projected lights use face zero. Invalid entries have Metadata.z == 0.
struct XII_GRAPHICSCORE_DLL xiiLocalShadowAtlasData
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] xiiUInt32 GetReflectedFaceCount() const { return 6U; }
  [[nodiscard]] xiiMat4   GetReflectedViewProjection(xiiUInt32 uiIndex) const { return m_ViewProjection[uiIndex]; }
  [[nodiscard]] xiiVec4   GetReflectedAtlasScaleBias(xiiUInt32 uiIndex) const { return m_AtlasScaleBias[uiIndex]; }

  xiiMat4    m_ViewProjection[6];
  xiiVec4    m_AtlasScaleBias[6]; ///< xy = scale, zw = bias for each face.
  xiiVec4    m_LightPositionAndInvRange;
  xiiVec4U32 m_Metadata; ///< x = face count, y = tile size, z = valid, w = light type.
};

static_assert(sizeof(xiiLocalShadowAtlasData) == 512U, "Local shadow data must remain byte-compatible with LightingData.h.");
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLocalShadowAtlasData);

using xiiLocalShadowAtlasDataArray = xiiDynamicArray<xiiLocalShadowAtlasData, xiiAlignedAllocatorWrapper>;

/// Deterministic local-shadow atlas planning settings.
struct XII_GRAPHICSCORE_DLL xiiLocalShadowAtlasSettings
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiAtlasSize       = 4096U;
  xiiUInt32 m_uiTileSize        = 256U;
  float     m_fMinimumNearPlane = 0.01f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLocalShadowAtlasSettings);

/// Per-frame atlas planning diagnostics exposed to tools and profilers.
struct XII_GRAPHICSCORE_DLL xiiLocalShadowAtlasStatistics
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiRequestedLights = 0U;
  xiiUInt32 m_uiAllocatedLights = 0U;
  xiiUInt32 m_uiDroppedLights   = 0U;
  xiiUInt32 m_uiAllocatedFaces  = 0U;
  xiiUInt32 m_uiTileCapacity    = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLocalShadowAtlasStatistics);

/// Builds collision-free atlas allocations and local-light view-projection matrices.
class XII_GRAPHICSCORE_DLL xiiLocalShadowAtlasBuilder
{
public:
  xiiLocalShadowAtlasBuilder() = delete;

  static void Build(const xiiLocalShadowAtlasSettings& settings, xiiArrayPtr<const xiiGpuLightData> lights,
                    xiiLocalShadowAtlasDataArray& out_shadowData, xiiLocalShadowAtlasStatistics& out_statistics);
};
