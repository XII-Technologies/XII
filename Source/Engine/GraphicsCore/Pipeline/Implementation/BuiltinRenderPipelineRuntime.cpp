/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

using namespace xiiBuiltinPipelineUtils;

xiiBuiltinRenderPipelineRuntime::~xiiBuiltinRenderPipelineRuntime()
{
  xiiGpuVisibilityManager::DestroyContext(m_hInstanceVisibility);
}

xiiBuiltinRenderPipelineRuntime* xiiBuiltinRenderPipelineRuntime::GetSubgraphRuntime(xiiStringView scope)
{
  if (scope.IsEmpty())
    return this;
  xiiUniquePtr<xiiBuiltinRenderPipelineRuntime>* runtime = nullptr;
  if (!m_Subgraphs.TryGetValue(scope, runtime))
  {
    auto child                 = XII_DEFAULT_NEW(xiiBuiltinRenderPipelineRuntime, m_View);
    child->m_bSubgraphInstance = true;
    child->m_pServiceOwner     = this;
    m_Subgraphs.Insert(scope, std::move(child));
    m_Subgraphs.TryGetValue(scope, runtime);
  }
  return runtime->Borrow();
}

void xiiBuiltinRenderPipelineRuntime::ClearSubgraphs()
{
  m_Subgraphs.Clear();
}

xiiGpuVisibilityContextHandle xiiBuiltinRenderPipelineRuntime::GetVisibilityContext() const
{
  return m_bSubgraphInstance ? m_hInstanceVisibility : m_View.m_hGpuVisibilityContext;
}

xiiResult xiiBuiltinRenderPipelineRuntime::PrepareInstanceResources()
{

  if (m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::GpuScene) && m_bSubgraphInstance && m_hInstanceVisibility.IsInvalidated())
  {
    m_hInstanceVisibility = xiiGpuVisibilityManager::CreateContext();
    if (m_hInstanceVisibility.IsInvalidated())
      return XII_FAILURE;
  }
  if (m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::Lighting) && !m_Resources.m_LightingSystem.IsInitialized())
    XII_SUCCEED_OR_RETURN(m_Resources.m_LightingSystem.Initialize());
  return XII_SUCCESS;
}

void xiiBuiltinRenderPipelineRuntime::BuildPrepareFrame(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  if (!m_CommonSamplers[0])
  {
    auto                             device = xiiGALDevice::GetDefaultDevice();
    xiiGALSamplerCreationDescription description;
    description.m_MinFilter          = xiiGALFilterType::Point;
    description.m_MagFilter          = xiiGALFilterType::Point;
    description.m_MipFilter          = xiiGALFilterType::Point;
    m_CommonSamplers[0]              = device->CreateSampler(description);
    description.m_MinFilter          = xiiGALFilterType::Linear;
    description.m_MagFilter          = xiiGALFilterType::Linear;
    description.m_MipFilter          = xiiGALFilterType::Linear;
    m_CommonSamplers[1]              = device->CreateSampler(description);
    description.m_AddressU           = xiiGALTextureAddressMode::Wrap;
    description.m_AddressV           = xiiGALTextureAddressMode::Wrap;
    description.m_AddressW           = xiiGALTextureAddressMode::Wrap;
    m_CommonSamplers[2]              = device->CreateSampler(description);
    description.m_MinFilter          = xiiGALFilterType::Anisotropic;
    description.m_MagFilter          = xiiGALFilterType::Anisotropic;
    description.m_MipFilter          = xiiGALFilterType::Anisotropic;
    description.m_uiMaxAnisotropy    = 8;
    m_CommonSamplers[3]              = device->CreateSampler(description);
    description.m_AddressU           = xiiGALTextureAddressMode::Clamp;
    description.m_AddressV           = xiiGALTextureAddressMode::Clamp;
    description.m_AddressW           = xiiGALTextureAddressMode::Clamp;
    description.m_MinFilter          = xiiGALFilterType::ComparisonLinear;
    description.m_MagFilter          = xiiGALFilterType::ComparisonLinear;
    description.m_MipFilter          = xiiGALFilterType::Linear;
    description.m_ComparisonFunction = xiiGALComparisonFunction::GreaterEqual;
    m_CommonSamplers[4]              = device->CreateSampler(description);
  }

  auto& uiFrameIndex   = m_Frame.uiFrameIndex;
  auto& geometry       = m_Frame.geometry;
  auto& materials      = m_Frame.materials;
  auto& mainVisibility = m_Frame.mainVisibility;

  // CPU dynamic resolution PID (pre-graph). View owns scale/resolution state.

  uiFrameIndex = graph.GetFrameIndex();

  // Geometry residency and visibility are process services. The view keeps only generation-
  // checked context handles, so their allocator-backed state is guaranteed to be constructed and
  // destroyed by the GraphicsCore subsystem while the Foundation allocator and GAL device exist.
  auto* serviceOwner = m_pServiceOwner ? m_pServiceOwner : this;
  auto& services     = serviceOwner->m_Frame;
  if (!services.m_bSharedServicesPrepared)
  {
    const xiiString previousScope = graph.GetResourceNamespace();
    graph.SetResourceNamespace({});
    const auto features = m_View.GetRuntimeFeatures();
    if (features.IsSet(xiiRenderPipelineFeature::GpuScene))
    {
      services.geometry  = xiiGeometryResidencyManager::AddUploadPass(graph, uiFrameIndex);
      services.materials = xiiMaterialManager::AddUploadPasses(graph);
    }
    if (features.IsSet(xiiRenderPipelineFeature::RayTracing) && m_View.GetGpuInstanceData())
    {
      if (!serviceOwner->m_RayTracingResources)
        serviceOwner->m_RayTracingResources = XII_DEFAULT_NEW(xiiRayTracingFrameResources);
      const auto selection = m_View.GetGpuInstanceSelection();
      services.rayTracing  = m_View.GetWorld()->GetModule<xiiRenderWorldModule>()->AddRayTracingBuildPass(graph, uiFrameIndex, serviceOwner->m_RayTracingResources.Borrow(), &selection);
    }
    graph.SetResourceNamespace(previousScope);
    services.m_bSharedServicesPrepared = true;
  }
  geometry  = services.geometry;
  materials = services.materials;
  blackboard.Set(xiiRGBlackboardKeys::k_GpuGeometryBaseIndex, geometry.m_uiGeometryBaseIndex);
  blackboard.Set(xiiRGBlackboardKeys::k_GpuMaximumResidentMeshlets, geometry.m_uiMaximumResidentMeshletCount);
  blackboard.Set(xiiRGBlackboardKeys::k_GpuSurfaceMaterialBaseIndex, materials.m_uiSurfaceBaseIndex);

  // Build the primary view's compact meshlet stream from the subsystem-owned scene. Frustum and
  // LOD culling are refined by the previous frame's subsystem-owned Hi-Z history when valid; the
  // depth generated below refreshes that history without introducing a same-frame dependency cycle.
  xiiRenderGraphTextureHandle hPreviousGpuHiZ;
  if (m_View.m_pCamera != nullptr && m_View.GetGpuInstanceData() != nullptr && xiiGpuVisibilityManager::IsValid(GetVisibilityContext()) && geometry.m_hGeometryMetadata.IsValid() && geometry.m_hMeshletMetadata.IsValid())
  {
    const xiiUInt32 uiRenderWidth  = m_View.GetRenderResolutionWidth();
    const xiiUInt32 uiRenderHeight = m_View.GetRenderResolutionHeight();
    const auto&     depthHistory   = m_Resources.m_DepthPasses;
    if (xiiGpuVisibilityManager::PrepareHiZHistory(GetVisibilityContext(), uiRenderWidth, uiRenderHeight).Succeeded())
    {
      if (depthHistory.m_bMotionHistoryValid)
        hPreviousGpuHiZ = xiiGpuVisibilityManager::ImportPreviousHiZ(GetVisibilityContext(), graph, uiFrameIndex);
      else
        xiiGpuVisibilityManager::InvalidateHiZHistory(GetVisibilityContext());
    }

    const xiiMat4 viewProjection = m_View.GetViewProjectionMatrix(xiiCameraEye::Left);
    xiiFrustum    viewFrustum;
    m_View.ComputeCullingFrustum(viewFrustum);
    // The raster camera still governs Hi-Z, LOD and cone-facing decisions; the explicit culling
    // camera controls frustum rejection, just as it does in the indexed visibility path.
    xiiGpuVisibilityView visibilityView            = xiiGpuVisibilitySystem::BuildView(viewProjection, viewFrustum, m_View.m_pCamera->GetPosition(), uiRenderWidth, uiRenderHeight, xiiGpuVisibilityManager::GetHiZMipLevelCount(GetVisibilityContext()), m_View.GetGpuInstanceData()->GetInstanceCount());
    visibilityView.m_OcclusionViewProjectionMatrix = depthHistory.m_bMotionHistoryValid ? depthHistory.m_PreviousViewProjectionMatrix : viewProjection;
    visibilityView.m_uiRequiredFlags               = xiiGpuInstanceFlags::Enabled;
    visibilityView.m_uiExcludedFlags               = xiiGpuInstanceFlags::Transparent;

    xiiGpuVisibilityPassDescription visibilityDescription;
    visibilityDescription.m_sName          = "Default View Primary";
    visibilityDescription.m_bUseCandidates = true;
    visibilityDescription.m_Candidates     = m_View.GetGpuInstanceSelection();
    visibilityDescription.m_Purpose        = xiiGpuVisibilityPurpose::MainView;
    visibilityDescription.m_bAsyncCompute  = true;
    mainVisibility                         = xiiGpuVisibilityManager::AddPasses(GetVisibilityContext(), graph, uiFrameIndex, *m_View.GetGpuInstanceData(), visibilityView, geometry, visibilityDescription, hPreviousGpuHiZ);
  }

  const xiiRayTracingResources::BuildHandles rayTracingScene = m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::RayTracing) ? services.rayTracing : xiiRayTracingResources::BuildHandles{};
  m_Resources.m_LightingPasses.m_pRayTracingScene            = rayTracingScene.m_pTopLevelAS;
  m_Resources.m_LightingPasses.m_hRayTracingSceneDependency  = rayTracingScene.m_hSceneDependency;
  m_Resources.m_LightingPasses.m_hRayTracingMaterialData     = rayTracingScene.m_hMaterialData;
  m_Resources.m_LightingPasses.m_hRayTracingGeometryData     = rayTracingScene.m_hGeometryData;

  if (m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::Lighting) && m_View.m_pExtractedData != nullptr)
  {
    // Lighting's shader sampling token is intentionally 32-bit; residency and capture lifetimes
    // above retain the complete render-graph frame identity.
    m_Resources.m_LightingSystem.BuildFrameData(m_View, *m_View.m_pExtractedData, static_cast<xiiUInt32>(uiFrameIndex));
  }

  xiiEmissiveLightManager::BuildDescription emissiveBuildDescription;
  emissiveBuildDescription.m_Visibility                      = mainVisibility;
  emissiveBuildDescription.m_Geometry                        = geometry;
  emissiveBuildDescription.m_Materials                       = materials;
  emissiveBuildDescription.m_uiInstanceCount                 = m_View.GetGpuInstanceData() != nullptr ? m_View.GetGpuInstanceData()->GetInstanceCount() : 0U;
  emissiveBuildDescription.m_uiVertexStride                  = sizeof(xiiMeshPackedVertex);
  emissiveBuildDescription.m_uiTexCoordOffset                = static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vTexCoord0));
  const xiiEmissiveLightManager::BuildOutputs emissiveLights = m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::Lighting) && m_RuntimeFeatures.IsSet(xiiRenderPipelineFeature::GpuScene) ? xiiEmissiveLightManager::AddBuildPass(graph, uiFrameIndex, emissiveBuildDescription) : xiiEmissiveLightManager::BuildOutputs{};
  m_Resources.m_LightingPasses.m_hEmissiveTriangles          = emissiveLights.m_hTriangles;
  m_Resources.m_LightingPasses.m_hEmissiveLightHeader        = emissiveLights.m_hHeader;
  m_Resources.m_LightingPasses.m_hEmissiveTriangleAliases    = emissiveLights.m_hTriangleAliases;
  m_Resources.m_LightingPasses.m_hEmissiveBlockAliases       = emissiveLights.m_hBlockAliases;
  m_Resources.m_LightingPasses.m_hEmissiveAliasHeader        = emissiveLights.m_hAliasHeader;

  xiiAtmosphereLUTHandle& hAtmosphereLUT = m_Resources.m_LightingPrepPasses.m_hAtmosphereLUT;
  hAtmosphereLUT                         = m_ExtractionRequirements.IsSet(xiiRenderExtractionCategory::Atmosphere) ? xiiAtmosphereManager::GetDefaultLUTHandle() : xiiAtmosphereLUTHandle{};
  if (m_ExtractionRequirements.IsSet(xiiRenderExtractionCategory::Atmosphere) && m_View.m_pExtractedData != nullptr)
  {
    if (const xiiSkyAtmosphereRenderData* pAtmosphere = xiiSelectSkyAtmosphere(m_View.m_pExtractedData->GetAllRenderData()))
    {
      if (xiiAtmosphereManager::AcquireLUTs(pAtmosphere->m_AtmosphereSettings, hAtmosphereLUT).Failed())
      {
        hAtmosphereLUT = xiiAtmosphereManager::GetDefaultLUTHandle();
      }
    }
  }

  auto& cloudState = m_Resources.m_LightingPasses.m_CloudShadowState;
  cloudState       = {};
  if (m_ExtractionRequirements.IsSet(xiiRenderExtractionCategory::Atmosphere) && m_View.m_pExtractedData != nullptr)
  {
    if (const xiiVolumetricCloudRenderData* pCloud = SelectVolumetricCloudLayer(m_View.m_pExtractedData->GetAllRenderData()))
    {
      const xiiVolumetricCloudSettings& settings  = pCloud->m_Settings;
      const float                       fInvScale = 1.0f / xiiMath::Max(settings.m_fShadowScaleMeters, 1.0f);
      cloudState.m_vLayerOriginAndInvScale        = xiiVec4(pCloud->m_vLayerOrigin.x, pCloud->m_vLayerOrigin.y, pCloud->m_vLayerOrigin.z, fInvScale);
      cloudState.m_vProjectionAxisUAndDetail      = xiiVec4(pCloud->m_vProjectionAxisU.x, pCloud->m_vProjectionAxisU.y, pCloud->m_vProjectionAxisU.z, xiiMath::Max(settings.m_fDetailScale, 1.0f));
      cloudState.m_vProjectionAxisVAndCoverage    = xiiVec4(pCloud->m_vProjectionAxisV.x, pCloud->m_vProjectionAxisV.y, pCloud->m_vProjectionAxisV.z, xiiMath::Saturate(settings.m_fCoverage));
      cloudState.m_vLayerNormalAndOpticalDepth    = xiiVec4(pCloud->m_vLayerNormal.x, pCloud->m_vLayerNormal.y, pCloud->m_vLayerNormal.z, xiiMath::Max(settings.m_fOpticalDepth, 0.0f));
      cloudState.m_vWindStrengthAndEnabled        = xiiVec4(settings.m_vWindVelocityMetersPerSecond.x, settings.m_vWindVelocityMetersPerSecond.y, xiiMath::Saturate(settings.m_fShadowStrength), settings.m_bCastShadows ? 1.0f : 0.0f);
    }
  }
  m_Resources.m_LightingSystem.WriteBlackboard(blackboard);
  m_Resources.m_LightingPasses.m_uiFrameIndex = static_cast<xiiUInt32>(uiFrameIndex);
}

void xiiBuiltinRenderPipelineRuntime::BuildVisibility(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  auto& lightListPass     = m_Frame.lightListPass;
  auto& lightSamplingPass = m_Frame.lightSamplingPass;
  // Each stage adds its passes to the graph. Dependency ordering is handled by the render graph compiler (topological sort + culling).

  graph.AddPass<xiiLightingDataUploadData>("LightingDataUpload", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupLightingDataUpload, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteLightingDataUpload, this));

  // Visibility preparation passes, which produce data consumed by the main render passes in later stages.
  graph.AddPass<xiiOcclusionReadbackData>("GpuOcclusionReadback", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupOcclusionReadback, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteOcclusionReadback, this));
  graph.AddPass<xiiFrustumCullData>("FrustumCulling", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupFrustumCull, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteFrustumCull, this));
  graph.AddPass<xiiLODSelectData>("LODSelection", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupLODSelect, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteLODSelect, this));
  graph.AddPass<xiiInstanceUpdateData>("InstanceUpdate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupInstanceUpdate, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteInstanceUpdate, this));
  graph.AddPass<xiiDrawBuildData>("CoarseDrawCommandBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupCoarseDrawBuild, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteDrawBuild, this));
  graph.AddPass<xiiClusterBuildData>("ClusterGridBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupClusterBuild, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteClusterBuild, this));
  graph.AddPass<xiiLightListClearData>("LightListClear", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupLightListClear, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteLightListClear, this));
  lightListPass     = graph.AddPass<xiiLightListData>("LightListBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupLightListBuild, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteLightListBuild, this));
  lightSamplingPass = graph.AddPass<xiiLightSamplingData>("LightSamplingBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupLightSampling, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteLightSampling, this));
  graph.AddPass<xiiReflectionProbeSelectData>("ReflectionProbeSelection", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupReflectionProbeSelect, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteReflectionProbeSelect, this));
  graph.AddPass<xiiFroxelAllocationData>("VolumetricGridAllocation", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupFroxelAllocation, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteFroxelAllocation, this));
}

xiiResult xiiBuiltinRenderPipelineRuntime::BuildShadows(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  auto& uiFrameIndex         = m_Frame.uiFrameIndex;
  auto& geometry             = m_Frame.geometry;
  auto& materials            = m_Frame.materials;
  auto& virtualShadowHandles = m_Frame.virtualShadowHandles;
  auto& shadowCascadePass    = m_Frame.shadowCascadePass;
  // Shadow preparation passes, which produce data consumed by the main shadow pass in later stages.
  shadowCascadePass = graph.AddPass<xiiShadowCascadeSetupData>("ShadowCascadeSetup", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupShadowCascadeSetup, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteShadowCascadeSetup, this));

  XII_SUCCEED_OR_RETURN(PrepareShadowResources());

  // Feed the subsystem-owned GPU scene through the same meshlet visibility path used by samples.
  // The farthest directional cascade encloses the useful caster range for every nearer virtual
  // cascade and therefore supplies one conservative caster stream for all dirty VSM pages.
  if (m_View.m_pCamera != nullptr && shadowCascadePass.first->m_uiActiveCascades > 0U && m_View.GetGpuInstanceData() != nullptr && xiiGpuVisibilityManager::IsValid(GetVisibilityContext()) && geometry.m_hGeometryMetadata.IsValid() && geometry.m_hMeshletMetadata.IsValid())
  {
    const xiiDirectionalLightRenderData* pMainDirectional =
      m_View.m_pExtractedData != nullptr ? SelectMainDirectionalLight(m_View.m_pExtractedData->GetAllRenderData()) : nullptr;
    if (pMainDirectional != nullptr && pMainDirectional->m_bCastShadows)
    {
      const xiiUInt32  uiLastCascade        = shadowCascadePass.first->m_uiActiveCascades - 1U;
      const xiiMat4&   shadowViewProjection = shadowCascadePass.first->m_CascadeViewProjection[uiLastCascade];
      const xiiFrustum shadowFrustum        = xiiFrustum::MakeFromMVP(shadowViewProjection, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);

      xiiGpuVisibilityView shadowView = xiiGpuVisibilitySystem::BuildView(shadowViewProjection, shadowFrustum, m_View.m_pCamera->GetPosition(), k_uiDirectionalShadowAtlasWidth, k_uiDirectionalShadowAtlasHeight, 0U, m_View.GetGpuInstanceData()->GetInstanceCount());
      shadowView.m_uiRequiredFlags    = (xiiGpuInstanceFlags::Enabled | xiiGpuInstanceFlags::CastShadows).GetValue();

      xiiGpuVisibilityPassDescription visibilityDescription;
      visibilityDescription.m_sName                  = "Default View Directional Shadow";
      visibilityDescription.m_bUseCandidates         = true;
      visibilityDescription.m_Candidates             = m_View.GetGpuInstanceSelection();
      visibilityDescription.m_Purpose                = xiiGpuVisibilityPurpose::Shadow;
      visibilityDescription.m_bAsyncCompute          = true;
      const xiiGpuVisibilityOutputs shadowVisibility = xiiGpuVisibilityManager::AddPasses(GetVisibilityContext(), graph, uiFrameIndex, *m_View.GetGpuInstanceData(), shadowView, geometry, visibilityDescription);

      const xiiUInt32 uiDirectionalLightId = static_cast<xiiUInt32>(pMainDirectional->m_uiSortingKey) & 0x00FFFFFFU;
      XII_IGNORE_UNUSED(xiiVirtualShadowMapManager::AddRasterPasses(graph, virtualShadowHandles, shadowVisibility, geometry, materials, xiiArrayPtr<const xiiMat4>(shadowCascadePass.first->m_CascadeViewProjection, shadowCascadePass.first->m_uiActiveCascades), uiDirectionalLightId, sizeof(xiiMeshPackedVertex), static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vTexCoord0)), xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(GetVisibilityContext()), xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(GetVisibilityContext())));
    }
  }

  graph.AddPass<xiiShadowCasterBuildData>("ShadowCasterListBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupShadowCasterBuild, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteShadowCasterBuild, this));
  graph.AddPass<xiiLocalShadowAtlasAllocationData>("LocalShadowAtlasUpload", xiiGALCommandQueueFlags::Transfer, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupLocalShadowAtlasAllocation, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteLocalShadowAtlasAllocation, this));
  graph.AddPass<xiiDirectionalShadowData>("DirectionalShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupDirectionalShadowData, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteDirectionalShadowData, this));

  // Local-light faces share one bounded visibility scratch set. Each face produces a new graph
  // version before rasterizing its atlas tile, so async culling can overlap unrelated graphics
  // while scratch reuse and partial atlas writes remain deterministically ordered.
  bool bGpuLocalShadows = false;
  if (xiiGpuShadowRasterManager::IsSupported() && m_View.GetGpuInstanceData() != nullptr && xiiGpuVisibilityManager::IsValid(GetVisibilityContext()) && geometry.m_hGeometryMetadata.IsValid() && geometry.m_hMeshletMetadata.IsValid() && materials.m_hSurfaceData.IsValid())
  {
    const xiiUInt32 uiAtlasSize = m_Resources.m_LightingSystem.GetSettings().m_uiLocalShadowAtlasSize;

    auto atlasClear = graph.AddPass<xiiLocalShadowAtlasClearData>("GPU Local Shadow Atlas Clear", xiiGALCommandQueueFlags::Graphics, [pAtlas = m_Resources.m_ShadowPasses.m_pLocalShadowAtlas](xiiLocalShadowAtlasClearData& data, xiiRenderGraphBuilder& builder) {
        data.m_hLocalShadowAtlas = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, pAtlas, pAtlas->GetResourceState()), xiiGALResourceStateFlags::DepthWrite);
        builder.SetPassAllowMerge(false); }, [](const xiiLocalShadowAtlasClearData& data, xiiRenderGraphPassContext& context) { ClearDepthPreservingState(context.GetCommandList(), context.GetTexture(data.m_hLocalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0.0f, 0U); });

    XII_IGNORE_UNUSED(xiiGpuShadowRasterManager::AddLocalLightPasses(graph, uiFrameIndex, GetVisibilityContext(), *m_View.GetGpuInstanceData(), atlasClear.first->m_hLocalShadowAtlas, m_Resources.m_LightingSystem.GetLocalShadowData(), uiAtlasSize, geometry, materials, sizeof(xiiMeshPackedVertex), static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vTexCoord0)), m_View.GetGpuInstanceSelection()));
    bGpuLocalShadows = true;
  }

  if (!bGpuLocalShadows)
  {
    graph.AddPass<xiiSpotShadowData>("SpotShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupSpotShadowData, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteSpotShadowData, this));
    graph.AddPass<xiiPointShadowData>("PointShadowData", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupPointShadowData, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecutePointShadowData, this));
  }
  return XII_SUCCESS;
}

void xiiBuiltinRenderPipelineRuntime::BuildDepthAndGeometry(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  auto& uiFrameIndex         = m_Frame.uiFrameIndex;
  auto& geometry             = m_Frame.geometry;
  auto& materials            = m_Frame.materials;
  auto& mainVisibility       = m_Frame.mainVisibility;
  auto& shadowCascadePass    = m_Frame.shadowCascadePass;
  auto& hSceneDepth          = m_Frame.hSceneDepth;
  auto& gpuRasterDescription = m_Frame.gpuRasterDescription;
  // Depth and motion prepasses, which produce depth and motion data consumed by later passes.
  // Mesh-shader capable devices rasterize the exact compact visibility stream. The existing
  // indexed pass remains the compatibility path for devices without mesh shader support.
  if (xiiGpuSceneRasterManager::IsSupported() && mainVisibility.m_hIndirectCommands.IsValid())
  {
    xiiGpuSceneDepthRasterDescription depthDescription;
    depthDescription.m_ViewProjectionMatrix      = m_View.GetViewProjectionMatrix(xiiCameraEye::Left);
    depthDescription.m_uiWidth                   = m_View.GetRenderResolutionWidth();
    depthDescription.m_uiHeight                  = m_View.GetRenderResolutionHeight();
    depthDescription.m_uiVertexStride            = sizeof(xiiMeshPackedVertex);
    depthDescription.m_uiTexCoordOffset          = static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vTexCoord0));
    depthDescription.m_uiMeshDispatchGroupCountX = xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(GetVisibilityContext());
    depthDescription.m_uiMeshDispatchGroupCountY = xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(GetVisibilityContext());
    hSceneDepth                                  = xiiGpuSceneRasterManager::AddDepthPrepass(graph, "GPU Scene Depth Prepass", mainVisibility, geometry, materials, depthDescription);
  }
  if (!hSceneDepth.IsValid())
  {
    auto depthPrepass = graph.AddPass<xiiDepthPrepassData>("DepthPrepass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupDepthPrepass, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteDepthPrepass, this));
    hSceneDepth       = depthPrepass.first->m_hSceneDepth;
  }
  xiiGpuVisibilityManager::AddHiZBuildPass(GetVisibilityContext(), graph, uiFrameIndex, hSceneDepth, true);
  if (m_View.m_pExtractedData != nullptr)
  {
    const xiiDirectionalLightRenderData* pMainDirectional = SelectMainDirectionalLight(m_View.m_pExtractedData->GetAllRenderData());
    if (pMainDirectional != nullptr && pMainDirectional->m_bCastShadows && hSceneDepth.IsValid())
    {
      xiiVirtualShadowMapManager::AddFeedbackPasses(graph, hSceneDepth, shadowCascadePass.first->m_hCascadeMatrices, m_View.GetRenderResolutionWidth(), m_View.GetRenderResolutionHeight(), m_View.GetInverseViewProjectionMatrix(xiiCameraEye::Left), m_View.m_pCamera != nullptr ? m_View.m_pCamera->GetNearPlane() : 0.1f, static_cast<xiiUInt32>(pMainDirectional->m_uiSortingKey), uiFrameIndex);
    }
  }
  graph.AddPass<xiiHiZPyramidData>("HiZPyramid", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupHiZPyramid, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteHiZPyramid, this));
  graph.AddPass<xiiHiZOcclusionCullData>("HiZOcclusionCull", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupHiZOcclusionCull, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteHiZOcclusionCull, this));
  graph.AddPass<xiiDrawBuildData>("DrawCommandBuild", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupDrawBuild, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteDrawBuild, this));
  auto motionVectors = graph.AddPass<xiiMotionVectorsData>("MotionVectors", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupMotionVectors, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteMotionVectors, this));

  // Mesh-shader capable devices shade the compact visibility stream directly from canonical
  // surface records. Extra MRTs replace duplicate normal/roughness and object-velocity draws.
  const auto& motionHistory                           = m_Resources.m_DepthPasses;
  gpuRasterDescription.m_ViewProjectionMatrix         = m_View.GetViewProjectionMatrix(xiiCameraEye::Left);
  gpuRasterDescription.m_PreviousViewProjectionMatrix = motionHistory.m_bMotionHistoryValid ? motionHistory.m_PreviousViewProjectionMatrix : gpuRasterDescription.m_ViewProjectionMatrix;
  gpuRasterDescription.m_uiWidth                      = m_View.GetRenderResolutionWidth();
  gpuRasterDescription.m_uiHeight                     = m_View.GetRenderResolutionHeight();
  gpuRasterDescription.m_uiVertexStride               = sizeof(xiiMeshPackedVertex);
  gpuRasterDescription.m_uiNormalOffset               = static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vNormal));
  gpuRasterDescription.m_uiTangentOffset              = static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vTangent));
  gpuRasterDescription.m_uiTexCoordOffset             = static_cast<xiiUInt32>(offsetof(xiiMeshPackedVertex, m_vTexCoord0));
  gpuRasterDescription.m_uiMeshDispatchGroupCountX    = xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(GetVisibilityContext());
  gpuRasterDescription.m_uiMeshDispatchGroupCountY    = xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(GetVisibilityContext());

  xiiGpuSceneGBufferOutputs gpuGBuffer;
  if (xiiGpuSceneRasterManager::IsSupported() && hSceneDepth.IsValid() && motionVectors.first->m_hVelocityBuffer.IsValid() && mainVisibility.m_hIndirectCommands.IsValid() && materials.m_hSurfaceData.IsValid())
  {
    gpuRasterDescription.m_vCurrentJitter  = motionVectors.first->m_vCurrentJitter;
    gpuRasterDescription.m_vPreviousJitter = motionHistory.m_bMotionHistoryValid ? motionHistory.m_vPreviousJitter : gpuRasterDescription.m_vCurrentJitter;
    gpuGBuffer                             = xiiGpuSceneRasterManager::AddGBufferPass(graph, "GPU Scene GBuffer", hSceneDepth, motionVectors.first->m_hVelocityBuffer, mainVisibility, geometry, materials, gpuRasterDescription);
  }
  if (!gpuGBuffer.IsValid())
  {
    graph.AddPass<xiiGBufferBaseData>("GBufferBase", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupGBufferBase, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteGBufferBase, this));
    graph.AddPass<xiiNormalRoughnessPrepassData>("NormalRoughnessPrepass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupNormalRoughnessPrepass, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteNormalRoughnessPrepass, this));
  }
  graph.AddPass<xiiVelocityDilationData>("VelocityDilation", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupVelocityDilation, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteVelocityDilation, this));

  // Decals update the G-Buffer before any lighting or screen-space shading consumes it.
  graph.AddPass<xiiDecalUploadData>("DecalUpload", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupDecalUpload, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteDecalUpload, this));
  graph.AddPass<xiiDecalCullBatchData>("DecalCullBatch", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupDecalCullBatch, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteDecalCullBatch, this));
  graph.AddPass<xiiProjectedDecalResolveData>("ProjectedDecalResolve", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupProjectedDecalResolve, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteProjectedDecalResolve, this));
  graph.AddPass<xiiMeshDecalDrawData>("MeshDecalDraw", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupMeshDecalDraw, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteMeshDecalDraw, this));
  m_Frame.contactShadowPass = graph.AddPass<xiiContactShadowData>("ContactShadow", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupContactShadowData, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteContactShadowData, this));
}

xiiResult xiiBuiltinRenderPipelineRuntime::BuildLightingPreparation(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // Lighting preparation passes, which generate lookup textures and lighting auxiliaries.
  graph.AddPass<xiiBRDFLutGenerationData>("BRDFLUTGenerate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupBRDFLutGeneration, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteBRDFLutGeneration, this));
  graph.AddPass<xiiAtmosphereTransmittanceData>("AtmosphereTransmittanceLUT", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupAtmosphereTransmittance, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteAtmosphereTransmittance, this));
  graph.AddPass<xiiAtmosphereMultiScatterData>("AtmosphereMultiScatterLUT", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupAtmosphereMultiScatter, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteAtmosphereMultiScatter, this));
  graph.AddPass<xiiSkyIrradianceConvolutionData>("SkyIrradianceConvolution", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupSkyIrradianceConvolution, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteSkyIrradianceConvolution, this));
  graph.AddPass<xiiVolumetricFogInitializationData>("VolumetricFogInitialization", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupVolumetricFogInitialization, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricFogInitialization, this));
  auto* lighting = m_Resources.m_LightingSystem.BorrowSystem();
  if (m_View.GetCamera() == nullptr)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(lighting->GetDDGIContext().BeginFrame(m_View.GetCamera()->GetPosition(), graph.GetFrameIndex()));
  XII_SUCCEED_OR_RETURN(lighting->GetSparseVoxelRadianceContext().BeginFrame(m_View.GetCamera()->GetPosition(), graph.GetFrameIndex()));
  const auto sparse = lighting->GetSparseVoxelRadianceContext().AddUpdatePass(graph, lighting);
  if (!sparse.m_hRadiancePool.IsValid() || !sparse.m_hPageTable.IsValid() || !sparse.m_hLevelData.IsValid() || !sparse.m_hConstants.IsValid())
    return XII_FAILURE;
  graph.AddPass<xiiSparseVoxelRadianceGatherData>("SparseVoxelRadianceGather", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupSparseVoxelRadianceGather, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteSparseVoxelRadianceGather, this));
  const auto ddgi = lighting->GetDDGIContext().AddUpdatePass(graph, lighting);
  if (!ddgi.m_hIrradianceAtlas.IsValid() || !ddgi.m_hDistanceAtlas.IsValid() || !ddgi.m_hProbeStates.IsValid() || !ddgi.m_hProbeConstants.IsValid())
    return XII_FAILURE;
  graph.AddPass<xiiDDGIProbeSamplingData>("DDGIProbeSampling", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupDDGIProbeSampling, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteDDGIProbeSampling, this));
  graph.AddPass<xiiGroundTruthAmbientOcclusionData>("GroundTruthAmbientOcclusion", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupGroundTruthAmbientOcclusion, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteGroundTruthAmbientOcclusion, this));
  return XII_SUCCESS;
}

void xiiBuiltinRenderPipelineRuntime::BuildLighting(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  auto& geometry             = m_Frame.geometry;
  auto& materials            = m_Frame.materials;
  auto& mainVisibility       = m_Frame.mainVisibility;
  auto& lightListPass        = m_Frame.lightListPass;
  auto& lightSamplingPass    = m_Frame.lightSamplingPass;
  auto& contactShadowPass    = m_Frame.contactShadowPass;
  auto& hSceneDepth          = m_Frame.hSceneDepth;
  auto& gpuRasterDescription = m_Frame.gpuRasterDescription;
  // Main lighting passes, which produce direct and indirect lighting results.
  graph.AddPass<xiiShadowSamplingConstantsData>("ShadowSamplingConstants", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupShadowSamplingConstants, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteShadowSamplingConstants, this));
  graph.AddPass<xiiReSTIRDITemporalData>("ReSTIRDITemporal", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupReSTIRDITemporal, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRDITemporal, this));
  graph.AddPass<xiiReSTIRDISpatialData>("ReSTIRDISpatial", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupReSTIRDISpatial, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRDISpatial, this));
  graph.AddPass<xiiReSTIRDIVisibilityData>("ReSTIRDIVisibility", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupReSTIRDIVisibility, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRDIVisibility, this));
  auto restirDIEmissiveVisibilityPass = graph.AddPass<xiiReSTIRDIEmissiveVisibilityData>("ReSTIRDIEmissiveVisibility", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupReSTIRDIEmissiveVisibility, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRDIEmissiveVisibility, this));
  graph.AddPass<xiiRayTracedShadowData>("RayTracedShadowData", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupRayTracedShadowData, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedShadowData, this));
  auto shadowDenoisePass           = graph.AddPass<xiiShadowDenoiseData>("ShadowDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupShadowDenoiseData, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteShadowDenoiseData, this));
  auto ambientOcclusionDenoisePass = graph.AddPass<xiiGroundTruthAmbientOcclusionDenoiseData>("GroundTruthAmbientOcclusionDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupGroundTruthAmbientOcclusionDenoise, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteGroundTruthAmbientOcclusionDenoise, this));
  bool bForwardPlusDirectLighting  = false;
  if (m_View.m_Data.m_RenderPath == xiiRenderPath::ForwardPlus && xiiGpuSceneRasterManager::IsForwardPlusSupported())
  {
    const xiiRenderGraphTextureHandle hAmbientOcclusion = ambientOcclusionDenoisePass.first->m_hStableAmbientOcclusion;
    const xiiRenderGraphTextureHandle hRayTracedShadow  = shadowDenoisePass.first->m_hRTFinalShadowMask;
    const xiiRenderGraphTextureHandle hContactShadow    = contactShadowPass.first->m_hContactShadow;
    const xiiRenderGraphTextureHandle hDirectReservoir  = restirDIEmissiveVisibilityPass.first->m_hReservoir;
    const xiiRenderGraphBufferHandle  hLightGrid        = lightListPass.first->m_hLightGridBuffer;
    const xiiRenderGraphBufferHandle  hLightIndices     = lightSamplingPass.first->m_hLightIndexBuffer;

    bForwardPlusDirectLighting = xiiGpuSceneRasterManager::AddForwardPlusPass(graph, "GPU Scene Forward Plus", hSceneDepth, hAmbientOcclusion, hRayTracedShadow, hContactShadow, hDirectReservoir, hLightGrid, hLightIndices, m_Resources.m_LightingPasses.m_hEmissiveTriangles, m_Resources.m_LightingPasses.m_hEmissiveLightHeader, mainVisibility, geometry, materials, *m_Resources.m_LightingSystem.BorrowSystem(), gpuRasterDescription)
                                   .IsValid();
  }
  if (!bForwardPlusDirectLighting)
  {
    graph.AddPass<xiiDeferredDirectLightingData>("DeferredDirectLighting", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupDirectLighting, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteDirectLighting, this));
  }
  graph.AddPass<xiiDeferredIndirectLightingData>("DeferredIndirectLighting", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupIndirectLighting, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteIndirectLighting, this));
  graph.AddPass<xiiRayTracedGlobalIlluminationData>("RTGIFinalGather", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupRayTracedGlobalIllumination, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedGlobalIllumination, this));
  graph.AddPass<xiiReSTIRGITemporalData>("ReSTIRGITemporal", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupReSTIRGITemporal, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRGITemporal, this));
  graph.AddPass<xiiReSTIRGISpatialData>("ReSTIRGISpatial", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupReSTIRGISpatial, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRGISpatial, this));
  graph.AddPass<xiiRayTracedGlobalIlluminationDenoiseData>("RTGITemporalDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupRayTracedGlobalIlluminationDenoise, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedGlobalIlluminationDenoise, this));
  graph.AddPass<xiiRayTracedReflectionsData>("RTReflections", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupRayTracedReflections, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedReflections, this));
  graph.AddPass<xiiRayTracedReflectionsDenoiseData>("RTReflectionTemporalDenoise", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupRayTracedReflectionsDenoise, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedReflectionsDenoise, this));
  graph.AddPass<xiiVolumetricLightInjectionData>("VolumetricLightInjection", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupVolumetricLightInjection, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricLightInjection, this));
  graph.AddPass<xiiVolumetricFogIntegrationData>("VolumetricFogIntegrate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupVolumetricFogIntegration, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricFogIntegration, this));
  graph.AddPass<xiiVolumetricFogResolveData>("VolumetricFogResolve", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupVolumetricFogResolve, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricFogResolve, this));
  graph.AddPass<xiiVolumetricFogTemporalReprojectionData>("VolumetricFogTemporalRep", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupVolumetricFogTemporalReprojection, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricFogTemporalReprojection, this));
}

void xiiBuiltinRenderPipelineRuntime::BuildOpaqueComposition(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // Composite the selected opaque lighting path with indirect, emissive, near-field GI, and volumetrics.
  graph.AddPass<xiiOpaqueLightingCompositeData>("OpaqueLightingComposite", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupOpaqueLightingComposite, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteOpaqueLightingComposite, this));
  graph.AddPass<xiiHairRenderingData>("HairRendering", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupHairRendering, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteHairRendering, this));
  graph.AddPass<xiiPlanarReflectionsData>("PlanarReflections", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupPlanarReflections, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecutePlanarReflections, this));
  graph.AddPass<xiiWaterRenderingData>("WaterRendering", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupWaterRendering, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteWaterRendering, this));
  graph.AddPass<xiiSubsurfaceScatteringData>("SubsurfaceScattering", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupSubsurfaceScattering, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteSubsurfaceScattering, this));
  graph.AddPass<xiiEyeShaderData>("EyeShader", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupEyeShader, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteEyeShader, this));

  // Resolve screen-space rays against complete opaque scene radiance, then
  // replace Tier 2 environment specular with the best valid Tier 3 / Tier 1
  // result. This ordering avoids the previous HDR read-before-create cycle.
  graph.AddPass<xiiScreenSpaceReflectionsData>("SSR", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupScreenSpaceReflections, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteScreenSpaceReflections, this));
  graph.AddPass<xiiScreenSpaceReflectionsTemporalData>("SSRTemporal", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupScreenSpaceReflectionsTemporal, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteScreenSpaceReflectionsTemporal, this));
  graph.AddPass<xiiHybridReflectionCompositeData>("HybridReflectionComposite", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupHybridReflectionComposite, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteHybridReflectionComposite, this));
}

void xiiBuiltinRenderPipelineRuntime::BuildTransparency(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  // Transparency and special material passes.
  graph.AddPass<xiiGPUParticleSimulateData>("GPUParticleSimulate", xiiGALCommandQueueFlags::Compute, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupGPUParticleSimulate, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteGPUParticleSimulate, this));
  graph.AddPass<xiiWeightedBlendedOITData>("WeightedBlendedOIT", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::SetupWeightedBlendedOIT, this), xiiMakeDelegate(&xiiBuiltinRenderPipelineRuntime::ExecuteWeightedBlendedOIT, this));
}

xiiResult xiiBuiltinRenderPipelineRuntime::BuildDebug(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiRenderGraphTextureHandle& color, xiiRenderGraphTextureHandle& depth)
{
  // Debug and visualization passes.
  XII_SUCCEED_OR_RETURN(BuildViewRenderMode(graph, color));
  return xiiDebugRenderer::AddRenderGraphPasses(graph, color, depth);
}

void xiiBuiltinRenderPipelineRuntime::BeginFrame(const xiiVirtualShadowMapManager::UploadHandles& shadows)
{
  m_Frame                      = {};
  m_Frame.virtualShadowHandles = shadows;
}

xiiResult xiiBuiltinRenderPipelineRuntime::BuildStage(xiiStringView stage, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  m_StageSetupResult = XII_SUCCESS;
  XII_SUCCEED_OR_RETURN(BuildStageInternal(stage, graph, blackboard));
  return m_StageSetupResult;
}

xiiResult xiiBuiltinRenderPipelineRuntime::BuildStageInternal(xiiStringView stage, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  XII_SUCCEED_OR_RETURN(PrepareTemporalResources(stage));
  if (stage == "PrepareFrame")
  {
    BuildPrepareFrame(graph, blackboard);
    return XII_SUCCESS;
  }
  if (stage == "Visibility")
  {
    BuildVisibility(graph, blackboard);
    return XII_SUCCESS;
  }
  if (stage == "Shadows")
  {
    return BuildShadows(graph, blackboard);
  }
  if (stage == "DepthAndGeometry")
  {
    BuildDepthAndGeometry(graph, blackboard);
    return XII_SUCCESS;
  }
  if (stage == "LightingPreparation")
    return BuildLightingPreparation(graph, blackboard);
  if (stage == "Lighting")
  {
    BuildLighting(graph, blackboard);
    return XII_SUCCESS;
  }
  if (stage == "OpaqueComposition")
  {
    BuildOpaqueComposition(graph, blackboard);
    return XII_SUCCESS;
  }
  if (stage == "Transparency")
  {
    BuildTransparency(graph, blackboard);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiBuiltinRenderPipelineRuntime::InvalidateTemporalHistory()
{
  xiiGpuVisibilityManager::InvalidateHiZHistory(m_hInstanceVisibility);
  for (auto it = m_Subgraphs.GetIterator(); it.IsValid(); ++it)
    it.Value()->InvalidateTemporalHistory();
  if (m_Resources.m_LightingSystem.IsInitialized())
    m_Resources.m_LightingSystem.BorrowSystem()->InvalidateGlobalIlluminationHistory();
  m_Resources.m_DepthPasses.m_bMotionHistoryValid                  = false;
  m_Resources.m_ShadowPasses.m_bRayTracedShadowHistoryValid        = false;
  m_Resources.m_LightingPrepPasses.m_bAmbientOcclusionHistoryValid = false;
  m_Resources.m_LightingPasses.m_bRTGIReservoirHistoryValid        = false;
  m_Resources.m_LightingPasses.m_bRTGIHistoryValid                 = false;
  m_Resources.m_LightingPasses.m_bRTReflectionHistoryValid         = false;
  m_Resources.m_LightingPasses.m_bSSRHistoryValid                  = false;
  m_Resources.m_LightingPasses.m_bDirectReservoirHistoryValid      = false;
  m_Resources.m_LightingPasses.m_bVolumetricHistoryValid           = false;
}

void xiiBuiltinRenderPipelineRuntime::BindFrameResources(xiiGALCommandList& commandList) const
{
  if (m_Resources.m_LightingSystem.IsInitialized())
    m_Resources.m_LightingSystem.BindFrameConstants(commandList, xiiGALShaderType::Unknown);
  static const char* names[] = {"g_sPointClamp", "g_sLinearClamp", "g_sLinearWrap", "g_sAnisotropicWrap", "g_sShadowPCF"};
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(names); ++i)
    commandList.ResolveAndSetSampler(xiiTempHashedString(xiiStringView(names[i])), m_CommonSamplers[i].Borrow());
}
