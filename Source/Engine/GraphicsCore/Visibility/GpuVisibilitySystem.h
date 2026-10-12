/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Scene/SceneDatabase.h>
#include <GraphicsCore/Visibility/GpuHiZPyramid.h>

class xiiGpuVisibilityManager;

struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuVisibilityView
{
  XII_DECLARE_POD_TYPE();

  xiiMat4 m_ViewProjectionMatrix;
  /// Matrix that produced the supplied Hi-Z image. This is normally the previous frame's
  /// view projection while m_ViewProjectionMatrix remains the current culling matrix.
  xiiMat4   m_OcclusionViewProjectionMatrix;
  xiiVec4   m_FrustumPlanes[6];
  xiiVec4   m_CameraPosition;
  xiiVec4   m_ViewportAndHiZ; // width, height, Hi-Z mip count, occlusion bias
  xiiVec4   m_LodParameters;  // hysteresis, current history token, previous history token, reserved
  xiiUInt32 m_uiInstanceCount      = 0U;
  xiiUInt32 m_uiVisibilityMask     = 0xFFFFFFFFU;
  xiiUInt32 m_uiRequiredFlags      = xiiSceneObjectFlags::Enabled;
  xiiUInt32 m_uiExcludedFlags      = 0U;
  xiiUInt32 m_uiGeometryBaseIndex  = 0U;
  xiiUInt32 m_uiMaxVisibleMeshlets = 0U;
  xiiUInt32 m_uiPadding[2]         = {};

  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetFrustumPlaneCount() const { return 6U; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiVec4   GetFrustumPlane(xiiUInt32 uiIndex) const { return m_FrustumPlanes[uiIndex]; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityView);

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
  xiiString                        m_sName = "Main View";
  xiiString                        m_sResourceSetName; ///< Optional scratch-allocation identity. Empty uses m_sName.
  xiiEnum<xiiGpuVisibilityPurpose> m_Purpose              = xiiGpuVisibilityPurpose::MainView;
  float                            m_fLodScreenScale      = 1.0f;  ///< Scales projected coverage for purpose-specific LOD selection.
  float                            m_fLodHysteresis       = 0.10f; ///< Relative dead band around LOD coverage thresholds.
  xiiUInt32                        m_uiMaxVisibleMeshlets = 0U;    ///< Zero uses the visibility context capacity.
  bool                             m_bEnableLodFeedback   = true;
  bool                             m_bAsyncCompute        = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityPassDescription);

struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityDescription
{
  xiiUInt32 m_uiMaxInstances       = 65536U;
  xiiUInt32 m_uiMaxVisibleMeshlets = 1024U * 1024U;
  /// Advisory content budget for meshlets in one resident LOD. Dispatch dimensions are derived
  /// from residency metadata so exceeding this value reports a warning but never drops work.
  xiiUInt32 m_uiMaxMeshletsPerGeometry = 4096U;
  /// Reserved for compatibility with multi-command consumers. The mesh-shader path tiles the
  /// compact list into one indirect command and therefore requires exactly one command slot.
  xiiUInt32 m_uiMaxDrawCommands   = 1U;
  xiiUInt32 m_uiFramesInFlight    = 3U;
  xiiUInt32 m_uiMaxVisibilitySets = 16U;
  /// Power-of-two number of generation-validated, GPU-deduplicated LOD requests retained per
  /// visibility set. Slot zero is a counter header and is allocated separately.
  xiiUInt32 m_uiLodFeedbackCapacity = 4096U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityDescription);

/// GPU hash-table entry used to compact missing-LOD demand without a CPU-visible raw request list.
/// Entry zero in each buffer reuses the first three fields as raw, unique, and overflow counters.
struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuLodFeedback
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiKey                = 0U;
  xiiUInt32 m_uiGeometryGeneration = 0U;
  xiiUInt32 m_uiDesiredLod         = 0U;
  xiiUInt32 m_uiSelectedLod        = 0U;
  xiiUInt32 m_uiPurposeMask        = 0U;
  xiiUInt32 m_uiObjectUserData     = 0U;
  xiiUInt32 m_uiPriority           = 0U;
  xiiUInt32 m_uiPadding            = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuLodFeedback);
static_assert(sizeof(xiiGpuLodFeedback) == 32U, "GPU LOD feedback must match the HLSL structured-buffer stride.");

/// Most recently consumed asynchronous LOD-feedback statistics for one visibility context.
struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityStreamingStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiRawRequestCount       = 0U;
  xiiUInt32 m_uiUniqueRequestCount    = 0U;
  xiiUInt32 m_uiOverflowRequestCount  = 0U;
  xiiUInt32 m_uiAcceptedRequestCount  = 0U;
  xiiUInt32 m_uiStaleRequestCount     = 0U;
  xiiUInt32 m_uiReadbackLatencyFrames = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityStreamingStats);

struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityOutputs
{
  xiiRenderGraphBufferHandle m_hSceneInstances;
  xiiRenderGraphBufferHandle m_hVisibleInstances;
  xiiRenderGraphBufferHandle m_hVisibleInstanceCount;
  xiiRenderGraphBufferHandle m_hVisibleMeshlets;
  xiiRenderGraphBufferHandle m_hVisibleMeshletCount;
  xiiRenderGraphBufferHandle m_hIndirectCommands;
  xiiRenderGraphBufferHandle m_hIndirectCommandCount;
  /// Capacity of the compact visible-meshlet stream, shared by mesh and indexed consumers.
  xiiUInt32 m_uiMaxVisibleMeshlets = 0U;
};

/// GPU-driven visibility pipeline shared by scene, shadow, sensor, and editor views.
///
/// Per-frame scene buffers avoid CPU/GPU write hazards. Render-graph versions connect transfer,
/// async-compute, and graphics consumers, allowing automatic split barriers and queue fences.
class XII_GRAPHICSCORE_DLL xiiGpuVisibilitySystem
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuVisibilitySystem);

public:
  ~xiiGpuVisibilitySystem();

  static xiiGpuVisibilityView BuildView(const xiiMat4& viewProjectionMatrix, const xiiFrustum& frustum, const xiiVec3& vCameraPosition, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiHiZMipCount, xiiUInt32 uiInstanceCount, xiiUInt32 uiVisibilityMask = 0xFFFFFFFFU);

private:
  friend class xiiGpuVisibilityManager;

  xiiGpuVisibilitySystem() = default;

  xiiResult Initialize(xiiGALDevice* pDevice, const xiiGpuVisibilityDescription& description = {});
  void      Shutdown();

  /// Adds upload and async-compute visibility passes. The geometry handles must be returned by
  /// xiiGeometryResidencyManager::AddUploadPass in the same graph setup.
  [[nodiscard]] xiiGpuVisibilityOutputs AddPasses(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiSceneDatabase& scene, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuVisibilityPassDescription& description, xiiRenderGraphTextureHandle hHiZ = {});

  /// Compatibility overload for the main view.
  [[nodiscard]] xiiGpuVisibilityOutputs AddPasses(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiSceneDatabase& scene, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, xiiRenderGraphTextureHandle hHiZ = {})
  {
    return AddPasses(graph, uiFrameIndex, scene, view, geometry, xiiGpuVisibilityPassDescription{}, hHiZ);
  }

  [[nodiscard]] xiiUInt32 GetMeshDispatchGroupCountX() const { return m_uiMeshDispatchGroupCountX; }
  [[nodiscard]] xiiUInt32 GetMeshDispatchGroupCountY() const { return m_uiMeshDispatchGroupCountY; }

  xiiSharedPtr<xiiGALComputePipelineState> LoadComputePipeline(xiiStringView sShaderPath);
  xiiUInt32                                GetOrCreateVisibilitySetIndex(xiiStringView sName);
  void                                     BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame);

  xiiGpuVisibilityDescription m_Description;
  xiiGALDevice*               m_pDevice = nullptr;
  /// One scene snapshot per frame slot is shared by every view. View constants remain
  /// independently frame-sliced because masks, frusta, and Hi-Z inputs differ per pass.
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pSceneBuffers;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pViewBuffers;
  /// One generation-validated LOD history per visibility set and frame slot. Meshlet culling
  /// reads the preceding slot and writes the current slot, preserving queue overlap without
  /// reading and writing the same allocation in one frame.
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pLodHistoryBuffers;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pLodFeedbackBuffers;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>> m_pLodFeedbackReadbackBuffers;
  xiiDynamicArray<xiiUInt64>                  m_LodFeedbackReadbackFrames;
  xiiDynamicArray<xiiUInt64>                  m_LodFeedbackClearFrames;
  /// Last contents uploaded to each frame-in-flight scene buffer. A mirror per slot is
  /// required because consecutive CPU frames rotate across different GPU allocations.
  xiiDynamicArray<xiiDynamicArray<xiiGpuSceneInstance>> m_SceneBufferMirrors;
  xiiRenderGraph*                                       m_pPreparedGraph  = nullptr;
  const xiiSceneDatabase*                               m_pPreparedScene  = nullptr;
  xiiUInt64                                             m_uiPreparedFrame = xiiMath::MaxValue<xiiUInt64>();
  xiiRenderGraphBufferHandle                            m_hPreparedScene;
  xiiHashTable<xiiHashedString, xiiUInt32>              m_VisibilitySetIndices;
  xiiSharedPtr<xiiGALComputePipelineState>              m_pInstanceCullPipeline;
  xiiSharedPtr<xiiGALComputePipelineState>              m_pHiZOcclusionPipeline;
  xiiSharedPtr<xiiGALComputePipelineState>              m_pMeshletDispatchBuildPipeline;
  xiiSharedPtr<xiiGALComputePipelineState>              m_pMeshletCullPipeline;
  xiiSharedPtr<xiiGALComputePipelineState>              m_pMeshletHiZCullPipeline;
  xiiSharedPtr<xiiGALComputePipelineState>              m_pLodFeedbackClearPipeline;
  xiiSharedPtr<xiiGALComputePipelineState>              m_pCommandBuildPipeline;
  xiiGpuHiZPyramid                                      m_HiZHistory;
  xiiUInt32                                             m_uiLargestReportedInstanceCount = 0U;
  xiiUInt32                                             m_uiLargestReportedMeshletCount  = 0U;
  xiiUInt32                                             m_uiMeshDispatchGroupCountX      = 0U;
  xiiUInt32                                             m_uiMeshDispatchGroupCountY      = 0U;
  xiiUInt32                                             m_uiMeshDispatchGroupTotalCount  = 0U;
  xiiGpuVisibilityStreamingStats                        m_StreamingStats;
};
