/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Device/SwapChain.h>

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/Pipeline/ViewRenderResourceManager.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>

class xiiBuiltinRenderPipelineRuntime;
class xiiRenderPipelineInstance;
class xiiFrustum;
class xiiWorld;
class xiiRenderGraph;
class xiiRenderGraphBuilder;
class xiiRenderGraphPassContext;
class xiiExtractedRenderData;
class xiiGALBuffer;
class xiiGALTexture;
class xiiGALSampler;
class xiiGALComputePipelineState;
class xiiGALGraphicsPipelineState;
class xiiGALRayTracingPipelineState;
class xiiGALTopLevelAS;

/// Encapsulates a view on the given world through the given camera
/// and rendered with the specified RenderPipeline into the given render target setup.
///
/// The view owns a generation-checked context for its rendering pipeline. Persistent GPU resources
/// are stored by xiiViewRenderResourceManager so they are released before allocator and GAL
/// shutdown regardless of world lifetime. A selected render pipeline resource populates the graph each frame.
class XII_GRAPHICSCORE_DLL xiiView : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiView, xiiReflectedClass);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiView);

private:
  /// Use xiiRenderWorldModule::CreateView to create a view.
  xiiView(xiiWorld* pWorld);
  ~xiiView();

public:
  xiiWorld* GetWorld() const;

  xiiViewHandle GetHandle() const;

  xiiStringView GetName() const;
  void          SetName(xiiStringView sName);

  xiiGALTextureView* GetRenderTargetView() const;
  /// Retains the offscreen output view and its texture until replaced or this view is destroyed.
  void SetRenderTargetView(xiiGALTextureView* pRenderTargetView);

  /// Sets the swapchain that this view will be rendering into.
  const xiiGALSwapChain* GetSwapChain() const;
  void                   SetSwapChain(const xiiGALSwapChain* pSwapChain);

  void             SetCamera(xiiCamera* pCamera);
  xiiCamera*       GetCamera();
  const xiiCamera* GetCamera() const;

  void             SetCullingCamera(const xiiCamera* pCamera);
  const xiiCamera* GetCullingCamera() const;

  void             SetLodCamera(const xiiCamera* pCamera);
  const xiiCamera* GetLodCamera() const;

  xiiEnum<xiiCameraUsageHint> GetCameraUsageHint() const;
  void                        SetCameraUsageHint(xiiEnum<xiiCameraUsageHint> hint);

  void                       SetViewRenderMode(xiiEnum<xiiViewRenderMode> value);
  xiiEnum<xiiViewRenderMode> GetViewRenderMode() const;

  /// Selects Forward+ or deferred opaque direct lighting for this view. Forward+ automatically
  /// falls back to deferred when the active device cannot execute the GPU-scene mesh path.
  void                   SetRenderPath(xiiEnum<xiiRenderPath> value);
  xiiEnum<xiiRenderPath> GetRenderPath() const;

  /// Enables calibrated sensor output for this view. Passing an invalid handle disables it.
  [[nodiscard]] xiiResult              SetSensorProfile(xiiSensorProfileHandle hProfile);
  [[nodiscard]] xiiSensorProfileHandle GetSensorProfile() const;
  /// Returns shared ownership of the last allocated sensor output. Empty after profile changes or
  /// disabling output, until the next successful pipeline build. Retain this pointer for asynchronous
  /// readback; synchronize GPU completion through GAL before accessing the image contents.
  [[nodiscard]] xiiSharedPtr<xiiGALTexture> GetSensorOutputTexture() const;

  /// Applies validated, view-local exposure, tone-mapping and display calibration.
  [[nodiscard]] xiiResult                       SetDisplayOutputSettings(const xiiDisplayOutputSettings& settings);
  [[nodiscard]] const xiiDisplayOutputSettings& GetDisplayOutputSettings() const;

  /// Invalidates all per-view temporal lighting and reconstruction history after a camera cut or teleport.
  void InvalidateTemporalHistory();

  void                SetViewport(const xiiRectFloat& viewport);
  const xiiRectFloat& GetViewport() const;

  /// Sets the per-view render scale factor (1.0 = native).
  ///        This integrates with dynamic resolution as a baseline multiplier.
  void SetRenderScale(float fRenderScale);

  /// Returns the configured per-view render scale factor (1.0 = native).
  float GetRenderScale() const;

  /// Returns the dynamic render scale applied to this view (1.0 = native viewport resolution).
  float GetRenderResolutionScale() const;

  /// Returns the dynamic internal render resolution width for this view.
  xiiUInt32 GetRenderResolutionWidth() const;

  /// Returns the dynamic internal render resolution height for this view.
  xiiUInt32 GetRenderResolutionHeight() const;

  const xiiViewData& GetData() const;

  bool IsValid() const;

  /// Controls render-data extraction and graph execution by the owning world module.
  /// Disabled views retain their camera, targets, pipeline instance, and cached resources.
  /// This does not change IsValid(). Call InvalidateTemporalHistory() when resuming after a camera cut.
  /// Like other view mutations, this requires exclusive access to the owning world.
  void SetRenderEnabled(bool bEnabled) { m_bRenderEnabled = bEnabled; }
  bool IsRenderEnabled() const { return m_bRenderEnabled; }

  /// Calculates the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fNormalizedScreenPosX and fNormalizedScreenPosY are expected to be in [0; 1] range (normalized screen coordinates).
  /// If no ray can be computed, XII_FAILURE is returned.
  xiiResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vRayStartPos, xiiVec3& out_vRayDir) const;

  /// Calculates the normalized screen-space coordinate ([0; 1] range) that the given world-space point projects to.
  ///
  /// Returns XII_FAILURE, if the point could not be projected into screen-space.
  xiiResult ComputeScreenSpacePos(const xiiVec3& vWorldPos, xiiVec3& out_vScreenPosNormalized) const;

  /// Calculates the near-plane world position at the normalized screen coordinates.
  xiiResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vWorldPos) const;

  /// Converts a screen-space position from pixel coordinates to normalized coordinates.
  void ConvertScreenPixelPosToNormalizedPos(xiiVec3& inout_vPixelPos);

  /// Converts a screen-space position from normalized coordinates to pixel coordinates.
  void ConvertScreenNormalizedPosToPixelPos(xiiVec3& inout_vNormalizedPos);

  /// Native projections use ZeroToOne reversed Z (near=1, far=0), unlike Core camera projections.
  const xiiMat4& GetProjectionMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetInverseProjectionMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetViewMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetInverseViewMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetViewProjectionMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetInverseViewProjectionMatrix(xiiCameraEye eye) const;

  void ComputeCullingFrustum(xiiFrustum& out_frustum) const;

  void                                   SetRenderPipelineResource(const xiiRenderPipelineResourceHandle& pipeline);
  const xiiRenderPipelineResourceHandle& GetRenderPipelineResource() const { return m_hRenderPipeline; }

  /// Read-only view-local pass state from the most recent pipeline build. Call only at a CPU frame
  /// boundary while holding world access. Pass pointers/arrays are borrowed until the next build;
  /// resource replacement or a failed synchronization can destroy them. Empty before the first build.
  const xiiRenderPipelineInstance&         GetRenderPipelineInstance() const;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const { return m_ExtractionRequirements; }
  const xiiRenderExtractionStatistics&     GetExtractionStatistics() const { return m_ExtractionStatistics; }
  /// Custom graph builders default to All for compatibility; specialize their demand explicitly.
  void                                  SetRenderGraphExtractionRequirements(xiiBitflags<xiiRenderExtractionCategory> requirements) { m_CustomExtractionRequirements = requirements; }
  bool                                  IsGpuSceneInitialized() const { return GetGpuInstanceData() != nullptr; }
  const xiiGpuInstanceData*             GetGpuInstanceData() const;
  xiiArrayPtr<const xiiUInt32>          GetGpuInstanceSelection() const { return m_GpuInstanceSelection; }
  xiiBitflags<xiiRenderPipelineFeature> GetRuntimeFeatures() const { return m_RuntimeFeatures; }
  void                                  SetRenderGraphRuntimeFeatures(xiiBitflags<xiiRenderPipelineFeature> features) { m_CustomRuntimeFeatures = features; }

  /// Resolves an object ID produced by this view's native GPU scene to a live mesh owner.
  /// Call at a CPU frame boundary under world access. The ID must come from this view;
  /// asynchronous callers must also retain/validate their view, camera and capture state.
  /// Returns failure and clears output for background, retired packets or deleted owners.
  [[nodiscard]] xiiResult     ResolveRenderedMesh(xiiUInt32 uiGpuObjectId, xiiGpuInstanceSource& out_source) const;
  void                        SetPipelineParameter(xiiStringView name, const xiiVariant& value);
  void                        RemovePipelineParameter(xiiStringView name);
  const xiiVariantDictionary& GetPipelineParameters() const { return m_PipelineParameters; }

  using RenderGraphBuilder = xiiDelegate<void(xiiView&, xiiRenderGraph&, xiiRenderGraphBlackboard&)>;
  void                      SetRenderGraphBuilder(RenderGraphBuilder builder);
  const RenderGraphBuilder& GetRenderGraphBuilder() const;
  xiiUInt32                 GetRenderGraphBuilderVersion() const;

  /// Extract active objects with any of these tags. An empty set includes all active objects.
  /// Tags apply to each object independently; parent tags are not inherited.
  xiiTagSet m_IncludeTags;
  /// Exclude objects with any of these tags, even if they also match m_IncludeTags.
  /// Both filters are evaluated each extraction, including for cached static render data.
  xiiTagSet m_ExcludeTags;

  xiiRenderGraph*       GetRenderGraph() { return &m_RenderGraphContext.GetGraph(); }
  const xiiRenderGraph* GetRenderGraph() const { return &m_RenderGraphContext.GetGraph(); }

  xiiExtractedRenderData*       GetExtractedRenderData() { return m_pExtractedData; }
  const xiiExtractedRenderData* GetExtractedRenderData() const { return m_pExtractedData; }

  xiiRenderGraphBlackboard&       GetBlackboard() { return m_RenderGraphContext.GetBlackboard(); }
  const xiiRenderGraphBlackboard& GetBlackboard() const { return m_RenderGraphContext.GetBlackboard(); }

  xiiRenderGraphResourceCache&       GetResourceCache() { return m_RenderGraphContext.GetResourceCache(); }
  const xiiRenderGraphResourceCache& GetResourceCache() const { return m_RenderGraphContext.GetResourceCache(); }

  /// Returns the per-view GPU timestamp profiler. Used by xiiRenderWorldModule to pass into graph execution.
  xiiRenderGraphTimestampProfiler& GetProfiler() { return m_RenderGraphContext.GetProfiler(); }

private:
  friend class xiiRenderWorldModule;
  friend class xiiBuiltinRenderPipelineRuntime;
  friend class xiiBuiltinRenderPipelinePass;
  friend class xiiViewRenderResourceManager;
  friend class xiiViewRenderResourceManagerState;
  friend class xiiMemoryUtils;

  void SetExtractedRenderData(xiiExtractedRenderData* pExtractedData) { m_pExtractedData = pExtractedData; }

  void      UpdateCachedMatrices() const;
  void      UpdateRenderResolutionState() const;
  xiiResult SynchronizeRenderPipeline();
  xiiResult PrepareRenderResources();

  /// Instantiates the selected resource and emits its frame-specific render graph.
  ///        Called by xiiRenderWorldModule::ExecuteRenderGraphs each frame.
  xiiResult BuildRenderPipeline(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiVirtualShadowMapManager::UploadHandles& virtualShadowHandles);

  // CPU PID dynamic resolution (runs before BeginSetup)
  void RunDynamicResolutionPID();



private:
  xiiWorld* const m_pWorld;

  xiiViewId m_InternalId;

  xiiHashedString m_sName;

  xiiUInt32          m_uiRenderGraphBuilderVersion = 0;
  bool               m_bRenderEnabled              = true;
  xiiCamera*         m_pCamera                     = nullptr;
  const xiiCamera*   m_pCullingCamera              = nullptr;
  const xiiCamera*   m_pLodCamera                  = nullptr;
  RenderGraphBuilder m_RenderGraphBuilder;

  mutable xiiUInt32 m_uiLastCameraSettingsModification    = 0;
  mutable xiiUInt32 m_uiLastCameraOrientationModification = 0;
  mutable float     m_fLastViewportAspectRatio            = 1.0f;

  mutable xiiViewData m_Data;

  xiiRenderGraphContext         m_RenderGraphContext;
  xiiDynamicArray<xiiUInt32>    m_GpuInstanceSelection;
  xiiGpuVisibilityContextHandle m_hGpuVisibilityContext;

  /// Non-owning pointer. The extracted data lifetime is managed by xiiRenderWorldModule.
  xiiExtractedRenderData* m_pExtractedData = nullptr;

  /// Non-owning pointer to the swapchain this view renders into.
  /// Set via SetSwapChain(); may be nullptr for off-screen views.
  const xiiGALSwapChain*          m_pSwapChain = nullptr;
  xiiSharedPtr<xiiGALTextureView> m_pRenderTargetView;

  xiiViewRenderResourceContextHandle       m_hPipelineRuntime;
  xiiRenderPipelineResourceHandle          m_hRenderPipeline;
  xiiVariantDictionary                     m_PipelineParameters;
  xiiBitflags<xiiRenderExtractionCategory> m_ExtractionRequirements;
  xiiBitflags<xiiRenderExtractionCategory> m_CustomExtractionRequirements = xiiRenderExtractionCategory::All;
  xiiRenderExtractionStatistics            m_ExtractionStatistics;
  xiiBitflags<xiiRenderPipelineFeature>    m_RuntimeFeatures;
  xiiBitflags<xiiRenderPipelineFeature>    m_CustomRuntimeFeatures = xiiRenderPipelineFeature::All;
  xiiBuiltinRenderPipelineRuntime*         GetBuiltinRuntime() const;

  xiiDisplayOutputSettings m_DisplayOutputSettings;
};

#include <GraphicsCore/Pipeline/Implementation/View_inl.h>
