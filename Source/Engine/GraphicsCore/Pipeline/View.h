/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Device/SwapChain.h>

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Lighting/LightingSystem.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

class xiiFrustum;
class xiiWorld;
class xiiRenderGraph;
class xiiRGBuilder;
class xiiRGPassContext;
class xiiExtractedRenderData;
class xiiGALBuffer;
class xiiGALTexture;
class xiiGALSampler;
class xiiGALComputePipelineState;
class xiiGALGraphicsPipelineState;

struct xiiOcclusionReadbackData;
struct xiiFrustumCullData;
struct xiiLODSelectData;
struct xiiInstanceUpdateData;
struct xiiDrawBuildData;
struct xiiShadowCasterBuildData;
struct xiiLightingDataUploadData;
struct xiiClusterBuildData;
struct xiiLightListData;
struct xiiReflectionProbeSelectData;
struct xiiFroxelAllocationData;

struct xiiShadowCascadeSetupData;
struct xiiLocalShadowAtlasAllocationData;
struct xiiDirectionalShadowData;
struct xiiSpotShadowData;
struct xiiPointShadowData;
struct xiiRayTracedShadowData;
struct xiiShadowDenoiseData;
struct xiiContactShadowData;

struct xiiDepthPrepassData;
struct xiiHiZPyramidData;
struct xiiHiZOcclusionCullData;
struct xiiMotionVectorsData;
struct xiiVelocityDilationData;

struct xiiGBufferBaseData;
struct xiiNormalRoughnessPrepassData;

struct xiiBRDFLutGenerationData;
struct xiiAtmosphereTransmittanceData;
struct xiiAtmosphereMultiScatterData;
struct xiiSkyIrradianceConvolutionData;
struct xiiReflectionProbeConvolutionData;
struct xiiVolumetricFogInitializationData;
struct xiiDDGIProbeSamplingData;
struct xiiGroundTruthAmbientOcclusionData;
struct xiiGroundTruthAmbientOcclusionDenoiseData;

struct xiiDeferredDirectLightingData;
struct xiiDeferredIndirectLightingData;
struct xiiRayTracedGlobalIlluminationData;
struct xiiRayTracedReflectionsData;
struct xiiScreenSpaceReflectionsData;
struct xiiVolumetricFogIntegrationData;
struct xiiVolumetricFogTemporalReprojectionData;
struct xiiAtmosphereCompositeData;

struct xiiForwardOpaqueData;
struct xiiForwardMaskedData;
struct xiiHairRenderingData;
struct xiiWaterRenderingData;
struct xiiSubsurfaceScatteringData;
struct xiiEyeShaderData;

struct xiiGPUParticleSimulateData;
struct xiiDecalUploadData;
struct xiiDecalCullBatchData;
struct xiiProjectedDecalResolveData;
struct xiiMeshDecalDrawData;
struct xiiWeightedBlendedOITData;

struct xiiScreenSpaceGlobalIlluminationData;
struct xiiScreenSpaceRefractionData;
struct xiiPlanarReflectionsData;

struct xiiLuminanceHistogramData;
struct xiiAutoExposureData;
struct xiiTemporalAntiAliasingData;
struct xiiUpscaleData;

struct xiiBloomData;
struct xiiColorGradingData;
struct xiiToneMappingData;

struct xiiFinalBlitData;

/// Encapsulates a view on the given world through the given camera
/// and rendered with the specified RenderPipeline into the given render target setup.
///
/// The view owns its entire rendering pipeline: every pass's persistent GPU resources
/// (pipeline states, persistent textures, ring buffers) live in m_ViewPassResources.
/// BuildDefaultRenderGraph() populates the render graph each frame.
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
  void               SetRenderTargetView(xiiGALTextureView* pRenderTargetView);

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

  /// Calculates the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fNormalizedScreenPosX and fNormalizedScreenPosY are expected to be in [0; 1] range (normalized screen coordinates).
  /// If no ray can be computed, EZ_FAILURE is returned.
  xiiResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vRayStartPos, xiiVec3& out_vRayDir) const;

  /// Calculates the normalized screen-space coordinate ([0; 1] range) that the given world-space point projects to.
  ///
  /// Returns EZ_FAILURE, if the point could not be projected into screen-space.
  xiiResult ComputeScreenSpacePos(const xiiVec3& vWorldPos, xiiVec3& out_vScreenPosNormalized) const;

  /// Calculates the world-space position that the given normalized screen-space coordinate maps to
  xiiResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vWorldPos) const;

  /// Converts a screen-space position from pixel coordinates to normalized coordinates.
  void ConvertScreenPixelPosToNormalizedPos(xiiVec3& inout_vPixelPos);

  /// Converts a screen-space position from normalized coordinates to pixel coordinates.
  void ConvertScreenNormalizedPosToPixelPos(xiiVec3& inout_vNormalizedPos);

  const xiiMat4& GetProjectionMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetInverseProjectionMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetViewMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetInverseViewMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetViewProjectionMatrix(xiiCameraEye eye) const;
  const xiiMat4& GetInverseViewProjectionMatrix(xiiCameraEye eye) const;

  void ComputeCullingFrustum(xiiFrustum& out_frustum) const;

  using RenderGraphBuilder = xiiDelegate<void(xiiView&, xiiRenderGraph&, xiiRenderGraphBlackboard&)>;
  void                      SetRenderGraphBuilder(RenderGraphBuilder builder);
  const RenderGraphBuilder& GetRenderGraphBuilder() const;
  xiiUInt32                 GetRenderGraphBuilderVersion() const;

  xiiTagSet m_IncludeTags;
  xiiTagSet m_ExcludeTags;

  xiiRenderGraph*       GetRenderGraph() { return m_pRenderGraph.Borrow(); }
  const xiiRenderGraph* GetRenderGraph() const { return m_pRenderGraph.Borrow(); }

  xiiExtractedRenderData*       GetExtractedRenderData() { return m_pExtractedData; }
  const xiiExtractedRenderData* GetExtractedRenderData() const { return m_pExtractedData; }

  xiiRenderGraphBlackboard&       GetBlackboard() { return m_Blackboard; }
  const xiiRenderGraphBlackboard& GetBlackboard() const { return m_Blackboard; }

  xiiRenderGraphResourceCache&       GetResourceCache() { return m_ResourceCache; }
  const xiiRenderGraphResourceCache& GetResourceCache() const { return m_ResourceCache; }

  /// Returns the per-view GPU timestamp profiler. Used by xiiRenderWorldModule to pass into graph execution.
  xiiRenderGraphTimestampProfiler& GetProfiler() { return m_ViewPassResources.m_Profiler; }

private:
  friend class xiiRenderWorldModule;
  friend class xiiMemoryUtils;

  void SetExtractedRenderData(xiiExtractedRenderData* pExtractedData) { m_pExtractedData = pExtractedData; }

  void UpdateCachedMatrices() const;
  void UpdateRenderResolutionState() const;

  /// Populates the render graph for default (non-custom) views.
  ///        Called by xiiRenderWorldModule::ExecuteRenderGraphs each frame.
  void BuildDefaultRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  // CPU PID dynamic resolution (runs before BeginSetup)
  void RunDynamicResolutionPID();


  void SetupOcclusionReadback(xiiOcclusionReadbackData& data, xiiRGBuilder& builder);
  void ExecuteOcclusionReadback(const xiiOcclusionReadbackData& data, xiiRGPassContext& context);

  void SetupFrustumCull(xiiFrustumCullData& data, xiiRGBuilder& builder);
  void ExecuteFrustumCull(const xiiFrustumCullData& data, xiiRGPassContext& context);

  void SetupLODSelect(xiiLODSelectData& data, xiiRGBuilder& builder);
  void ExecuteLODSelect(const xiiLODSelectData& data, xiiRGPassContext& context);

  void SetupInstanceUpdate(xiiInstanceUpdateData& data, xiiRGBuilder& builder);
  void ExecuteInstanceUpdate(const xiiInstanceUpdateData& data, xiiRGPassContext& context);

  void SetupDrawBuild(xiiDrawBuildData& data, xiiRGBuilder& builder);
  void ExecuteDrawBuild(const xiiDrawBuildData& data, xiiRGPassContext& context);

  void SetupShadowCasterBuild(xiiShadowCasterBuildData& data, xiiRGBuilder& builder);
  void ExecuteShadowCasterBuild(const xiiShadowCasterBuildData& data, xiiRGPassContext& context);

  void SetupLightingDataUpload(xiiLightingDataUploadData& data, xiiRGBuilder& builder);
  void ExecuteLightingDataUpload(const xiiLightingDataUploadData& data, xiiRGPassContext& context);

  void SetupClusterBuild(xiiClusterBuildData& data, xiiRGBuilder& builder);
  void ExecuteClusterBuild(const xiiClusterBuildData& data, xiiRGPassContext& context);

  void SetupLightListBuild(xiiLightListData& data, xiiRGBuilder& builder);
  void ExecuteLightListBuild(const xiiLightListData& data, xiiRGPassContext& context);

  void SetupReflectionProbeSelect(xiiReflectionProbeSelectData& data, xiiRGBuilder& builder);
  void ExecuteReflectionProbeSelect(const xiiReflectionProbeSelectData& data, xiiRGPassContext& context);

  void SetupFroxelAllocation(xiiFroxelAllocationData& data, xiiRGBuilder& builder);
  void ExecuteFroxelAllocation(const xiiFroxelAllocationData& data, xiiRGPassContext& context);


  void SetupShadowCascadeSetup(xiiShadowCascadeSetupData& data, xiiRGBuilder& builder);
  void ExecuteShadowCascadeSetup(const xiiShadowCascadeSetupData& data, xiiRGPassContext& context);

  void SetupLocalShadowAtlasAllocation(xiiLocalShadowAtlasAllocationData& data, xiiRGBuilder& builder);
  void ExecuteLocalShadowAtlasAllocation(const xiiLocalShadowAtlasAllocationData& data, xiiRGPassContext& context);

  void SetupDirectionalShadowData(xiiDirectionalShadowData& data, xiiRGBuilder& builder);
  void ExecuteDirectionalShadowData(const xiiDirectionalShadowData& data, xiiRGPassContext& context);

  void SetupSpotShadowData(xiiSpotShadowData& data, xiiRGBuilder& builder);
  void ExecuteSpotShadowData(const xiiSpotShadowData& data, xiiRGPassContext& context);

  void SetupPointShadowData(xiiPointShadowData& data, xiiRGBuilder& builder);
  void ExecutePointShadowData(const xiiPointShadowData& data, xiiRGPassContext& context);

  void SetupRayTracedShadowData(xiiRayTracedShadowData& data, xiiRGBuilder& builder);
  void ExecuteRayTracedShadowData(const xiiRayTracedShadowData& data, xiiRGPassContext& context);

  void SetupShadowDenoiseData(xiiShadowDenoiseData& data, xiiRGBuilder& builder);
  void ExecuteShadowDenoiseData(const xiiShadowDenoiseData& data, xiiRGPassContext& context);

  void SetupContactShadowData(xiiContactShadowData& data, xiiRGBuilder& builder);
  void ExecuteContactShadowData(const xiiContactShadowData& data, xiiRGPassContext& context);


  void SetupDepthPrepass(xiiDepthPrepassData& data, xiiRGBuilder& builder);
  void ExecuteDepthPrepass(const xiiDepthPrepassData& data, xiiRGPassContext& context);

  void SetupHiZPyramid(xiiHiZPyramidData& data, xiiRGBuilder& builder);
  void ExecuteHiZPyramid(const xiiHiZPyramidData& data, xiiRGPassContext& context);

  void SetupHiZOcclusionCull(xiiHiZOcclusionCullData& data, xiiRGBuilder& builder);
  void ExecuteHiZOcclusionCull(const xiiHiZOcclusionCullData& data, xiiRGPassContext& context);

  void SetupMotionVectors(xiiMotionVectorsData& data, xiiRGBuilder& builder);
  void ExecuteMotionVectors(const xiiMotionVectorsData& data, xiiRGPassContext& context);

  void SetupVelocityDilation(xiiVelocityDilationData& data, xiiRGBuilder& builder);
  void ExecuteVelocityDilation(const xiiVelocityDilationData& data, xiiRGPassContext& context);


  void SetupGBufferBase(xiiGBufferBaseData& data, xiiRGBuilder& builder);
  void ExecuteGBufferBase(const xiiGBufferBaseData& data, xiiRGPassContext& context);

  void SetupNormalRoughnessPrepass(xiiNormalRoughnessPrepassData& data, xiiRGBuilder& builder);
  void ExecuteNormalRoughnessPrepass(const xiiNormalRoughnessPrepassData& data, xiiRGPassContext& context);


  void SetupBRDFLutGeneration(xiiBRDFLutGenerationData& data, xiiRGBuilder& builder);
  void ExecuteBRDFLutGeneration(const xiiBRDFLutGenerationData& data, xiiRGPassContext& context);

  void SetupAtmosphereTransmittance(xiiAtmosphereTransmittanceData& data, xiiRGBuilder& builder);
  void ExecuteAtmosphereTransmittance(const xiiAtmosphereTransmittanceData& data, xiiRGPassContext& context);

  void SetupAtmosphereMultiScatter(xiiAtmosphereMultiScatterData& data, xiiRGBuilder& builder);
  void ExecuteAtmosphereMultiScatter(const xiiAtmosphereMultiScatterData& data, xiiRGPassContext& context);

  void SetupSkyIrradianceConvolution(xiiSkyIrradianceConvolutionData& data, xiiRGBuilder& builder);
  void ExecuteSkyIrradianceConvolution(const xiiSkyIrradianceConvolutionData& data, xiiRGPassContext& context);

  void SetupReflectionProbeConvolution(xiiReflectionProbeConvolutionData& data, xiiRGBuilder& builder);
  void ExecuteReflectionProbeConvolution(const xiiReflectionProbeConvolutionData& data, xiiRGPassContext& context);

  void SetupVolumetricFogInitialization(xiiVolumetricFogInitializationData& data, xiiRGBuilder& builder);
  void ExecuteVolumetricFogInitialization(const xiiVolumetricFogInitializationData& data, xiiRGPassContext& context);

  void SetupDDGIProbeSampling(xiiDDGIProbeSamplingData& data, xiiRGBuilder& builder);
  void ExecuteDDGIProbeSampling(const xiiDDGIProbeSamplingData& data, xiiRGPassContext& context);

  void SetupGroundTruthAmbientOcclusion(xiiGroundTruthAmbientOcclusionData& data, xiiRGBuilder& builder);
  void ExecuteGroundTruthAmbientOcclusion(const xiiGroundTruthAmbientOcclusionData& data, xiiRGPassContext& context);

  void SetupGroundTruthAmbientOcclusionDenoise(xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRGBuilder& builder);
  void ExecuteGroundTruthAmbientOcclusionDenoise(const xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRGPassContext& context);


  void SetupDirectLighting(xiiDeferredDirectLightingData& data, xiiRGBuilder& builder);
  void ExecuteDirectLighting(const xiiDeferredDirectLightingData& data, xiiRGPassContext& context);

  void SetupIndirectLighting(xiiDeferredIndirectLightingData& data, xiiRGBuilder& builder);
  void ExecuteIndirectLighting(const xiiDeferredIndirectLightingData& data, xiiRGPassContext& context);

  void SetupRayTracedGlobalIllumination(xiiRayTracedGlobalIlluminationData& data, xiiRGBuilder& builder);
  void ExecuteRayTracedGlobalIllumination(const xiiRayTracedGlobalIlluminationData& data, xiiRGPassContext& context);

  void SetupRayTracedReflections(xiiRayTracedReflectionsData& data, xiiRGBuilder& builder);
  void ExecuteRayTracedReflections(const xiiRayTracedReflectionsData& data, xiiRGPassContext& context);

  void SetupScreenSpaceReflections(xiiScreenSpaceReflectionsData& data, xiiRGBuilder& builder);
  void ExecuteScreenSpaceReflections(const xiiScreenSpaceReflectionsData& data, xiiRGPassContext& context);

  void SetupVolumetricFogIntegration(xiiVolumetricFogIntegrationData& data, xiiRGBuilder& builder);
  void ExecuteVolumetricFogIntegration(const xiiVolumetricFogIntegrationData& data, xiiRGPassContext& context);

  void SetupVolumetricFogTemporalReprojection(xiiVolumetricFogTemporalReprojectionData& data, xiiRGBuilder& builder);
  void ExecuteVolumetricFogTemporalReprojection(const xiiVolumetricFogTemporalReprojectionData& data, xiiRGPassContext& context);

  void SetupAtmosphereComposite(xiiAtmosphereCompositeData& data, xiiRGBuilder& builder);
  void ExecuteAtmosphereComposite(const xiiAtmosphereCompositeData& data, xiiRGPassContext& context);


  void SetupForwardOpaque(xiiForwardOpaqueData& data, xiiRGBuilder& builder);
  void ExecuteForwardOpaque(const xiiForwardOpaqueData& data, xiiRGPassContext& context);

  void SetupForwardMasked(xiiForwardMaskedData& data, xiiRGBuilder& builder);
  void ExecuteForwardMasked(const xiiForwardMaskedData& data, xiiRGPassContext& context);

  void SetupHairRendering(xiiHairRenderingData& data, xiiRGBuilder& builder);
  void ExecuteHairRendering(const xiiHairRenderingData& data, xiiRGPassContext& context);

  void SetupWaterRendering(xiiWaterRenderingData& data, xiiRGBuilder& builder);
  void ExecuteWaterRendering(const xiiWaterRenderingData& data, xiiRGPassContext& context);

  void SetupSubsurfaceScattering(xiiSubsurfaceScatteringData& data, xiiRGBuilder& builder);
  void ExecuteSubsurfaceScattering(const xiiSubsurfaceScatteringData& data, xiiRGPassContext& context);

  void SetupEyeShader(xiiEyeShaderData& data, xiiRGBuilder& builder);
  void ExecuteEyeShader(const xiiEyeShaderData& data, xiiRGPassContext& context);


  void SetupGPUParticleSimulate(xiiGPUParticleSimulateData& data, xiiRGBuilder& builder);
  void ExecuteGPUParticleSimulate(const xiiGPUParticleSimulateData& data, xiiRGPassContext& context);

  void SetupDecalUpload(xiiDecalUploadData& data, xiiRGBuilder& builder);
  void ExecuteDecalUpload(const xiiDecalUploadData& data, xiiRGPassContext& context);

  void SetupDecalCullBatch(xiiDecalCullBatchData& data, xiiRGBuilder& builder);
  void ExecuteDecalCullBatch(const xiiDecalCullBatchData& data, xiiRGPassContext& context);

  void SetupProjectedDecalResolve(xiiProjectedDecalResolveData& data, xiiRGBuilder& builder);
  void ExecuteProjectedDecalResolve(const xiiProjectedDecalResolveData& data, xiiRGPassContext& context);

  void SetupMeshDecalDraw(xiiMeshDecalDrawData& data, xiiRGBuilder& builder);
  void ExecuteMeshDecalDraw(const xiiMeshDecalDrawData& data, xiiRGPassContext& context);

  void SetupWeightedBlendedOIT(xiiWeightedBlendedOITData& data, xiiRGBuilder& builder);
  void ExecuteWeightedBlendedOIT(const xiiWeightedBlendedOITData& data, xiiRGPassContext& context);


  void SetupScreenSpaceGlobalIllumination(xiiScreenSpaceGlobalIlluminationData& data, xiiRGBuilder& builder);
  void ExecuteScreenSpaceGlobalIllumination(const xiiScreenSpaceGlobalIlluminationData& data, xiiRGPassContext& context);

  void SetupScreenSpaceRefraction(xiiScreenSpaceRefractionData& data, xiiRGBuilder& builder);
  void ExecuteScreenSpaceRefraction(const xiiScreenSpaceRefractionData& data, xiiRGPassContext& context);

  void SetupPlanarReflections(xiiPlanarReflectionsData& data, xiiRGBuilder& builder);
  void ExecutePlanarReflections(const xiiPlanarReflectionsData& data, xiiRGPassContext& context);


  void SetupLuminanceHistogram(xiiLuminanceHistogramData& data, xiiRGBuilder& builder);
  void ExecuteLuminanceHistogram(const xiiLuminanceHistogramData& data, xiiRGPassContext& context);

  void SetupAutoExposure(xiiAutoExposureData& data, xiiRGBuilder& builder);
  void ExecuteAutoExposure(const xiiAutoExposureData& data, xiiRGPassContext& context);

  void SetupTemporalAntiAliasing(xiiTemporalAntiAliasingData& data, xiiRGBuilder& builder);
  void ExecuteTemporalAntiAliasing(const xiiTemporalAntiAliasingData& data, xiiRGPassContext& context);

  void SetupUpscale(xiiUpscaleData& data, xiiRGBuilder& builder);
  void ExecuteUpscale(const xiiUpscaleData& data, xiiRGPassContext& context);

  void SetupBloom(xiiBloomData& data, xiiRGBuilder& builder);
  void ExecuteBloom(const xiiBloomData& data, xiiRGPassContext& context);

  void SetupColorGrading(xiiColorGradingData& data, xiiRGBuilder& builder);
  void ExecuteColorGrading(const xiiColorGradingData& data, xiiRGPassContext& context);

  void SetupToneMapping(xiiToneMappingData& data, xiiRGBuilder& builder);
  void ExecuteToneMapping(const xiiToneMappingData& data, xiiRGPassContext& context);

  void SetupFinalBlit(xiiFinalBlitData& data, xiiRGBuilder& builder);
  void ExecuteFinalBlit(const xiiFinalBlitData& data, xiiRGPassContext& context);

  /// Lazy-initialise a compute pipeline from a shader path + empty permutation set.
  ///        If the pipeline already exists this is a no-op.
  static xiiSharedPtr<xiiGALComputePipelineState> EnsureComputePipeline(xiiSharedPtr<xiiGALComputePipelineState>& inout_pPipeline, xiiStringView sShaderPath);

private:
  friend class xiiRenderWorldModule;

  xiiWorld* const m_pWorld;

  xiiViewId m_InternalId;

  xiiHashedString m_sName;

  xiiUInt32          m_uiRenderGraphBuilderVersion = 0;
  xiiCamera*         m_pCamera                     = nullptr;
  const xiiCamera*   m_pCullingCamera              = nullptr;
  const xiiCamera*   m_pLodCamera                  = nullptr;
  RenderGraphBuilder m_RenderGraphBuilder;

  mutable xiiUInt32 m_uiLastCameraSettingsModification    = 0;
  mutable xiiUInt32 m_uiLastCameraOrientationModification = 0;
  mutable float     m_fLastViewportAspectRatio            = 1.0f;

  mutable xiiViewData m_Data;

  xiiUniquePtr<xiiRenderGraph> m_pRenderGraph;

  /// Non-owning pointer. The extracted data lifetime is managed by xiiRenderWorldModule.
  xiiExtractedRenderData* m_pExtractedData = nullptr;

  /// Non-owning pointer to the swapchain this view renders into.
  /// Set via SetSwapChain(); may be nullptr for off-screen views.
  const xiiGALSwapChain* m_pSwapChain        = nullptr;
  xiiGALTextureView*     m_pRenderTargetView = nullptr;

  xiiRenderGraphBlackboard    m_Blackboard;
  xiiRenderGraphResourceCache m_ResourceCache;

  struct ViewPassResources
  {
    // GPU timestamp profiler (Duration queries, 3-frame ring)
    xiiRenderGraphTimestampProfiler m_Profiler;

    //  CPU PID state for dynamic resolution
    struct DynamicResolution
    {
      float m_fCurrentScale       = 1.0f;
      float m_fErrorIntegral      = 0.0f;
      float m_fPreviousError      = 0.0f;
      float m_fSmoothedScale      = 1.0f;
      float m_fLastGpuFrameTimeMs = 0.0f; // resolved GPU time from profiler (2 frames ago)
      float m_fRenderScale        = 1.0f; // camera/view scale baseline (1.0 = native).
    } m_DynamicResolution;

    //  Stage 1 - Visibility & Setup
    struct VisibilityPasses
    {
      static constexpr xiiUInt32 s_uiReadbackRingSize = 3U;
      xiiSharedPtr<xiiGALBuffer> m_pOcclusionReadbackRing[s_uiReadbackRingSize];
      xiiUInt32                  m_uiReadbackWriteSlot = 0U;

      xiiSharedPtr<xiiGALBuffer> m_pDrawIndirectArgBuffer; // persistent, resized on demand
      xiiSharedPtr<xiiGALBuffer> m_pInstanceBoundsBuffer;  // StructuredBuffer<InstanceBounds>
      xiiSharedPtr<xiiGALBuffer> m_pInstanceMatrixBuffer;  // StructuredBuffer<float4x3>

      xiiSharedPtr<xiiGALComputePipelineState> m_pFrustumCullPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pLODSelectPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pInstanceUpdatePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pDrawBuildPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pShadowCasterBuildPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pClusterBuildPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pLightListPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pProbeSelectPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pFroxelSetupPipeline;
    } m_VisibilityPasses;

    //  Frame lighting data uploaded once and consumed by clustered, deferred, and forward lighting passes.
    xiiLightingSystem m_LightingSystem;

    //  Stage 2 - Shadows
    struct ShadowPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState>  m_pCascadeSetupPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pLocalShadowAtlasAllocationPipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pShadowDepthPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pRayTracedShadowPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pShadowDenoisePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pContactShadowPipeline;
      xiiSharedPtr<xiiGALTexture>               m_pDirectionalShadowAtlas; // D32F[4] 4096x4096
      xiiSharedPtr<xiiGALTexture>               m_pLocalShadowAtlas;       // D32F 2D 4096x4096
    } m_ShadowPasses;

    //  Stage 3 - Depth & Hi-Z
    struct DepthPasses
    {
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pDepthPrepassPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pHiZBuildPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pHiZOcclusionCullPipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pMotionVectorPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pVelocityDilationPipeline;
    } m_DepthPasses;

    //  Stage 4 - G-Buffer
    struct GBufferPasses
    {
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pGBufferPipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pNormalRoughnessPipeline;
    } m_GBufferPasses;

    //  Stage 5 - Lighting Preparation
    struct LightingPrepPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState> m_pBRDFLutPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pAtmTransmittancePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pAtmMultiScatterPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pSkyIrradiancePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pReflProbeConvPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pFroxelFogInitPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pDDGIProbePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pGTAOPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pGTAODenoisePipeline;
      // Persistent once-generated textures
      xiiSharedPtr<xiiGALTexture> m_pBRDFLut;             // 256x256 R16G16F, generated once
      xiiSharedPtr<xiiGALTexture> m_pAtmTransmittanceLUT; // 256x64 R16G16B16A16F
      xiiSharedPtr<xiiGALTexture> m_pAtmMultiScatterLUT;  // 32x32  R16G16B16A16F
      bool                        m_bBRDFLutGenerated = false;
      bool                        m_bAtmLutsGenerated = false;
    } m_LightingPrepPasses;

    //  Stage 6 - Main Lighting
    struct LightingPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState> m_pDirectLightingPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pIndirectLightingPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pSSRPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pVolumetricIntegratePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pVolumetricTemporalPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pAtmosphereCompositePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pRTGIPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pRTReflectionPipeline;
      xiiSharedPtr<xiiGALTexture>              m_pFroxelHistoryBuffer; // prev-frame froxel
    } m_LightingPasses;

    //  Stage 7 - Forward Passes
    struct ForwardPasses
    {
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pForwardOpaquePipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pForwardMaskedPipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pHairPipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pWaterPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pSSSComputePipeline; // screen-space SSS blur
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pEyePipeline;
    } m_ForwardPasses;

    //  Stage 8 - Transparency & Special
    struct TransparencyPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState>  m_pParticleSimulatePipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pParticleRenderPipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pDecalRenderPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pDecalCullBatchPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pSSDecalClassifyPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pSSDecalResolvePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pMeshDecalResolvePipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pTranslucentPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pOITResolvePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pRTTransparencyPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pSSTranslucencyPipeline;
      // Persistent particle simulation state
      xiiSharedPtr<xiiGALBuffer> m_pParticleStateBuffer;
      xiiUInt32                  m_uiParticleCapacity = 0U;

      xiiSharedPtr<xiiGALTexture> m_pFallbackDecalAlbedoAtlasTexture;
      xiiSharedPtr<xiiGALTexture> m_pFallbackDecalNormalAtlasTexture;
      xiiSharedPtr<xiiGALTexture> m_pFallbackDecalMaterialAtlasTexture;
      xiiSharedPtr<xiiGALTexture> m_pFallbackDecalEmissiveAtlasTexture;
      xiiSharedPtr<xiiGALSampler> m_pFallbackDecalAtlasSampler;
    } m_TransparencyPasses;

    //  Stage 9 - Screen-Space Effects
    struct ScreenSpacePasses
    {
      xiiSharedPtr<xiiGALComputePipelineState> m_pSSGIPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pSSRefractionPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pSSSSPipeline; // screen-space SSS
      xiiSharedPtr<xiiGALComputePipelineState> m_pSSCausticsPipeline;
      xiiSharedPtr<xiiGALTexture>              m_pPlanarReflectionTarget;
    } m_ScreenSpacePasses;

    //  Stage 10 - Temporal Reconstruction
    struct TemporalPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState> m_pLuminanceHistogramPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pAutoExposurePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pTAAPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pUpscalePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pSharpenPipeline;
      xiiSharedPtr<xiiGALTexture>              m_pTAAHistoryBuffer; // prev-frame resolved color
      xiiSharedPtr<xiiGALBuffer>               m_pExposureBuffer;   // persistent float EV100
    } m_TemporalPasses;

    //  Stage 11 - Post-Processing
    struct PostProcessPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState> m_pBloomPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pLensDirtPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pLensFlarePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pChromaticAberrPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pVignettePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pColorGradingPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pToneMappingPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pFilmGrainPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pDepthOfFieldPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pMotionBlurPipeline;
      xiiTexture2DResourceHandle               m_hLensDirtTexture;
    } m_PostProcessPasses;

    //  Stage 12 - Final Output
    struct OutputPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState>  m_pHDRtoSDRPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pFinalResolvePipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pFinalBlitPipeline;
    } m_OutputPasses;

  } m_ViewPassResources;
};

#include <GraphicsCore/Pipeline/Implementation/View_inl.h>
