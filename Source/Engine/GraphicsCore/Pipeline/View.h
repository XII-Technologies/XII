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
class xiiRenderGraphBuilder;
class xiiRenderGraphPassContext;
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


  void SetupOcclusionReadback(xiiOcclusionReadbackData& data, xiiRenderGraphBuilder& builder);
  void ExecuteOcclusionReadback(const xiiOcclusionReadbackData& data, xiiRenderGraphPassContext& context);

  void SetupFrustumCull(xiiFrustumCullData& data, xiiRenderGraphBuilder& builder);
  void ExecuteFrustumCull(const xiiFrustumCullData& data, xiiRenderGraphPassContext& context);

  void SetupLODSelect(xiiLODSelectData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLODSelect(const xiiLODSelectData& data, xiiRenderGraphPassContext& context);

  void SetupInstanceUpdate(xiiInstanceUpdateData& data, xiiRenderGraphBuilder& builder);
  void ExecuteInstanceUpdate(const xiiInstanceUpdateData& data, xiiRenderGraphPassContext& context);

  void SetupDrawBuild(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDrawBuild(const xiiDrawBuildData& data, xiiRenderGraphPassContext& context);

  void SetupShadowCasterBuild(xiiShadowCasterBuildData& data, xiiRenderGraphBuilder& builder);
  void ExecuteShadowCasterBuild(const xiiShadowCasterBuildData& data, xiiRenderGraphPassContext& context);

  void SetupLightingDataUpload(xiiLightingDataUploadData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLightingDataUpload(const xiiLightingDataUploadData& data, xiiRenderGraphPassContext& context);

  void SetupClusterBuild(xiiClusterBuildData& data, xiiRenderGraphBuilder& builder);
  void ExecuteClusterBuild(const xiiClusterBuildData& data, xiiRenderGraphPassContext& context);

  void SetupLightListBuild(xiiLightListData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLightListBuild(const xiiLightListData& data, xiiRenderGraphPassContext& context);

  void SetupReflectionProbeSelect(xiiReflectionProbeSelectData& data, xiiRenderGraphBuilder& builder);
  void ExecuteReflectionProbeSelect(const xiiReflectionProbeSelectData& data, xiiRenderGraphPassContext& context);

  void SetupFroxelAllocation(xiiFroxelAllocationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteFroxelAllocation(const xiiFroxelAllocationData& data, xiiRenderGraphPassContext& context);


  void SetupShadowCascadeSetup(xiiShadowCascadeSetupData& data, xiiRenderGraphBuilder& builder);
  void ExecuteShadowCascadeSetup(const xiiShadowCascadeSetupData& data, xiiRenderGraphPassContext& context);

  void SetupLocalShadowAtlasAllocation(xiiLocalShadowAtlasAllocationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLocalShadowAtlasAllocation(const xiiLocalShadowAtlasAllocationData& data, xiiRenderGraphPassContext& context);

  void SetupDirectionalShadowData(xiiDirectionalShadowData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDirectionalShadowData(const xiiDirectionalShadowData& data, xiiRenderGraphPassContext& context);

  void SetupSpotShadowData(xiiSpotShadowData& data, xiiRenderGraphBuilder& builder);
  void ExecuteSpotShadowData(const xiiSpotShadowData& data, xiiRenderGraphPassContext& context);

  void SetupPointShadowData(xiiPointShadowData& data, xiiRenderGraphBuilder& builder);
  void ExecutePointShadowData(const xiiPointShadowData& data, xiiRenderGraphPassContext& context);

  void SetupRayTracedShadowData(xiiRayTracedShadowData& data, xiiRenderGraphBuilder& builder);
  void ExecuteRayTracedShadowData(const xiiRayTracedShadowData& data, xiiRenderGraphPassContext& context);

  void SetupShadowDenoiseData(xiiShadowDenoiseData& data, xiiRenderGraphBuilder& builder);
  void ExecuteShadowDenoiseData(const xiiShadowDenoiseData& data, xiiRenderGraphPassContext& context);

  void SetupContactShadowData(xiiContactShadowData& data, xiiRenderGraphBuilder& builder);
  void ExecuteContactShadowData(const xiiContactShadowData& data, xiiRenderGraphPassContext& context);


  void SetupDepthPrepass(xiiDepthPrepassData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDepthPrepass(const xiiDepthPrepassData& data, xiiRenderGraphPassContext& context);

  void SetupHiZPyramid(xiiHiZPyramidData& data, xiiRenderGraphBuilder& builder);
  void ExecuteHiZPyramid(const xiiHiZPyramidData& data, xiiRenderGraphPassContext& context);

  void SetupHiZOcclusionCull(xiiHiZOcclusionCullData& data, xiiRenderGraphBuilder& builder);
  void ExecuteHiZOcclusionCull(const xiiHiZOcclusionCullData& data, xiiRenderGraphPassContext& context);

  void SetupMotionVectors(xiiMotionVectorsData& data, xiiRenderGraphBuilder& builder);
  void ExecuteMotionVectors(const xiiMotionVectorsData& data, xiiRenderGraphPassContext& context);

  void SetupVelocityDilation(xiiVelocityDilationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVelocityDilation(const xiiVelocityDilationData& data, xiiRenderGraphPassContext& context);


  void SetupGBufferBase(xiiGBufferBaseData& data, xiiRenderGraphBuilder& builder);
  void ExecuteGBufferBase(const xiiGBufferBaseData& data, xiiRenderGraphPassContext& context);

  void SetupNormalRoughnessPrepass(xiiNormalRoughnessPrepassData& data, xiiRenderGraphBuilder& builder);
  void ExecuteNormalRoughnessPrepass(const xiiNormalRoughnessPrepassData& data, xiiRenderGraphPassContext& context);


  void SetupBRDFLutGeneration(xiiBRDFLutGenerationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteBRDFLutGeneration(const xiiBRDFLutGenerationData& data, xiiRenderGraphPassContext& context);

  void SetupAtmosphereTransmittance(xiiAtmosphereTransmittanceData& data, xiiRenderGraphBuilder& builder);
  void ExecuteAtmosphereTransmittance(const xiiAtmosphereTransmittanceData& data, xiiRenderGraphPassContext& context);

  void SetupAtmosphereMultiScatter(xiiAtmosphereMultiScatterData& data, xiiRenderGraphBuilder& builder);
  void ExecuteAtmosphereMultiScatter(const xiiAtmosphereMultiScatterData& data, xiiRenderGraphPassContext& context);

  void SetupSkyIrradianceConvolution(xiiSkyIrradianceConvolutionData& data, xiiRenderGraphBuilder& builder);
  void ExecuteSkyIrradianceConvolution(const xiiSkyIrradianceConvolutionData& data, xiiRenderGraphPassContext& context);

  void SetupReflectionProbeConvolution(xiiReflectionProbeConvolutionData& data, xiiRenderGraphBuilder& builder);
  void ExecuteReflectionProbeConvolution(const xiiReflectionProbeConvolutionData& data, xiiRenderGraphPassContext& context);

  void SetupVolumetricFogInitialization(xiiVolumetricFogInitializationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVolumetricFogInitialization(const xiiVolumetricFogInitializationData& data, xiiRenderGraphPassContext& context);

  void SetupDDGIProbeSampling(xiiDDGIProbeSamplingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDDGIProbeSampling(const xiiDDGIProbeSamplingData& data, xiiRenderGraphPassContext& context);

  void SetupGroundTruthAmbientOcclusion(xiiGroundTruthAmbientOcclusionData& data, xiiRenderGraphBuilder& builder);
  void ExecuteGroundTruthAmbientOcclusion(const xiiGroundTruthAmbientOcclusionData& data, xiiRenderGraphPassContext& context);

  void SetupGroundTruthAmbientOcclusionDenoise(xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRenderGraphBuilder& builder);
  void ExecuteGroundTruthAmbientOcclusionDenoise(const xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRenderGraphPassContext& context);


  void SetupDirectLighting(xiiDeferredDirectLightingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDirectLighting(const xiiDeferredDirectLightingData& data, xiiRenderGraphPassContext& context);

  void SetupIndirectLighting(xiiDeferredIndirectLightingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteIndirectLighting(const xiiDeferredIndirectLightingData& data, xiiRenderGraphPassContext& context);

  void SetupRayTracedGlobalIllumination(xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteRayTracedGlobalIllumination(const xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphPassContext& context);

  void SetupRayTracedReflections(xiiRayTracedReflectionsData& data, xiiRenderGraphBuilder& builder);
  void ExecuteRayTracedReflections(const xiiRayTracedReflectionsData& data, xiiRenderGraphPassContext& context);

  void SetupScreenSpaceReflections(xiiScreenSpaceReflectionsData& data, xiiRenderGraphBuilder& builder);
  void ExecuteScreenSpaceReflections(const xiiScreenSpaceReflectionsData& data, xiiRenderGraphPassContext& context);

  void SetupVolumetricFogIntegration(xiiVolumetricFogIntegrationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVolumetricFogIntegration(const xiiVolumetricFogIntegrationData& data, xiiRenderGraphPassContext& context);

  void SetupVolumetricFogTemporalReprojection(xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVolumetricFogTemporalReprojection(const xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphPassContext& context);

  void SetupAtmosphereComposite(xiiAtmosphereCompositeData& data, xiiRenderGraphBuilder& builder);
  void ExecuteAtmosphereComposite(const xiiAtmosphereCompositeData& data, xiiRenderGraphPassContext& context);


  void SetupForwardOpaque(xiiForwardOpaqueData& data, xiiRenderGraphBuilder& builder);
  void ExecuteForwardOpaque(const xiiForwardOpaqueData& data, xiiRenderGraphPassContext& context);

  void SetupForwardMasked(xiiForwardMaskedData& data, xiiRenderGraphBuilder& builder);
  void ExecuteForwardMasked(const xiiForwardMaskedData& data, xiiRenderGraphPassContext& context);

  void SetupHairRendering(xiiHairRenderingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteHairRendering(const xiiHairRenderingData& data, xiiRenderGraphPassContext& context);

  void SetupWaterRendering(xiiWaterRenderingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteWaterRendering(const xiiWaterRenderingData& data, xiiRenderGraphPassContext& context);

  void SetupSubsurfaceScattering(xiiSubsurfaceScatteringData& data, xiiRenderGraphBuilder& builder);
  void ExecuteSubsurfaceScattering(const xiiSubsurfaceScatteringData& data, xiiRenderGraphPassContext& context);

  void SetupEyeShader(xiiEyeShaderData& data, xiiRenderGraphBuilder& builder);
  void ExecuteEyeShader(const xiiEyeShaderData& data, xiiRenderGraphPassContext& context);


  void SetupGPUParticleSimulate(xiiGPUParticleSimulateData& data, xiiRenderGraphBuilder& builder);
  void ExecuteGPUParticleSimulate(const xiiGPUParticleSimulateData& data, xiiRenderGraphPassContext& context);

  void SetupDecalUpload(xiiDecalUploadData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDecalUpload(const xiiDecalUploadData& data, xiiRenderGraphPassContext& context);

  void SetupDecalCullBatch(xiiDecalCullBatchData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDecalCullBatch(const xiiDecalCullBatchData& data, xiiRenderGraphPassContext& context);

  void SetupProjectedDecalResolve(xiiProjectedDecalResolveData& data, xiiRenderGraphBuilder& builder);
  void ExecuteProjectedDecalResolve(const xiiProjectedDecalResolveData& data, xiiRenderGraphPassContext& context);

  void SetupMeshDecalDraw(xiiMeshDecalDrawData& data, xiiRenderGraphBuilder& builder);
  void ExecuteMeshDecalDraw(const xiiMeshDecalDrawData& data, xiiRenderGraphPassContext& context);

  void SetupWeightedBlendedOIT(xiiWeightedBlendedOITData& data, xiiRenderGraphBuilder& builder);
  void ExecuteWeightedBlendedOIT(const xiiWeightedBlendedOITData& data, xiiRenderGraphPassContext& context);


  void SetupScreenSpaceGlobalIllumination(xiiScreenSpaceGlobalIlluminationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteScreenSpaceGlobalIllumination(const xiiScreenSpaceGlobalIlluminationData& data, xiiRenderGraphPassContext& context);

  void SetupScreenSpaceRefraction(xiiScreenSpaceRefractionData& data, xiiRenderGraphBuilder& builder);
  void ExecuteScreenSpaceRefraction(const xiiScreenSpaceRefractionData& data, xiiRenderGraphPassContext& context);

  void SetupPlanarReflections(xiiPlanarReflectionsData& data, xiiRenderGraphBuilder& builder);
  void ExecutePlanarReflections(const xiiPlanarReflectionsData& data, xiiRenderGraphPassContext& context);


  void SetupLuminanceHistogram(xiiLuminanceHistogramData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLuminanceHistogram(const xiiLuminanceHistogramData& data, xiiRenderGraphPassContext& context);

  void SetupAutoExposure(xiiAutoExposureData& data, xiiRenderGraphBuilder& builder);
  void ExecuteAutoExposure(const xiiAutoExposureData& data, xiiRenderGraphPassContext& context);

  void SetupTemporalAntiAliasing(xiiTemporalAntiAliasingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteTemporalAntiAliasing(const xiiTemporalAntiAliasingData& data, xiiRenderGraphPassContext& context);

  void SetupUpscale(xiiUpscaleData& data, xiiRenderGraphBuilder& builder);
  void ExecuteUpscale(const xiiUpscaleData& data, xiiRenderGraphPassContext& context);

  void SetupBloom(xiiBloomData& data, xiiRenderGraphBuilder& builder);
  void ExecuteBloom(const xiiBloomData& data, xiiRenderGraphPassContext& context);

  void SetupColorGrading(xiiColorGradingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteColorGrading(const xiiColorGradingData& data, xiiRenderGraphPassContext& context);

  void SetupToneMapping(xiiToneMappingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteToneMapping(const xiiToneMappingData& data, xiiRenderGraphPassContext& context);

  void SetupFinalBlit(xiiFinalBlitData& data, xiiRenderGraphBuilder& builder);
  void ExecuteFinalBlit(const xiiFinalBlitData& data, xiiRenderGraphPassContext& context);

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
