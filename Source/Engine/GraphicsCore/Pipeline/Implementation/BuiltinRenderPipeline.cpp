/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineRuntime.h>
#include <GraphicsCore/Pipeline/View.h>

class xiiBuiltinRenderPipelinePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBuiltinRenderPipelinePass, xiiRenderPipelinePass);

public:
  xiiString m_sStage;
  xiiResult Configure(const xiiVariantDictionary& properties) override;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBuiltinRenderPipelinePass, 1, xiiRTTIDefaultAllocator<xiiBuiltinRenderPipelinePass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Stage", m_sStage),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult xiiBuiltinRenderPipelinePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_sStage == "PrepareFrame" || m_sStage == "Visibility" || m_sStage == "Shadows" || m_sStage == "DepthAndGeometry" || m_sStage == "LightingPreparation" || m_sStage == "Lighting" || m_sStage == "OpaqueComposition" || m_sStage == "Transparency" || m_sStage == "ScreenEffects" || m_sStage == "TemporalReconstruction" || m_sStage == "PostProcessing" || m_sStage == "Debug" || m_sStage == "ColorGrading" || m_sStage == "Present" ? XII_SUCCESS : XII_FAILURE;
}
void xiiBuiltinRenderPipelinePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  pins.PushBack({"Before", xiiRenderPipelinePin::Order, true, true});
  pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
  if (m_sStage == "PrepareFrame")
  {
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
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"ContactShadows", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"ExtractedDrawCommands", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"InstanceBounds", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LocalShadowAtlas", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "DepthAndGeometry")
  {
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"CoarseDrawIndirectCommands", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
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
    pins.PushBack({"PlanarReflectionMap", xiiRenderPipelinePin::Texture, false, true});
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
  if (m_sStage == "TemporalReconstruction")
  {
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"LuminanceHistogram", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"Exposure Histogram Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"LuminanceHistogram", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"Exposure Adaptation Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"TAA Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"TAAResolved", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"UpscaledColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"HDRSceneColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"DilatedVelocityBuffer", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"TAAResolved", xiiRenderPipelinePin::Texture, false, true});
  }
  if (m_sStage == "PostProcessing")
  {
    pins.PushBack({"UpscaledColor", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"BloomTexture", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"Bloom Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"BloomTexture", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"CurrentExposure", xiiRenderPipelinePin::Buffer, true, true});
    pins.PushBack({"DisplayLinearColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"Tone Mapping Constants", xiiRenderPipelinePin::Buffer, false, true});
    pins.PushBack({"UpscaledColor", xiiRenderPipelinePin::Texture, false, true});
    pins.PushBack({"CurrentExposure", xiiRenderPipelinePin::Buffer, false, true});
  }
  if (m_sStage == "Debug")
  {
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
}
xiiResult xiiBuiltinRenderPipelinePass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView) return XII_FAILURE;
  auto* runtime = context.m_pView->GetBuiltinRuntime();
  context.m_Graph.ClearSetupBindings();
  for (xiiUInt32 i = 0; i < context.m_Pins.GetCount(); ++i)
  {
    const auto& pin     = context.m_Pins[i];
    const auto& binding = context.m_Bindings[i];
    if (pin.m_bInput && binding.m_hTexture.IsValid()) context.m_Graph.SetSetupBinding(pin.m_sName, binding.m_hTexture);
    if (pin.m_bInput && binding.m_hBuffer.IsValid()) context.m_Graph.SetSetupBinding(pin.m_sName, binding.m_hBuffer);
  }
  const auto result = runtime->BuildStage(m_sStage, context.m_Graph, context.m_Blackboard);
  context.m_Graph.ClearSetupBindings();
  for (xiiUInt32 i = 0; i < context.m_Pins.GetCount(); ++i)
  {
    const auto& pin = context.m_Pins[i];
    if (!pin.m_bInput && pin.m_Type == xiiRenderPipelinePin::Texture) context.m_Bindings[i].m_hTexture = context.m_Graph.FindTexture(pin.m_sName);
    if (!pin.m_bInput && pin.m_Type == xiiRenderPipelinePin::Buffer) context.m_Bindings[i].m_hBuffer = context.m_Graph.FindBuffer(pin.m_sName);
  }
  return result;
}

void xiiCreateDefaultRenderPipeline(xiiRenderPipelineResourceDescriptor& descriptor)
{
  xiiVariantDictionary properties;
  properties.Insert("Stage", "PrepareFrame");
  descriptor.AddPass("PrepareFrame", "xiiBuiltinRenderPipelinePass", properties);
  properties.Insert("Stage", "Visibility");
  descriptor.AddPass("Visibility", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(0, "After", 1, "Before").IgnoreResult();
  properties.Insert("Stage", "Shadows");
  descriptor.AddPass("Shadows", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(1, "After", 2, "Before").IgnoreResult();
  properties.Insert("Stage", "DepthAndGeometry");
  descriptor.AddPass("DepthAndGeometry", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(2, "After", 3, "Before").IgnoreResult();
  properties.Insert("Stage", "LightingPreparation");
  descriptor.AddPass("LightingPreparation", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(3, "After", 4, "Before").IgnoreResult();
  properties.Insert("Stage", "Lighting");
  descriptor.AddPass("Lighting", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(4, "After", 5, "Before").IgnoreResult();
  properties.Insert("Stage", "OpaqueComposition");
  descriptor.AddPass("OpaqueComposition", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(5, "After", 6, "Before").IgnoreResult();
  properties.Insert("Stage", "Transparency");
  descriptor.AddPass("Transparency", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(6, "After", 7, "Before").IgnoreResult();
  properties.Insert("Stage", "ScreenEffects");
  descriptor.AddPass("ScreenEffects", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(7, "After", 8, "Before").IgnoreResult();
  properties.Insert("Stage", "TemporalReconstruction");
  descriptor.AddPass("TemporalReconstruction", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(8, "After", 9, "Before").IgnoreResult();
  properties.Insert("Stage", "PostProcessing");
  descriptor.AddPass("PostProcessing", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(9, "After", 10, "Before").IgnoreResult();
  properties.Insert("Stage", "Debug");
  descriptor.AddPass("Debug", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(10, "After", 11, "Before").IgnoreResult();
  properties.Insert("Stage", "ColorGrading");
  descriptor.AddPass("ColorGrading", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(11, "After", 12, "Before").IgnoreResult();
  properties.Insert("Stage", "Present");
  descriptor.AddPass("Present", "xiiBuiltinRenderPipelinePass", properties);
  descriptor.Connect(12, "After", 13, "Before").IgnoreResult();
  descriptor.Connect(1, "InstanceBounds", 2, "InstanceBounds").IgnoreResult();
  descriptor.Connect(2, "SceneDepth", 3, "SceneDepth").IgnoreResult();
  descriptor.Connect(1, "VisibleCandidates", 3, "VisibleCandidates").IgnoreResult();
  descriptor.Connect(2, "InstanceBounds", 3, "InstanceBounds").IgnoreResult();
  descriptor.Connect(1, "FroxelMetadata", 4, "FroxelMetadata").IgnoreResult();
  descriptor.Connect(1, "FroxelScattering", 4, "FroxelScattering").IgnoreResult();
  descriptor.Connect(3, "SceneDepth", 4, "SceneDepth").IgnoreResult();
  descriptor.Connect(3, "GBufferNormal", 4, "GBufferNormal").IgnoreResult();
  descriptor.Connect(3, "NormalRoughness", 4, "NormalRoughness").IgnoreResult();
  descriptor.Connect(4, "SceneDepth", 5, "SceneDepth").IgnoreResult();
  descriptor.Connect(3, "GBufferAlbedo", 5, "GBufferAlbedo").IgnoreResult();
  descriptor.Connect(4, "GBufferNormal", 5, "GBufferNormal").IgnoreResult();
  descriptor.Connect(3, "GBufferMaterial", 5, "GBufferMaterial").IgnoreResult();
  descriptor.Connect(3, "GBufferEmissive", 5, "GBufferEmissive").IgnoreResult();
  descriptor.Connect(3, "DilatedVelocityBuffer", 5, "DilatedVelocityBuffer").IgnoreResult();
  descriptor.Connect(1, "LightGrid", 5, "LightGrid").IgnoreResult();
  descriptor.Connect(1, "LightIndexBuffer", 5, "LightIndexBuffer").IgnoreResult();
  descriptor.Connect(1, "LightSamplingTotals", 5, "LightSamplingTotals").IgnoreResult();
  descriptor.Connect(2, "LocalShadowAtlas", 5, "LocalShadowAtlas").IgnoreResult();
  descriptor.Connect(2, "LocalShadowAtlasDescs", 5, "LocalShadowAtlasDescs").IgnoreResult();
  descriptor.Connect(4, "RawAO", 5, "RawAO").IgnoreResult();
  descriptor.Connect(2, "ContactShadows", 5, "ContactShadows").IgnoreResult();
  descriptor.Connect(2, "ShadowCascadeMatrices", 5, "ShadowCascadeMatrices").IgnoreResult();
  descriptor.Connect(4, "DDGIIrradiance", 5, "DDGIIrradiance").IgnoreResult();
  descriptor.Connect(4, "SparseVoxelIrradiance", 5, "SparseVoxelIrradiance").IgnoreResult();
  descriptor.Connect(4, "SkyRadiance", 5, "SkyRadiance").IgnoreResult();
  descriptor.Connect(1, "ReflectionProbeData", 5, "ReflectionProbeData").IgnoreResult();
  descriptor.Connect(1, "ReflectionProbeClusters", 5, "ReflectionProbeClusters").IgnoreResult();
  descriptor.Connect(1, "LightingDataReady", 5, "LightingDataReady").IgnoreResult();
  descriptor.Connect(1, "ReflectionProbeConstants", 5, "ReflectionProbeConstants").IgnoreResult();
  descriptor.Connect(4, "AtmTransmittance", 5, "AtmTransmittance").IgnoreResult();
  descriptor.Connect(4, "AtmMultiScatter", 5, "AtmMultiScatter").IgnoreResult();
  descriptor.Connect(3, "GBufferObjectId", 5, "GBufferObjectId").IgnoreResult();
  descriptor.Connect(4, "FroxelScattering", 5, "FroxelScattering").IgnoreResult();
  descriptor.Connect(4, "FroxelMetadata", 5, "FroxelMetadata").IgnoreResult();
  descriptor.Connect(5, "DirectLighting", 6, "DirectLighting").IgnoreResult();
  descriptor.Connect(5, "IndirectLighting", 6, "IndirectLighting").IgnoreResult();
  descriptor.Connect(5, "GBufferEmissive", 6, "GBufferEmissive").IgnoreResult();
  descriptor.Connect(5, "SceneDepth", 6, "SceneDepth").IgnoreResult();
  descriptor.Connect(3, "DrawIndirectCommands", 6, "DrawIndirectCommands").IgnoreResult();
  descriptor.Connect(5, "GBufferMaterial", 6, "GBufferMaterial").IgnoreResult();
  descriptor.Connect(5, "GBufferNormal", 6, "GBufferNormal").IgnoreResult();
  descriptor.Connect(5, "GBufferAlbedo", 6, "GBufferAlbedo").IgnoreResult();
  descriptor.Connect(5, "BRDFLut", 6, "BRDFLut").IgnoreResult();
  descriptor.Connect(3, "HiZClosestPyramid", 6, "HiZClosestPyramid").IgnoreResult();
  descriptor.Connect(3, "HiZPyramid", 6, "HiZPyramid").IgnoreResult();
  descriptor.Connect(5, "GBufferObjectId", 6, "GBufferObjectId").IgnoreResult();
  descriptor.Connect(5, "DilatedVelocityBuffer", 6, "DilatedVelocityBuffer").IgnoreResult();
  descriptor.Connect(5, "ReSTIRDISurfacePrevious", 6, "ReSTIRDISurfacePrevious").IgnoreResult();
  descriptor.Connect(5, "EnvironmentSpecular", 6, "EnvironmentSpecular").IgnoreResult();
  descriptor.Connect(6, "HDRSceneColor", 7, "HDRSceneColor").IgnoreResult();
  descriptor.Connect(6, "SceneDepth", 7, "SceneDepth").IgnoreResult();
  descriptor.Connect(6, "DrawIndirectCommands", 7, "DrawIndirectCommands").IgnoreResult();
  descriptor.Connect(7, "SceneDepth", 8, "SceneDepth").IgnoreResult();
  descriptor.Connect(6, "GBufferNormal", 8, "GBufferNormal").IgnoreResult();
  descriptor.Connect(6, "GBufferAlbedo", 8, "GBufferAlbedo").IgnoreResult();
  descriptor.Connect(6, "GBufferMaterial", 8, "GBufferMaterial").IgnoreResult();
  descriptor.Connect(7, "HDRSceneColor", 8, "HDRSceneColor").IgnoreResult();
  descriptor.Connect(6, "GBufferEmissive", 8, "GBufferEmissive").IgnoreResult();
  descriptor.Connect(5, "AtmTransmittance", 8, "AtmTransmittance").IgnoreResult();
  descriptor.Connect(5, "AtmMultiScatter", 8, "AtmMultiScatter").IgnoreResult();
  descriptor.Connect(6, "DilatedVelocityBuffer", 8, "DilatedVelocityBuffer").IgnoreResult();
  descriptor.Connect(8, "HDRSceneColor", 9, "HDRSceneColor").IgnoreResult();
  descriptor.Connect(8, "DilatedVelocityBuffer", 9, "DilatedVelocityBuffer").IgnoreResult();
  descriptor.Connect(8, "SceneDepth", 9, "SceneDepth").IgnoreResult();
  descriptor.Connect(9, "UpscaledColor", 10, "UpscaledColor").IgnoreResult();
  descriptor.Connect(10, "DisplayLinearColor", 11, "DisplayLinearColor").IgnoreResult();
  descriptor.Connect(9, "SceneDepth", 11, "SceneDepth").IgnoreResult();
  descriptor.Connect(11, "DisplayLinearColor", 12, "DisplayLinearColor").IgnoreResult();
  descriptor.Connect(12, "GradedColor", 13, "GradedColor").IgnoreResult();
}

// Implemented by the built-in pass catalog. No resource handle survives subsystem shutdown.
void                            xiiCreateDefaultRenderPipeline(xiiRenderPipelineResourceDescriptor& descriptor);
xiiRenderPipelineResourceHandle xiiRenderPipelineResource::GetDefaultPipeline()
{
  auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>("{ XII Default Render Pipeline }");
  if (existing.IsValid()) return existing;
  xiiRenderPipelineResourceDescriptor descriptor;
  xiiCreateDefaultRenderPipeline(descriptor);
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>("{ XII Default Render Pipeline }", std::move(descriptor));
}
