/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/Implementation/SpotShadowPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Transparency/TransparentRasterPass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Materials/GpuSurfaceMaterial.h>
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
#include <Shaders/Pipeline/Passes/Geometry/GpuSceneGBufferConstants.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
#include <Shaders/Pipeline/Passes/Refraction/ForwardRefractionConstants.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>
#include <Shaders/Pipeline/Passes/Transparency/TransparentRasterConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTransparentRasterPass,3,xiiRTTIDefaultAllocator<xiiTransparentRasterPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("VisibilityView",m_VisibilityView)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SceneInstances",m_SceneInstances)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SkinningMatrices",m_SkinningMatrices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VisibleMeshlets",m_VisibleMeshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata",m_GeometryMetadata)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata",m_MeshletMetadata)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SurfaceMaterials",m_SurfaceMaterials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RasterConstants",m_RasterConstants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommands",m_DrawCommands)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DrawCommandCount",m_DrawCommandCount)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Environment",m_Environment)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lights",m_Lights)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightCounts",m_LightCounts)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightCandidates",m_LightCandidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("IESProfiles",m_IESProfiles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightGrid",m_LightGrid)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightIndices",m_LightIndices)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightOverflow",m_LightOverflow)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("BackgroundRadiance",m_BackgroundRadiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightLayout",m_LightLayout)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Cascades",m_Cascades)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ShadowAtlas",m_ShadowAtlas)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SpotShadows",m_Spots)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SpotShadowAtlas",m_SpotAtlas)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Atmosphere",m_Atmosphere)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Transmittance",m_Transmittance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MultiScatter",m_MultiScatter)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Accumulation",m_Accumulation)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Revealage",m_Revealage)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Additive",m_Additive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Modulation",m_Modulation)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("HistoryTrust",m_HistoryTrust)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnableRefraction",m_bEnableRefraction)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("RefractionDistance",m_fRefractionDistance)->AddAttributes(new xiiDefaultValueAttribute(100.0f)),
    XII_MEMBER_PROPERTY("RefractionThicknessTolerance",m_fRefractionThicknessTolerance)->AddAttributes(new xiiDefaultValueAttribute(0.1f)),
    XII_MEMBER_PROPERTY("RefractionSteps",m_uiRefractionSteps)->AddAttributes(new xiiDefaultValueAttribute(32U)),
    XII_MEMBER_PROPERTY("LightingScale",m_fLightingScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("WeightDepthScale",m_fWeightDepthScale)->AddAttributes(new xiiDefaultValueAttribute(5.0f)),
    XII_MEMBER_PROPERTY("MaximumWeight",m_fMaximumWeight)->AddAttributes(new xiiDefaultValueAttribute(3000.0f)),
    XII_MEMBER_PROPERTY("DepthBias",m_fDepthBias)->AddAttributes(new xiiDefaultValueAttribute(0.0005f)),
    XII_MEMBER_PROPERTY("NormalBias",m_fNormalBias)->AddAttributes(new xiiDefaultValueAttribute(0.02f)),
    XII_MEMBER_PROPERTY("NormalBiasTexels",m_fNormalBiasTexels)->AddAttributes(new xiiDefaultValueAttribute(1.5f)),
    XII_ENUM_MEMBER_PROPERTY("FillMode",xiiGALFillMode,m_FillMode)->AddAttributes(new xiiDefaultValueAttribute(xiiGALFillMode::Solid)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Transparency")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiTransparentRasterPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (!xiiMath::IsFinite(m_fRefractionDistance) || m_fRefractionDistance <= 0 || m_fRefractionDistance > 100000 ||
      !xiiMath::IsFinite(m_fRefractionThicknessTolerance) || m_fRefractionThicknessTolerance <= 0 || m_fRefractionThicknessTolerance > 10 ||
      m_uiRefractionSteps == 0 || m_uiRefractionSteps > 128)
    return XII_FAILURE;
  return xiiMath::IsFinite(m_fLightingScale) && m_fLightingScale >= 0 && xiiMath::IsFinite(m_fWeightDepthScale) && m_fWeightDepthScale > 0 &&
      xiiMath::IsFinite(m_fMaximumWeight) && m_fMaximumWeight >= 0.01f && m_fMaximumWeight <= 1000000 &&
      xiiMath::IsFinite(m_fDepthBias) && m_fDepthBias >= 0 && m_fDepthBias <= 0.1f && xiiMath::IsFinite(m_fNormalBias) && m_fNormalBias >= 0 && m_fNormalBias <= 10 &&
      xiiMath::IsFinite(m_fNormalBiasTexels) && m_fNormalBiasTexels >= 0 && m_fNormalBiasTexels <= 4 && (m_FillMode == xiiGALFillMode::Solid || m_FillMode == xiiGALFillMode::Wireframe) ?
    XII_SUCCESS :
    XII_FAILURE;
}
xiiResult xiiTransparentRasterPass::Build(xiiRenderPipelinePassContext& context)
{
  // Query presence controls setup; explicit indirect draws carry the actual GPU dependency.
  const auto query = context.GetInputBuffer(m_VisibilityView);
  if (!query.IsValid())
    return XII_SUCCESS;
  const auto* queryDescription = context.m_Graph.GetBufferDescription(query);
  if (!queryDescription || queryDescription->m_Mode != xiiGALBufferMode::Structured || queryDescription->m_uiElementByteStride != sizeof(xiiGpuVisibilityView) || queryDescription->m_uiSize != sizeof(xiiGpuVisibilityView))
    return context.ReportFailure("Transparent rasterization requires one canonical visibility query.");
  if (!context.m_pView || !context.m_pView->GetGpuInstanceData() || !xiiGALDevice::HasDefaultDevice())
    return context.ReportFailure("Transparent rasterization requires authoritative world extraction and a device.");
  const auto  device  = xiiGALDevice::GetDefaultDevice();
  const auto& adapter = device->GetGraphicsDeviceAdapterProperties();
  if (device->GetFeatures().m_IndependentBlend != xiiGALDeviceFeatureState::Enabled || adapter.m_DeviceLimits.m_uiMaxRenderTargets < 5)
    return context.ReportFailure("Transparent layers require independent blending and five color attachments; disable the authored transparent visibility workload on unsupported devices.");
  if (m_FillMode == xiiGALFillMode::Wireframe && device->GetFeatures().m_WireframeFill != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("Transparent wireframe rasterization is unsupported by this device.");
  const auto  depth            = context.GetInputTexture(m_Depth);
  const auto* depthDescription = context.m_Graph.GetTextureDescription(depth);
  if (!depthDescription || depthDescription->m_Format != xiiGALResourceFormat::D32Float || depthDescription->m_Type != xiiGALResourceDimension::Texture2D || depthDescription->m_uiArraySizeOrDepth != 1 || depthDescription->m_uiSampleCount != 1 || !depthDescription->m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil) || !depthDescription->m_Size.HasNonZeroArea() || depthDescription->m_Size.width != context.m_pView->GetRenderResolutionWidth() || depthDescription->m_Size.height != context.m_pView->GetRenderResolutionHeight())
    return context.ReportFailure("Transparent rasterization requires viewport-sized single-sample D32 depth.");
  const auto depthSnapshot = *depthDescription;
  const auto valid         = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* d = context.m_Graph.GetBufferDescription(handle);
    return d && d->m_Mode == xiiGALBufferMode::Structured && d->m_uiElementByteStride == stride && d->m_uiSize >= stride && d->m_uiSize % stride == 0 && d->m_uiSize / stride <= xiiMath::MaxValue<xiiUInt32>();
  };
  const auto sceneInstances   = context.GetInputBuffer(m_SceneInstances);
  const auto skinningMatrices = context.GetInputBuffer(m_SkinningMatrices);
  const auto visibleMeshlets  = context.GetInputBuffer(m_VisibleMeshlets);
  const auto geometryMetadata = context.GetInputBuffer(m_GeometryMetadata);
  const auto meshletMetadata  = context.GetInputBuffer(m_MeshletMetadata);
  const auto surfaceMaterials = context.GetInputBuffer(m_SurfaceMaterials);
  const auto rasterConstants  = context.GetInputBuffer(m_RasterConstants);
  const auto drawCommands     = context.GetInputBuffer(m_DrawCommands);
  const auto drawCommandCount = context.GetInputBuffer(m_DrawCommandCount);
  const auto camera           = context.GetInputBuffer(m_Camera);
  const auto environment      = context.GetInputBuffer(m_Environment);
  const auto lights           = context.GetInputBuffer(m_Lights);
  const auto lightCounts      = context.GetInputBuffer(m_LightCounts);
  const auto lightCandidates  = context.GetInputBuffer(m_LightCandidates);
  const auto iesProfiles      = context.GetInputBuffer(m_IESProfiles);
  const auto lightGrid        = context.GetInputBuffer(m_LightGrid);
  const auto lightIndices     = context.GetInputBuffer(m_LightIndices);
  const auto lightOverflow    = context.GetInputBuffer(m_LightOverflow);
  if (!valid(sceneInstances, sizeof(xiiGpuSceneInstance)) || !valid(skinningMatrices, sizeof(xiiMat4)) || !valid(visibleMeshlets, sizeof(xiiVec2U32)) || !valid(geometryMetadata, sizeof(xiiGpuGeometryRecord)) || !valid(meshletMetadata, sizeof(xiiMeshlet)) || !valid(surfaceMaterials, sizeof(xiiGpuSurfaceMaterial)) || !valid(drawCommands, sizeof(xiiVec4U32)) || !valid(drawCommandCount, sizeof(xiiUInt32)) || !xiiScreenSpacePassUtils::Uniform(context, rasterConstants, sizeof(xiiGpuSceneGBufferConstants)) || !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)) || !xiiScreenSpacePassUtils::Uniform(context, environment, sizeof(xiiEnvironmentLightConstants)))
    return context.ReportFailure("Transparent rasterization requires canonical geometry, draw commands and exact camera/raster/environment uniforms.");
  const auto capacity = context.m_Graph.GetBufferDescription(drawCommands)->m_uiSize / sizeof(xiiVec4U32);
  if (capacity > adapter.m_DrawCommandProperties.m_uiMaxDrawIndirectCount || !adapter.m_DrawCommandProperties.m_CapabilityFlags.IsSet(xiiGALDrawCommandCapabilityFlags::DrawIndirectCounterBuffer))
    return context.ReportFailure("Transparent indirect command capacity or counter support is insufficient.");
  const auto* layout = xiiLightClusterPassUtils::Layout(context, m_LightLayout);
  if (!layout || layout->m_Constants.Viewport.x != depthDescription->m_Size.width || layout->m_Constants.Viewport.y != depthDescription->m_Size.height || !valid(lights, sizeof(xiiGpuLightData)) || !valid(lightCounts, sizeof(xiiVec4U32)) || !valid(lightCandidates, sizeof(xiiUInt32)) || !valid(iesProfiles, sizeof(float)) || !valid(lightGrid, sizeof(xiiVec2U32)) || !valid(lightIndices, sizeof(xiiUInt32)) || !valid(lightOverflow, sizeof(xiiUInt32)) || context.m_Graph.GetBufferDescription(lightGrid)->m_uiSize / sizeof(xiiVec2U32) < layout->m_Constants.ClusterCounts.w || context.m_Graph.GetBufferDescription(lightIndices)->m_uiSize / sizeof(xiiUInt32) < layout->m_Constants.Work.y)
    return context.ReportFailure("Transparent lighting requires matching canonical view-local light and cluster resources.");
  const auto& cluster = layout->m_Constants;
  if (!cluster.Viewport.z || !cluster.Viewport.w || !cluster.ClusterCounts.x || !cluster.ClusterCounts.y || !cluster.ClusterCounts.z || xiiUInt64(cluster.ClusterCounts.x) * cluster.ClusterCounts.y * cluster.ClusterCounts.z != cluster.ClusterCounts.w || !cluster.DepthRange.IsValid() || cluster.DepthRange.y <= cluster.DepthRange.x || (cluster.DepthRange.w > 0 && (cluster.DepthRange.x <= 0 || cluster.DepthRange.z <= 0)))
    return context.ReportFailure("Transparent light layout requires finite depth slices and a consistent nonzero cluster extent.");
  const auto  shadowData  = context.GetInputData(m_Cascades);
  const auto* cascades    = xiiDynamicCast<const xiiShadowCascadeSnapshot*>(shadowData.Borrow());
  const auto  shadowAtlas = context.GetInputTexture(m_ShadowAtlas);
  const bool  shadows     = cascades != nullptr;
  if (shadowData && !cascades)
    return context.ReportFailure("Invalid transparent cascade snapshot.");
  if (shadows != shadowAtlas.IsValid())
    return context.ReportFailure("Transparent shadows require both cascade snapshot and completed atlas.");
  if (shadows)
  {
    const auto* atlas      = context.m_Graph.GetTextureDescription(shadowAtlas);
    const auto  resolution = cascades->m_uiResolution, count = cascades->m_Cascades.GetCount();
    if (!atlas || !resolution || !count || count > 4 || atlas->m_Size != xiiSizeU32(resolution * (count > 1 ? 2U : 1U), resolution * (count > 2 ? 2U : 1U)) || !xiiScreenSpacePassUtils::Uniform(context, cascades->m_hConstants, sizeof(xiiShadowCascadeConstants)))
      return context.ReportFailure("Transparent cascade atlas and constants disagree.");
  }
  const auto spots     = context.GetInputData(m_Spots).Downcast<const xiiSpotShadowSnapshot>();
  const auto spotAtlas = context.GetInputTexture(m_SpotAtlas);
  XII_SUCCEED_OR_RETURN(xiiSpotShadowPassUtils::Validate(context, spots.Borrow(), spotAtlas));
  const auto atmosphere    = context.GetInputBuffer(m_Atmosphere);
  const auto transmittance = context.GetInputTexture(m_Transmittance), multiScatter = context.GetInputTexture(m_MultiScatter);
  const bool atmospheric = atmosphere.IsValid();
  if (atmospheric && (!xiiScreenSpacePassUtils::Uniform(context, atmosphere, sizeof(xiiAtmosphereConstants)) || !xiiScreenSpacePassUtils::Image(context, transmittance, 3) || !xiiScreenSpacePassUtils::Image(context, multiScatter, 3)))
    return context.ReportFailure("Transparent atmosphere requires exact physical constants and both readable LUTs.");
  if (!atmospheric && (transmittance.IsValid() || multiScatter.IsValid()))
    return context.ReportFailure("Transparent atmosphere LUTs require physical constants.");
  bool refractive = false;
  if (m_bEnableRefraction)
  {
    const auto* instances = context.m_pView->GetGpuInstanceData();
    const auto  records   = instances->GetInstances();
    for (xiiUInt32 index = 0; index < records.GetCount(); ++index)
    {
      const auto required = (xiiGpuInstanceFlags::Enabled | xiiGpuInstanceFlags::Transparent).GetValue();
      if ((records[index].m_uiFlags & required) != required)
        continue;
      const auto material = xiiMaterialManager::GetGpuStorage().GetMaterial(instances->GetMaterialHandle(index));
      if (!material)
        continue; // The canonical absent-material surface has zero transmission.
      xiiMaterialInstanceSnapshot snapshot;
      material->CreateSnapshot(snapshot);
      if (!snapshot.m_RuntimeState.m_FeatureFlags.IsSet(xiiMaterialFeatureFlags::Transmission))
        continue;
      const auto optics = xiiResolveGpuSurfaceMaterial(snapshot);
      if (!xiiMath::IsFinite(optics.AdvancedParameters.y) || optics.AdvancedParameters.y < 1 ||
          !xiiMath::IsFinite(optics.AdvancedParameters.x) || optics.AdvancedParameters.x < 0 ||
          !xiiMath::IsFinite(optics.SurfaceParameters.z))
        return context.ReportFailure("Refraction materials require finite transmission, IOR at least one and nonnegative world-space thickness.");
      refractive = true;
    }
  }
  const auto background = context.GetInputTexture(m_BackgroundRadiance);
  if (refractive)
  {
    const auto* image = xiiScreenSpacePassUtils::Image(context, background, 4, &depthDescription->m_Size);
    if (!image || (image->m_Format != xiiGALResourceFormat::RGBA32Float && image->m_Format != xiiGALResourceFormat::RGBA16Float) ||
        !depthDescription->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
      return context.ReportFailure("Active refraction requires matching immutable HDR background and shader-readable opaque D32 depth.");
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_RefractionSampler));
  }
  const xiiUInt32 permutation = (shadows ? 1U : 0U) | (atmospheric ? 2U : 0U) | (refractive ? 4U : 0U) | (spots ? 8U : 0U);
  if (!m_hShaders[permutation].IsValid())
  {
    constexpr const char*                          paths[] = {"Shaders/Geometry/TransparentVertexPull.xiiShader", "Shaders/Geometry/TransparentShadowVertexPull.xiiShader", "Shaders/Geometry/TransparentAtmosphereVertexPull.xiiShader", "Shaders/Geometry/TransparentWorldVertexPull.xiiShader"};
    const auto                                     shader  = xiiResourceManager::LoadResource<xiiShaderResource>(paths[permutation & 3U]);
    xiiHashTable<xiiHashedString, xiiHashedString> variables;
    xiiHashedString                                name;
    xiiHashedString                                value;
    name.Assign("FORWARD_REFRACTION");
    value.Assign(refractive ? "TRUE" : "FALSE");
    variables.Insert(name, value);
    name.Assign("FORWARD_SPOT_SHADOWS");
    value.Assign(spots ? "TRUE" : "FALSE");
    variables.Insert(name, value);
    m_hShaders[permutation] = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, variables, false);
  }
  xiiResourceLock<xiiShaderPermutationResource> shader(m_hShaders[permutation], xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Transparent raster shader is unavailable.");
  if (!m_MaterialSampler)
  {
    xiiGALSamplerCreationDescription description;
    description.m_MinFilter       = xiiGALFilterType::Anisotropic;
    description.m_MagFilter       = xiiGALFilterType::Anisotropic;
    description.m_MipFilter       = xiiGALFilterType::Anisotropic;
    description.m_AddressU        = xiiGALTextureAddressMode::Wrap;
    description.m_AddressV        = xiiGALTextureAddressMode::Wrap;
    description.m_AddressW        = xiiGALTextureAddressMode::Wrap;
    description.m_uiMaxAnisotropy = 8;
    m_MaterialSampler             = device->CreateSampler(description);
  }
  if (!m_MaterialSampler)
    return context.ReportFailure("Could not create transparent material sampler.");
  if (atmospheric)
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_AtmosphereSampler));
  auto rasterizer       = shader->GetRasterizerState()->GetDescription();
  rasterizer.m_FillMode = m_FillMode;
  m_Rasterizer          = device->CreateRasterizerState(rasterizer);
  if (!m_Rasterizer)
    return context.ReportFailure("Could not create transparent rasterizer state.");
  xiiRenderPassResourceScope scope(context);
  xiiHashTable<xiiHashedString, xiiHashedString> materialVariables;
  const auto                                     feature = [&](xiiStringView key, bool enabled) {
    xiiHashedString name, value;
    name.Assign(key);
    value.Assign(enabled ? "TRUE" : "FALSE");
    materialVariables.Insert(name, value);
  };
  feature("FORWARD_DIRECTIONAL_SHADOWS", shadows);
  feature("FORWARD_ATMOSPHERE", atmospheric);
  feature("FORWARD_REFRACTION", refractive);
  feature("FORWARD_SPOT_SHADOWS", spots != nullptr);
  xiiDynamicArray<xiiMaterialRasterBatch> batches;
  xiiRenderGraphBufferHandle              sortedVisible, materialDraws;
  XII_SUCCEED_OR_RETURN(m_MaterialBatches.Build(context, visibleMeshlets, drawCommandCount, batches, sortedVisible, materialDraws, xiiMaterialRasterBatches::Pass::Transparent, &materialVariables));

  struct RefractionData
  {
    xiiRenderGraphTextureHandle   m_SourceDepth;
    xiiRenderGraphTextureHandle   m_SampledDepth;
    xiiRenderGraphBufferHandle    m_Constants;
    xiiForwardRefractionConstants m_Parameters;
  };
  xiiRenderGraphTextureHandle sampledDepth;
  xiiRenderGraphBufferHandle  refractionConstants;
  if (refractive)
  {
    const auto snapshot = context.m_Graph.AddPass<RefractionData>("Refraction opaque snapshot", xiiGALCommandQueueFlags::Transfer, [&](RefractionData& data, xiiRenderGraphBuilder& builder) {
      const auto description = depthSnapshot;
      data.m_SourceDepth = builder.ReadTexture(depth, xiiGALResourceStateFlags::CopySource);
      data.m_SampledDepth = builder.WriteTexture("Refraction sampled depth", description, xiiGALResourceStateFlags::CopyDestination);
      data.m_Constants = xiiRenderPassUtils::Constants(builder, "Dielectric refraction settings", sizeof(xiiForwardRefractionConstants));
      data.m_Parameters.RefractionTrace = xiiVec4(m_fRefractionDistance, m_fRefractionThicknessTolerance, float(m_uiRefractionSteps), 0); }, [](const RefractionData& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.CopyTexture(execute.GetTexture(data.m_SourceDepth), execute.GetTexture(data.m_SampledDepth));
      xiiGALMapHelper<xiiForwardRefractionConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_Parameters; });
    sampledDepth        = snapshot.first->m_SampledDepth;
    refractionConstants = snapshot.first->m_Constants;
  }
  struct Uniforms
  {
    xiiRenderGraphBufferHandle    m_Layout, m_Settings;
    xiiLightClusterConstants      m_ClusterSnapshot;
    xiiTransparentRasterConstants m_SettingsSnapshot;
  };
  const auto uniforms = context.m_Graph.AddPass<Uniforms>("Transparent lighting constants", xiiGALCommandQueueFlags::Transfer, [&](Uniforms& data, xiiRenderGraphBuilder& builder) {
    data.m_Layout=xiiRenderPassUtils::Constants(builder,"Transparent light layout",sizeof(xiiLightClusterConstants));
    data.m_Settings=xiiRenderPassUtils::Constants(builder,"Transparent layer settings",sizeof(xiiTransparentRasterConstants));
    data.m_ClusterSnapshot=layout->m_Constants;data.m_SettingsSnapshot.TransparentWeightsAndScale=xiiVec4(m_fWeightDepthScale,m_fMaximumWeight,m_fLightingScale,0);data.m_SettingsSnapshot.TransparentShadowBias=xiiVec4(m_fDepthBias,m_fNormalBias,m_fNormalBiasTexels,shadows?float(cascades->m_uiResolution):0); }, [](const Uniforms& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    {xiiGALMapHelper<xiiLightClusterConstants> mapped(commands,execute.GetBuffer(data.m_Layout),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_ClusterSnapshot;}
    {xiiGALMapHelper<xiiTransparentRasterConstants> mapped(commands,execute.GetBuffer(data.m_Settings),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_SettingsSnapshot;}
    xiiGALStateTransitionDescription transitions[2];transitions[0].m_pResource=execute.GetBuffer(data.m_Layout);transitions[1].m_pResource=execute.GetBuffer(data.m_Settings);
    for(auto& transition : transitions){transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer;transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;}
    commands.TransitionResourceStates(transitions); });
  struct Data
  {
    xiiDynamicArray<xiiMaterialRasterBatch>       m_Batches;
    xiiRenderGraphBufferHandle                    m_SceneInstances;
    xiiRenderGraphBufferHandle                    m_SkinningMatrices;
    xiiRenderGraphBufferHandle                    m_VisibleMeshlets;
    xiiRenderGraphBufferHandle                    m_GeometryMetadata;
    xiiRenderGraphBufferHandle                    m_MeshletMetadata;
    xiiRenderGraphBufferHandle                    m_SurfaceMaterials;
    xiiRenderGraphBufferHandle                    m_RasterConstants;
    xiiRenderGraphBufferHandle                    m_DrawCommands;
    xiiRenderGraphBufferHandle                    m_DrawCommandCount;
    xiiRenderGraphBufferHandle                    m_Camera;
    xiiRenderGraphBufferHandle                    m_Environment;
    xiiRenderGraphBufferHandle                    m_Lights;
    xiiRenderGraphBufferHandle                    m_LightCounts;
    xiiRenderGraphBufferHandle                    m_LightCandidates;
    xiiRenderGraphBufferHandle                    m_IESProfiles;
    xiiRenderGraphBufferHandle                    m_LightGrid;
    xiiRenderGraphBufferHandle                    m_LightIndices;
    xiiRenderGraphBufferHandle                    m_LightOverflow;
    xiiRenderGraphBufferHandle                    m_ClusterConstants;
    xiiRenderGraphBufferHandle                    m_LayerConstants;
    xiiRenderGraphBufferHandle                    m_CascadeConstants;
    xiiRenderGraphBufferHandle                    m_Atmosphere;
    xiiRenderGraphTextureHandle                   m_Depth;
    xiiRenderGraphTextureHandle                   m_RefractionDepth, m_Background;
    xiiRenderGraphBufferHandle                    m_RefractionConstants;
    xiiRenderGraphTextureHandle                   m_ShadowAtlas;
    xiiRenderGraphTextureHandle                   m_SpotAtlas;
    xiiRenderGraphBufferHandle                    m_SpotConstants;
    xiiRenderGraphTextureHandle                   m_Transmittance;
    xiiRenderGraphTextureHandle                   m_MultiScatter;
    xiiRenderGraphTextureHandle                   m_Accumulation;
    xiiRenderGraphTextureHandle                   m_Revealage;
    xiiRenderGraphTextureHandle                   m_Additive;
    xiiRenderGraphTextureHandle                   m_Modulation;
    xiiRenderGraphTextureHandle                   m_HistoryTrust;
    xiiSharedPtr<xiiGALShader>                    m_Vertex, m_Pixel;
    xiiSharedPtr<xiiGALPipelineResourceSignature> m_Signature;
    xiiSharedPtr<xiiGALBlendState>                m_Blend;
    xiiSharedPtr<xiiGALDepthStencilState>         m_DepthState;
    xiiSharedPtr<xiiGALRasterizerState>           m_Rasterizer;
    xiiSharedPtr<xiiGALSampler>                   m_MaterialSampler, m_AtmosphereSampler, m_RefractionSampler;
    xiiSizeU32                                    m_Size;
    xiiUInt32                                     m_uiCapacity;
    bool                                          m_bShadows, m_bAtmosphere, m_bRefraction;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Batches = batches;
    for (xiiUInt32 i = 1; i < data.m_Batches.GetCount(); ++i)
    {
      auto& batch = data.m_Batches[i];
      batch.m_ViewConstants = builder.ReadBuffer(batch.m_ViewConstants, xiiGALResourceStateFlags::ConstantBuffer);
      if (batch.m_Constants.IsValid()) batch.m_Constants = builder.ReadBuffer(batch.m_Constants, xiiGALResourceStateFlags::ConstantBuffer);
      for (xiiUInt32 texture = 0; texture < batch.m_Binding.m_Textures.GetCount(); ++texture)
      {
        xiiStringBuilder name; name.SetFormat("Material {} Texture {}", i, texture);
        const auto handle = builder.ImportTexture(name, batch.m_Binding.m_Textures[texture].m_Texture, batch.m_Binding.m_Textures[texture].m_Texture->GetResourceState());
        [[maybe_unused]] const auto read = builder.ReadTexture(handle, xiiGALResourceStateFlags::ShaderResource);
      }
    }
    builder.SetPassAllowMerge(false);
    data.m_SceneInstances=builder.ReadBuffer(sceneInstances,xiiGALResourceStateFlags::ShaderResource);
    data.m_SkinningMatrices=builder.ReadBuffer(skinningMatrices,xiiGALResourceStateFlags::ShaderResource);
    data.m_VisibleMeshlets=builder.ReadBuffer(batches.IsEmpty() ? visibleMeshlets : sortedVisible,xiiGALResourceStateFlags::ShaderResource);
    data.m_GeometryMetadata=builder.ReadBuffer(geometryMetadata,xiiGALResourceStateFlags::ShaderResource);
    data.m_MeshletMetadata=builder.ReadBuffer(meshletMetadata,xiiGALResourceStateFlags::ShaderResource);
    data.m_SurfaceMaterials=builder.ReadBuffer(surfaceMaterials,xiiGALResourceStateFlags::ShaderResource);
    data.m_RasterConstants=builder.ReadBuffer(rasterConstants,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_DrawCommands=builder.ReadBuffer(batches.IsEmpty() ? drawCommands : materialDraws,xiiGALResourceStateFlags::IndirectArgument);
    data.m_DrawCommandCount=builder.ReadBuffer(drawCommandCount,xiiGALResourceStateFlags::IndirectArgument);
    data.m_Camera=builder.ReadBuffer(camera,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Environment=builder.ReadBuffer(environment,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Lights=builder.ReadBuffer(lights,xiiGALResourceStateFlags::ShaderResource);
    data.m_LightCounts=builder.ReadBuffer(lightCounts,xiiGALResourceStateFlags::ShaderResource);
    data.m_LightCandidates=builder.ReadBuffer(lightCandidates,xiiGALResourceStateFlags::ShaderResource);
    data.m_IESProfiles=builder.ReadBuffer(iesProfiles,xiiGALResourceStateFlags::ShaderResource);
    data.m_LightGrid=builder.ReadBuffer(lightGrid,xiiGALResourceStateFlags::ShaderResource);
    data.m_LightIndices=builder.ReadBuffer(lightIndices,xiiGALResourceStateFlags::ShaderResource);
    data.m_LightOverflow=builder.ReadBuffer(lightOverflow,xiiGALResourceStateFlags::ShaderResource);
    data.m_ClusterConstants=builder.ReadBuffer(uniforms.first->m_Layout,xiiGALResourceStateFlags::ConstantBuffer);data.m_LayerConstants=builder.ReadBuffer(uniforms.first->m_Settings,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Depth=builder.ReadTexture(depth,xiiGALResourceStateFlags::DepthRead);
    data.m_bShadows=shadows;data.m_bAtmosphere=atmospheric;data.m_bRefraction=refractive;
    if(refractive){data.m_Background=builder.ReadTexture(background,xiiGALResourceStateFlags::ShaderResource);data.m_RefractionDepth=builder.ReadTexture(sampledDepth,xiiGALResourceStateFlags::ShaderResource);data.m_RefractionConstants=builder.ReadBuffer(refractionConstants,xiiGALResourceStateFlags::ConstantBuffer);}
    if(shadows){data.m_ShadowAtlas=builder.ReadTexture(shadowAtlas,xiiGALResourceStateFlags::ShaderResource);data.m_CascadeConstants=builder.ReadBuffer(cascades->m_hConstants,xiiGALResourceStateFlags::ConstantBuffer);}
    if (spots)
    {
      data.m_SpotAtlas     = builder.ReadTexture(spotAtlas, xiiGALResourceStateFlags::ShaderResource);
      data.m_SpotConstants = builder.ReadBuffer(spots->m_hConstants, xiiGALResourceStateFlags::ConstantBuffer);
    }
    if(atmospheric){data.m_Atmosphere=builder.ReadBuffer(atmosphere,xiiGALResourceStateFlags::ConstantBuffer);data.m_Transmittance=builder.ReadTexture(transmittance,xiiGALResourceStateFlags::ShaderResource);data.m_MultiScatter=builder.ReadTexture(multiScatter,xiiGALResourceStateFlags::ShaderResource);}
    xiiGALTextureCreationDescription target;target.m_Type=xiiGALResourceDimension::Texture2D;target.m_Size=depthSnapshot.m_Size;target.m_BindFlags=xiiGALBindFlags::RenderTarget|xiiGALBindFlags::ShaderResource;
    target.m_Format=xiiGALResourceFormat::RGBA32Float;data.m_Accumulation=builder.WriteTexture("Transparent Accumulation",target,xiiGALResourceStateFlags::RenderTarget);
    target.m_Format=xiiGALResourceFormat::R32Float;data.m_Revealage=builder.WriteTexture("Transparent Revealage",target,xiiGALResourceStateFlags::RenderTarget);
    target.m_Format=xiiGALResourceFormat::RGBA32Float;data.m_Additive=builder.WriteTexture("Transparent Additive",target,xiiGALResourceStateFlags::RenderTarget);
    target.m_Format=xiiGALResourceFormat::RGBA32Float;data.m_Modulation=builder.WriteTexture("Transparent Modulation",target,xiiGALResourceStateFlags::RenderTarget);
    target.m_Format=xiiGALResourceFormat::R32Float;data.m_HistoryTrust=builder.WriteTexture("Transparent HistoryTrust",target,xiiGALResourceStateFlags::RenderTarget);
    builder.SetPassRenderPassManaged();
    data.m_Vertex=shader->GetGALShader(xiiGALShaderType::Vertex);data.m_Pixel=shader->GetGALShader(xiiGALShaderType::Pixel);data.m_Signature=shader->GetPipelineResourceSignature();data.m_Blend=shader->GetBlendState();data.m_DepthState=shader->GetDepthStencilState();data.m_Rasterizer=m_Rasterizer;data.m_MaterialSampler=m_MaterialSampler;data.m_AtmosphereSampler=m_AtmosphereSampler;data.m_RefractionSampler=m_RefractionSampler;data.m_Size=target.m_Size;data.m_uiCapacity=static_cast<xiiUInt32>(capacity); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    xiiRenderPassUtils::ClearRenderTargetPreservingState(commands,execute.GetTexture(data.m_Accumulation)->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor::MakeZero());
    xiiRenderPassUtils::ClearRenderTargetPreservingState(commands,execute.GetTexture(data.m_Revealage)->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor::White);
    xiiRenderPassUtils::ClearRenderTargetPreservingState(commands,execute.GetTexture(data.m_Additive)->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor::MakeZero());
    xiiRenderPassUtils::ClearRenderTargetPreservingState(commands,execute.GetTexture(data.m_Modulation)->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor::White);
    xiiRenderPassUtils::ClearRenderTargetPreservingState(commands,execute.GetTexture(data.m_HistoryTrust)->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor::White);
    for (xiiUInt32 bin = 0; bin < xiiMath::Max(1u, data.m_Batches.GetCount()); ++bin)
    {
    xiiGALGraphicsPipelineStateCreationDescription pipeline;pipeline.m_PipelineType=xiiGALPipelineType::Graphics;pipeline.m_pPipelineResourceSignature=data.m_Signature;pipeline.m_pVertexShader=data.m_Vertex;pipeline.m_pPixelShader=data.m_Pixel;pipeline.m_GraphicsPipeline.m_pBlendState=data.m_Blend;pipeline.m_GraphicsPipeline.m_pDepthStencilState=data.m_DepthState;pipeline.m_GraphicsPipeline.m_pRasterizerState=data.m_Rasterizer;pipeline.m_GraphicsPipeline.m_pRenderPass=execute.GetRenderPass();pipeline.m_GraphicsPipeline.m_uiSubpassIndex=static_cast<xiiUInt8>(execute.GetSubpassIndex());pipeline.m_GraphicsPipeline.m_PrimitiveTopology=xiiGALPrimitiveTopology::TriangleList;
    if (bin > 0)
    {
      const auto& binding = data.m_Batches[bin].m_Binding;
      pipeline.m_pPipelineResourceSignature = binding.m_Signature; pipeline.m_pVertexShader = binding.m_Vertex; pipeline.m_pPixelShader = binding.m_Pixel;
      pipeline.m_GraphicsPipeline.m_pBlendState = binding.m_Blend; pipeline.m_GraphicsPipeline.m_pDepthStencilState = binding.m_Depth; pipeline.m_GraphicsPipeline.m_pRasterizerState = binding.m_Rasterizer;
    }
    const auto state=xiiGALPipelineCache::GetPipeline(pipeline);if(!state){execute.ReportFailure("Could not create transparent layer raster pipeline, including required floating-point attachment blending.");return;}
    commands.SetPipelineState(state.Borrow());commands.SetViewport({0,0,float(data.m_Size.width),float(data.m_Size.height),0,1});
    commands.ResolveAndSetSampler("g_sAnisotropicWrap",data.m_MaterialSampler.Borrow());commands.ResolveAndSetSampler("g_sLinearWrap",data.m_MaterialSampler.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiGpuSceneGBufferConstants",execute.GetBuffer(data.m_RasterConstants));
    commands.ResolveAndSetConstantBuffer("PerFrameCamera",execute.GetBuffer(data.m_Camera),xiiGALShaderType::Pixel);
    commands.ResolveAndSetConstantBuffer("xiiEnvironmentLightConstants",execute.GetBuffer(data.m_Environment),xiiGALShaderType::Pixel);
    commands.ResolveAndSetConstantBuffer("xiiLightClusterConstants",execute.GetBuffer(data.m_ClusterConstants),xiiGALShaderType::Pixel);
    commands.ResolveAndSetConstantBuffer("xiiTransparentRasterConstants",execute.GetBuffer(data.m_LayerConstants),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_SceneInstances",execute.GetBuffer(data.m_SceneInstances)->GetDefaultView(xiiGALBufferViewType::ShaderResource));
    commands.ResolveAndSetShaderResourceBufferView("g_SkinningMatrices",execute.GetBuffer(data.m_SkinningMatrices)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Vertex);
    commands.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets",execute.GetBuffer(data.m_VisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Vertex);
    commands.ResolveAndSetShaderResourceBufferView("g_Geometry",execute.GetBuffer(data.m_GeometryMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Vertex);
    commands.ResolveAndSetShaderResourceBufferView("g_Meshlets",execute.GetBuffer(data.m_MeshletMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Vertex);
    commands.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials",execute.GetBuffer(data.m_SurfaceMaterials)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_Lights",execute.GetBuffer(data.m_Lights)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_Counts",execute.GetBuffer(data.m_LightCounts)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_Candidates",execute.GetBuffer(data.m_LightCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_IESProfiles",execute.GetBuffer(data.m_IESProfiles)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_LightGrid",execute.GetBuffer(data.m_LightGrid)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_LightIndices",execute.GetBuffer(data.m_LightIndices)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_Overflow",execute.GetBuffer(data.m_LightOverflow)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);
    if(data.m_bRefraction)
    {
      commands.ResolveAndSetConstantBuffer("xiiForwardRefractionConstants",execute.GetBuffer(data.m_RefractionConstants),xiiGALShaderType::Pixel);
      commands.ResolveAndSetSampler("g_sRefractionLinearClamp",data.m_RefractionSampler.Borrow());
      commands.ResolveAndSetShaderResourceTextureView("g_RefractionDepth",execute.GetTexture(data.m_RefractionDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Pixel);
      commands.ResolveAndSetShaderResourceTextureView("g_RefractionBackground",execute.GetTexture(data.m_Background)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Pixel);
    }
    if(data.m_bShadows){commands.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants",execute.GetBuffer(data.m_CascadeConstants),xiiGALShaderType::Pixel);commands.ResolveAndSetShaderResourceTextureView("g_ShadowAtlas",execute.GetTexture(data.m_ShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Pixel);}
    if (data.m_SpotAtlas.IsValid())
    {
      commands.ResolveAndSetConstantBuffer("xiiSpotShadowConstants", execute.GetBuffer(data.m_SpotConstants), xiiGALShaderType::Pixel);
      commands.ResolveAndSetShaderResourceTextureView("g_SpotShadowAtlas", execute.GetTexture(data.m_SpotAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
    }
    if(data.m_bAtmosphere){commands.ResolveAndSetSampler("LinearClampSampler",data.m_AtmosphereSampler.Borrow());commands.ResolveAndSetConstantBuffer("xiiAtmosphereConstants",execute.GetBuffer(data.m_Atmosphere),xiiGALShaderType::Pixel);commands.ResolveAndSetShaderResourceTextureView("g_Transmittance",execute.GetTexture(data.m_Transmittance)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Pixel);commands.ResolveAndSetShaderResourceTextureView("g_MultiScatter",execute.GetTexture(data.m_MultiScatter)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Pixel);}
    xiiGALBindlessResourceTable::BindBufferSRVs(commands,"g_Buffers",xiiGALShaderType::Vertex);xiiGALBindlessResourceTable::BindTextureSRVs(commands,"g_Textures",xiiGALShaderType::Pixel);xiiGALBindlessResourceTable::BindSamplers(commands,"g_MaterialSamplers",xiiGALShaderType::Pixel);
    if (bin > 0)
    {
      commands.ResolveAndSetConstantBuffer("xiiNativeMaterialViewConstants", execute.GetBuffer(data.m_Batches[bin].m_ViewConstants));
      data.m_Batches[bin].m_Binding.Bind(commands, data.m_Batches[bin].m_Constants.IsValid() ? execute.GetBuffer(data.m_Batches[bin].m_Constants) : nullptr);
    }
    if(commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Failed()){execute.ReportFailure("Could not bind transparent geometry and lighting resources.");return;}
    if (!data.m_Batches.IsEmpty()) commands.DrawIndirect({execute.GetBuffer(data.m_DrawCommands), 1, bin * sizeof(xiiVec4U32), sizeof(xiiVec4U32), xiiGALStateTransitionMode::None});
    else commands.DrawIndirect({execute.GetBuffer(data.m_DrawCommands),data.m_uiCapacity,0,sizeof(xiiVec4U32),xiiGALStateTransitionMode::None,execute.GetBuffer(data.m_DrawCommandCount)});
    } });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Accumulation, emitted.first->m_Accumulation));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Revealage, emitted.first->m_Revealage));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Additive, emitted.first->m_Additive));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Modulation, emitted.first->m_Modulation));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_HistoryTrust, emitted.first->m_HistoryTrust));
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Transparency_Implementation_TransparentRasterPass);
