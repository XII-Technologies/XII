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
#include <GraphicsCore/Lighting/Atmosphere.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Lighting/LightingManager.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Lighting/SensorRendering.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphProfiler.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/Pipeline/ViewRenderResourceManager.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>

#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineConfiguration.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>

class xiiView;
struct xiiFrustumCullData;
struct xiiLODSelectData;
struct xiiInstanceUpdateData;
struct xiiDrawBuildData;
struct xiiShadowCasterBuildData;
struct xiiLightingDataUploadData;
struct xiiClusterBuildData;
struct xiiLightListClearData;
struct xiiLightListData;
struct xiiLightSamplingData;
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
struct xiiVolumetricFogInitializationData;

struct xiiReSTIRDITemporalData;
struct xiiReSTIRDISpatialData;
struct xiiReSTIRDIVisibilityData;
struct xiiReSTIRDIEmissiveVisibilityData;
struct xiiShadowSamplingConstantsData;
struct xiiDeferredDirectLightingData;
struct xiiDeferredIndirectLightingData;
struct xiiRayTracedGlobalIlluminationData;
struct xiiReSTIRGITemporalData;
struct xiiReSTIRGISpatialData;
struct xiiRayTracedGlobalIlluminationDenoiseData;
struct xiiRayTracedReflectionsData;
struct xiiRayTracedReflectionsDenoiseData;
struct xiiVolumetricLightInjectionData;
struct xiiVolumetricFogIntegrationData;
struct xiiVolumetricFogResolveData;
struct xiiVolumetricFogTemporalReprojectionData;


struct xiiDecalUploadData;
struct xiiDecalCullBatchData;
struct xiiProjectedDecalResolveData;
struct xiiMeshDecalDrawData;


/// Built-in rendering implementation and isolated per-view history, owned by the existing
/// high-level resource manager so GAL state is released before device/allocator shutdown.

class xiiBuiltinRenderPipelineRuntime
{
public:
  explicit xiiBuiltinRenderPipelineRuntime(xiiView& view) : m_View(view) {}
  ~xiiBuiltinRenderPipelineRuntime();
  xiiBuiltinRenderPipelineRuntime*                                       GetSubgraphRuntime(xiiStringView scope);
  void                                                                   ClearSubgraphs();
  xiiResult                                                              PrepareInstanceResources();
  xiiGpuVisibilityContextHandle                                          GetVisibilityContext() const;
  xiiBitflags<xiiRenderExtractionCategory>                               m_ExtractionRequirements;
  xiiBitflags<xiiRenderPipelineFeature>                                  m_RuntimeFeatures;
  void                                                                   BuildPrepareFrame(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  void                                                                   BuildVisibility(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  xiiResult                                                              BuildShadows(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  void                                                                   BuildDepthAndGeometry(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  xiiResult                                                              BuildLightingPreparation(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  void                                                                   BuildLighting(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  void                                                                   BeginFrame(const xiiVirtualShadowMapManager::UploadHandles& shadows);
  void                                                                   InvalidateTemporalHistory();
  void                                                                   BindFrameResources(xiiGALCommandList& commandList) const;
  xiiSharedPtr<xiiGALSampler>                                            m_CommonSamplers[5];
  xiiResult                                                              BuildStage(xiiStringView stage, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  xiiResult                                                              PrepareLightingHistory();
  xiiResult                                                              PrepareShadowResources();
  xiiUniquePtr<xiiRayTracingFrameResources>                              m_RayTracingResources;
  xiiRenderPipelineInstance                                              m_Instance;
  xiiHashTable<xiiString, xiiUniquePtr<xiiBuiltinRenderPipelineRuntime>> m_Subgraphs;
  xiiGpuVisibilityContextHandle                                          m_hInstanceVisibility;
  bool                                                                   m_bSubgraphInstance = false;
  xiiBuiltinRenderPipelineRuntime*                                       m_pServiceOwner     = nullptr;
  xiiDisplayOutputSettings                                               m_DisplayOutputSettings;
  struct Resources
  {
    //  Stage 1 - Visibility & Setup
    struct VisibilityPasses
    {

      xiiSharedPtr<xiiGALBuffer> m_pInstanceBoundsBuffer; // StructuredBuffer<InstanceBounds>
      xiiSharedPtr<xiiGALBuffer> m_pInstanceMatrixBuffer; // StructuredBuffer<float4x3>

      xiiSharedPtr<xiiGALComputePipelineState> m_pFrustumCullPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pLODSelectPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pInstanceUpdatePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pDrawBuildPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pShadowCasterBuildPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pClusterBuildPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pLightListClearPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pLightListPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pLightSamplingPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pProbeSelectPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pFroxelSetupPipeline;
    } m_VisibilityPasses;

    //  Frame lighting data uploaded once and consumed by clustered, deferred, and forward lighting passes.
    xiiLightingContext m_LightingSystem;

    //  Stage 2 - Shadows
    struct ShadowPasses
    {
      xiiSharedPtr<xiiGALGraphicsPipelineState>   m_pShadowDepthPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pRayTracedShadowFallbackPipeline;
      xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRayTracedShadowPipeline;
      xiiSharedPtr<xiiGALBuffer>                  m_pRayTracedShadowShaderBindingTable;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pShadowDenoisePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pContactShadowPipeline;
      xiiSharedPtr<xiiGALTexture>                 m_pDirectionalShadowAtlas;           // D32F[4] 4096x4096
      xiiSharedPtr<xiiGALTextureView>             m_pDirectionalShadowCascadeViews[4]; // Persistent single-slice DSVs; must outlive recorded command lists.
      xiiSharedPtr<xiiGALTexture>                 m_pLocalShadowAtlas;                 // D32F 2D 4096x4096
      xiiSharedPtr<xiiGALTexture>                 m_pRayTracedShadowHistory[2];
      xiiUInt32                                   m_uiActiveCascadeCount                = 0U;
      xiiUInt32                                   m_uiRayTracedShadowShaderRecordStride = 0U;
      xiiUInt32                                   m_uiRayTracedShadowHistoryWriteIndex  = 0U; ///< Rotates after this effect records its persistent output.
      bool                                        m_bRayTracedShadowHistoryValid        = false;
    } m_ShadowPasses;

    //  Stage 3 - Depth & Hi-Z
    struct DepthPasses
    {
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pDepthPrepassPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pHiZBuildPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pHiZOcclusionCullPipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pMotionVectorPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pVelocityDilationPipeline;
      xiiMat4                                   m_PreviousViewProjectionMatrix = xiiMat4::MakeIdentity();
      xiiVec2                                   m_vPreviousJitter              = xiiVec2::MakeZero();
      bool                                      m_bMotionHistoryValid          = false;
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
      xiiAtmosphereLUTHandle                           m_hAtmosphereLUT;
      xiiSharedPtr<xiiGALComputePipelineState>         m_pBRDFLutPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>         m_pAtmTransmittancePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>         m_pAtmMultiScatterPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>         m_pSkyIrradiancePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>         m_pFroxelFogInitPipeline;
      xiiHybridArray<xiiSharedPtr<xiiGALTexture>, 64U> m_ReflectionProbeTextures;
      xiiSharedPtr<xiiGALTexture>                      m_pFallbackReflectionProbeTexture;
    } m_LightingPrepPasses;

    //  Stage 6 - Main Lighting
    struct LightingPasses
    {
      struct CloudShadowState
      {
        xiiVec4 m_vLayerOriginAndInvScale     = xiiVec4::MakeZero();
        xiiVec4 m_vProjectionAxisUAndDetail   = xiiVec4::MakeZero();
        xiiVec4 m_vProjectionAxisVAndCoverage = xiiVec4::MakeZero();
        xiiVec4 m_vLayerNormalAndOpticalDepth = xiiVec4::MakeZero();
        xiiVec4 m_vWindStrengthAndEnabled     = xiiVec4::MakeZero();
      } m_CloudShadowState;

      xiiSharedPtr<xiiGALComputePipelineState>    m_pReSTIRDITemporalPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pReSTIRDISpatialPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pReSTIRDIVisibilityPipeline;
      xiiSharedPtr<xiiGALRayTracingPipelineState> m_pReSTIRDIEmissiveVisibilityPipeline;
      xiiSharedPtr<xiiGALBuffer>                  m_pReSTIRDIEmissiveVisibilityShaderBindingTable;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pDirectLightingPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pIndirectLightingPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pVolumetricLightInjectionPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pVolumetricIntegratePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pVolumetricResolvePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pVolumetricTemporalPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pRTGIFallbackPipeline;
      xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRTGIPipeline;
      xiiSharedPtr<xiiGALBuffer>                  m_pRTGIShaderBindingTable;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pReSTIRGITemporalPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pReSTIRGISpatialPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pRTGITemporalDenoisePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pRTReflectionFallbackPipeline;
      xiiSharedPtr<xiiGALRayTracingPipelineState> m_pRTReflectionPipeline;
      xiiSharedPtr<xiiGALBuffer>                  m_pRTReflectionShaderBindingTable;
      xiiSharedPtr<xiiGALComputePipelineState>    m_pRTReflectionTemporalDenoisePipeline;
      xiiSharedPtr<xiiGALTopLevelAS>              m_pRayTracingScene;
      xiiRenderGraphBufferHandle                  m_hRayTracingSceneDependency;
      xiiRenderGraphBufferHandle                  m_hRayTracingMaterialData;
      xiiRenderGraphBufferHandle                  m_hRayTracingGeometryData;
      xiiRenderGraphBufferHandle                  m_hEmissiveTriangles;
      xiiRenderGraphBufferHandle                  m_hEmissiveLightHeader;
      xiiRenderGraphBufferHandle                  m_hEmissiveTriangleAliases;
      xiiRenderGraphBufferHandle                  m_hEmissiveBlockAliases;
      xiiRenderGraphBufferHandle                  m_hEmissiveAliasHeader;
      xiiUInt32                                   m_uiRTGIShaderRecordStride                       = 0U;
      xiiUInt32                                   m_uiRTReflectionShaderRecordStride               = 0U;
      xiiUInt32                                   m_uiReSTIRDIEmissiveVisibilityShaderRecordStride = 0U;
      xiiSharedPtr<xiiGALTexture>                 m_pRTGIHistory[2];
      xiiUInt32                                   m_uiRTGIHistoryWriteIndex = 0U; ///< Rotates after this effect records its persistent output.
      bool                                        m_bRTGIHistoryValid       = false;
      bool                                        m_bRTGIAvailableThisFrame = false;
      xiiSharedPtr<xiiGALTexture>                 m_pRTGIReservoirSampleHistory[2];
      xiiSharedPtr<xiiGALTexture>                 m_pRTGIReservoirStateHistory[2];
      xiiSharedPtr<xiiGALTexture>                 m_pRTGIReservoirSurfaceHistory[2];
      xiiUInt32                                   m_uiRTGIReservoirHistoryWriteIndex = 0U; ///< Rotates after this effect records its persistent output.
      bool                                        m_bRTGIReservoirHistoryValid       = false;
      xiiSharedPtr<xiiGALTexture>                 m_pRTReflectionHistory[2];
      xiiSharedPtr<xiiGALTexture>                 m_pRTReflectionHitDistanceHistory[2];   ///< R16_FLOAT primary hit distance; zero denotes environment termination.
      xiiSharedPtr<xiiGALTexture>                 m_pRTReflectionMomentsHistory[2];       ///< RG16_FLOAT log-luminance first and second moments.
      xiiSharedPtr<xiiGALTexture>                 m_pRTReflectionIdentityHistory[2];      ///< RG32_UINT exact packed material signature and stable object identity.
      xiiUInt32                                   m_uiRTReflectionHistoryWriteIndex = 0U; ///< Rotates after this effect records its persistent output.
      bool                                        m_bRTReflectionHistoryValid       = false;
      bool                                        m_bRTReflectionAvailableThisFrame = false;
      xiiSharedPtr<xiiGALTexture>                 m_pDirectReservoirHistory[2];              ///< RGBA32_UINT: sample identity, weight, M, content revision, and quantized visibility.
      xiiSharedPtr<xiiGALTexture>                 m_pDirectSurfaceHistory[2];                ///< RGBA16_FLOAT: previous normal encoding, linear depth, and roughness.
      xiiUInt32                                   m_uiDirectReservoirHistoryWriteIndex = 0U; ///< Rotates after this effect records its persistent output.
      bool                                        m_bDirectReservoirHistoryValid       = false;
      xiiSharedPtr<xiiGALTexture>                 m_pVolumetricHistory[2];              ///< Ping-pong integrated scattering history at render resolution.
      xiiUInt32                                   m_uiVolumetricHistoryWriteIndex = 0U; ///< Rotates after this effect records its persistent output.
      bool                                        m_bVolumetricHistoryValid       = false;
      xiiUInt32                                   m_uiFrameIndex                  = 0U;
    } m_LightingPasses;



    // Decal classification and surface resolve resources.
    struct DecalPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState> m_pDecalCullBatchPipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pSSDecalResolvePipeline;
      xiiSharedPtr<xiiGALComputePipelineState> m_pMeshDecalResolvePipeline;

      xiiSharedPtr<xiiGALTexture> m_pFallbackDecalAlbedoAtlasTexture;
      xiiSharedPtr<xiiGALTexture> m_pFallbackDecalNormalAtlasTexture;
      xiiSharedPtr<xiiGALTexture> m_pFallbackDecalMaterialAtlasTexture;
      xiiSharedPtr<xiiGALTexture> m_pFallbackDecalEmissiveAtlasTexture;
      xiiSharedPtr<xiiGALSampler> m_pFallbackDecalAtlasSampler;
    } m_DecalPasses;
  };
  Resources m_Resources;

private:
  friend class xiiBuiltinRenderPipelinePass;
  xiiView& m_View;
  // Render-graph setup callbacks are void. Accumulate pipeline creation failures
  // during one stage so its resource-backed Build() can reject GPU execution.
  xiiResult m_StageSetupResult = XII_SUCCESS;
  xiiResult BuildStageInternal(xiiStringView stage, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);
  struct Frame
  {
    xiiUInt64                                                       uiFrameIndex = 0;
    xiiGeometryResidencyManager::UploadHandles                      geometry;
    xiiMaterialGpuStorage::UploadHandles                            materials;
    xiiGpuVisibilityOutputs                                         mainVisibility;
    xiiVirtualShadowMapManager::UploadHandles                       virtualShadowHandles;
    xiiRayTracingResources::BuildHandles                            rayTracing;
    bool                                                            m_bSharedServicesPrepared = false;
    std::pair<xiiLightListData*, xiiRenderGraphPassHandle>          lightListPass;
    std::pair<xiiLightSamplingData*, xiiRenderGraphPassHandle>      lightSamplingPass;
    std::pair<xiiShadowCascadeSetupData*, xiiRenderGraphPassHandle> shadowCascadePass;
    std::pair<xiiContactShadowData*, xiiRenderGraphPassHandle>      contactShadowPass;
    xiiRenderGraphTextureHandle                                     hSceneDepth;
    xiiGpuSceneGBufferRasterDescription                             gpuRasterDescription;
  } m_Frame;

  void SetupFrustumCull(xiiFrustumCullData& data, xiiRenderGraphBuilder& builder);
  void ExecuteFrustumCull(const xiiFrustumCullData& data, xiiRenderGraphPassContext& context);

  void SetupLODSelect(xiiLODSelectData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLODSelect(const xiiLODSelectData& data, xiiRenderGraphPassContext& context);

  void SetupInstanceUpdate(xiiInstanceUpdateData& data, xiiRenderGraphBuilder& builder);
  void ExecuteInstanceUpdate(const xiiInstanceUpdateData& data, xiiRenderGraphPassContext& context);

  void SetupDrawBuild(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder);
  void SetupCoarseDrawBuild(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder);
  void SetupDrawBuildResources(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder, xiiStringView sCandidateBuffer, xiiStringView sCommandBuffer, xiiStringView sCountBuffer, bool bUploadSource);
  void ExecuteDrawBuild(const xiiDrawBuildData& data, xiiRenderGraphPassContext& context);

  void SetupShadowCasterBuild(xiiShadowCasterBuildData& data, xiiRenderGraphBuilder& builder);
  void ExecuteShadowCasterBuild(const xiiShadowCasterBuildData& data, xiiRenderGraphPassContext& context);

  void SetupLightingDataUpload(xiiLightingDataUploadData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLightingDataUpload(const xiiLightingDataUploadData& data, xiiRenderGraphPassContext& context);

  void SetupClusterBuild(xiiClusterBuildData& data, xiiRenderGraphBuilder& builder);
  void ExecuteClusterBuild(const xiiClusterBuildData& data, xiiRenderGraphPassContext& context);

  void SetupLightListClear(xiiLightListClearData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLightListClear(const xiiLightListClearData& data, xiiRenderGraphPassContext& context);

  void SetupLightListBuild(xiiLightListData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLightListBuild(const xiiLightListData& data, xiiRenderGraphPassContext& context);

  void SetupLightSampling(xiiLightSamplingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteLightSampling(const xiiLightSamplingData& data, xiiRenderGraphPassContext& context);

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


  void SetupVolumetricFogInitialization(xiiVolumetricFogInitializationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVolumetricFogInitialization(const xiiVolumetricFogInitializationData& data, xiiRenderGraphPassContext& context);



  void SetupReSTIRDITemporal(xiiReSTIRDITemporalData& data, xiiRenderGraphBuilder& builder);
  void ExecuteReSTIRDITemporal(const xiiReSTIRDITemporalData& data, xiiRenderGraphPassContext& context);

  void SetupReSTIRDISpatial(xiiReSTIRDISpatialData& data, xiiRenderGraphBuilder& builder);
  void ExecuteReSTIRDISpatial(const xiiReSTIRDISpatialData& data, xiiRenderGraphPassContext& context);

  void SetupReSTIRDIVisibility(xiiReSTIRDIVisibilityData& data, xiiRenderGraphBuilder& builder);
  void ExecuteReSTIRDIVisibility(const xiiReSTIRDIVisibilityData& data, xiiRenderGraphPassContext& context);

  void SetupReSTIRDIEmissiveVisibility(xiiReSTIRDIEmissiveVisibilityData& data, xiiRenderGraphBuilder& builder);
  void ExecuteReSTIRDIEmissiveVisibility(const xiiReSTIRDIEmissiveVisibilityData& data, xiiRenderGraphPassContext& context);

  void SetupShadowSamplingConstants(xiiShadowSamplingConstantsData& data, xiiRenderGraphBuilder& builder);
  void ExecuteShadowSamplingConstants(const xiiShadowSamplingConstantsData& data, xiiRenderGraphPassContext& context);

  void SetupDirectLighting(xiiDeferredDirectLightingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDirectLighting(const xiiDeferredDirectLightingData& data, xiiRenderGraphPassContext& context);

  void SetupIndirectLighting(xiiDeferredIndirectLightingData& data, xiiRenderGraphBuilder& builder);
  void ExecuteIndirectLighting(const xiiDeferredIndirectLightingData& data, xiiRenderGraphPassContext& context);

  void SetupRayTracedGlobalIllumination(xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteRayTracedGlobalIllumination(const xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphPassContext& context);

  void SetupReSTIRGITemporal(xiiReSTIRGITemporalData& data, xiiRenderGraphBuilder& builder);
  void ExecuteReSTIRGITemporal(const xiiReSTIRGITemporalData& data, xiiRenderGraphPassContext& context);

  void SetupReSTIRGISpatial(xiiReSTIRGISpatialData& data, xiiRenderGraphBuilder& builder);
  void ExecuteReSTIRGISpatial(const xiiReSTIRGISpatialData& data, xiiRenderGraphPassContext& context);

  void SetupRayTracedGlobalIlluminationDenoise(xiiRayTracedGlobalIlluminationDenoiseData& data, xiiRenderGraphBuilder& builder);
  void ExecuteRayTracedGlobalIlluminationDenoise(const xiiRayTracedGlobalIlluminationDenoiseData& data, xiiRenderGraphPassContext& context);

  void SetupRayTracedReflections(xiiRayTracedReflectionsData& data, xiiRenderGraphBuilder& builder);
  void ExecuteRayTracedReflections(const xiiRayTracedReflectionsData& data, xiiRenderGraphPassContext& context);

  void SetupRayTracedReflectionsDenoise(xiiRayTracedReflectionsDenoiseData& data, xiiRenderGraphBuilder& builder);
  void ExecuteRayTracedReflectionsDenoise(const xiiRayTracedReflectionsDenoiseData& data, xiiRenderGraphPassContext& context);



  void SetupVolumetricLightInjection(xiiVolumetricLightInjectionData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVolumetricLightInjection(const xiiVolumetricLightInjectionData& data, xiiRenderGraphPassContext& context);

  void SetupVolumetricFogIntegration(xiiVolumetricFogIntegrationData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVolumetricFogIntegration(const xiiVolumetricFogIntegrationData& data, xiiRenderGraphPassContext& context);

  void SetupVolumetricFogResolve(xiiVolumetricFogResolveData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVolumetricFogResolve(const xiiVolumetricFogResolveData& data, xiiRenderGraphPassContext& context);

  void SetupVolumetricFogTemporalReprojection(xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphBuilder& builder);
  void ExecuteVolumetricFogTemporalReprojection(const xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphPassContext& context);



  void SetupDecalUpload(xiiDecalUploadData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDecalUpload(const xiiDecalUploadData& data, xiiRenderGraphPassContext& context);

  void SetupDecalCullBatch(xiiDecalCullBatchData& data, xiiRenderGraphBuilder& builder);
  void ExecuteDecalCullBatch(const xiiDecalCullBatchData& data, xiiRenderGraphPassContext& context);

  void SetupProjectedDecalResolve(xiiProjectedDecalResolveData& data, xiiRenderGraphBuilder& builder);
  void ExecuteProjectedDecalResolve(const xiiProjectedDecalResolveData& data, xiiRenderGraphPassContext& context);

  void SetupMeshDecalDraw(xiiMeshDecalDrawData& data, xiiRenderGraphBuilder& builder);
  void ExecuteMeshDecalDraw(const xiiMeshDecalDrawData& data, xiiRenderGraphPassContext& context);



  /// Lazy-initialise a compute pipeline from a shader path + empty permutation set.
  ///        If the pipeline already exists this is a no-op.
  xiiSharedPtr<xiiGALComputePipelineState>         EnsureComputePipeline(xiiSharedPtr<xiiGALComputePipelineState>& inout_pPipeline, xiiStringView sShaderPath);
  static xiiSharedPtr<xiiGALGraphicsPipelineState> EnsureGraphicsPipeline(xiiSharedPtr<xiiGALGraphicsPipelineState>& inout_pPipeline, xiiStringView sShaderPath, const xiiSharedPtr<xiiGALRenderPass>& pRenderPass, xiiUInt32 uiSubpassIndex);
  bool                                             EnsureRayTracingReSTIRDIEmissiveVisibilityResources();
  bool                                             EnsureRayTracingShadowResources();
  bool                                             EnsureRayTracingGlobalIlluminationResources();
  bool                                             EnsureRayTracingReflectionResources();
};
