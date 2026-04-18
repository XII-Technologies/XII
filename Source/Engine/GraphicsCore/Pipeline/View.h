#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Device/SwapChain.h>

#include <GraphicsCore/Declarations.h>
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
class xiiGALComputePipelineState;
class xiiGALGraphicsPipelineState;

struct xiiOcclusionReadbackData;
struct xiiFrustumCullData;
struct xiiLODSelectData;
struct xiiInstanceUpdateData;
struct xiiDrawBuildData;
struct xiiShadowCasterBuildData;
struct xiiClusterBuildData;
struct xiiLightListData;
struct xiiReflectionProbeSelectData;
struct xiiFroxelAllocationData;

struct xiiShadowCascadeSetupData;
struct xiiDirectionalShadowData;
struct xiiSpotShadowData;
struct xiiPointShadowData;
struct xiiRayTracedShadowData;
struct xiiShadowDenoiseData;
struct xiiContactShadowData;

struct xiiDepthPrepassData;
struct xiiHiZPyramidData;
struct xiiHiZOcclusionCullData;

/// \brief Encapsulates a view on the given world through the given camera
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
  /// \brief Use xiiRenderWorldModule::CreateView to create a view.
  xiiView();
  ~xiiView();

public:
  void          SetName(xiiStringView sName);
  xiiStringView GetName() const;

  /// \brief Sets the swapchain that this view will be rendering into.
  void             SetSwapChain(xiiGALSwapChain* pSwapChain);
  xiiGALSwapChain* GetSwapChain() const;

  void             SetCamera(xiiCamera* pCamera);
  xiiCamera*       GetCamera();
  const xiiCamera* GetCamera() const;

  void             SetCullingCamera(const xiiCamera* pCamera);
  const xiiCamera* GetCullingCamera() const;

  void             SetLodCamera(const xiiCamera* pCamera);
  const xiiCamera* GetLodCamera() const;

  xiiEnum<xiiCameraUsageHint> GetCameraUsageHint() const;
  void                        SetCameraUsageHint(xiiEnum<xiiCameraUsageHint> val);

  void                       SetViewRenderMode(xiiEnum<xiiViewRenderMode> value);
  xiiEnum<xiiViewRenderMode> GetViewRenderMode() const;

  void                SetViewport(const xiiRectFloat& viewport);
  const xiiRectFloat& GetViewport() const;

  const xiiViewData& GetData() const;

  bool IsValid() const;

  xiiResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vRayStartPos, xiiVec3& out_vRayDir) const;
  xiiResult ComputeScreenSpacePos(const xiiVec3& vWorldPos, xiiVec3& out_vScreenPosNormalized) const;
  xiiResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vWorldPos) const;
  void      ConvertScreenPixelPosToNormalizedPos(xiiVec3& inout_vPixelPos);
  void      ConvertScreenNormalizedPosToPixelPos(xiiVec3& inout_vNormalizedPos);

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

  /// \brief Returns the per-view GPU timestamp profiler. Used by xiiRenderWorldModule to pass into graph execution.
  xiiRenderGraphTimestampProfiler& GetProfiler() { return m_ViewPassResources.m_Profiler; }

private:
  friend class xiiRenderWorldModule;
  friend class xiiMemoryUtils;

  void SetExtractedRenderData(xiiExtractedRenderData* pExtractedData) { m_pExtractedData = pExtractedData; }

  void UpdateCachedMatrices() const;

  /// \brief Populates the render graph for default (non-custom) views.
  ///        Called by xiiRenderWorldModule::ExecuteRenderGraphs each frame.
  void BuildDefaultRenderGraph(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard);

  // CPU PID dynamic resolution (runs before BeginSetup)
  void RunDynamicResolutionPID(xiiRenderGraphBlackboard& blackboard);


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

  /// \brief Lazy-initialise a compute pipeline from a shader path + empty permutation set.
  ///        If the pipeline already exists this is a no-op.
  static xiiSharedPtr<xiiGALComputePipelineState> EnsureComputePipeline(xiiSharedPtr<xiiGALComputePipelineState>& inout_pPipeline, xiiStringView sShaderPath);

private:
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
  xiiGALSwapChain* m_pSwapChain = nullptr;

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

    //  Stage 2 - Shadows
    struct ShadowPasses
    {
      xiiSharedPtr<xiiGALComputePipelineState>  m_pCascadeSetupPipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pShadowDepthPipeline;
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
      xiiSharedPtr<xiiGALComputePipelineState>  m_pSSDecalClassifyPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pSSDecalResolvePipeline;
      xiiSharedPtr<xiiGALGraphicsPipelineState> m_pTranslucentPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pOITResolvePipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pRTTransparencyPipeline;
      xiiSharedPtr<xiiGALComputePipelineState>  m_pSSTranslucencyPipeline;
      // Persistent particle simulation state
      xiiSharedPtr<xiiGALBuffer> m_pParticleStateBuffer;
      xiiUInt32                  m_uiParticleCapacity = 0U;
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
