/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/BuiltinRenderPipelinePass.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineRuntime.h>
#include <GraphicsCore/Pipeline/View.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBuiltinRenderPipelinePass, 6, xiiRTTIDefaultAllocator<xiiBuiltinRenderPipelinePass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("RTFinalGI", m_OutputRTFinalGI)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("RTFinalReflections", m_OutputRTFinalReflections)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("VolumetricScattering", m_OutputVolumetricScattering)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Stage", m_sStage)->AddAttributes(new xiiDefaultValueAttribute("PrepareFrame")),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiBitflags<xiiRenderExtractionCategory> xiiBuiltinRenderPipelinePass::GetExtractionRequirements() const
{
  if (m_sStage == "Visibility" || m_sStage == "Shadows")
    return xiiRenderExtractionCategory::Geometry | xiiRenderExtractionCategory::Lights | xiiRenderExtractionCategory::Decals;
  if (m_sStage == "DepthAndGeometry")
    return xiiRenderExtractionCategory::Geometry;
  if (m_sStage == "LightingPreparation" || m_sStage == "Lighting")
    return xiiRenderExtractionCategory::Lights | xiiRenderExtractionCategory::Atmosphere | xiiRenderExtractionCategory::Geometry;
  if (m_sStage == "Transparency")
    return xiiRenderExtractionCategory::Geometry | xiiRenderExtractionCategory::Particles | xiiRenderExtractionCategory::Decals;
  return {};
}

xiiBitflags<xiiRenderPipelineFeature> xiiBuiltinRenderPipelinePass::GetRuntimeFeatures() const
{
  if (m_sStage == "Visibility" || m_sStage == "DepthAndGeometry" || m_sStage == "Transparency")
    return xiiRenderPipelineFeature::GpuScene;
  if (m_sStage == "Shadows")
    return xiiRenderPipelineFeature::GpuScene | xiiRenderPipelineFeature::VirtualShadows | xiiRenderPipelineFeature::Lighting | xiiRenderPipelineFeature::RayTracing;
  if (m_sStage == "LightingPreparation" || m_sStage == "Lighting")
    return xiiRenderPipelineFeature::Lighting | xiiRenderPipelineFeature::RayTracing | xiiRenderPipelineFeature::GpuScene;
  return {};
}

xiiResult xiiBuiltinRenderPipelinePass::ValidatePipeline(const xiiRenderPipelineResourceDescriptor& descriptor, xiiArrayPtr<const xiiUInt32> order, xiiUInt32 index, xiiStringBuilder* error) const
{
  if (m_sStage == "PrepareFrame")
  {
    xiiDisplayOutputSettings settings;
    if (xiiResolvePipelineDisplaySettings(descriptor.m_Passes[index].m_Parameters, settings).Failed())
    {
      if (error)
        error->Set("Invalid DisplayOutput pipeline parameters.");
      return XII_FAILURE;
    }
  }
  return XII_SUCCESS;
}
void xiiBuiltinRenderPipelinePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  pins.PushBack({"Before", xiiRenderPipelinePin::Order, true, true});
  pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
  if (m_sStage == "PrepareFrame")
  {
    for (xiiStringView name : {"SceneInstances", "SkinningMatrices", "VisibleMeshlets", "VisibleMeshletCount", "MeshIndirectCommands", "MeshIndirectCommandCount", "GeometryMetadata", "MeshletMetadata", "SurfaceMaterials"})
      pins.PushBack({name, xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"GPU Geometry Metadata", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"GPU Meshlet Metadata", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"Surface Material GPU Storage", xiiRenderPipelinePin::Buffer, false, true});
  }
  if (m_sStage == "Visibility")
  {
    pins.PushBack({"PerFrameCamera", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"PerFrameLight", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"PerFrameGlobal", xiiRenderPipelinePin::Buffer, false, true});

    pins.PushBack({"LightingDataReady", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"VisibleCandidates", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"xiiFrustumCullingConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"VisibleCandidates", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"ExtractedLODMetadata", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"InstanceLOD", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"xiiLODSelectionConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ExtractedInstanceSource", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"InstanceBounds", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"xiiInstanceUpdateConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LightingDataReady", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"ClusterDescriptors", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"xiiLightClusteringConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"xiiLightClusteringConstants", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"LightGrid", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ClusterDescriptors", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"LightIndexBuffer", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LightGrid", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"LightIndexBuffer", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"LightSamplingTotals", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ReflectionProbeData", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ReflectionProbeConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ReflectionProbeClusters", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"FroxelMetadata", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"FroxelScattering", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"Volumetric Media", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"Volumetric Medium Constants", xiiRenderPipelinePin::Buffer, false, true});
  }
  if (m_sStage == "Shadows")
  {
    pins.PushBack({"ShadowCascadeMatrices", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ExtractedDrawCommands", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"InstanceBounds", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"ShadowCascadeMatrices", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DrawShadowCasterCommands", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ShadowCasterCommandCounts", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"xiiShadowCasterCullingConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LocalShadowAtlasDescs", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DrawShadowCasterCommands", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"LocalShadowAtlasDescs", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"LocalShadowAtlas", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ExtractedDrawCommands", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"InstanceBounds", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LocalShadowAtlas", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "DepthAndGeometry")
  {
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"CoarseDrawIndirectCommands", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"HiZPyramid", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"HiZClosestPyramid", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"xiiHiZBuildConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"HiZPyramid", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"VisibleCandidates", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"InstanceBounds", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"SurvivingInstances", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"xiiHiZOcclusionConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"VelocityBuffer", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"Motion Vector Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DrawIndirectCommands", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferObjectId", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"NormalRoughness", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"VelocityBuffer", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DecalData", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DecalData", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DecalVisibleList", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DecalTileList", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DecalDrawCommands", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DecalTileList", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DecalAtlasAlbedo", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DecalAtlasNormal", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DecalAtlasMaterial", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DecalAtlasEmissive", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DecalDrawCommands", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"CoarseDrawIndirectCommands", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"VisibleCandidates", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"InstanceBounds", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DrawIndirectCommands", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DecalAtlasAlbedo", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DecalAtlasNormal", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DecalAtlasMaterial", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DecalAtlasEmissive", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ContactShadows", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "LightingPreparation")
  {
    pins.PushBack({"AtmTransmittance", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"AtmMultiScatter", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SkyRadiance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"FroxelMetadata", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"FroxelScattering", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SparseVoxelRadiancePool", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"SparseVoxelPageTable", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"SparseVoxelLevelData", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"SparseVoxelConstants", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"SparseVoxelIrradiance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DDGIProbeIrradianceAtlas", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DDGIProbeDistanceAtlas", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DDGIProbeStates", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DDGIProbeConstants", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DDGIIrradiance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"NormalRoughness", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RawAO", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"AtmTransmittance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"AtmMultiScatter", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"FroxelMetadata", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"FroxelScattering", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SparseVoxelRadiancePool", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"SparseVoxelPageTable", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"SparseVoxelLevelData", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"SparseVoxelConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DDGIProbeIrradianceAtlas", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DDGIProbeDistanceAtlas", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DDGIProbeStates", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DDGIProbeConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"NormalRoughness", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "Lighting")
  {
    xiiDynamicArray<xiiRenderPipelinePin> reflected;
    xiiRenderPipelinePass::GetPins(reflected);
    for (auto& pin : reflected)
      if (pin.m_Type != xiiRenderPipelinePin::Order)
        pins.PushBack(std::move(pin));

    pins.PushBack({"HardwareGI", xiiRenderPipelinePin::Texture, false, true});

    pins.PushBack({"CloudShadowConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"VirtualShadowSamplingConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"LightGrid", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"LightIndexBuffer", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"LightSamplingTotals", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"xiiReSTIRDIConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ReSTIRDITemporalReservoir", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ReSTIRDITemporalReservoir", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"LocalShadowAtlas", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"LocalShadowAtlasDescs", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DirectLightReservoir", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RTRawShadows", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"RTRawShadows", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ReSTIRDISurfacePrevious", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RawAO", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RTFinalShadows", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ContactShadows", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ShadowCascadeMatrices", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DirShadowAtlas", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DirectLightReservoirSurface", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"CloudShadowConstants", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"VirtualShadowPageTable", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"VirtualShadowAtlas", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"VirtualShadowSamplingConstants", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DirectLighting", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"StableAO", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"BRDFLut", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DDGIIrradiance", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SparseVoxelIrradiance", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SkyRadiance", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ReflectionProbeData", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"ReflectionProbeClusters", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"IndirectLighting", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"EnvironmentSpecular", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"LightingDataReady", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"RTRawGI", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ReflectionProbeConstants", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"AtmTransmittance", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"AtmMultiScatter", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RTRawGI", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ReSTIR GI Temporal Sample", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ReSTIR GI Temporal State", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ReSTIR GI Temporal Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ReSTIR GI Temporal Sample", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ReSTIR GI Temporal State", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RTResampledGI", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ReSTIR GI Spatial Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"RTResampledGI", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RTRawReflections", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"RTRawReflectionHitDistance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"RTRawReflections", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RTRawReflectionHitDistance", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferObjectId", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"xiiRTReflectionDenoiseConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"FroxelScattering", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"FroxelMetadata", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"FroxelIntegrated", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"FroxelIntegrated", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"VolumetricScatteringRaw", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"VolumetricScatteringRaw", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"Volumetric Temporal Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"LightGrid", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LightIndexBuffer", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LightSamplingTotals", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LocalShadowAtlas", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"LocalShadowAtlasDescs", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DirectLightReservoir", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ReSTIRDISurfacePrevious", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"RawAO", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"RTFinalShadows", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ContactShadows", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ShadowCascadeMatrices", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DirShadowAtlas", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DirectLightReservoirSurface", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"VirtualShadowPageTable", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"VirtualShadowAtlas", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"StableAO", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"BRDFLut", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DDGIIrradiance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SparseVoxelIrradiance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SkyRadiance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ReflectionProbeData", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ReflectionProbeClusters", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LightingDataReady", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"ReflectionProbeConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"AtmTransmittance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"AtmMultiScatter", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferObjectId", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"FroxelScattering", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"FroxelMetadata", xiiRenderPipelinePin::Buffer, false, true});
  }

  if (m_sStage == "Transparency")
  {
    pins.PushBack({"ParticleSimConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DrawIndirectCommands", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"OITAccumulate", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"OITReveal", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DrawIndirectCommands", xiiRenderPipelinePin::Buffer, false, true});
  }
}
xiiResult xiiBuiltinRenderPipelinePass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView)
    return XII_FAILURE;
  auto* rootRuntime                 = context.m_pView->GetBuiltinRuntime();
  auto* runtime                     = rootRuntime->GetSubgraphRuntime(context.m_sInstanceScope);
  runtime->m_ExtractionRequirements = context.m_ExtractionRequirements;
  runtime->m_RuntimeFeatures        = context.m_RuntimeFeatures;
  if (m_sStage == "PrepareFrame")
  {
    if (runtime != rootRuntime)
      runtime->BeginFrame(rootRuntime->m_Frame.virtualShadowHandles);
    XII_SUCCEED_OR_RETURN(runtime->PrepareInstanceResources());
    runtime->m_DisplayOutputSettings = context.m_pView->GetDisplayOutputSettings();
    XII_SUCCEED_OR_RETURN(xiiResolvePipelineDisplaySettings(context.m_Parameters, runtime->m_DisplayOutputSettings));
  }
  context.m_Graph.ClearSetupBindings();
  for (xiiUInt32 i = 0; i < context.m_Pins.GetCount(); ++i)
  {
    const auto& pin     = context.m_Pins[i];
    const auto& binding = context.m_Bindings[i];
    if (pin.m_bInput && binding.m_hTexture.IsValid())
      context.m_Graph.SetSetupBinding(pin.m_sName, binding.m_hTexture);
    if (pin.m_bInput && binding.m_hBuffer.IsValid())
      context.m_Graph.SetSetupBinding(pin.m_sName, binding.m_hBuffer);
  }

  const auto result = runtime->BuildStage(m_sStage, context.m_Graph, context.m_Blackboard);
  context.m_Graph.ClearSetupBindings();
  for (xiiUInt32 i = 0; i < context.m_Pins.GetCount(); ++i)
  {
    const auto& pin = context.m_Pins[i];
    if (!pin.m_bInput && pin.m_Type == xiiRenderPipelinePin::Texture)
      context.m_Bindings[i].m_hTexture = pin.m_sName == "HDRSceneColor" && runtime->m_Frame.hTransparencyColor.IsValid() ? runtime->m_Frame.hTransparencyColor : pin.m_sName == "HardwareGI" ? (runtime->m_Resources.m_LightingPasses.m_bRTGIAvailableThisFrame ? context.m_Graph.FindTexture("RTRawGI") : xiiRenderGraphTextureHandle()) :
                                                                                                                                                                                               context.m_Graph.FindTexture(pin.m_sName);
    if (!pin.m_bInput && pin.m_Type == xiiRenderPipelinePin::Buffer)
      context.m_Bindings[i].m_hBuffer = context.m_Graph.FindBuffer(pin.m_sName);
  }
  if (m_sStage == "PrepareFrame")
  {
    const xiiString previousScope = context.m_Graph.GetResourceNamespace();
    context.m_Graph.SetResourceNamespace({});
    const auto& frame = runtime->m_Frame;
    for (xiiUInt32 i = 0; i < context.m_Pins.GetCount(); ++i)
    {
      const auto& pin = context.m_Pins[i];
      if (pin.m_bInput || pin.m_Type != xiiRenderPipelinePin::Buffer)
        continue;
      auto& buffer = context.m_Bindings[i].m_hBuffer;
      if (pin.m_sName == "SceneInstances")
        buffer = frame.mainVisibility.m_hSceneInstances;
      else if (pin.m_sName == "SkinningMatrices")
        buffer = frame.mainVisibility.m_hSkinningMatrices;
      else if (pin.m_sName == "VisibleMeshlets")
        buffer = frame.mainVisibility.m_hVisibleMeshlets;
      else if (pin.m_sName == "VisibleMeshletCount")
        buffer = frame.mainVisibility.m_hVisibleMeshletCount;
      else if (pin.m_sName == "MeshIndirectCommands")
        buffer = frame.mainVisibility.m_hIndirectCommands;
      else if (pin.m_sName == "MeshIndirectCommandCount")
        buffer = frame.mainVisibility.m_hIndirectCommandCount;
      else if (pin.m_sName == "GeometryMetadata")
        buffer = frame.geometry.m_hGeometryMetadata;
      else if (pin.m_sName == "MeshletMetadata")
        buffer = frame.geometry.m_hMeshletMetadata;
      else if (pin.m_sName == "SurfaceMaterials")
        buffer = frame.materials.m_hSurfaceData;
      else
        buffer = context.m_Graph.FindBuffer(pin.m_sName);
    }
    context.m_Graph.SetResourceNamespace(previousScope);
  }

  return result;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_BuiltinRenderPipeline);
