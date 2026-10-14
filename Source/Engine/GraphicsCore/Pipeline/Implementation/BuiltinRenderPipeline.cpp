/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/BuiltinRenderPipelinePass.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineRuntime.h>
#include <GraphicsCore/Pipeline/View.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBuiltinRenderPipelinePass, 1, xiiRTTIDefaultAllocator<xiiBuiltinRenderPipelinePass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Stage", m_sStage)->AddAttributes(new xiiDefaultValueAttribute("Bloom")),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiArrayPtr<const xiiStringView> xiiBuiltinRenderPipelinePass::GetStages()
{
  static constexpr xiiStringView stages[] = {"PrepareFrame", "Visibility", "Shadows", "DepthAndGeometry", "LightingPreparation", "Lighting", "OpaqueComposition", "Transparency", "ScreenEffects", "Debug", "ColorGrading", "Present", "LuminanceHistogram", "AutoExposure", "TemporalAntiAliasing", "Upscale", "Bloom", "ToneMapping"};
  return xiiMakeArrayPtr(stages);
}

xiiResult xiiBuiltinRenderPipelinePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  for (const auto stage : GetStages())
    if (m_sStage == stage)
      return XII_SUCCESS;
  return XII_FAILURE;
}

xiiResult xiiBuiltinRenderPipelinePass::ValidatePipeline(const xiiRenderPipelineResourceDescriptor& descriptor, xiiArrayPtr<const xiiUInt32> order, xiiUInt32 index, xiiStringBuilder* error) const
{
  if (m_sStage == "PrepareFrame")
  {
    xiiDisplayOutputSettings settings;
    if (xiiResolvePipelineDisplaySettings(descriptor.m_Parameters, settings).Failed())
    {
      if (error)
        error->Set("Invalid DisplayOutput pipeline parameters.");
      return XII_FAILURE;
    }
  }
  xiiHashSet<xiiString> precedingStages;
  bool                  foundSelf = false;
  for (auto passIndex : order)
  {
    const auto& definition = descriptor.m_Passes[passIndex];
    if (definition.m_sType != "xiiBuiltinRenderPipelinePass")
      continue;
    xiiString  stage = "Bloom";
    xiiVariant value;
    if (definition.m_Properties.TryGetValue("Stage", value))
      stage = value.ConvertTo<xiiString>();
    if (passIndex == index)
    {
      foundSelf = true;
      continue;
    }
    if (stage == m_sStage)
    {
      if (error)
        error->SetFormat("Built-in stage '{}' occurs more than once; use independent custom pass instances for branched effects.", m_sStage);
      return XII_FAILURE;
    }
    if (!foundSelf)
      precedingStages.Insert(stage);
  }
  const auto require = [&](xiiStringView prerequisite) -> xiiResult {
    if (precedingStages.Contains(xiiString(prerequisite)))
      return XII_SUCCESS;
    if (error)
      error->SetFormat("Built-in stage '{}' requires an earlier '{}' stage.", m_sStage, prerequisite);
    return XII_FAILURE;
  };
  if (m_sStage != "PrepareFrame")
    XII_SUCCEED_OR_RETURN(require("PrepareFrame"));
  if (m_sStage == "Shadows" || m_sStage == "DepthAndGeometry" || m_sStage == "Lighting")
    XII_SUCCEED_OR_RETURN(require("Visibility"));
  if (m_sStage == "DepthAndGeometry" || m_sStage == "Lighting")
    XII_SUCCEED_OR_RETURN(require("Shadows"));
  if (m_sStage == "LightingPreparation" || m_sStage == "Lighting")
    XII_SUCCEED_OR_RETURN(require("DepthAndGeometry"));
  if (m_sStage == "Lighting")
    XII_SUCCEED_OR_RETURN(require("LightingPreparation"));
  if (m_sStage == "OpaqueComposition")
    XII_SUCCEED_OR_RETURN(require("Lighting"));
  return XII_SUCCESS;
}
void xiiBuiltinRenderPipelinePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  pins.PushBack({"Before", xiiRenderPipelinePin::Order, true, true});
  pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
  if (m_sStage == "PrepareFrame")
  {
    pins.PushBack({"GPU Geometry Metadata", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"GPU Meshlet Metadata", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"Surface Material GPU Storage", xiiRenderPipelinePin::Buffer, false, true});
  }
  if (m_sStage == "Visibility")
  {
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
  if (m_sStage == "OpaqueComposition")
  {
    pins.PushBack({"PlanarReflectionMap", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DirectLighting", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"IndirectLighting", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RTFinalGI", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"VolumetricScattering", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DrawIndirectCommands", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"PlanarReflectionMap", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"BRDFLut", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"HiZClosestPyramid", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"HiZPyramid", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SSRRawTerm", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"xiiSSRConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"SSRRawTerm", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferObjectId", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ReSTIRDISurfacePrevious", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"xiiSSRTemporalConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"EnvironmentSpecular", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SSRTerm", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RTFinalReflections", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DirectLighting", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"IndirectLighting", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"RTFinalGI", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"VolumetricScattering", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DrawIndirectCommands", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"BRDFLut", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"HiZClosestPyramid", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"HiZPyramid", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferObjectId", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ReSTIRDISurfacePrevious", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"EnvironmentSpecular", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SSRTerm", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"RTFinalReflections", xiiRenderPipelinePin::Texture, false, true});
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
  if (m_sStage == "ScreenEffects")
  {
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SSGITerm", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"xiiSSGIConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"SSGITerm", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RefractionSceneColorInput", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"RefractionSceneColorInput", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"xiiSSRefractionConstants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"AtmTransmittance", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"AtmMultiScatter", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"Sensor Output Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferNormal", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferAlbedo", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferMaterial", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"GBufferEmissive", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"AtmTransmittance", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"AtmMultiScatter", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "Debug")
  {
    for (xiiStringView name : {"GBufferAlbedo", "GBufferNormal", "GBufferMaterial", "GBufferEmissive", "GBufferObjectId", "DilatedVelocityBuffer", "StableAO", "IndirectLighting", "EnvironmentSpecular", "BloomTexture"})
      pins.PushBack({name, xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DisplayLinearColor", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"DisplayLinearColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "ColorGrading")
  {
    pins.PushBack({"DisplayLinearColor", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"GradedColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"Color Grading Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DisplayLinearColor", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "Present")
  {
    pins.PushBack({"GradedColor", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"Final Blit Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"GradedColor", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "LuminanceHistogram")
  {
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, false});
    pins.PushBack({"LuminanceHistogram", xiiRenderPipelinePin::Buffer, false, false});
  }

  if (m_sStage == "AutoExposure")
  {
    pins.PushBack({"LuminanceHistogram", xiiRenderPipelinePin::Buffer, true, false});
    pins.PushBack({"CurrentExposure", xiiRenderPipelinePin::Buffer, false, false});
  }

  if (m_sStage == "TemporalAntiAliasing")
  {
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, false});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, true, false});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, false});
    pins.PushBack({"TAAResolved", xiiRenderPipelinePin::Texture, false, false});
  }

  if (m_sStage == "Upscale")
  {
    pins.PushBack({"TAAResolved", xiiRenderPipelinePin::Texture, true, false});
    pins.PushBack({"UpscaledColor", xiiRenderPipelinePin::Texture, false, false});
  }

  if (m_sStage == "Bloom")
  {
    pins.PushBack({"UpscaledColor", xiiRenderPipelinePin::Texture, true, false});
    pins.PushBack({"BloomTexture", xiiRenderPipelinePin::Texture, false, false});
  }

  if (m_sStage == "ToneMapping")
  {
    pins.PushBack({"UpscaledColor", xiiRenderPipelinePin::Texture, true, false});
    pins.PushBack({"BloomTexture", xiiRenderPipelinePin::Texture, true, false});
    pins.PushBack({"CurrentExposure", xiiRenderPipelinePin::Buffer, true, false});
    pins.PushBack({"DisplayLinearColor", xiiRenderPipelinePin::Texture, false, false});
  }
  if (m_sStage == "ColorGrading" || m_sStage == "Present")
    for (auto& pin : pins)
      if (pin.m_Type == xiiRenderPipelinePin::Texture && pin.m_bInput)
        pin.m_bOptional = false;
}
xiiResult xiiBuiltinRenderPipelinePass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView)
    return XII_FAILURE;
  auto* runtime = context.m_pView->GetBuiltinRuntime();
  if (m_sStage == "PrepareFrame")
  {
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
      context.m_Bindings[i].m_hTexture = context.m_Graph.FindTexture(pin.m_sName);
    if (!pin.m_bInput && pin.m_Type == xiiRenderPipelinePin::Buffer)
      context.m_Bindings[i].m_hBuffer = context.m_Graph.FindBuffer(pin.m_sName);
  }
  return result;
}

void xiiRenderPipelineResource::GetDefaultDescriptor(xiiRenderPipelineResourceDescriptor& descriptor)
{
  descriptor                                  = {};
  static constexpr xiiStringView     stages[] = {"PrepareFrame", "Visibility", "Shadows", "DepthAndGeometry", "LightingPreparation", "Lighting", "OpaqueComposition", "Transparency", "ScreenEffects", "LuminanceHistogram", "AutoExposure", "TemporalAntiAliasing", "Upscale", "Bloom", "ToneMapping", "Debug", "ColorGrading", "Present"};
  xiiHashTable<xiiString, xiiUInt32> textures, buffers;
  for (xiiUInt32 index = 0; index < XII_ARRAY_SIZE(stages); ++index)
  {
    xiiVariantDictionary properties;
    properties.Insert("Stage", stages[index]);
    descriptor.AddPass(stages[index], "xiiBuiltinRenderPipelinePass", properties);
    if (index > 0)
      descriptor.Connect(index - 1, "After", index, "Before").IgnoreResult();
    xiiBuiltinRenderPipelinePass prototype;
    prototype.m_sStage = stages[index];
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype.GetPins(pins);
    for (const auto& pin : pins)
    {
      if (!pin.m_bInput || pin.m_Type == xiiRenderPipelinePin::Order)
        continue;
      const auto& producers = pin.m_Type == xiiRenderPipelinePin::Texture ? textures : buffers;
      xiiUInt32   source;
      if (producers.TryGetValue(pin.m_sName, source))
        descriptor.Connect(source, pin.m_sName, index, pin.m_sName).IgnoreResult();
    }
    for (const auto& pin : pins)
    {
      if (pin.m_bInput || pin.m_Type == xiiRenderPipelinePin::Order)
        continue;
      auto& producers = pin.m_Type == xiiRenderPipelinePin::Texture ? textures : buffers;
      producers.Insert(pin.m_sName, index);
    }
  }
}

xiiRenderPipelineResourceHandle xiiRenderPipelineResource::GetDefaultPipeline()
{
  auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>("{ XII Default Render Pipeline }");
  if (existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor descriptor;
  GetDefaultDescriptor(descriptor);
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>("{ XII Default Render Pipeline }", std::move(descriptor));
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_BuiltinRenderPipeline);
