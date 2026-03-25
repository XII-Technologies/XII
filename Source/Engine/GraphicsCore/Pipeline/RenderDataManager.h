#pragma once

#include <Core/World/WorldModule.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsFoundation/Shader/Types.h>
#include <GraphicsFoundation/Tools/DynamicBuffer.h>
#include <Shaders/Pipeline/Orchestration/PerFrameUploadData.h>

struct xiiPerInstanceData;
struct xiiRenderWorldExtractionEvent;
struct xiiRenderGraphPassExecutionContext;
class xiiGALCommandList;
class xiiGALFence;
class xiiRenderGraphRuntime;
class xiiRenderGraphFrameSetupPass;
class xiiRenderGraphGpuVisibilityPass;
class xiiRenderGraphInstanceUpdatePass;
class xiiRenderGraphCoarseFrustumCullingPass;
class xiiRenderGraphDrawCommandBuildPass;
class xiiRenderGraphHiZBuildPass;
class xiiRenderGraphHiZOcclusionCullingPass;
class xiiRenderGraphLodSelectionPass;
class xiiRenderGraphOccluderDepthPass;
class xiiRenderGraphDynamicResolutionPass;
class xiiRenderGraphSkinningPass;
class xiiRenderGraphPerFrameBufferUploadPass;
class xiiRenderGraphRayTracedShadowsPass;

struct XII_GRAPHICSCORE_DLL xiiInstanceDataOffset
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiOffset    = xiiInvalidIndex;
  xiiUInt8  m_uiIsDynamic = 0;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsInvalidated() const
  {
    return m_uiOffset == xiiInvalidIndex;
  }
};

struct XII_GRAPHICSCORE_DLL xiiCustomInstanceDataOffset
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiOffset = xiiInvalidIndex;

  [[nodiscard]] XII_ALWAYS_INLINE bool IsInvalidated() const
  {
    return m_uiOffset == xiiInvalidIndex;
  }
};

struct XII_GRAPHICSCORE_DLL xiiMsgCustomInstanceDataOffsetChanged : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgCustomInstanceDataOffsetChanged, xiiMessage);

  xiiCustomInstanceDataOffset m_NewOffset;
};

/// \brief CPU-side staging entry for GPU-driven visibility and indirect submission.
struct XII_GRAPHICSCORE_DLL xiiGpuDrivenInstance
{
  XII_DECLARE_POD_TYPE();

  xiiMat4           m_ObjectToWorld = xiiMat4::MakeIdentity();
  xiiBoundingSphere m_Bounds        = xiiBoundingSphere::MakeInvalid();
  xiiUInt32         m_uiMeshId      = 0U;
  xiiUInt32         m_uiMaterialId  = 0U;
  xiiUInt32         m_uiFlags       = 0U;
};

/// \brief Manager for render data and instance data buffers.
///
/// Render data is used to extract rendering information from components during the extraction phase that is then used for rendering.
/// If many objects should be rendered with one instanced draw call, instance data buffers are used to hold the per-instance information.
/// For that the render data should derive from xiiInstanceableRenderData. See xiiPerInstanceData what data is supported by default for each instance.
/// When more per instance data is needed it is possible to register a custom instance data buffer and attach that to the render data as well.
class XII_GRAPHICSCORE_DLL xiiRenderDataManager : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();

  XII_ADD_DYNAMIC_REFLECTION(xiiRenderDataManager, xiiWorldModule);

public:
  xiiRenderDataManager(xiiWorld* pWorld);
  virtual ~xiiRenderDataManager();

  virtual void Initialize() override;

  /// \brief Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
  template <typename T>
  T* CreateRenderDataForThisFrame(const xiiGameObject* pOwner) const;

  // TODO: move render data caching into this world module as well

  /// \brief Gets or creates per-instance data for the given instance data offset.
  ///
  /// This function is thread-safe and is typically called in an xiiMsgExtractRenderData message handler.
  /// The render data manager holds two instance data buffers, one for static objects and one for dynamic objects.
  /// Typically one would pass GetOwner()->IsDynamic() as bDynamic. If the corresponding render data is not cached
  /// it is better to always pass true so that the static buffer does not need to be uploaded every frame.
  xiiArrayPtr<xiiPerInstanceData> GetOrCreateInstanceData(const xiiComponent* pOwnerComponent, bool bDynamic, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount = 1) const;

  /// \brief Deletes the instance data associated with the given instance data offset.
  ///
  /// This function is thread-safe but is typically called in the OnDeactivated function of a component.
  void DeleteInstanceData(xiiInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Helper function to fill xiiPerInstanceData.
  static void FillPerInstanceData(xiiPerInstanceData& out_perInstanceData, const xiiGameObject* pObject, const xiiTransform& globalTransform, xiiUInt32 uiUniqueID = 0, const xiiColor& color = xiiColor::White, float fBoundingSphereRadius = 1.0f, xiiUInt32 uiRandomSeed = 0);

  /// \brief Helper function that combines GetOrCreateInstanceData and FillPerInstanceData.
  xiiSharedPtr<xiiGALDynamicBuffer> GetOrCreateInstanceDataAndFill(const xiiComponent& ownerComponent, bool bDynamic, const xiiTransform& globalTransform, xiiInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiUniqueID = 0, const xiiColor& color = xiiColor::White) const;


  /// \brief Registers a custom instance data buffer that can be used to store additional per-instance data.
  ///
  /// The beforeUploadCallback is called just before the buffer is uploaded each frame, so it can be used to e.g. wait for a task that generated the data.
  xiiUInt32 RegisterCustomInstanceData(const xiiGALBufferCreationDescription& desc, xiiStringView sDebugName, xiiDelegate<void()> beforeUploadCallback = {});

  /// \brief Gets or creates custom per-instance data for the given instance data offset.
  ///
  /// This function is thread-safe and is typically called in an xiiMsgExtractRenderData message handler.
  template <typename T>
  xiiArrayPtr<T> GetOrCreateCustomInstanceData(xiiUInt32 uiCustomDataIndex, const xiiComponent* pOwnerComponent, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount = 1) const;

  /// \brief Deletes the custom instance data associated with the given instance data offset.
  ///
  /// This function is thread-safe but is typically called in the OnDeactivated function of a component.
  void DeleteCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiCustomInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Helper function that combines GetOrCreateCustomInstanceData and fills it with the given data.
  template <typename T>
  xiiSharedPtr<xiiGALDynamicBuffer> GetOrCreateCustomInstanceDataAndFill(xiiUInt32 uiCustomDataIndex, const xiiComponent& ownerComponent, xiiCustomInstanceDataOffset& inout_instanceDataOffset, const T& data) const;

  /// \brief Returns the underlying dynamic buffer for the given custom instance data buffer index.
  xiiSharedPtr<xiiGALDynamicBuffer> GetCustomInstanceDataBuffer(xiiUInt32 uiCustomDataIndex) const;

  /// \brief Compacts the given custom instance data buffer to reduce fragmentation.
  ///
  /// This is only necessary if allocations with different counts were created and deleted over time.
  void CompactCustomInstanceDataBuffer(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiMaxSteps = 16);


  /// \brief Gets or creates skinning data for the given instance data offset.
  ///
  /// xiiSkinningState wraps around these functions to manage skinning data for skinned meshes
  /// and should be preferred instead of calling these functions directly.
  xiiArrayPtr<xiiShaderTransform> GetOrCreateSkinningData(const xiiComponent* pOwnerComponent, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiNumTransforms) const;

  /// \brief Gets the skinning data for reading for the given instance data offset.
  xiiArrayPtr<const xiiShaderTransform> GetSkinningData(const xiiCustomInstanceDataOffset& instanceDataOffset) const;

  /// \brief Deletes the skinning data associated with the given instance data offset.
  void DeleteSkinningData(xiiCustomInstanceDataOffset& inout_instanceDataOffset) const;

  /// \brief Returns the underlying dynamic buffer that holds the skinning data.
  xiiSharedPtr<xiiGALDynamicBuffer> GetSkinningDataBuffer() const;

  /// \brief Starts collecting GPU-driven scene instances for the next visibility pass.
  void BeginGpuDrivenBuild();

  /// \brief Appends one instance entry to the GPU-driven scene staging buffer.
  void AddGpuDrivenInstance(const xiiTransform& globalTransform, const xiiBoundingSphere& bounds, xiiUInt32 uiMeshId, xiiUInt32 uiMaterialId, xiiUInt32 uiFlags = 0U);

  /// \brief Finalizes GPU-driven scene staging before pass registration.
  void EndGpuDrivenBuild();

  /// \brief Returns a snapshot of staged GPU-driven instances for debug/profiling.
  xiiArrayPtr<const xiiGpuDrivenInstance> GetGpuDrivenInstances() const;

  /// \brief Stores the latest visible-instance list produced by visibility processing.
  void SetGpuDrivenVisibleInstanceIndices(xiiArrayPtr<const xiiUInt32> visibleInstanceIndices);

  /// \brief Returns a snapshot of currently visible instance indices.
  xiiArrayPtr<const xiiUInt32> GetGpuDrivenVisibleInstanceIndices() const;

  /// \brief Returns the current GPU scene-instances buffer used by visibility dispatch.
  xiiSharedPtr<xiiGALBuffer> GetGpuDrivenSceneInstancesBuffer() const;

  /// \brief Returns the current GPU visible-instance indices buffer written by visibility dispatch.
  xiiSharedPtr<xiiGALBuffer> GetGpuDrivenVisibleInstancesBuffer() const;

  /// \brief Returns the current GPU visible-instance count buffer written by visibility dispatch.
  xiiSharedPtr<xiiGALBuffer> GetGpuDrivenVisibleInstanceCountBuffer() const;

  /// \brief Attempts to read back the latest GPU-written visible-instance count.
  bool TryGetGpuDrivenVisibleInstanceCountReadback(xiiUInt32& out_uiVisibleInstanceCount, bool bWaitForCompletion = false) const;

  /// \brief Registers the GPU-driven visibility pass into the provided render graph runtime.
  void AddGpuDrivenVisibilityPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Configures direct-dispatch thread group size for GPU-driven visibility.
  void SetGpuDrivenVisibilityThreadGroupSize(xiiUInt32 uiThreadGroupSize) const;

  /// \brief Configures explicit direct-dispatch group counts. X = 0 switches back to auto-derived count.
  void SetGpuDrivenVisibilityDirectDispatchThreadGroupCount(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY = 1U, xiiUInt32 uiThreadGroupCountZ = 1U) const;

  /// \brief Configures indirect dispatch argument buffer for GPU-driven visibility.
  void SetGpuDrivenVisibilityIndirectDispatchArguments(xiiSharedPtr<xiiGALBuffer> pIndirectDispatchArguments, xiiUInt64 uiDispatchArgumentOffset = 0U, xiiEnum<xiiGALStateTransitionMode> bufferTransitionMode = xiiGALStateTransitionMode::Transition) const;

  /// \brief Enables or disables the manager-owned indirect dispatch path for GPU-driven visibility.
  void SetGpuDrivenVisibilityUseInternalIndirectDispatch(bool bEnable) const;

  /// \brief Sets a callback that can bind compute PSO/resources before visibility dispatch.
  void SetGpuDrivenVisibilitySetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const;

  /// \brief Clears the optional visibility setup callback.
  void ClearGpuDrivenVisibilitySetupFunc() const;

  /// \brief Registers the ray-traced shadows pass into the provided render graph runtime.
  void AddRayTracedShadowsPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Registers the frame-setup pass and required frame-level orchestration resources.
  void AddFrameSetupPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass = true) const;

  /// \brief Registers the dynamic-resolution pass and required resources into the provided render graph runtime.
  void AddDynamicResolutionPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = true) const;

  /// \brief Registers the skinning+morph compute pass for early async deformer evaluation.
  void AddSkinningAndMorphPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Registers the async instance transform and bounds update pass.
  void AddInstanceTransformAndBoundsUpdatePass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Registers the async coarse frustum culling reduction pass before depth tests.
  void AddCoarseFrustumCullingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Registers the low-cost occluder depth prepass on the graphics queue.
  void AddOccluderDepthPrepassPass(xiiRenderGraphRuntime& inout_runtime, bool bEnablePass = false) const;

  /// \brief Registers the async Hi-Z pyramid build pass from occluder depth.
  void AddHiZPyramidBuildPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Registers the async Hi-Z occlusion culling pass from candidates + depth pyramid.
  void AddHiZOcclusionCullingPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Registers the async draw-indirect build and compaction pass from visible instances + material bins.
  void AddDrawIndirectCommandBuildPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Registers the async LOD selection and meshlet classification pass.
  void AddLodSelectionAndMeshletClassificationPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableDispatch = false) const;

  /// \brief Registers the per-frame upload pass for camera, light and global buffers.
  void AddPerFrameBufferUploadPass(xiiRenderGraphRuntime& inout_runtime, bool bEnableUploads = true) const;

  /// \brief Updates the frame timing sample consumed by the dynamic-resolution shader.
  void SetDynamicResolutionFrameTimingSample(const xiiVec4& vFrameTimingSample) const;

  /// \brief Updates the camera velocity sample consumed by the dynamic-resolution shader.
  void SetDynamicResolutionCameraVelocitySample(const xiiVec4& vCameraVelocitySample) const;

  /// \brief Updates a representative skinning-input sample consumed by the skinning+morph pass.
  void SetSkinningInputSample(const xiiVec4& vSkinningInputSample) const;

  /// \brief Updates a representative morph-weights sample consumed by the skinning+morph pass.
  void SetMorphWeightsSample(const xiiVec4& vMorphWeightsSample) const;

  /// \brief Updates a representative scene-transform sample consumed by the instance update pass.
  void SetSceneTransformsSample(const xiiShaderTransform& sceneTransformSample) const;

  /// \brief Updates staged camera frustum planes consumed by coarse frustum culling.
  void SetCoarseFrustumPlaneSample(xiiUInt32 uiPlaneIndex, const xiiVec4& vPlane) const;

  /// \brief Supplies the depth resource written by the occluder depth prepass.
  void SetOccluderDepthPrepassDepthResource(xiiSharedPtr<xiiGALResource> pDepthResource) const;

  /// \brief Supplies the occluder instance list consumed by the occluder depth prepass.
  void SetOccluderDepthPrepassInstanceListResource(xiiSharedPtr<xiiGALBuffer> pInstanceListResource) const;

  /// \brief Supplies the source depth resource consumed by Hi-Z pyramid build.
  void SetHiZDepthSourceResource(xiiSharedPtr<xiiGALResource> pDepthResource) const;

  /// \brief Supplies the destination depth-pyramid resource consumed and written by Hi-Z build.
  void SetHiZDepthPyramidResource(xiiSharedPtr<xiiGALResource> pDepthPyramidResource) const;

  /// \brief Supplies candidate instance list consumed by Hi-Z occlusion culling.
  void SetHiZOcclusionCandidateInstancesResource(xiiSharedPtr<xiiGALBuffer> pCandidateInstancesResource) const;

  /// \brief Supplies candidate instance count consumed by Hi-Z occlusion culling.
  void SetHiZOcclusionCandidateInstanceCountResource(xiiSharedPtr<xiiGALBuffer> pCandidateInstanceCountResource) const;

  /// \brief Supplies material-bin metadata consumed by draw-indirect command build.
  void SetDrawIndirectMaterialBinsResource(xiiSharedPtr<xiiGALBuffer> pMaterialBinsResource) const;

  /// \brief Supplies external indirect-draw command output buffer used by draw command compaction.
  void SetDrawIndirectCommandBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCommandBufferResource) const;

  /// \brief Supplies external indirect-draw count output buffer used by draw command compaction.
  void SetDrawIndirectCountBufferResource(xiiSharedPtr<xiiGALBuffer> pIndirectCountBufferResource) const;

  /// \brief Sets an optional callback for lightweight graphics state setup before occluder drawing.
  void SetOccluderDepthPrepassSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const;

  /// \brief Sets an optional callback that records geometry-only occluder draw commands.
  void SetOccluderDepthPrepassDrawFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> drawFunc) const;

  /// \brief Clears the optional occluder depth prepass setup callback.
  void ClearOccluderDepthPrepassSetupFunc() const;

  /// \brief Clears the optional occluder depth prepass draw callback.
  void ClearOccluderDepthPrepassDrawFunc() const;

  /// \brief Updates staged camera constants payload uploaded by the per-frame upload pass.
  void SetPerFrameUploadCameraConstantsSample(const xiiPerFrameCameraUploadData& cameraConstantsSample) const;

  /// \brief Updates staged light-data payload uploaded by the per-frame upload pass.
  void SetPerFrameUploadLightDataSample(const xiiPerFrameLightUploadData& lightDataSample) const;

  /// \brief Updates staged global-params payload uploaded by the per-frame upload pass.
  void SetPerFrameUploadGlobalParamsSample(const xiiPerFrameGlobalUploadData& globalParamsSample) const;

  /// \brief Updates the previous-frame stats sampled by the frame-setup kickoff pass.
  void SetPreviousFrameStatsSample(const xiiPreviousFrameStats& previousFrameStatsSample) const;

  /// \brief Enables or disables denoiser-history IO resources for ray-traced shadows.
  void SetRayTracedShadowsDenoiserHistoryEnabled(bool bEnable) const;

  /// \brief Sets render-graph resource names used by the ray-traced shadows pass.
  void SetRayTracedShadowsSceneTlasResourceName(xiiHashedString sResourceName) const;
  void SetRayTracedShadowsDepthResourceName(xiiHashedString sResourceName) const;
  void SetRayTracedShadowsNormalResourceName(xiiHashedString sResourceName) const;
  void SetRayTracedShadowsLightDataResourceName(xiiHashedString sResourceName) const;
  void SetRayTracedShadowsShadowMaskResourceName(xiiHashedString sResourceName) const;
  void SetRayTracedShadowsHistoryInputResourceName(xiiHashedString sResourceName) const;
  void SetRayTracedShadowsHistoryOutputResourceName(xiiHashedString sResourceName) const;

  /// \brief Sets a callback that can bind RT PSO/resources before shadow ray dispatch.
  void SetRayTracedShadowsSetupFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> setupFunc) const;

  /// \brief Sets a callback that records the actual ray tracing dispatch command.
  void SetRayTracedShadowsDispatchFunc(xiiDelegate<void(xiiGALCommandList&, const xiiRenderGraphPassExecutionContext&)> dispatchFunc) const;

  /// \brief Clears the optional ray-traced shadows setup callback.
  void ClearRayTracedShadowsSetupFunc() const;

  /// \brief Clears the optional ray-traced shadows dispatch callback.
  void ClearRayTracedShadowsDispatchFunc() const;

private:
  xiiByteArrayPtr GetOrCreateCustomInstanceData(xiiUInt32 uiCustomDataIndex, xiiUInt32 uiStructByteSize, const xiiComponent* pOwnerComponent, xiiSharedPtr<xiiGALDynamicBuffer>& out_pBuffer, xiiCustomInstanceDataOffset& inout_instanceDataOffset, xiiUInt32 uiCount) const;

  void CompactSkinningDataBuffer(const UpdateContext& context);
  void OnExtractionEvent(const xiiRenderWorldExtractionEvent& e);

  void EnsureGpuDrivenVisibilityResources(xiiUInt32 uiInstanceCapacity) const;
  void EnsureDynamicResolutionResources(xiiUInt32 uiElementCount) const;
  void EnsureSkinningAndMorphResources(xiiUInt32 uiElementCount) const;
  void EnsureInstanceUpdateResources(xiiUInt32 uiElementCount) const;
  void EnsureCoarseFrustumCullingResources(xiiUInt32 uiElementCount) const;
  void EnsureLodSelectionResources(xiiUInt32 uiElementCount) const;
  void EnsureDrawIndirectCommandBuildResources(xiiUInt32 uiDrawCapacity) const;
  void EnsurePerFrameUploadResources(xiiUInt32 uiRingSize) const;
  void EnsureFrameSetupResources() const;
  void SetupGpuDrivenVisibilityCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupDynamicResolutionCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupSkinningAndMorphCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupInstanceUpdateCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupCoarseFrustumCullingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupOccluderDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void DrawOccluderDepthPrepassCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupHiZPyramidBuildCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupHiZOcclusionCullingCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupDrawIndirectCommandBuildCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupLodSelectionCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupFrameSetupCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void UploadPerFrameBufferDataCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void OnGpuDrivenVisibilityPostDispatch(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void SetupRayTracedShadowsCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;
  void DispatchRayTracedShadowsCommandList(xiiGALCommandList& commandList, const xiiRenderGraphPassExecutionContext& executionContext) const;

  mutable xiiMutex m_Mutex;

  xiiHybridArray<xiiSharedPtr<xiiGALDynamicBuffer>, 16> m_Buffers;
  xiiDynamicArray<xiiDelegate<void()>>                  m_BeforeUploadCallbacks;

  struct ExtractionData
  {
    xiiHybridArray<xiiSharedPtr<xiiGALDynamicBuffer>, 16> m_pBuffers;
  };

  ExtractionData m_ExtractionData;

  mutable xiiDynamicArray<xiiGpuDrivenInstance>             m_GpuDrivenInstances;
  mutable xiiDynamicArray<xiiUInt32>                        m_GpuDrivenVisibleInstanceIndices;
  mutable xiiShaderResourceHandle                           m_hGpuDrivenVisibilityShader;
  mutable xiiShaderResourceHandle                           m_hDynamicResolutionShader;
  mutable xiiShaderResourceHandle                           m_hSkinningShader;
  mutable xiiShaderResourceHandle                           m_hInstanceUpdateShader;
  mutable xiiShaderResourceHandle                           m_hCoarseFrustumCullingShader;
  mutable xiiShaderResourceHandle                           m_hHiZBuildShader;
  mutable xiiShaderResourceHandle                           m_hHiZOcclusionCullingShader;
  mutable xiiShaderResourceHandle                           m_hDrawCommandBuildShader;
  mutable xiiShaderResourceHandle                           m_hLodSelectionShader;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pGpuDrivenVisibilityPipelineState;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pDynamicResolutionPipelineState;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pSkinningPipelineState;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pInstanceUpdatePipelineState;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pCoarseFrustumCullingPipelineState;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pHiZBuildPipelineState;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pHiZOcclusionCullingPipelineState;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pDrawCommandBuildPipelineState;
  mutable xiiSharedPtr<xiiGALComputePipelineState>          m_pLodSelectionPipelineState;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuSceneInstancesBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuVisibleInstancesBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuVisibleInstanceCountBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pHiZOcclusionVisibleInstancesBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pHiZOcclusionVisibleInstanceCountBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pOccluderInstanceListBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pHiZOcclusionCandidateInstancesBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pHiZOcclusionCandidateInstanceCountBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuMaterialBinsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuIndirectDrawCommandsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuIndirectDrawCountsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuVisibleInstanceCountReadbackBuffer;
  mutable xiiSharedPtr<xiiGALResource>                      m_pOccluderDepthResource;
  mutable xiiSharedPtr<xiiGALResource>                      m_pHiZDepthSourceResource;
  mutable xiiSharedPtr<xiiGALResource>                      m_pHiZDepthPyramidResource;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuVisibilityDispatchArgumentsBuffer;
  mutable xiiSharedPtr<xiiGALFence>                         m_pGpuVisibilityReadbackFence;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pDynamicResolutionFrameTimingBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pDynamicResolutionCameraVelocityBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pDynamicResolutionBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pSkinningInputBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pSkinningBonePaletteBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pMorphWeightsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pSkinnedVerticesBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pSceneTransformsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pUpdatedGpuSceneInstancesBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuSceneBoundsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pCameraFrustumPlanesBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuLodSelectionsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pGpuDrawMetadataBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pPreviousFrameStatsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pFrameConstantsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pFrameTimestampRangesBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pPerFrameCameraConstantsBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pPerFrameLightDataBuffer;
  mutable xiiSharedPtr<xiiGALBuffer>                        m_pPerFrameGlobalParamsBuffer;
  mutable xiiUInt64                                         m_uiGpuVisibilityReadbackFenceValue         = 0U;
  mutable xiiUInt64                                         m_uiGpuVisibilityReadbackCompletedValue     = 0U;
  mutable xiiUInt32                                         m_uiGpuVisibilityThreadGroupSize            = 64U;
  mutable xiiUInt32                                         m_uiPerFrameUploadRingSize                 = 3U;
  mutable xiiUInt32                                         m_uiPerFrameUploadWriteIndex               = 0U;
  mutable bool                                              m_bGpuVisibilityUseInternalIndirectDispatch = false;
  mutable xiiVec4                                           m_vDynamicResolutionFrameTimingSample       = xiiVec4(16.666f, 0.0f, 0.0f, 0.0f);
  mutable xiiVec4                                           m_vDynamicResolutionCameraVelocitySample    = xiiVec4::MakeZero();
  mutable xiiVec4                                           m_vSkinningInputSample                      = xiiVec4::MakeZero();
  mutable xiiVec4                                           m_vMorphWeightsSample                       = xiiVec4(1.0f, 0.0f, 0.0f, 0.0f);
  mutable xiiShaderTransform                                m_SceneTransformsSample                     = {};
  mutable xiiVec4                                           m_vCoarseFrustumPlaneSamples[6]            = {xiiVec4(1.0f, 0.0f, 0.0f, 1.0f), xiiVec4(-1.0f, 0.0f, 0.0f, 1.0f), xiiVec4(0.0f, 1.0f, 0.0f, 1.0f), xiiVec4(0.0f, -1.0f, 0.0f, 1.0f), xiiVec4(0.0f, 0.0f, 1.0f, 0.0f), xiiVec4(0.0f, 0.0f, -1.0f, 1.0f)};
  mutable xiiPreviousFrameStats                             m_PreviousFrameStatsSample;
  mutable xiiPerFrameCameraUploadData                       m_PerFrameCameraConstantsSample = {};
  mutable xiiPerFrameLightUploadData                        m_PerFrameLightDataSample       = {};
  mutable xiiPerFrameGlobalUploadData                       m_PerFrameGlobalParamsSample    = {};
  mutable xiiUniquePtr<xiiRenderGraphFrameSetupPass>        m_pFrameSetupPass;
  mutable xiiUniquePtr<xiiRenderGraphGpuVisibilityPass>     m_pGpuDrivenVisibilityPass;
  mutable xiiUniquePtr<xiiRenderGraphInstanceUpdatePass>    m_pInstanceUpdatePass;
  mutable xiiUniquePtr<xiiRenderGraphCoarseFrustumCullingPass> m_pCoarseFrustumCullingPass;
  mutable xiiUniquePtr<xiiRenderGraphOccluderDepthPass>     m_pOccluderDepthPass;
  mutable xiiUniquePtr<xiiRenderGraphHiZBuildPass>          m_pHiZBuildPass;
  mutable xiiUniquePtr<xiiRenderGraphHiZOcclusionCullingPass> m_pHiZOcclusionCullingPass;
  mutable xiiUniquePtr<xiiRenderGraphDrawCommandBuildPass>  m_pDrawCommandBuildPass;
  mutable xiiUniquePtr<xiiRenderGraphLodSelectionPass>      m_pLodSelectionPass;
  mutable xiiUniquePtr<xiiRenderGraphDynamicResolutionPass> m_pDynamicResolutionPass;
  mutable xiiUniquePtr<xiiRenderGraphSkinningPass>          m_pSkinningPass;
  mutable xiiUniquePtr<xiiRenderGraphPerFrameBufferUploadPass> m_pPerFrameBufferUploadPass;
  mutable xiiUniquePtr<xiiRenderGraphRayTracedShadowsPass>  m_pRayTracedShadowsPass;
};

#include <GraphicsCore/Pipeline/Implementation/RenderDataManager_inl.h>
