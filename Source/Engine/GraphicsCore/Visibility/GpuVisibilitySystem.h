/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Frustum.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Scene/SceneDatabase.h>

struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuVisibilityView
{
  XII_DECLARE_POD_TYPE();

  xiiMat4   m_ViewProjectionMatrix;
  xiiVec4   m_FrustumPlanes[6];
  xiiVec4   m_CameraPosition;
  xiiVec4   m_ViewportAndHiZ; // width, height, Hi-Z mip count, occlusion bias
  xiiUInt32 m_uiInstanceCount = 0U;
  xiiUInt32 m_uiVisibilityMask = 0xFFFFFFFFU;
  xiiUInt32 m_uiRequiredFlags = xiiSceneObjectFlags::Enabled;
  xiiUInt32 m_uiExcludedFlags = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityDescription
{
  xiiUInt32 m_uiMaxInstances = 65536U;
  xiiUInt32 m_uiMaxVisibleMeshlets = 1024U * 1024U;
  xiiUInt32 m_uiMaxDrawCommands = 1024U * 1024U;
  xiiUInt32 m_uiFramesInFlight = 3U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityDescription);

struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityOutputs
{
  xiiRenderGraphBufferHandle m_hSceneInstances;
  xiiRenderGraphBufferHandle m_hVisibleInstances;
  xiiRenderGraphBufferHandle m_hVisibleInstanceCount;
  xiiRenderGraphBufferHandle m_hVisibleMeshlets;
  xiiRenderGraphBufferHandle m_hVisibleMeshletCount;
  xiiRenderGraphBufferHandle m_hIndirectCommands;
  xiiRenderGraphBufferHandle m_hIndirectCommandCount;
};

/// GPU-driven visibility pipeline shared by scene, shadow, sensor, and editor views.
///
/// Per-frame scene buffers avoid CPU/GPU write hazards. Render-graph versions connect transfer,
/// async-compute, and graphics consumers, allowing automatic split barriers and queue fences.
class XII_GRAPHICSCORE_DLL xiiGpuVisibilitySystem
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuVisibilitySystem);

public:
  xiiGpuVisibilitySystem() = default;
  ~xiiGpuVisibilitySystem();

  xiiResult Initialize(xiiGALDevice* pDevice, const xiiGpuVisibilityDescription& description = {});
  void Shutdown();

  static xiiGpuVisibilityView BuildView(const xiiMat4& viewProjectionMatrix, const xiiFrustum& frustum, const xiiVec3& vCameraPosition, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiHiZMipCount, xiiUInt32 uiInstanceCount, xiiUInt32 uiVisibilityMask = 0xFFFFFFFFU);

  /// Adds upload and async-compute visibility passes. The geometry handles must be returned by
  /// xiiGeometryResidencyManager::AddUploadPass in the same graph setup.
  [[nodiscard]] xiiGpuVisibilityOutputs AddPasses(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiSceneDatabase& scene, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, xiiRenderGraphTextureHandle hHiZ = {});

private:
  xiiSharedPtr<xiiGALComputePipelineState> LoadComputePipeline(xiiStringView sShaderPath);

  xiiGpuVisibilityDescription m_Description;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pSceneBuffers;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pViewBuffers;
  xiiSharedPtr<xiiGALComputePipelineState> m_pInstanceCullPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_pHiZOcclusionPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_pMeshletCullPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_pCommandBuildPipeline;
};
