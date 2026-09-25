/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Strings/HashedString.h>
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
  xiiUInt32 m_uiGeometryBaseIndex = 0U;
  xiiUInt32 m_uiPadding[3] = {};
};

/// Identifies an independently culled visibility set for profiling and editor tooling.
struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityPurpose
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    MainView,
    Shadow,
    Reflection,
    Sensor,
    Editor,

    Default = MainView
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityPurpose);

/// Names and schedules one per-pass visibility set. Multiple descriptions can be submitted to the
/// same graph without resource-name collisions.
struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityPassDescription
{
  xiiString                         m_sName = "Main View";
  xiiEnum<xiiGpuVisibilityPurpose>  m_Purpose = xiiGpuVisibilityPurpose::MainView;
  bool                              m_bAsyncCompute = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityPassDescription);

struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityDescription
{
  xiiUInt32 m_uiMaxInstances = 65536U;
  xiiUInt32 m_uiMaxVisibleMeshlets = 1024U * 1024U;
  xiiUInt32 m_uiMaxDrawCommands = 1024U * 1024U;
  xiiUInt32 m_uiFramesInFlight = 3U;
  xiiUInt32 m_uiMaxVisibilitySets = 16U;
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
  [[nodiscard]] xiiGpuVisibilityOutputs AddPasses(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiSceneDatabase& scene, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuVisibilityPassDescription& description, xiiRenderGraphTextureHandle hHiZ = {});

  /// Compatibility overload for the main view.
  [[nodiscard]] xiiGpuVisibilityOutputs AddPasses(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiSceneDatabase& scene, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, xiiRenderGraphTextureHandle hHiZ = {})
  {
    return AddPasses(graph, uiFrameIndex, scene, view, geometry, xiiGpuVisibilityPassDescription{}, hHiZ);
  }

private:
  xiiSharedPtr<xiiGALComputePipelineState> LoadComputePipeline(xiiStringView sShaderPath);
  xiiUInt32 GetOrCreateVisibilitySetIndex(xiiStringView sName);

  xiiGpuVisibilityDescription m_Description;
  xiiGALDevice* m_pDevice = nullptr;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pSceneBuffers;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pViewBuffers;
  /// Last contents uploaded to each frame-in-flight scene buffer. A mirror per slot is
  /// required because consecutive CPU frames rotate across different GPU allocations.
  xiiDynamicArray<xiiDynamicArray<xiiGpuSceneInstance>> m_SceneBufferMirrors;
  xiiHashTable<xiiHashedString, xiiUInt32> m_VisibilitySetIndices;
  xiiSharedPtr<xiiGALComputePipelineState> m_pInstanceCullPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_pHiZOcclusionPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_pMeshletCullPipeline;
  xiiSharedPtr<xiiGALComputePipelineState> m_pCommandBuildPipeline;
};
