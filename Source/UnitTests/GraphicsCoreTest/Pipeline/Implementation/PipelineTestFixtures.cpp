/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCoreTest/Pipeline/PipelineTestFixtures.h>
#include <initializer_list>
#include <utility>

namespace
{
  xiiVariantDictionary Properties(std::initializer_list<std::pair<const char*, xiiVariant>> values)
  {
    xiiVariantDictionary result;
    for (const auto& value : values)
      result.Insert(value.first, value.second);
    return result;
  }
} // namespace

xiiRenderPipelineResourceHandle xiiCreateAmbientOcclusionTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/AmbientOcclusion";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(10817180464166299532ULL, 4511769226720793077ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("AmbientOcclusionTemporalPass 1", xiiRTTI::FindTypeByName("xiiAmbientOcclusionTemporalPass"));
  definition.m_Passes[0].m_Id = xiiUuid(16518977122320713266ULL, 2938839651578200626ULL);
  definition.AddPass("AmbientOcclusionPass 1", xiiRTTI::FindTypeByName("xiiAmbientOcclusionPass"));
  definition.m_Passes[1].m_Id = xiiUuid(2277786167653236592ULL, 13231530913296876199ULL);
  definition.Connect(1, "RawAO", 0, "RawAO").AssertSuccess();
  definition.m_Interface.PushBack({"NormalRoughness", true, xiiUuid(2277786167653236592ULL, 13231530913296876199ULL), "NormalRoughness"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(2277786167653236592ULL, 13231530913296876199ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(16518977122320713266ULL, 2938839651578200626ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"RayTracingGeometryData", true, xiiUuid(2277786167653236592ULL, 13231530913296876199ULL), "RayTracingGeometryData"});
  definition.m_Interface.PushBack({"RayTracingMaterialData", true, xiiUuid(2277786167653236592ULL, 13231530913296876199ULL), "RayTracingMaterialData"});
  definition.m_Interface.PushBack({"DilatedVelocityBuffer", true, xiiUuid(16518977122320713266ULL, 2938839651578200626ULL), "DilatedVelocityBuffer"});
  definition.m_Interface.PushBack({"GBufferNormal", true, xiiUuid(16518977122320713266ULL, 2938839651578200626ULL), "GBufferNormal"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(16518977122320713266ULL, 2938839651578200626ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(2277786167653236592ULL, 13231530913296876199ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"PreviousAO", false, xiiUuid(16518977122320713266ULL, 2938839651578200626ULL), "PreviousAO"});
  definition.m_Interface.PushBack({"PerFrameGlobal", true, xiiUuid(2277786167653236592ULL, 13231530913296876199ULL), "PerFrameGlobal"});
  definition.m_Interface.PushBack({"PerFrameGlobal", true, xiiUuid(16518977122320713266ULL, 2938839651578200626ULL), "PerFrameGlobal"});
  definition.m_Interface.PushBack({"RayTracingScene", true, xiiUuid(2277786167653236592ULL, 13231530913296876199ULL), "RayTracingScene"});
  definition.m_Interface.PushBack({"RawAO", false, xiiUuid(2277786167653236592ULL, 13231530913296876199ULL), "RawAO"});
  definition.m_Interface.PushBack({"StableAO", false, xiiUuid(16518977122320713266ULL, 2938839651578200626ULL), "StableAO"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateAtmosphereApplyTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/AtmosphereApply";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(7553243271867203308ULL, 8087766272731247487ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("AtmosphereApplyPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereApplyPass"));
  definition.m_Passes[0].m_Id = xiiUuid(739759169137089504ULL, 2062648204206868003ULL);
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(739759169137089504ULL, 2062648204206868003ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(739759169137089504ULL, 2062648204206868003ULL), "Radiance"});
  definition.m_Interface.PushBack({"MultiScatter", true, xiiUuid(739759169137089504ULL, 2062648204206868003ULL), "MultiScatter"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(739759169137089504ULL, 2062648204206868003ULL), "Camera"});
  definition.m_Interface.PushBack({"Atmosphere", true, xiiUuid(739759169137089504ULL, 2062648204206868003ULL), "Atmosphere"});
  definition.m_Interface.PushBack({"AtmosphereRadiance", false, xiiUuid(739759169137089504ULL, 2062648204206868003ULL), "AtmosphereRadiance"});
  definition.m_Interface.PushBack({"Transmittance", true, xiiUuid(739759169137089504ULL, 2062648204206868003ULL), "Transmittance"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(739759169137089504ULL, 2062648204206868003ULL), "Depth"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateAtmosphereLookupTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/AtmosphereLookup";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(15773300109601246685ULL, 32687901187023269ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("AtmosphereMultiScatterPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereMultiScatterPass"));
  definition.m_Passes[0].m_Id = xiiUuid(5111712283722787470ULL, 4130465445640748481ULL);
  definition.AddPass("AtmosphereTransmittancePass 1", xiiRTTI::FindTypeByName("xiiAtmosphereTransmittancePass"));
  definition.m_Passes[1].m_Id = xiiUuid(10300144591585318467ULL, 7058527724338516345ULL);
  definition.AddPass("AtmosphereConstantsPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereConstantsPass"));
  definition.m_Passes[2].m_Id = xiiUuid(11441899366409579898ULL, 8884235238908248405ULL);
  definition.Connect(1, "Transmittance", 0, "Transmittance").AssertSuccess();
  definition.Connect(2, "AtmosphereConstants", 1, "Atmosphere").AssertSuccess();
  definition.Connect(2, "AtmosphereConstants", 0, "Atmosphere").AssertSuccess();
  definition.m_Interface.PushBack({"Transmittance", false, xiiUuid(10300144591585318467ULL, 7058527724338516345ULL), "Transmittance"});
  definition.m_Interface.PushBack({"Atmosphere", false, xiiUuid(11441899366409579898ULL, 8884235238908248405ULL), "AtmosphereConstants"});
  definition.m_Interface.PushBack({"MultiScatter", false, xiiUuid(5111712283722787470ULL, 4130465445640748481ULL), "MultiScatter"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateAtmosphereMultiScatterTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/AtmosphereMultiScatter";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(12368691981169640282ULL, 1520504578894176723ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("AtmosphereMultiScatterPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereMultiScatterPass"));
  definition.m_Passes[0].m_Id = xiiUuid(18237424774978946331ULL, 11884225369570519303ULL);
  definition.m_Interface.PushBack({"Atmosphere", true, xiiUuid(18237424774978946331ULL, 11884225369570519303ULL), "Atmosphere"});
  definition.m_Interface.PushBack({"MultiScatter", false, xiiUuid(18237424774978946331ULL, 11884225369570519303ULL), "MultiScatter"});
  definition.m_Interface.PushBack({"Transmittance", true, xiiUuid(18237424774978946331ULL, 11884225369570519303ULL), "Transmittance"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateAtmosphereTransmittanceTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/AtmosphereTransmittance";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(18169156887748205256ULL, 18310698952513035253ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("AtmosphereTransmittancePass 1", xiiRTTI::FindTypeByName("xiiAtmosphereTransmittancePass"));
  definition.m_Passes[0].m_Id = xiiUuid(11658385687369790198ULL, 2966698990205521503ULL);
  definition.m_Interface.PushBack({"Atmosphere", true, xiiUuid(11658385687369790198ULL, 2966698990205521503ULL), "Atmosphere"});
  definition.m_Interface.PushBack({"Transmittance", false, xiiUuid(11658385687369790198ULL, 2966698990205521503ULL), "Transmittance"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateBufferExportTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/BufferExport";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(2352668974862133132ULL, 13139274231003560350ULL);
  definition.AddPass("BufferExportPass 1", xiiRTTI::FindTypeByName("xiiBufferExportPass"));
  definition.m_Passes[0].m_Id = xiiUuid(2564864292385371529ULL, 21749418531907388ULL);
  definition.m_Interface.PushBack({"Output", false, xiiUuid(2564864292385371529ULL, 21749418531907388ULL), "Output"});
  definition.m_Interface.PushBack({"Input", true, xiiUuid(2564864292385371529ULL, 21749418531907388ULL), "Input"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}



xiiRenderPipelineResourceHandle xiiCreateClusterLightListsTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ClusterLightLists";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(1405144565159600166ULL, 11795138568450683188ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("LightClusterViewPass 1", xiiRTTI::FindTypeByName("xiiLightClusterViewPass"));
  definition.m_Passes[0].m_Id = xiiUuid(15212207397050017475ULL, 1066538985502179835ULL);
  definition.AddPass("ClusterListClearPass 1", xiiRTTI::FindTypeByName("xiiClusterListClearPass"));
  definition.m_Passes[1].m_Id = xiiUuid(18099982298149125465ULL, 3954580528648133472ULL);
  definition.AddPass("ClusterBoundsPass 1", xiiRTTI::FindTypeByName("xiiClusterBoundsPass"));
  definition.m_Passes[2].m_Id = xiiUuid(12830612456816546228ULL, 6570216533743022366ULL);
  definition.AddPass("ClusterLightSamplingPass 1", xiiRTTI::FindTypeByName("xiiClusterLightSamplingPass"));
  definition.m_Passes[3].m_Id = xiiUuid(2957997467281777084ULL, 10292504118088325146ULL);
  definition.AddPass("ClusterLightAssignmentPass 1", xiiRTTI::FindTypeByName("xiiClusterLightAssignmentPass"));
  definition.m_Passes[4].m_Id = xiiUuid(7141894210070365168ULL, 13409534750007394259ULL);
  definition.Connect(0, "Layout", 2, "Layout").AssertSuccess();
  definition.Connect(1, "LightGrid", 4, "InitializedGrid").AssertSuccess();
  definition.Connect(2, "Clusters", 4, "Clusters").AssertSuccess();
  definition.Connect(1, "LightIndices", 4, "InitializedIndices").AssertSuccess();
  definition.Connect(0, "Layout", 1, "Layout").AssertSuccess();
  definition.Connect(0, "Layout", 4, "Layout").AssertSuccess();
  definition.Connect(4, "LightGrid", 3, "LightGrid").AssertSuccess();
  definition.Connect(0, "Layout", 3, "Layout").AssertSuccess();
  definition.Connect(1, "Overflow", 4, "InitializedOverflow").AssertSuccess();
  definition.Connect(4, "LightIndices", 3, "LightIndices").AssertSuccess();
  definition.m_Interface.PushBack({"Lights", true, xiiUuid(2957997467281777084ULL, 10292504118088325146ULL), "Lights"});
  definition.m_Interface.PushBack({"Lights", true, xiiUuid(7141894210070365168ULL, 13409534750007394259ULL), "Lights"});
  definition.m_Interface.PushBack({"Overflow", false, xiiUuid(7141894210070365168ULL, 13409534750007394259ULL), "Overflow"});
  definition.m_Interface.PushBack({"LightGrid", false, xiiUuid(7141894210070365168ULL, 13409534750007394259ULL), "LightGrid"});
  definition.m_Interface.PushBack({"PackedIndices", false, xiiUuid(2957997467281777084ULL, 10292504118088325146ULL), "PackedIndices"});
  definition.m_Interface.PushBack({"SamplingTotals", false, xiiUuid(2957997467281777084ULL, 10292504118088325146ULL), "SamplingTotals"});
  definition.m_Interface.PushBack({"Candidates", true, xiiUuid(7141894210070365168ULL, 13409534750007394259ULL), "Candidates"});
  definition.m_Interface.PushBack({"LightIndices", false, xiiUuid(7141894210070365168ULL, 13409534750007394259ULL), "LightIndices"});
  definition.m_Interface.PushBack({"Clusters", false, xiiUuid(12830612456816546228ULL, 6570216533743022366ULL), "Clusters"});
  definition.m_Interface.PushBack({"Layout", false, xiiUuid(15212207397050017475ULL, 1066538985502179835ULL), "Layout"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}



xiiRenderPipelineResourceHandle xiiCreateDDGIEnvironmentProbesTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DDGIEnvironmentProbes";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(14923011842101368261ULL, 2203593717265797155ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("DDGIEnvironmentSeedPass 1", xiiRTTI::FindTypeByName("xiiDDGIEnvironmentSeedPass"));
  definition.m_Passes[0].m_Id = xiiUuid(10609997558256114642ULL, 3623139168192557810ULL);
  definition.AddPass("TextureHistoryPass 1", xiiRTTI::FindTypeByName("xiiTextureHistoryPass"), Properties({{"AsyncCopy", false}}));
  definition.m_Passes[1].m_Id = xiiUuid(11458673393902121076ULL, 4143128643786354541ULL);
  definition.AddPass("DDGIAtlasPreparePass 1", xiiRTTI::FindTypeByName("xiiDDGIAtlasPreparePass"));
  definition.m_Passes[2].m_Id = xiiUuid(8539252164757054687ULL, 5093176988579977588ULL);
  definition.AddPass("TextureHistoryPass 2", xiiRTTI::FindTypeByName("xiiTextureHistoryPass"), Properties({{"AsyncCopy", false}}));
  definition.m_Passes[3].m_Id = xiiUuid(6452824439085057937ULL, 6530480044763222482ULL);
  definition.AddPass("DDGIProbeUploadPass 1", xiiRTTI::FindTypeByName("xiiDDGIProbeUploadPass"));
  definition.m_Passes[4].m_Id = xiiUuid(13732997665415316893ULL, 7367359516963588889ULL);
  definition.AddPass("DDGIProbeSchedulePass 1", xiiRTTI::FindTypeByName("xiiDDGIProbeSchedulePass"));
  definition.m_Passes[5].m_Id = xiiUuid(6527658302303941476ULL, 12833624199530291004ULL);
  definition.Connect(2, "Distance", 0, "InitialDistance").AssertSuccess();
  definition.Connect(1, "PreviousTexture", 5, "PreviousIrradiance").AssertSuccess();
  definition.Connect(0, "Irradiance", 1, "Texture").AssertSuccess();
  definition.Connect(0, "Distance", 3, "Texture").AssertSuccess();
  definition.Connect(2, "Irradiance", 0, "InitialIrradiance").AssertSuccess();
  definition.Connect(4, "ProbeConstants", 0, "ProbeConstants").AssertSuccess();
  definition.Connect(5, "Schedule", 4, "Schedule").AssertSuccess();
  definition.Connect(4, "ProbeUpdates", 0, "ProbeUpdates").AssertSuccess();
  definition.Connect(5, "Schedule", 0, "Schedule").AssertSuccess();
  definition.Connect(3, "PreviousTexture", 2, "PreviousDistance").AssertSuccess();
  definition.Connect(1, "PreviousTexture", 2, "PreviousIrradiance").AssertSuccess();
  definition.Connect(3, "PreviousTexture", 5, "PreviousDistance").AssertSuccess();
  definition.Connect(4, "ProbeStates", 0, "InitialProbeStates").AssertSuccess();
  definition.Connect(5, "Schedule", 2, "Schedule").AssertSuccess();
  definition.m_Interface.PushBack({"ProbeConstants", false, xiiUuid(13732997665415316893ULL, 7367359516963588889ULL), "ProbeConstants"});
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(10609997558256114642ULL, 3623139168192557810ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"Schedule", false, xiiUuid(6527658302303941476ULL, 12833624199530291004ULL), "Schedule"});
  definition.m_Interface.PushBack({"Distance", false, xiiUuid(10609997558256114642ULL, 3623139168192557810ULL), "Distance"});
  definition.m_Interface.PushBack({"ProbeStates", false, xiiUuid(10609997558256114642ULL, 3623139168192557810ULL), "ProbeStates"});
  definition.m_Interface.PushBack({"Irradiance", false, xiiUuid(10609997558256114642ULL, 3623139168192557810ULL), "Irradiance"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}



xiiRenderPipelineResourceHandle xiiCreateDebugVisualizationTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DebugVisualization";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(6063896203113337820ULL, 16122221436934967066ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("ViewRenderModePass 1", xiiRTTI::FindTypeByName("xiiViewRenderModePass"), Properties({{"UseViewMode", true}}));
  definition.m_Passes[0].m_Id = xiiUuid(2420089902878635979ULL, 12330943483042783873ULL);
  definition.AddPass("DebugOverlayPass 1", xiiRTTI::FindTypeByName("xiiDebugOverlayPass"));
  definition.m_Passes[1].m_Id = xiiUuid(12436864947101006108ULL, 12579880299694059694ULL);
  definition.Connect(0, "DisplayLinearColor", 1, "DisplayLinearColor").AssertSuccess();
  definition.m_Interface.PushBack({"GBufferMaterial", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "GBufferMaterial"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(12436864947101006108ULL, 12579880299694059694ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"BloomTexture", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "BloomTexture"});
  definition.m_Interface.PushBack({"SceneDepth", false, xiiUuid(12436864947101006108ULL, 12579880299694059694ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"DilatedVelocityBuffer", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "DilatedVelocityBuffer"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"GBufferObjectId", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "GBufferObjectId"});
  definition.m_Interface.PushBack({"StableAO", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "StableAO"});
  definition.m_Interface.PushBack({"DisplayLinearColor", false, xiiUuid(12436864947101006108ULL, 12579880299694059694ULL), "DisplayLinearColor"});
  definition.m_Interface.PushBack({"EnvironmentSpecular", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "EnvironmentSpecular"});
  definition.m_Interface.PushBack({"DisplayLinearColor", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "DisplayLinearColor"});
  definition.m_Interface.PushBack({"GBufferNormal", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "GBufferNormal"});
  definition.m_Interface.PushBack({"GBufferEmissive", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "GBufferEmissive"});
  definition.m_Interface.PushBack({"IndirectLighting", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "IndirectLighting"});
  definition.m_Interface.PushBack({"WireframeSurface", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "WireframeSurface"});
  definition.m_Interface.PushBack({"GBufferAlbedo", true, xiiUuid(2420089902878635979ULL, 12330943483042783873ULL), "GBufferAlbedo"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateDepthOnlyTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DepthOnly";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(16112939336347213242ULL, 2709124710275780278ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("DepthTargetPass 1", xiiRTTI::FindTypeByName("xiiDepthTargetPass"), Properties({{"DepthTarget", Properties({{"Format", xiiInt64(40LL)}, {"Requirements", Properties({{"Dimension", xiiInt64(0LL)}, {"Height", xiiUInt32(0U)}, {"RequiredBindFlags", xiiInt64(0LL)}, {"Samples", xiiUInt32(0U)}, {"Width", xiiUInt32(0U)}})}, {"Resolution", Properties({{"Height", xiiUInt32(0U)}, {"Input", xiiString("")}, {"Mode", xiiInt64(1LL)}, {"Scale", 1.0f}, {"Width", xiiUInt32(0U)}})}})}}));
  definition.m_Passes[0].m_Id = xiiUuid(15522643030973709623ULL, 6731979659213826334ULL);
  definition.AddPass("DepthRasterPass 1", xiiRTTI::FindTypeByName("xiiDepthRasterPass"));
  definition.m_Passes[1].m_Id = xiiUuid(7647187846476114300ULL, 9999977704609799799ULL);
  definition.AddPass("SurfaceRasterConstantsPass 1", xiiRTTI::FindTypeByName("xiiSurfaceRasterConstantsPass"));
  definition.m_Passes[2].m_Id = xiiUuid(7389437941332380908ULL, 11286092358035661928ULL);
  definition.AddPass("MaterialUploadPass 1", xiiRTTI::FindTypeByName("xiiMaterialUploadPass"));
  definition.m_Passes[3].m_Id = xiiUuid(1571542800340133167ULL, 11768187959665508422ULL);
  definition.AddPass("VertexPullDrawCommandsPass 1", xiiRTTI::FindTypeByName("xiiVertexPullDrawCommandsPass"));
  definition.m_Passes[4].m_Id = xiiUuid(5553708103507441642ULL, 14040569743825165185ULL);
  definition.AddPass("MeshletDispatchBoundsPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchBoundsPass"));
  definition.m_Passes[5].m_Id = xiiUuid(16210933231389949769ULL, 1892416581557897899ULL);
  definition.AddPass("MeshletCullPass 1", xiiRTTI::FindTypeByName("xiiMeshletCullPass"));
  definition.m_Passes[6].m_Id = xiiUuid(12307855200488476846ULL, 5199795338988476840ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[7].m_Id = xiiUuid(16781312817508970742ULL, 8976366869549145638ULL);
  definition.AddPass("CounterResetPass 2", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[8].m_Id = xiiUuid(8450680561774250514ULL, 12586983100898221035ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[9].m_Id = xiiUuid(15923204538718822240ULL, 3747864368180788529ULL);
  definition.AddPass("VisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiVisibilityViewPass"));
  definition.m_Passes[10].m_Id = xiiUuid(14068774022448509616ULL, 5190724302567945973ULL);
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[11].m_Id = xiiUuid(13162158252053806508ULL, 12469068022706172205ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[12].m_Id = xiiUuid(16100822085247852988ULL, 10921314863520238033ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[13].m_Id = xiiUuid(5104156036987944433ULL, 9663844915143123678ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[14].m_Id = xiiUuid(10509277666586981590ULL, 6098533393660608321ULL);
  definition.AddPass("CounterResetPass 3", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[15].m_Id = xiiUuid(643204170602037267ULL, 2605603811628566720ULL);
  definition.AddPass("CounterResetPass 4", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[16].m_Id = xiiUuid(8114981495557944801ULL, 10138129266326465859ULL);
  definition.AddPass("MeshletDispatchPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchPass"));
  definition.m_Passes[17].m_Id = xiiUuid(10226799469183634769ULL, 11871859595946476221ULL);
  definition.Connect(9, "GeometryMetadata", 14, "GeometryMetadata").AssertSuccess();
  definition.Connect(12, "LodHistory", 14, "InitializedLodHistory").AssertSuccess();
  definition.Connect(13, "SceneInstances", 14, "SceneInstances").AssertSuccess();
  definition.Connect(13, "SceneInstances", 12, "SceneInstances").AssertSuccess();
  definition.Connect(15, "Counter", 14, "Counter").AssertSuccess();
  definition.Connect(10, "Candidates", 14, "Candidates").AssertSuccess();
  definition.Connect(11, "PreviousBuffer", 14, "PreviousLodHistory").AssertSuccess();
  definition.Connect(14, "LodHistory", 11, "Buffer").AssertSuccess();
  definition.Connect(10, "VisibilityView", 14, "VisibilityView").AssertSuccess();
  definition.Connect(9, "GeometryMetadata", 6, "GeometryMetadata").AssertSuccess();
  definition.Connect(14, "LodHistory", 5, "LodHistory").AssertSuccess();
  definition.Connect(9, "MeshletMetadata", 17, "MeshletMetadata").AssertSuccess();
  definition.Connect(13, "SceneInstances", 5, "SceneInstances").AssertSuccess();
  definition.Connect(14, "VisibleInstanceCount", 6, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(7, "Counter", 6, "InitialCount").AssertSuccess();
  definition.Connect(10, "VisibilityView", 5, "VisibilityView").AssertSuccess();
  definition.Connect(14, "VisibleInstanceCount", 17, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(9, "GeometryMetadata", 5, "GeometryMetadata").AssertSuccess();
  definition.Connect(8, "Counter", 5, "InitializedMaximum").AssertSuccess();
  definition.Connect(13, "SceneInstances", 6, "SceneInstances").AssertSuccess();
  definition.Connect(14, "VisibleInstances", 6, "VisibleInstances").AssertSuccess();
  definition.Connect(5, "MaximumMeshletGroups", 17, "MaximumMeshletGroups").AssertSuccess();
  definition.Connect(14, "VisibleInstances", 17, "VisibleInstances").AssertSuccess();
  definition.Connect(14, "VisibleInstances", 5, "VisibleInstances").AssertSuccess();
  definition.Connect(16, "Counter", 6, "InitialOverflow").AssertSuccess();
  definition.Connect(17, "DispatchArguments", 6, "DispatchArguments").AssertSuccess();
  definition.Connect(9, "MeshletMetadata", 6, "MeshletMetadata").AssertSuccess();
  definition.Connect(14, "LodHistory", 6, "LodHistory").AssertSuccess();
  definition.Connect(14, "VisibleInstanceCount", 5, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(9, "MeshletMetadata", 5, "MeshletMetadata").AssertSuccess();
  definition.Connect(10, "VisibilityView", 6, "VisibilityView").AssertSuccess();
  definition.Connect(9, "GeometryMetadata", 4, "GeometryMetadata").AssertSuccess();
  definition.Connect(6, "VisibleMeshlets", 4, "VisibleMeshlets").AssertSuccess();
  definition.Connect(6, "VisibleMeshletCount", 4, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(10, "VisibilityView", 4, "VisibilityView").AssertSuccess();
  definition.Connect(9, "MeshletMetadata", 4, "MeshletMetadata").AssertSuccess();
  definition.Connect(13, "SceneInstances", 4, "SceneInstances").AssertSuccess();
  definition.Connect(6, "VisibleMeshlets", 1, "VisibleMeshlets").AssertSuccess();
  definition.Connect(13, "SceneInstances", 1, "SceneInstances").AssertSuccess();
  definition.Connect(2, "RasterConstants", 1, "RasterConstants").AssertSuccess();
  definition.Connect(0, "DepthTarget", 1, "DepthTarget").AssertSuccess();
  definition.Connect(3, "SurfaceMaterials", 1, "SurfaceMaterials").AssertSuccess();
  definition.Connect(9, "GeometryMetadata", 1, "GeometryMetadata").AssertSuccess();
  definition.Connect(4, "DrawCommands", 1, "DrawCommands").AssertSuccess();
  definition.Connect(9, "MeshletMetadata", 1, "MeshletMetadata").AssertSuccess();
  definition.Connect(4, "DrawCommandCount", 1, "DrawCommandCount").AssertSuccess();
  definition.Connect(6, "VisibleMeshletCount", 1, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(13, "SkinningMatrices", 1, "SkinningMatrices").AssertSuccess();
  definition.m_Interface.PushBack({"VisibleMeshlets", false, xiiUuid(12307855200488476846ULL, 5199795338988476840ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"VisibilityView", false, xiiUuid(14068774022448509616ULL, 5190724302567945973ULL), "VisibilityView"});
  definition.m_Interface.PushBack({"SceneDepth", false, xiiUuid(7647187846476114300ULL, 9999977704609799799ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(10509277666586981590ULL, 6098533393660608321ULL), "VisibleInstances"});
  definition.m_Interface.PushBack({"DrawCommands", false, xiiUuid(5553708103507441642ULL, 14040569743825165185ULL), "DrawCommands"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(10509277666586981590ULL, 6098533393660608321ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"SkinningMatrices", false, xiiUuid(5104156036987944433ULL, 9663844915143123678ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"DispatchArguments", false, xiiUuid(10226799469183634769ULL, 11871859595946476221ULL), "DispatchArguments"});
  definition.m_Interface.PushBack({"LodHistory", false, xiiUuid(10509277666586981590ULL, 6098533393660608321ULL), "LodHistory"});
  definition.m_Interface.PushBack({"DrawCommandCount", false, xiiUuid(5553708103507441642ULL, 14040569743825165185ULL), "DrawCommandCount"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", false, xiiUuid(12307855200488476846ULL, 5199795338988476840ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"MeshletMetadata", false, xiiUuid(15923204538718822240ULL, 3747864368180788529ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(14068774022448509616ULL, 5190724302567945973ULL), "InputCandidates"});
  definition.m_Interface.PushBack({"GeometryMetadata", false, xiiUuid(15923204538718822240ULL, 3747864368180788529ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"SceneInstances", false, xiiUuid(5104156036987944433ULL, 9663844915143123678ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"VisibleMeshletOverflow", false, xiiUuid(12307855200488476846ULL, 5199795338988476840ULL), "VisibleMeshletOverflow"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateDepthPyramidTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DepthPyramid";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(11805044691584136791ULL, 2145317346412637269ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("TextureHistoryPass 1", xiiRTTI::FindTypeByName("xiiTextureHistoryPass"));
  definition.m_Passes[0].m_Id = xiiUuid(18104523140738312173ULL, 2107539329648759783ULL);
  definition.AddPass("DepthPyramidPass 1", xiiRTTI::FindTypeByName("xiiDepthPyramidPass"));
  definition.m_Passes[1].m_Id = xiiUuid(4417256537383415398ULL, 14701306170736735886ULL);
  definition.Connect(1, "DepthPyramid", 0, "Texture").AssertSuccess();
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(4417256537383415398ULL, 14701306170736735886ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"PreviousDepthPyramid", false, xiiUuid(18104523140738312173ULL, 2107539329648759783ULL), "PreviousTexture"});
  definition.m_Interface.PushBack({"DepthPyramid", false, xiiUuid(4417256537383415398ULL, 14701306170736735886ULL), "DepthPyramid"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateDepthSensorTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DepthSensor";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(14684400692682223704ULL, 5060445169634149719ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("DepthSensorPass 1", xiiRTTI::FindTypeByName("xiiDepthSensorPass"), Properties({{"UseViewProfile", true}}));
  definition.m_Passes[0].m_Id = xiiUuid(5804882157241172325ULL, 6051730233749568449ULL);
  definition.m_Interface.PushBack({"Velocity", true, xiiUuid(5804882157241172325ULL, 6051730233749568449ULL), "Velocity"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(5804882157241172325ULL, 6051730233749568449ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"SensorOutput", false, xiiUuid(5804882157241172325ULL, 6051730233749568449ULL), "SensorOutput"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(5804882157241172325ULL, 6051730233749568449ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"PerFrameGlobal", true, xiiUuid(5804882157241172325ULL, 6051730233749568449ULL), "PerFrameGlobal"});
  definition.m_Interface.PushBack({"Constants", false, xiiUuid(5804882157241172325ULL, 6051730233749568449ULL), "Constants"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateDepthSensorDiagnosticTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DepthSensorDiagnostic";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(4600763476460351946ULL, 13352313408962113180ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("DepthSensorPass 1", xiiRTTI::FindTypeByName("xiiDepthSensorPass"), Properties({{"UseViewProfile", true}}));
  definition.m_Passes[0].m_Id = xiiUuid(11919884310377287626ULL, 2596004751773403432ULL);
  definition.AddPass("DepthTargetPass 1", xiiRTTI::FindTypeByName("xiiDepthTargetPass"), Properties({{"DepthTarget", Properties({{"Format", xiiInt64(40LL)}, {"Requirements", Properties({{"Dimension", xiiInt64(0LL)}, {"Height", xiiUInt32(0U)}, {"RequiredBindFlags", xiiInt64(0LL)}, {"Samples", xiiUInt32(0U)}, {"Width", xiiUInt32(0U)}})}, {"Resolution", Properties({{"Height", xiiUInt32(0U)}, {"Input", xiiString("")}, {"Mode", xiiInt64(1LL)}, {"Scale", 1.0f}, {"Width", xiiUInt32(0U)}})}})}}));
  definition.m_Passes[1].m_Id = xiiUuid(6300977796583796401ULL, 9704910339739432497ULL);
  definition.AddPass("ViewConstantsPass 1", xiiRTTI::FindTypeByName("xiiViewConstantsPass"));
  definition.m_Passes[2].m_Id = xiiUuid(1813495716347403131ULL, 13673822837050681267ULL);
  definition.Connect(1, "DepthTarget", 0, "SceneDepth").AssertSuccess();
  definition.Connect(2, "PerFrameCamera", 0, "PerFrameCamera").AssertSuccess();
  definition.m_Interface.PushBack({"SensorOutput", false, xiiUuid(11919884310377287626ULL, 2596004751773403432ULL), "SensorOutput"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateDiffuseSurfaceLightingTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DiffuseSurfaceLighting";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(4490164578578048703ULL, 3707439722108231650ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("DiffuseSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDiffuseSurfaceLightingPass"));
  definition.m_Passes[0].m_Id = xiiUuid(17653130721470608335ULL, 7401616852336203571ULL);
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(17653130721470608335ULL, 7401616852336203571ULL), "Emissive"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(17653130721470608335ULL, 7401616852336203571ULL), "Albedo"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(17653130721470608335ULL, 7401616852336203571ULL), "Material"});
  definition.m_Interface.PushBack({"Lighting", false, xiiUuid(17653130721470608335ULL, 7401616852336203571ULL), "Lighting"});
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(17653130721470608335ULL, 7401616852336203571ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"Irradiance", true, xiiUuid(17653130721470608335ULL, 7401616852336203571ULL), "Irradiance"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(17653130721470608335ULL, 7401616852336203571ULL), "Depth"});
  definition.m_Interface.PushBack({"Occlusion", true, xiiUuid(17653130721470608335ULL, 7401616852336203571ULL), "Occlusion"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateDirectionalMeshTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DirectionalMesh";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(11606990588085250878ULL, 17096219913639022395ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::MeshShaders | xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("VirtualShadowSamplingConstantsPass 1", xiiRTTI::FindTypeByName("xiiVirtualShadowSamplingConstantsPass"));
  definition.m_Passes[0].m_Id = xiiUuid(12765649302000131280ULL, 785134076852451644ULL);
  definition.AddPass("DirectionalMeshConstantsPass 1", xiiRTTI::FindTypeByName("xiiDirectionalMeshConstantsPass"), Properties({{"SelectionPulseStrength", 0.04f}}));
  definition.m_Passes[1].m_Id = xiiUuid(5663424434497073131ULL, 5002886986484291743ULL);
  definition.AddPass("DirectionalMeshRasterPass 1", xiiRTTI::FindTypeByName("xiiDirectionalMeshRasterPass"));
  definition.m_Passes[2].m_Id = xiiUuid(6728007245569749081ULL, 6441749707138653325ULL);
  definition.AddPass("MatrixRasterConstantsPass 1", xiiRTTI::FindTypeByName("xiiMatrixRasterConstantsPass"));
  definition.m_Passes[3].m_Id = xiiUuid(16847514613347761575ULL, 11504976314440139379ULL);
  definition.Connect(1, "Constants", 2, "Constants").AssertSuccess();
  definition.Connect(0, "Constants", 2, "VirtualShadowConstants").AssertSuccess();
  definition.Connect(3, "RasterConstants", 2, "RasterConstants").AssertSuccess();
  definition.m_Interface.PushBack({"SceneInstances", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"ColorTarget", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "ColorTarget"});
  definition.m_Interface.PushBack({"GeometryMetadata", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"ShadowMap", true, xiiUuid(5663424434497073131ULL, 5002886986484291743ULL), "ShadowMap"});
  definition.m_Interface.PushBack({"ShadowMap", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "ShadowMap"});
  definition.m_Interface.PushBack({"DepthTarget", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "DepthTarget"});
  definition.m_Interface.PushBack({"Cascades", true, xiiUuid(5663424434497073131ULL, 5002886986484291743ULL), "Cascades"});
  definition.m_Interface.PushBack({"MeshletMetadata", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"VirtualPageTable", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "VirtualPageTable"});
  definition.m_Interface.PushBack({"VirtualShadowAtlas", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "VirtualShadowAtlas"});
  definition.m_Interface.PushBack({"RasterConstants", false, xiiUuid(5663424434497073131ULL, 5002886986484291743ULL), "Constants"});
  definition.m_Interface.PushBack({"VisibleMeshlets", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"SurfaceMaterials", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "SurfaceMaterials"});
  definition.m_Interface.PushBack({"Residency", true, xiiUuid(12765649302000131280ULL, 785134076852451644ULL), "Residency"});
  definition.m_Interface.PushBack({"DrawCommands", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "DrawCommands"});
  definition.m_Interface.PushBack({"DrawCommandCount", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "DrawCommandCount"});
  definition.m_Interface.PushBack({"Depth", false, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "Depth"});
  definition.m_Interface.PushBack({"Color", false, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "Color"});
  definition.m_Interface.PushBack({"SamplingConstants", false, xiiUuid(12765649302000131280ULL, 785134076852451644ULL), "Constants"});
  definition.m_Interface.PushBack({"SkinningMatrices", true, xiiUuid(6728007245569749081ULL, 6441749707138653325ULL), "SkinningMatrices"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateDirectionalSurfaceLightingTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/DirectionalSurfaceLighting";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(879176183903411444ULL, 7621057251398077260ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("DirectionalSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDirectionalSurfaceLightingPass"));
  definition.m_Passes[0].m_Id = xiiUuid(14458275834381770257ULL, 2181553226365456582ULL);
  definition.m_Interface.PushBack({"Material", true, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "Material"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "Albedo"});
  definition.m_Interface.PushBack({"Lighting", false, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "Lighting"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "Camera"});
  definition.m_Interface.PushBack({"Visibility", true, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "Visibility"});
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "Emissive"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "Normal"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(14458275834381770257ULL, 2181553226365456582ULL), "Depth"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateEditorMeshPickingTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/EditorMeshPicking";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(2016042003980137488ULL, 16792215425151599521ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("SceneMeshPickingPass 1", xiiRTTI::FindTypeByName("xiiSceneMeshPickingPass"));
  definition.m_Passes[0].m_Id = xiiUuid(4854396790513520547ULL, 6022545871210208380ULL);
  definition.AddPass("MaterialUploadPass 1", xiiRTTI::FindTypeByName("xiiMaterialUploadPass"));
  definition.m_Passes[1].m_Id = xiiUuid(11941936243031057910ULL, 2408600430267426935ULL);
  definition.AddPass("SurfaceRasterConstantsPass 1", xiiRTTI::FindTypeByName("xiiSurfaceRasterConstantsPass"));
  definition.m_Passes[2].m_Id = xiiUuid(13230638659994923722ULL, 1567808414665761380ULL);
  definition.AddPass("VertexPullDrawCommandsPass 1", xiiRTTI::FindTypeByName("xiiVertexPullDrawCommandsPass"));
  definition.m_Passes[3].m_Id = xiiUuid(9089625888768838944ULL, 9446540878752988098ULL);
  definition.AddPass("MeshletDispatchBoundsPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchBoundsPass"));
  definition.m_Passes[4].m_Id = xiiUuid(3411156285910940910ULL, 3967163345486331706ULL);
  definition.AddPass("MeshletCullPass 1", xiiRTTI::FindTypeByName("xiiMeshletCullPass"));
  definition.m_Passes[5].m_Id = xiiUuid(13136373155116909535ULL, 17491041821327005457ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[6].m_Id = xiiUuid(10209661605208104810ULL, 1660761191999665989ULL);
  definition.AddPass("CounterResetPass 2", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[7].m_Id = xiiUuid(13809462257084790340ULL, 4941489938474644851ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[8].m_Id = xiiUuid(15034268950140550342ULL, 8502576116637878990ULL);
  definition.AddPass("VisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiVisibilityViewPass"));
  definition.m_Passes[9].m_Id = xiiUuid(1937761334324058718ULL, 17343254355585362756ULL);
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[10].m_Id = xiiUuid(16778845863619954150ULL, 12245030523477378360ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[11].m_Id = xiiUuid(3128445639483922452ULL, 12385943709847657492ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[12].m_Id = xiiUuid(13865986320594252665ULL, 8267112712263771492ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[13].m_Id = xiiUuid(15138148027456313687ULL, 10639692929907497098ULL);
  definition.AddPass("CounterResetPass 3", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[14].m_Id = xiiUuid(7612914378219770168ULL, 4930334433052411368ULL);
  definition.AddPass("CounterResetPass 4", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[15].m_Id = xiiUuid(3189802879142962013ULL, 16976680664894134186ULL);
  definition.AddPass("MeshletDispatchPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchPass"));
  definition.m_Passes[16].m_Id = xiiUuid(2083906050034283213ULL, 2584660271423641489ULL);
  definition.AddPass("SurfaceQueryPass 1", xiiRTTI::FindTypeByName("xiiSurfaceQueryPass"));
  definition.m_Passes[17].m_Id = xiiUuid(4331774394768675791ULL, 5233134465250740608ULL);
  definition.AddPass("SceneInstanceSelectionPass 1", xiiRTTI::FindTypeByName("xiiSceneInstanceSelectionPass"));
  definition.m_Passes[18].m_Id = xiiUuid(3315603995772407677ULL, 16677833202794641328ULL);
  definition.Connect(8, "GeometryMetadata", 13, "GeometryMetadata").AssertSuccess();
  definition.Connect(11, "LodHistory", 13, "InitializedLodHistory").AssertSuccess();
  definition.Connect(12, "SceneInstances", 13, "SceneInstances").AssertSuccess();
  definition.Connect(12, "SceneInstances", 11, "SceneInstances").AssertSuccess();
  definition.Connect(14, "Counter", 13, "Counter").AssertSuccess();
  definition.Connect(9, "Candidates", 13, "Candidates").AssertSuccess();
  definition.Connect(10, "PreviousBuffer", 13, "PreviousLodHistory").AssertSuccess();
  definition.Connect(13, "LodHistory", 10, "Buffer").AssertSuccess();
  definition.Connect(9, "VisibilityView", 13, "VisibilityView").AssertSuccess();
  definition.Connect(8, "GeometryMetadata", 5, "GeometryMetadata").AssertSuccess();
  definition.Connect(13, "LodHistory", 4, "LodHistory").AssertSuccess();
  definition.Connect(8, "MeshletMetadata", 16, "MeshletMetadata").AssertSuccess();
  definition.Connect(12, "SceneInstances", 4, "SceneInstances").AssertSuccess();
  definition.Connect(13, "VisibleInstanceCount", 5, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(6, "Counter", 5, "InitialCount").AssertSuccess();
  definition.Connect(9, "VisibilityView", 4, "VisibilityView").AssertSuccess();
  definition.Connect(13, "VisibleInstanceCount", 16, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(8, "GeometryMetadata", 4, "GeometryMetadata").AssertSuccess();
  definition.Connect(7, "Counter", 4, "InitializedMaximum").AssertSuccess();
  definition.Connect(12, "SceneInstances", 5, "SceneInstances").AssertSuccess();
  definition.Connect(13, "VisibleInstances", 5, "VisibleInstances").AssertSuccess();
  definition.Connect(4, "MaximumMeshletGroups", 16, "MaximumMeshletGroups").AssertSuccess();
  definition.Connect(13, "VisibleInstances", 16, "VisibleInstances").AssertSuccess();
  definition.Connect(13, "VisibleInstances", 4, "VisibleInstances").AssertSuccess();
  definition.Connect(15, "Counter", 5, "InitialOverflow").AssertSuccess();
  definition.Connect(16, "DispatchArguments", 5, "DispatchArguments").AssertSuccess();
  definition.Connect(8, "MeshletMetadata", 5, "MeshletMetadata").AssertSuccess();
  definition.Connect(13, "LodHistory", 5, "LodHistory").AssertSuccess();
  definition.Connect(13, "VisibleInstanceCount", 4, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(8, "MeshletMetadata", 4, "MeshletMetadata").AssertSuccess();
  definition.Connect(9, "VisibilityView", 5, "VisibilityView").AssertSuccess();
  definition.Connect(8, "GeometryMetadata", 3, "GeometryMetadata").AssertSuccess();
  definition.Connect(5, "VisibleMeshlets", 3, "VisibleMeshlets").AssertSuccess();
  definition.Connect(5, "VisibleMeshletCount", 3, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(9, "VisibilityView", 3, "VisibilityView").AssertSuccess();
  definition.Connect(8, "MeshletMetadata", 3, "MeshletMetadata").AssertSuccess();
  definition.Connect(12, "SceneInstances", 3, "SceneInstances").AssertSuccess();
  definition.Connect(5, "VisibleMeshletCount", 17, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(8, "GeometryMetadata", 17, "GeometryMetadata").AssertSuccess();
  definition.Connect(1, "SurfaceMaterials", 17, "SurfaceMaterials").AssertSuccess();
  definition.Connect(5, "VisibleMeshlets", 17, "VisibleMeshlets").AssertSuccess();
  definition.Connect(3, "DrawCommandCount", 17, "DrawCommandCount").AssertSuccess();
  definition.Connect(8, "MeshletMetadata", 17, "MeshletMetadata").AssertSuccess();
  definition.Connect(2, "RasterConstants", 17, "RasterConstants").AssertSuccess();
  definition.Connect(12, "SceneInstances", 17, "SceneInstances").AssertSuccess();
  definition.Connect(3, "DrawCommands", 17, "DrawCommands").AssertSuccess();
  definition.Connect(12, "SkinningMatrices", 17, "SkinningMatrices").AssertSuccess();
  definition.Connect(18, "Candidates", 9, "InputCandidates").AssertSuccess();
  definition.Connect(17, "Surface", 0, "Surface").AssertSuccess();
  definition.m_Interface.PushBack({"VisibleMeshletOverflow", false, xiiUuid(13136373155116909535ULL, 17491041821327005457ULL), "VisibleMeshletOverflow"});
  definition.m_Interface.PushBack({"Surface", false, xiiUuid(4854396790513520547ULL, 6022545871210208380ULL), "Surface"});
  definition.m_Interface.PushBack({"SceneDepth", false, xiiUuid(4331774394768675791ULL, 5233134465250740608ULL), "SceneDepth"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateEnvironmentLightConstantsTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/EnvironmentLightConstants";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(2148689299315010960ULL, 10465707846546199040ULL);
  definition.AddPass("EnvironmentLightConstantsPass 1", xiiRTTI::FindTypeByName("xiiEnvironmentLightConstantsPass"));
  definition.m_Passes[0].m_Id = xiiUuid(2312387130810435612ULL, 6292764103845671014ULL);
  definition.m_Interface.PushBack({"DirectionalLight", true, xiiUuid(2312387130810435612ULL, 6292764103845671014ULL), "DirectionalLight"});
  definition.m_Interface.PushBack({"EnvironmentLight", false, xiiUuid(2312387130810435612ULL, 6292764103845671014ULL), "EnvironmentLight"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateEnvironmentSpecularTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/EnvironmentSpecular";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(10176686104453876849ULL, 15812401497032462868ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("EnvironmentSpecularPass 1", xiiRTTI::FindTypeByName("xiiEnvironmentSpecularPass"));
  definition.m_Passes[0].m_Id = xiiUuid(956912044588826540ULL, 15272098794374798494ULL);
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "Albedo"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "Camera"});
  definition.m_Interface.PushBack({"EnvironmentRadiance", true, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "EnvironmentRadiance"});
  definition.m_Interface.PushBack({"Occlusion", true, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "Occlusion"});
  definition.m_Interface.PushBack({"BRDFLut", true, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "BRDFLut"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "Depth"});
  definition.m_Interface.PushBack({"Lighting", false, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "Lighting"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "Normal"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(956912044588826540ULL, 15272098794374798494ULL), "Material"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateGBufferTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/GBuffer";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(197933280306533873ULL, 1557581295529750309ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("MaterialUploadPass 1", xiiRTTI::FindTypeByName("xiiMaterialUploadPass"));
  definition.m_Passes[0].m_Id = xiiUuid(16009493270742654974ULL, 1919127135621892762ULL);
  definition.AddPass("DepthRasterPass 1", xiiRTTI::FindTypeByName("xiiDepthRasterPass"));
  definition.m_Passes[1].m_Id = xiiUuid(7965668487469958107ULL, 2825564402831997085ULL);
  definition.AddPass("GBufferRasterPass 1", xiiRTTI::FindTypeByName("xiiGBufferRasterPass"));
  definition.m_Passes[2].m_Id = xiiUuid(11074718749116644148ULL, 11401512850115132603ULL);
  definition.AddPass("SurfaceRasterConstantsPass 1", xiiRTTI::FindTypeByName("xiiSurfaceRasterConstantsPass"));
  definition.m_Passes[3].m_Id = xiiUuid(9228047572039997997ULL, 11533385921769223975ULL);
  definition.AddPass("DepthTargetPass 1", xiiRTTI::FindTypeByName("xiiDepthTargetPass"), Properties({{"DepthTarget", Properties({{"Format", xiiInt64(40LL)}, {"Requirements", Properties({{"Dimension", xiiInt64(0LL)}, {"Height", xiiUInt32(0U)}, {"RequiredBindFlags", xiiInt64(0LL)}, {"Samples", xiiUInt32(0U)}, {"Width", xiiUInt32(0U)}})}, {"Resolution", Properties({{"Height", xiiUInt32(0U)}, {"Input", xiiString("")}, {"Mode", xiiInt64(1LL)}, {"Scale", 1.0f}, {"Width", xiiUInt32(0U)}})}})}}));
  definition.m_Passes[4].m_Id = xiiUuid(16306669529901093610ULL, 14078272639416531874ULL);
  definition.AddPass("VertexPullDrawCommandsPass 1", xiiRTTI::FindTypeByName("xiiVertexPullDrawCommandsPass"));
  definition.m_Passes[5].m_Id = xiiUuid(15081836772212203722ULL, 12816169201286398047ULL);
  definition.AddPass("MeshletDispatchBoundsPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchBoundsPass"));
  definition.m_Passes[6].m_Id = xiiUuid(15557056619468610468ULL, 6162426942490700667ULL);
  definition.AddPass("MeshletCullPass 1", xiiRTTI::FindTypeByName("xiiMeshletCullPass"));
  definition.m_Passes[7].m_Id = xiiUuid(10571841888964616624ULL, 12396766792894259833ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[8].m_Id = xiiUuid(17390730851885701449ULL, 4383505892233784862ULL);
  definition.AddPass("CounterResetPass 2", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[9].m_Id = xiiUuid(13673316735870405335ULL, 7533721964656203630ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[10].m_Id = xiiUuid(10594021189348849682ULL, 14786296188124611524ULL);
  definition.AddPass("VisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiVisibilityViewPass"));
  definition.m_Passes[11].m_Id = xiiUuid(7948784683859860237ULL, 16104239694076203386ULL);
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[12].m_Id = xiiUuid(312237307776542047ULL, 1410042698805922614ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[13].m_Id = xiiUuid(16149983324074307137ULL, 12076266963451351592ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[14].m_Id = xiiUuid(9881320321211922028ULL, 9241152376405887611ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[15].m_Id = xiiUuid(10895021442666932533ULL, 265251920509815351ULL);
  definition.AddPass("CounterResetPass 3", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[16].m_Id = xiiUuid(16190084557763346894ULL, 2937166255999783564ULL);
  definition.AddPass("CounterResetPass 4", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[17].m_Id = xiiUuid(4909047812268531288ULL, 4684367591855848078ULL);
  definition.AddPass("MeshletDispatchPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchPass"));
  definition.m_Passes[18].m_Id = xiiUuid(4546533675568626956ULL, 11485176989826317448ULL);
  definition.Connect(10, "GeometryMetadata", 15, "GeometryMetadata").AssertSuccess();
  definition.Connect(13, "LodHistory", 15, "InitializedLodHistory").AssertSuccess();
  definition.Connect(14, "SceneInstances", 15, "SceneInstances").AssertSuccess();
  definition.Connect(14, "SceneInstances", 13, "SceneInstances").AssertSuccess();
  definition.Connect(16, "Counter", 15, "Counter").AssertSuccess();
  definition.Connect(11, "Candidates", 15, "Candidates").AssertSuccess();
  definition.Connect(12, "PreviousBuffer", 15, "PreviousLodHistory").AssertSuccess();
  definition.Connect(15, "LodHistory", 12, "Buffer").AssertSuccess();
  definition.Connect(11, "VisibilityView", 15, "VisibilityView").AssertSuccess();
  definition.Connect(10, "GeometryMetadata", 7, "GeometryMetadata").AssertSuccess();
  definition.Connect(15, "LodHistory", 6, "LodHistory").AssertSuccess();
  definition.Connect(10, "MeshletMetadata", 18, "MeshletMetadata").AssertSuccess();
  definition.Connect(14, "SceneInstances", 6, "SceneInstances").AssertSuccess();
  definition.Connect(15, "VisibleInstanceCount", 7, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(8, "Counter", 7, "InitialCount").AssertSuccess();
  definition.Connect(11, "VisibilityView", 6, "VisibilityView").AssertSuccess();
  definition.Connect(15, "VisibleInstanceCount", 18, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(10, "GeometryMetadata", 6, "GeometryMetadata").AssertSuccess();
  definition.Connect(9, "Counter", 6, "InitializedMaximum").AssertSuccess();
  definition.Connect(14, "SceneInstances", 7, "SceneInstances").AssertSuccess();
  definition.Connect(15, "VisibleInstances", 7, "VisibleInstances").AssertSuccess();
  definition.Connect(6, "MaximumMeshletGroups", 18, "MaximumMeshletGroups").AssertSuccess();
  definition.Connect(15, "VisibleInstances", 18, "VisibleInstances").AssertSuccess();
  definition.Connect(15, "VisibleInstances", 6, "VisibleInstances").AssertSuccess();
  definition.Connect(17, "Counter", 7, "InitialOverflow").AssertSuccess();
  definition.Connect(18, "DispatchArguments", 7, "DispatchArguments").AssertSuccess();
  definition.Connect(10, "MeshletMetadata", 7, "MeshletMetadata").AssertSuccess();
  definition.Connect(15, "LodHistory", 7, "LodHistory").AssertSuccess();
  definition.Connect(15, "VisibleInstanceCount", 6, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(10, "MeshletMetadata", 6, "MeshletMetadata").AssertSuccess();
  definition.Connect(11, "VisibilityView", 7, "VisibilityView").AssertSuccess();
  definition.Connect(10, "GeometryMetadata", 5, "GeometryMetadata").AssertSuccess();
  definition.Connect(7, "VisibleMeshlets", 5, "VisibleMeshlets").AssertSuccess();
  definition.Connect(7, "VisibleMeshletCount", 5, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(11, "VisibilityView", 5, "VisibilityView").AssertSuccess();
  definition.Connect(10, "MeshletMetadata", 5, "MeshletMetadata").AssertSuccess();
  definition.Connect(14, "SceneInstances", 5, "SceneInstances").AssertSuccess();
  definition.Connect(4, "DepthTarget", 1, "DepthTarget").AssertSuccess();
  definition.Connect(3, "RasterConstants", 1, "RasterConstants").AssertSuccess();
  definition.Connect(14, "SceneInstances", 2, "SceneInstances").AssertSuccess();
  definition.Connect(7, "VisibleMeshletCount", 1, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(5, "DrawCommandCount", 2, "DrawCommandCount").AssertSuccess();
  definition.Connect(14, "SkinningMatrices", 2, "SkinningMatrices").AssertSuccess();
  definition.Connect(0, "SurfaceMaterials", 2, "SurfaceMaterials").AssertSuccess();
  definition.Connect(10, "GeometryMetadata", 1, "GeometryMetadata").AssertSuccess();
  definition.Connect(7, "VisibleMeshlets", 1, "VisibleMeshlets").AssertSuccess();
  definition.Connect(10, "GeometryMetadata", 2, "GeometryMetadata").AssertSuccess();
  definition.Connect(14, "SkinningMatrices", 1, "SkinningMatrices").AssertSuccess();
  definition.Connect(7, "VisibleMeshletCount", 2, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(5, "DrawCommands", 1, "DrawCommands").AssertSuccess();
  definition.Connect(1, "SceneDepth", 2, "SceneDepth").AssertSuccess();
  definition.Connect(5, "DrawCommandCount", 1, "DrawCommandCount").AssertSuccess();
  definition.Connect(5, "DrawCommands", 2, "DrawCommands").AssertSuccess();
  definition.Connect(3, "RasterConstants", 2, "RasterConstants").AssertSuccess();
  definition.Connect(14, "SceneInstances", 1, "SceneInstances").AssertSuccess();
  definition.Connect(10, "MeshletMetadata", 1, "MeshletMetadata").AssertSuccess();
  definition.Connect(10, "MeshletMetadata", 2, "MeshletMetadata").AssertSuccess();
  definition.Connect(0, "SurfaceMaterials", 1, "SurfaceMaterials").AssertSuccess();
  definition.Connect(7, "VisibleMeshlets", 2, "VisibleMeshlets").AssertSuccess();
  definition.m_Interface.PushBack({"Emissive", false, xiiUuid(11074718749116644148ULL, 11401512850115132603ULL), "Emissive"});
  definition.m_Interface.PushBack({"VisibilityView", false, xiiUuid(7948784683859860237ULL, 16104239694076203386ULL), "VisibilityView"});
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(7948784683859860237ULL, 16104239694076203386ULL), "InputCandidates"});
  definition.m_Interface.PushBack({"SkinningMatrices", false, xiiUuid(9881320321211922028ULL, 9241152376405887611ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"SceneInstances", false, xiiUuid(9881320321211922028ULL, 9241152376405887611ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"NormalRoughness", false, xiiUuid(11074718749116644148ULL, 11401512850115132603ULL), "NormalRoughness"});
  definition.m_Interface.PushBack({"SceneDepth", false, xiiUuid(7965668487469958107ULL, 2825564402831997085ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"Material", false, xiiUuid(11074718749116644148ULL, 11401512850115132603ULL), "Material"});
  definition.m_Interface.PushBack({"DrawCommands", false, xiiUuid(15081836772212203722ULL, 12816169201286398047ULL), "DrawCommands"});
  definition.m_Interface.PushBack({"MeshletMetadata", false, xiiUuid(10594021189348849682ULL, 14786296188124611524ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"VisibleMeshlets", false, xiiUuid(10571841888964616624ULL, 12396766792894259833ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"GeometryMetadata", false, xiiUuid(10594021189348849682ULL, 14786296188124611524ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"ObjectId", false, xiiUuid(11074718749116644148ULL, 11401512850115132603ULL), "ObjectId"});
  definition.m_Interface.PushBack({"Normal", false, xiiUuid(11074718749116644148ULL, 11401512850115132603ULL), "Normal"});
  definition.m_Interface.PushBack({"Velocity", false, xiiUuid(11074718749116644148ULL, 11401512850115132603ULL), "Velocity"});
  definition.m_Interface.PushBack({"VisibleMeshletOverflow", false, xiiUuid(10571841888964616624ULL, 12396766792894259833ULL), "VisibleMeshletOverflow"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(10895021442666932533ULL, 265251920509815351ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"DispatchArguments", false, xiiUuid(4546533675568626956ULL, 11485176989826317448ULL), "DispatchArguments"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", false, xiiUuid(10571841888964616624ULL, 12396766792894259833ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"DrawCommandCount", false, xiiUuid(15081836772212203722ULL, 12816169201286398047ULL), "DrawCommandCount"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(10895021442666932533ULL, 265251920509815351ULL), "VisibleInstances"});
  definition.m_Interface.PushBack({"Albedo", false, xiiUuid(11074718749116644148ULL, 11401512850115132603ULL), "Albedo"});
  definition.m_Interface.PushBack({"LodHistory", false, xiiUuid(10895021442666932533ULL, 265251920509815351ULL), "LodHistory"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateGeometryLodFeedbackTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/GeometryLodFeedback";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(17130790169029486453ULL, 7503162385638866386ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("GeometryLodFeedbackPass 1", xiiRTTI::FindTypeByName("xiiGeometryLodFeedbackPass"));
  definition.m_Passes[0].m_Id = xiiUuid(17717623321112830152ULL, 4946895303778289198ULL);
  definition.AddPass("GeometryLodFeedbackResetPass 1", xiiRTTI::FindTypeByName("xiiGeometryLodFeedbackResetPass"));
  definition.m_Passes[1].m_Id = xiiUuid(15759095970475255049ULL, 13402772154960121245ULL);
  definition.AddPass("GeometryLodFeedbackReadbackPass 1", xiiRTTI::FindTypeByName("xiiGeometryLodFeedbackReadbackPass"));
  definition.m_Passes[2].m_Id = xiiUuid(6285407076620244252ULL, 17835838461450528663ULL);
  definition.Connect(0, "Requests", 2, "Requests").AssertSuccess();
  definition.Connect(1, "Requests", 0, "InitializedRequests").AssertSuccess();
  definition.m_Interface.PushBack({"VisibilityView", true, xiiUuid(17717623321112830152ULL, 4946895303778289198ULL), "VisibilityView"});
  definition.m_Interface.PushBack({"GeometryMetadata", true, xiiUuid(17717623321112830152ULL, 4946895303778289198ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"Requests", false, xiiUuid(17717623321112830152ULL, 4946895303778289198ULL), "Requests"});
  definition.m_Interface.PushBack({"SceneInstances", true, xiiUuid(17717623321112830152ULL, 4946895303778289198ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"Candidates", true, xiiUuid(17717623321112830152ULL, 4946895303778289198ULL), "Candidates"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateHdrDisplayTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/HdrDisplay";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(7502992633543278597ULL, 4481838088937842948ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.m_Settings.m_uiOutputSamples        = 1;
  definition.AddPass("BloomPass 1", xiiRTTI::FindTypeByName("xiiBloomPass"), Properties({{"UseViewDefaults", true}}));
  definition.m_Passes[0].m_Id = xiiUuid(2446913382989193284ULL, 1645428666931223408ULL);
  definition.AddPass("PresentPass 1", xiiRTTI::FindTypeByName("xiiPresentPass"), Properties({{"UseViewDefaults", true}}));
  definition.m_Passes[1].m_Id = xiiUuid(596495975167129835ULL, 1960928447964614656ULL);
  definition.AddPass("LuminanceHistogramPass 1", xiiRTTI::FindTypeByName("xiiLuminanceHistogramPass"), Properties({{"UseViewDefaults", true}}));
  definition.m_Passes[2].m_Id = xiiUuid(12767546743289897594ULL, 4364131878209094650ULL);
  definition.AddPass("ColorGradingPass 1", xiiRTTI::FindTypeByName("xiiColorGradingPass"), Properties({{"UseViewDefaults", true}}));
  definition.m_Passes[3].m_Id = xiiUuid(2017123995807645625ULL, 7794957348070043872ULL);
  definition.AddPass("AutoExposurePass 1", xiiRTTI::FindTypeByName("xiiAutoExposurePass"), Properties({{"UseViewDefaults", true}}));
  definition.m_Passes[4].m_Id = xiiUuid(16363137922061307474ULL, 8455368232496004000ULL);
  definition.AddPass("ToneMappingPass 1", xiiRTTI::FindTypeByName("xiiToneMappingPass"), Properties({{"UseViewDefaults", true}}));
  definition.m_Passes[5].m_Id = xiiUuid(12398359839466326311ULL, 16835979226790343298ULL);
  definition.Connect(3, "GradedColor", 1, "GradedColor").AssertSuccess();
  definition.Connect(0, "BloomTexture", 5, "BloomTexture").AssertSuccess();
  definition.Connect(2, "LuminanceHistogram", 4, "LuminanceHistogram").AssertSuccess();
  definition.Connect(4, "CurrentExposure", 5, "CurrentExposure").AssertSuccess();
  definition.Connect(5, "DisplayLinearColor", 3, "DisplayLinearColor").AssertSuccess();
  definition.m_Interface.PushBack({"ViewOutput", false, xiiUuid(596495975167129835ULL, 1960928447964614656ULL), "ViewOutput"});
  definition.m_Interface.PushBack({"CurrentExposure", false, xiiUuid(16363137922061307474ULL, 8455368232496004000ULL), "CurrentExposure"});
  definition.m_Interface.PushBack({"HDRSceneColor", true, xiiUuid(12398359839466326311ULL, 16835979226790343298ULL), "UpscaledColor"});
  definition.m_Interface.PushBack({"HDRSceneColor", true, xiiUuid(2446913382989193284ULL, 1645428666931223408ULL), "UpscaledColor"});
  definition.m_Interface.PushBack({"HDRSceneColor", true, xiiUuid(12767546743289897594ULL, 4364131878209094650ULL), "HDRSceneColor"});
  definition.m_Interface.PushBack({"GradedColor", false, xiiUuid(2017123995807645625ULL, 7794957348070043872ULL), "GradedColor"});
  definition.m_Interface.PushBack({"DisplayLinearColor", false, xiiUuid(12398359839466326311ULL, 16835979226790343298ULL), "DisplayLinearColor"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateInfraredSensorTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/InfraredSensor";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(12808831096391972262ULL, 7758547666871750431ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("InfraredSensorPass 1", xiiRTTI::FindTypeByName("xiiInfraredSensorPass"), Properties({{"UseViewProfile", true}}));
  definition.m_Passes[0].m_Id = xiiUuid(15691182991410857754ULL, 8801555089993149310ULL);
  definition.m_Interface.PushBack({"PerFrameGlobal", true, xiiUuid(15691182991410857754ULL, 8801555089993149310ULL), "PerFrameGlobal"});
  definition.m_Interface.PushBack({"SensorOutput", false, xiiUuid(15691182991410857754ULL, 8801555089993149310ULL), "SensorOutput"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(15691182991410857754ULL, 8801555089993149310ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(15691182991410857754ULL, 8801555089993149310ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"Velocity", true, xiiUuid(15691182991410857754ULL, 8801555089993149310ULL), "Velocity"});
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(15691182991410857754ULL, 8801555089993149310ULL), "Radiance"});
  definition.m_Interface.PushBack({"Constants", false, xiiUuid(15691182991410857754ULL, 8801555089993149310ULL), "Constants"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateInstanceOcclusionTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/InstanceOcclusion";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(9561305537020268155ULL, 5699960448616919435ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[0].m_Id = xiiUuid(5404007996547137546ULL, 6192088033850654140ULL);
  definition.AddPass("InstanceOcclusionPass 1", xiiRTTI::FindTypeByName("xiiInstanceOcclusionPass"));
  definition.m_Passes[1].m_Id = xiiUuid(5224541760324688853ULL, 18245112788321122748ULL);
  definition.Connect(0, "Counter", 1, "InitialCount").AssertSuccess();
  definition.m_Interface.PushBack({"OcclusionView", true, xiiUuid(5224541760324688853ULL, 18245112788321122748ULL), "OcclusionView"});
  definition.m_Interface.PushBack({"SceneInstances", true, xiiUuid(5224541760324688853ULL, 18245112788321122748ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"Candidates", true, xiiUuid(5224541760324688853ULL, 18245112788321122748ULL), "Candidates"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(5224541760324688853ULL, 18245112788321122748ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"DepthPyramid", true, xiiUuid(5224541760324688853ULL, 18245112788321122748ULL), "DepthPyramid"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(5224541760324688853ULL, 18245112788321122748ULL), "VisibleInstances"});
  definition.m_Interface.PushBack({"CandidateCount", true, xiiUuid(5224541760324688853ULL, 18245112788321122748ULL), "CandidateCount"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateInstanceVisibilityTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/InstanceVisibility";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(18297318096085663097ULL, 12236033535610330616ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[0].m_Id = xiiUuid(13789234341475564839ULL, 8268872759528048157ULL);
  definition.AddPass("VisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiVisibilityViewPass"));
  definition.m_Passes[1].m_Id = xiiUuid(16688542118377531499ULL, 9542663121930712809ULL);
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[2].m_Id = xiiUuid(11740925458640380747ULL, 9800113055530393872ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[3].m_Id = xiiUuid(8783797895452085433ULL, 9834847774856937385ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[4].m_Id = xiiUuid(5172721522049087600ULL, 11402076667508953982ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[5].m_Id = xiiUuid(3419321221088446302ULL, 15439103404839490721ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[6].m_Id = xiiUuid(7141269536772681857ULL, 17489794021477068939ULL);
  definition.Connect(0, "GeometryMetadata", 5, "GeometryMetadata").AssertSuccess();
  definition.Connect(3, "LodHistory", 5, "InitializedLodHistory").AssertSuccess();
  definition.Connect(4, "SceneInstances", 5, "SceneInstances").AssertSuccess();
  definition.Connect(4, "SceneInstances", 3, "SceneInstances").AssertSuccess();
  definition.Connect(6, "Counter", 5, "Counter").AssertSuccess();
  definition.Connect(1, "Candidates", 5, "Candidates").AssertSuccess();
  definition.Connect(2, "PreviousBuffer", 5, "PreviousLodHistory").AssertSuccess();
  definition.Connect(5, "LodHistory", 2, "Buffer").AssertSuccess();
  definition.Connect(1, "VisibilityView", 5, "VisibilityView").AssertSuccess();
  definition.m_Interface.PushBack({"SceneInstances", false, xiiUuid(5172721522049087600ULL, 11402076667508953982ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"LodHistory", false, xiiUuid(3419321221088446302ULL, 15439103404839490721ULL), "LodHistory"});
  definition.m_Interface.PushBack({"SkinningMatrices", false, xiiUuid(5172721522049087600ULL, 11402076667508953982ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"VisibilityView", false, xiiUuid(16688542118377531499ULL, 9542663121930712809ULL), "VisibilityView"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(3419321221088446302ULL, 15439103404839490721ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(16688542118377531499ULL, 9542663121930712809ULL), "InputCandidates"});
  definition.m_Interface.PushBack({"MeshletMetadata", false, xiiUuid(13789234341475564839ULL, 8268872759528048157ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(3419321221088446302ULL, 15439103404839490721ULL), "VisibleInstances"});
  definition.m_Interface.PushBack({"GeometryMetadata", false, xiiUuid(13789234341475564839ULL, 8268872759528048157ULL), "GeometryMetadata"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateIrradianceLayerTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/IrradianceLayer";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(18106726907120780455ULL, 9198306414875178857ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("IrradianceLayerPass 1", xiiRTTI::FindTypeByName("xiiIrradianceLayerPass"));
  definition.m_Passes[0].m_Id = xiiUuid(16243029066194874431ULL, 2502541630171141209ULL);
  definition.m_Interface.PushBack({"Irradiance", false, xiiUuid(16243029066194874431ULL, 2502541630171141209ULL), "Irradiance"});
  definition.m_Interface.PushBack({"CoarseIrradiance", true, xiiUuid(16243029066194874431ULL, 2502541630171141209ULL), "CoarseIrradiance"});
  definition.m_Interface.PushBack({"DetailIrradiance", true, xiiUuid(16243029066194874431ULL, 2502541630171141209ULL), "DetailIrradiance"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateLayeredDiffuseLightingTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/LayeredDiffuseLighting";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(18348534254417539673ULL, 1635844554369454961ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("DiffuseSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDiffuseSurfaceLightingPass"));
  definition.m_Passes[0].m_Id = xiiUuid(15948265801432089621ULL, 5458605410531472776ULL);
  definition.AddPass("IrradianceLayerPass 1", xiiRTTI::FindTypeByName("xiiIrradianceLayerPass"));
  definition.m_Passes[1].m_Id = xiiUuid(13137309955886705039ULL, 7855672903343648924ULL);
  definition.Connect(1, "Irradiance", 0, "Irradiance").AssertSuccess();
  definition.m_Interface.PushBack({"Material", true, xiiUuid(15948265801432089621ULL, 5458605410531472776ULL), "Material"});
  definition.m_Interface.PushBack({"DiffuseLighting", false, xiiUuid(15948265801432089621ULL, 5458605410531472776ULL), "Lighting"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(15948265801432089621ULL, 5458605410531472776ULL), "Depth"});
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(15948265801432089621ULL, 5458605410531472776ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"CombinedIrradiance", false, xiiUuid(13137309955886705039ULL, 7855672903343648924ULL), "Irradiance"});
  definition.m_Interface.PushBack({"DetailIrradiance", true, xiiUuid(13137309955886705039ULL, 7855672903343648924ULL), "DetailIrradiance"});
  definition.m_Interface.PushBack({"CoarseIrradiance", true, xiiUuid(13137309955886705039ULL, 7855672903343648924ULL), "CoarseIrradiance"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(15948265801432089621ULL, 5458605410531472776ULL), "Albedo"});
  definition.m_Interface.PushBack({"Occlusion", true, xiiUuid(15948265801432089621ULL, 5458605410531472776ULL), "Occlusion"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(15948265801432089621ULL, 5458605410531472776ULL), "Emissive"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateLidarSensorTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/LidarSensor";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(1299372271414522513ULL, 18331906827950869246ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("LidarSensorPass 1", xiiRTTI::FindTypeByName("xiiLidarSensorPass"), Properties({{"UseViewProfile", true}}));
  definition.m_Passes[0].m_Id = xiiUuid(2645940857558441884ULL, 6590811267619766085ULL);
  definition.m_Interface.PushBack({"Constants", false, xiiUuid(2645940857558441884ULL, 6590811267619766085ULL), "Constants"});
  definition.m_Interface.PushBack({"Velocity", true, xiiUuid(2645940857558441884ULL, 6590811267619766085ULL), "Velocity"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(2645940857558441884ULL, 6590811267619766085ULL), "Normal"});
  definition.m_Interface.PushBack({"SensorOutput", false, xiiUuid(2645940857558441884ULL, 6590811267619766085ULL), "SensorOutput"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(2645940857558441884ULL, 6590811267619766085ULL), "Albedo"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(2645940857558441884ULL, 6590811267619766085ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(2645940857558441884ULL, 6590811267619766085ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"PerFrameGlobal", true, xiiUuid(2645940857558441884ULL, 6590811267619766085ULL), "PerFrameGlobal"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateMatrixInstanceVisibilityTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/MatrixInstanceVisibility";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(3287232609963273774ULL, 11080267310839553369ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[0].m_Id = xiiUuid(14144846873325843338ULL, 1357738108069859298ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[1].m_Id = xiiUuid(4214265619549189051ULL, 2846374186192797279ULL);
  definition.AddPass("MatrixVisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiMatrixVisibilityViewPass"));
  definition.m_Passes[2].m_Id = xiiUuid(14619564900575181377ULL, 4357214642417863276ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[3].m_Id = xiiUuid(4733047850120851606ULL, 4366947491539545970ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[4].m_Id = xiiUuid(1161223531519894189ULL, 7027225433226204089ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[5].m_Id = xiiUuid(4338295567586322438ULL, 11366560358801015920ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[6].m_Id = xiiUuid(770823033606589817ULL, 15802545375927479409ULL);
  definition.Connect(2, "Candidates", 4, "Candidates").AssertSuccess();
  definition.Connect(2, "VisibilityView", 4, "VisibilityView").AssertSuccess();
  definition.Connect(1, "Counter", 4, "Counter").AssertSuccess();
  definition.Connect(0, "PreviousBuffer", 4, "PreviousLodHistory").AssertSuccess();
  definition.Connect(5, "SceneInstances", 4, "SceneInstances").AssertSuccess();
  definition.Connect(5, "SceneInstances", 3, "SceneInstances").AssertSuccess();
  definition.Connect(6, "GeometryMetadata", 4, "GeometryMetadata").AssertSuccess();
  definition.Connect(4, "LodHistory", 0, "Buffer").AssertSuccess();
  definition.Connect(3, "LodHistory", 4, "InitializedLodHistory").AssertSuccess();
  definition.m_Interface.PushBack({"MeshletMetadata", false, xiiUuid(770823033606589817ULL, 15802545375927479409ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(14619564900575181377ULL, 4357214642417863276ULL), "InputCandidates"});
  definition.m_Interface.PushBack({"VisibilityView", false, xiiUuid(14619564900575181377ULL, 4357214642417863276ULL), "VisibilityView"});
  definition.m_Interface.PushBack({"SceneInstances", false, xiiUuid(4338295567586322438ULL, 11366560358801015920ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(1161223531519894189ULL, 7027225433226204089ULL), "VisibleInstances"});
  definition.m_Interface.PushBack({"Candidates", false, xiiUuid(14619564900575181377ULL, 4357214642417863276ULL), "Candidates"});
  definition.m_Interface.PushBack({"GeometryMetadata", false, xiiUuid(770823033606589817ULL, 15802545375927479409ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(1161223531519894189ULL, 7027225433226204089ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"SkinningMatrices", false, xiiUuid(4338295567586322438ULL, 11366560358801015920ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"LodHistory", false, xiiUuid(1161223531519894189ULL, 7027225433226204089ULL), "LodHistory"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateMatrixMeshletVisibilityTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/MatrixMeshletVisibility";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(15640110843777395324ULL, 6795617616666632265ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[0].m_Id = xiiUuid(4195989255797149198ULL, 2043765408738664025ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[1].m_Id = xiiUuid(15186355015124444800ULL, 6999628554909552061ULL);
  definition.AddPass("MatrixVisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiMatrixVisibilityViewPass"));
  definition.m_Passes[2].m_Id = xiiUuid(13817118759351973894ULL, 10942790044117700719ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[3].m_Id = xiiUuid(140267091360002908ULL, 14351814489024662886ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[4].m_Id = xiiUuid(13393228747473440539ULL, 11852331930088239451ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[5].m_Id = xiiUuid(11592551489816687396ULL, 16814622883313945544ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[6].m_Id = xiiUuid(16255346912113213219ULL, 6656593868484424343ULL);
  definition.AddPass("MeshletDispatchPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchPass"));
  definition.m_Passes[7].m_Id = xiiUuid(12727830767403988692ULL, 5670967848076288390ULL);
  definition.AddPass("MeshletCullPass 1", xiiRTTI::FindTypeByName("xiiMeshletCullPass"));
  definition.m_Passes[8].m_Id = xiiUuid(16687548248442237478ULL, 13932099133845615317ULL);
  definition.AddPass("MeshletDispatchBoundsPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchBoundsPass"));
  definition.m_Passes[9].m_Id = xiiUuid(5444067898272983730ULL, 14072453052715434654ULL);
  definition.AddPass("CounterResetPass 2", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[10].m_Id = xiiUuid(10678785462372721873ULL, 14673320778139597949ULL);
  definition.AddPass("CounterResetPass 3", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[11].m_Id = xiiUuid(9346148371423960562ULL, 16094712393961129167ULL);
  definition.AddPass("CounterResetPass 4", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[12].m_Id = xiiUuid(9257507230270058511ULL, 16859431540964331467ULL);
  definition.Connect(2, "Candidates", 4, "Candidates").AssertSuccess();
  definition.Connect(2, "VisibilityView", 4, "VisibilityView").AssertSuccess();
  definition.Connect(1, "Counter", 4, "Counter").AssertSuccess();
  definition.Connect(0, "PreviousBuffer", 4, "PreviousLodHistory").AssertSuccess();
  definition.Connect(5, "SceneInstances", 4, "SceneInstances").AssertSuccess();
  definition.Connect(5, "SceneInstances", 3, "SceneInstances").AssertSuccess();
  definition.Connect(6, "GeometryMetadata", 4, "GeometryMetadata").AssertSuccess();
  definition.Connect(4, "LodHistory", 0, "Buffer").AssertSuccess();
  definition.Connect(3, "LodHistory", 4, "InitializedLodHistory").AssertSuccess();
  definition.Connect(2, "VisibilityView", 9, "VisibilityView").AssertSuccess();
  definition.Connect(6, "MeshletMetadata", 7, "MeshletMetadata").AssertSuccess();
  definition.Connect(7, "DispatchArguments", 8, "DispatchArguments").AssertSuccess();
  definition.Connect(4, "VisibleInstances", 7, "VisibleInstances").AssertSuccess();
  definition.Connect(6, "GeometryMetadata", 9, "GeometryMetadata").AssertSuccess();
  definition.Connect(4, "LodHistory", 9, "LodHistory").AssertSuccess();
  definition.Connect(4, "VisibleInstanceCount", 9, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(6, "MeshletMetadata", 9, "MeshletMetadata").AssertSuccess();
  definition.Connect(2, "VisibilityView", 8, "VisibilityView").AssertSuccess();
  definition.Connect(11, "Counter", 8, "InitialOverflow").AssertSuccess();
  definition.Connect(4, "LodHistory", 8, "LodHistory").AssertSuccess();
  definition.Connect(4, "VisibleInstances", 9, "VisibleInstances").AssertSuccess();
  definition.Connect(4, "VisibleInstanceCount", 8, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(10, "Counter", 8, "InitialCount").AssertSuccess();
  definition.Connect(6, "MeshletMetadata", 8, "MeshletMetadata").AssertSuccess();
  definition.Connect(5, "SceneInstances", 9, "SceneInstances").AssertSuccess();
  definition.Connect(12, "Counter", 9, "InitializedMaximum").AssertSuccess();
  definition.Connect(9, "MaximumMeshletGroups", 7, "MaximumMeshletGroups").AssertSuccess();
  definition.Connect(5, "SceneInstances", 8, "SceneInstances").AssertSuccess();
  definition.Connect(4, "VisibleInstanceCount", 7, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(4, "VisibleInstances", 8, "VisibleInstances").AssertSuccess();
  definition.Connect(6, "GeometryMetadata", 8, "GeometryMetadata").AssertSuccess();
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(13817118759351973894ULL, 10942790044117700719ULL), "InputCandidates"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", false, xiiUuid(16687548248442237478ULL, 13932099133845615317ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"VisibleMeshlets", false, xiiUuid(16687548248442237478ULL, 13932099133845615317ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"LodHistory", false, xiiUuid(13393228747473440539ULL, 11852331930088239451ULL), "LodHistory"});
  definition.m_Interface.PushBack({"DispatchArguments", false, xiiUuid(12727830767403988692ULL, 5670967848076288390ULL), "DispatchArguments"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(13393228747473440539ULL, 11852331930088239451ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"SceneInstances", false, xiiUuid(11592551489816687396ULL, 16814622883313945544ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"Candidates", false, xiiUuid(13817118759351973894ULL, 10942790044117700719ULL), "Candidates"});
  definition.m_Interface.PushBack({"GeometryMetadata", false, xiiUuid(16255346912113213219ULL, 6656593868484424343ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"VisibilityView", false, xiiUuid(13817118759351973894ULL, 10942790044117700719ULL), "VisibilityView"});
  definition.m_Interface.PushBack({"VisibleMeshletOverflow", false, xiiUuid(16687548248442237478ULL, 13932099133845615317ULL), "VisibleMeshletOverflow"});
  definition.m_Interface.PushBack({"SkinningMatrices", false, xiiUuid(11592551489816687396ULL, 16814622883313945544ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"MeshletMetadata", false, xiiUuid(16255346912113213219ULL, 6656593868484424343ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(13393228747473440539ULL, 11852331930088239451ULL), "VisibleInstances"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateMeshletOcclusionTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/MeshletOcclusion";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(9395614551580720958ULL, 7868138254915845541ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("MeshletOcclusionPass 1", xiiRTTI::FindTypeByName("xiiMeshletOcclusionPass"));
  definition.m_Passes[0].m_Id = xiiUuid(13079132965333101115ULL, 12103972249155221909ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[1].m_Id = xiiUuid(13909714461930665240ULL, 12663204063750564756ULL);
  definition.Connect(1, "Counter", 0, "InitialCount").AssertSuccess();
  definition.m_Interface.PushBack({"CandidateCount", true, xiiUuid(13079132965333101115ULL, 12103972249155221909ULL), "CandidateCount"});
  definition.m_Interface.PushBack({"OcclusionView", true, xiiUuid(13079132965333101115ULL, 12103972249155221909ULL), "OcclusionView"});
  definition.m_Interface.PushBack({"SceneInstances", true, xiiUuid(13079132965333101115ULL, 12103972249155221909ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"VisibleMeshlets", false, xiiUuid(13079132965333101115ULL, 12103972249155221909ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"Candidates", true, xiiUuid(13079132965333101115ULL, 12103972249155221909ULL), "Candidates"});
  definition.m_Interface.PushBack({"DepthPyramid", true, xiiUuid(13079132965333101115ULL, 12103972249155221909ULL), "DepthPyramid"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", false, xiiUuid(13079132965333101115ULL, 12103972249155221909ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"MeshletMetadata", true, xiiUuid(13079132965333101115ULL, 12103972249155221909ULL), "MeshletMetadata"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateMeshletVisibilityTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/MeshletVisibility";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(4235395081534519860ULL, 10297889791413546581ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("MeshletDispatchBoundsPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchBoundsPass"));
  definition.m_Passes[0].m_Id = xiiUuid(10345073419523890954ULL, 377831662642152682ULL);
  definition.AddPass("MeshletCullPass 1", xiiRTTI::FindTypeByName("xiiMeshletCullPass"));
  definition.m_Passes[1].m_Id = xiiUuid(4996144304626503536ULL, 2936267479205802730ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[2].m_Id = xiiUuid(2863448418517724900ULL, 4054723193158420086ULL);
  definition.AddPass("CounterResetPass 2", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[3].m_Id = xiiUuid(8169663217998486624ULL, 11471663941708054137ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[4].m_Id = xiiUuid(10614185370637245130ULL, 11210932510755767294ULL);
  definition.AddPass("VisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiVisibilityViewPass"));
  definition.m_Passes[5].m_Id = xiiUuid(15034820722799948330ULL, 6537206351329167549ULL);
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[6].m_Id = xiiUuid(3916767448559748542ULL, 3587852197715292074ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[7].m_Id = xiiUuid(8043419656214158353ULL, 16286703440896908853ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[8].m_Id = xiiUuid(13084865205747132442ULL, 8520298556219739594ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[9].m_Id = xiiUuid(6879097375821107539ULL, 13844643504832435000ULL);
  definition.AddPass("CounterResetPass 3", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[10].m_Id = xiiUuid(42334412586691535ULL, 13905295182088430627ULL);
  definition.AddPass("CounterResetPass 4", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[11].m_Id = xiiUuid(6002700649370708523ULL, 14753379014255740575ULL);
  definition.AddPass("MeshletDispatchPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchPass"));
  definition.m_Passes[12].m_Id = xiiUuid(13480682106763264556ULL, 18000249364659120258ULL);
  definition.Connect(4, "GeometryMetadata", 9, "GeometryMetadata").AssertSuccess();
  definition.Connect(7, "LodHistory", 9, "InitializedLodHistory").AssertSuccess();
  definition.Connect(8, "SceneInstances", 9, "SceneInstances").AssertSuccess();
  definition.Connect(8, "SceneInstances", 7, "SceneInstances").AssertSuccess();
  definition.Connect(10, "Counter", 9, "Counter").AssertSuccess();
  definition.Connect(5, "Candidates", 9, "Candidates").AssertSuccess();
  definition.Connect(6, "PreviousBuffer", 9, "PreviousLodHistory").AssertSuccess();
  definition.Connect(9, "LodHistory", 6, "Buffer").AssertSuccess();
  definition.Connect(5, "VisibilityView", 9, "VisibilityView").AssertSuccess();
  definition.Connect(4, "GeometryMetadata", 1, "GeometryMetadata").AssertSuccess();
  definition.Connect(9, "LodHistory", 0, "LodHistory").AssertSuccess();
  definition.Connect(4, "MeshletMetadata", 12, "MeshletMetadata").AssertSuccess();
  definition.Connect(8, "SceneInstances", 0, "SceneInstances").AssertSuccess();
  definition.Connect(9, "VisibleInstanceCount", 1, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(2, "Counter", 1, "InitialCount").AssertSuccess();
  definition.Connect(5, "VisibilityView", 0, "VisibilityView").AssertSuccess();
  definition.Connect(9, "VisibleInstanceCount", 12, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(4, "GeometryMetadata", 0, "GeometryMetadata").AssertSuccess();
  definition.Connect(3, "Counter", 0, "InitializedMaximum").AssertSuccess();
  definition.Connect(8, "SceneInstances", 1, "SceneInstances").AssertSuccess();
  definition.Connect(9, "VisibleInstances", 1, "VisibleInstances").AssertSuccess();
  definition.Connect(0, "MaximumMeshletGroups", 12, "MaximumMeshletGroups").AssertSuccess();
  definition.Connect(9, "VisibleInstances", 12, "VisibleInstances").AssertSuccess();
  definition.Connect(9, "VisibleInstances", 0, "VisibleInstances").AssertSuccess();
  definition.Connect(11, "Counter", 1, "InitialOverflow").AssertSuccess();
  definition.Connect(12, "DispatchArguments", 1, "DispatchArguments").AssertSuccess();
  definition.Connect(4, "MeshletMetadata", 1, "MeshletMetadata").AssertSuccess();
  definition.Connect(9, "LodHistory", 1, "LodHistory").AssertSuccess();
  definition.Connect(9, "VisibleInstanceCount", 0, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(4, "MeshletMetadata", 0, "MeshletMetadata").AssertSuccess();
  definition.Connect(5, "VisibilityView", 1, "VisibilityView").AssertSuccess();
  definition.m_Interface.PushBack({"GeometryMetadata", false, xiiUuid(10614185370637245130ULL, 11210932510755767294ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"SceneInstances", false, xiiUuid(13084865205747132442ULL, 8520298556219739594ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"SkinningMatrices", false, xiiUuid(13084865205747132442ULL, 8520298556219739594ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"MeshletMetadata", false, xiiUuid(10614185370637245130ULL, 11210932510755767294ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"VisibilityView", false, xiiUuid(15034820722799948330ULL, 6537206351329167549ULL), "VisibilityView"});
  definition.m_Interface.PushBack({"DispatchArguments", false, xiiUuid(13480682106763264556ULL, 18000249364659120258ULL), "DispatchArguments"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(6879097375821107539ULL, 13844643504832435000ULL), "VisibleInstances"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(6879097375821107539ULL, 13844643504832435000ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", false, xiiUuid(4996144304626503536ULL, 2936267479205802730ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"LodHistory", false, xiiUuid(6879097375821107539ULL, 13844643504832435000ULL), "LodHistory"});
  definition.m_Interface.PushBack({"VisibleMeshletOverflow", false, xiiUuid(4996144304626503536ULL, 2936267479205802730ULL), "VisibleMeshletOverflow"});
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(15034820722799948330ULL, 6537206351329167549ULL), "InputCandidates"});
  definition.m_Interface.PushBack({"VisibleMeshlets", false, xiiUuid(4996144304626503536ULL, 2936267479205802730ULL), "VisibleMeshlets"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateOpaqueCompositionTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/OpaqueComposition";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(8711745446236522806ULL, 8075879625039058199ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("ScreenSpaceReflectionsPass 1", xiiRTTI::FindTypeByName("xiiScreenSpaceReflectionsPass"));
  definition.m_Passes[0].m_Id = xiiUuid(14967694722601598231ULL, 1043517102153277010ULL);
  definition.AddPass("RadianceReplacePass 1", xiiRTTI::FindTypeByName("xiiRadianceReplacePass"));
  definition.m_Passes[1].m_Id = xiiUuid(16518978834382752911ULL, 12704962462092383471ULL);
  definition.AddPass("ConfidenceBlendPass 1", xiiRTTI::FindTypeByName("xiiConfidenceBlendPass"));
  definition.m_Passes[2].m_Id = xiiUuid(16665290791082630377ULL, 854878152842290275ULL);
  definition.AddPass("ConfidenceBlendPass 2", xiiRTTI::FindTypeByName("xiiConfidenceBlendPass"));
  definition.m_Passes[3].m_Id = xiiUuid(17930521259669939719ULL, 16866396203904746793ULL);
  definition.AddPass("TextureSelectionPass 1", xiiRTTI::FindTypeByName("xiiTextureSelectionPass"));
  definition.m_Passes[4].m_Id = xiiUuid(10138069768760136295ULL, 9211749055447873217ULL);
  definition.AddPass("RadianceAddPass 1", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[5].m_Id = xiiUuid(9619782667002607264ULL, 16915897179723175893ULL);
  definition.AddPass("RadianceAddPass 2", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[6].m_Id = xiiUuid(10081513483762030614ULL, 14278539684508424791ULL);
  definition.AddPass("ConfidenceRadiancePass 1", xiiRTTI::FindTypeByName("xiiConfidenceRadiancePass"));
  definition.m_Passes[7].m_Id = xiiUuid(15492376581874017846ULL, 14646418279785778079ULL);
  definition.AddPass("RadianceAddPass 3", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[8].m_Id = xiiUuid(16553670675949834078ULL, 2873164145789393412ULL);
  definition.AddPass("VolumetricCompositePass 1", xiiRTTI::FindTypeByName("xiiVolumetricCompositePass"));
  definition.m_Passes[9].m_Id = xiiUuid(8162471756539682251ULL, 16135519373173281570ULL);
  definition.AddPass("ScreenSpaceReflectionsTemporalPass 1", xiiRTTI::FindTypeByName("xiiScreenSpaceReflectionsTemporalPass"));
  definition.m_Passes[10].m_Id = xiiUuid(4636142280475155744ULL, 13924201128523914742ULL);
  definition.AddPass("TextureSelectionPass 2", xiiRTTI::FindTypeByName("xiiTextureSelectionPass"));
  definition.m_Passes[11].m_Id = xiiUuid(8573484341777117200ULL, 15153788602281734716ULL);
  definition.AddPass("TextureSelectionPass 3", xiiRTTI::FindTypeByName("xiiTextureSelectionPass"));
  definition.m_Passes[12].m_Id = xiiUuid(5854133695977897071ULL, 16582261059846360024ULL);
  definition.Connect(2, "Radiance", 3, "Background").AssertSuccess();
  definition.Connect(3, "Radiance", 1, "Replacement").AssertSuccess();
  definition.Connect(5, "Radiance", 8, "A").AssertSuccess();
  definition.Connect(8, "Radiance", 9, "SceneRadiance").AssertSuccess();
  definition.Connect(6, "Radiance", 5, "A").AssertSuccess();
  definition.Connect(7, "Radiance", 8, "B").AssertSuccess();
  definition.Connect(11, "Texture", 1, "Scene").AssertSuccess();
  definition.Connect(4, "Texture", 10, "SSRRawTerm").AssertSuccess();
  definition.Connect(10, "SSRTerm", 12, "Fallback").AssertSuccess();
  definition.Connect(11, "Texture", 0, "HDRSceneColor").AssertSuccess();
  definition.Connect(0, "SSRRawTerm", 4, "Fallback").AssertSuccess();
  definition.Connect(12, "Texture", 3, "Layer").AssertSuccess();
  definition.Connect(9, "Radiance", 11, "Fallback").AssertSuccess();
  definition.m_Interface.PushBack({"IndirectLighting", true, xiiUuid(10081513483762030614ULL, 14278539684508424791ULL), "B"});
  definition.m_Interface.PushBack({"GBufferAlbedo", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "GBufferAlbedo"});
  definition.m_Interface.PushBack({"EnvironmentSpecular", true, xiiUuid(16665290791082630377ULL, 854878152842290275ULL), "Background"});
  definition.m_Interface.PushBack({"EnvironmentSpecular", true, xiiUuid(16518978834382752911ULL, 12704962462092383471ULL), "PreviousContribution"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"GBufferMaterial", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "GBufferMaterial"});
  definition.m_Interface.PushBack({"GBufferMaterial", true, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "GBufferMaterial"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"DirectLighting", true, xiiUuid(10081513483762030614ULL, 14278539684508424791ULL), "A"});
  definition.m_Interface.PushBack({"VolumetricScattering", true, xiiUuid(8162471756539682251ULL, 16135519373173281570ULL), "ScatteringOpacity"});
  definition.m_Interface.PushBack({"HiZClosestPyramid", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "HiZClosestPyramid"});
  definition.m_Interface.PushBack({"HDRSceneColor", true, xiiUuid(8573484341777117200ULL, 15153788602281734716ULL), "Primary"});
  definition.m_Interface.PushBack({"SSRTerm", true, xiiUuid(5854133695977897071ULL, 16582261059846360024ULL), "Primary"});
  definition.m_Interface.PushBack({"PerFrameGlobal", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "PerFrameGlobal"});
  definition.m_Interface.PushBack({"HiZPyramid", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "HiZPyramid"});
  definition.m_Interface.PushBack({"ReSTIRDISurfacePrevious", true, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "ReSTIRDISurfacePrevious"});
  definition.m_Interface.PushBack({"GBufferNormal", true, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "GBufferNormal"});
  definition.m_Interface.PushBack({"GBufferNormal", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "GBufferNormal"});
  definition.m_Interface.PushBack({"SSRRawTerm", true, xiiUuid(10138069768760136295ULL, 9211749055447873217ULL), "Primary"});
  definition.m_Interface.PushBack({"SSRTerm", false, xiiUuid(5854133695977897071ULL, 16582261059846360024ULL), "Texture"});
  definition.m_Interface.PushBack({"RTFinalGI", true, xiiUuid(15492376581874017846ULL, 14646418279785778079ULL), "Signal"});
  definition.m_Interface.PushBack({"DilatedVelocityBuffer", true, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "DilatedVelocityBuffer"});
  definition.m_Interface.PushBack({"RTFinalReflections", true, xiiUuid(16665290791082630377ULL, 854878152842290275ULL), "Layer"});
  definition.m_Interface.PushBack({"GBufferEmissive", true, xiiUuid(9619782667002607264ULL, 16915897179723175893ULL), "B"});
  definition.m_Interface.PushBack({"SelectedReflection", false, xiiUuid(17930521259669939719ULL, 16866396203904746793ULL), "Radiance"});
  definition.m_Interface.PushBack({"PreviousReflection", false, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "PreviousReflection"});
  definition.m_Interface.PushBack({"xiiSSRConstants", false, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "xiiSSRConstants"});
  definition.m_Interface.PushBack({"xiiSSRTemporalConstants", false, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "xiiSSRTemporalConstants"});
  definition.m_Interface.PushBack({"BRDFLut", true, xiiUuid(14967694722601598231ULL, 1043517102153277010ULL), "BRDFLut"});
  definition.m_Interface.PushBack({"HDRSceneColor", false, xiiUuid(16518978834382752911ULL, 12704962462092383471ULL), "Radiance"});
  definition.m_Interface.PushBack({"GBufferObjectId", true, xiiUuid(4636142280475155744ULL, 13924201128523914742ULL), "GBufferObjectId"});
  definition.m_Interface.PushBack({"SSRRawTerm", false, xiiUuid(10138069768760136295ULL, 9211749055447873217ULL), "Texture"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}



xiiRenderPipelineResourceHandle xiiCreateReflectionCompositionTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ReflectionComposition";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(3040264461951337911ULL, 3150918154981013115ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("RadianceReplacePass 1", xiiRTTI::FindTypeByName("xiiRadianceReplacePass"));
  definition.m_Passes[0].m_Id = xiiUuid(12807320791531271412ULL, 3027488066675869735ULL);
  definition.AddPass("ConfidenceBlendPass 1", xiiRTTI::FindTypeByName("xiiConfidenceBlendPass"));
  definition.m_Passes[1].m_Id = xiiUuid(11888925004933453672ULL, 16572323728904579559ULL);
  definition.AddPass("ConfidenceBlendPass 2", xiiRTTI::FindTypeByName("xiiConfidenceBlendPass"));
  definition.m_Passes[2].m_Id = xiiUuid(16586920730771583552ULL, 18271719899202054484ULL);
  definition.Connect(1, "Radiance", 2, "Background").AssertSuccess();
  definition.Connect(2, "Radiance", 0, "Replacement").AssertSuccess();
  definition.m_Interface.PushBack({"SSRTerm", true, xiiUuid(16586920730771583552ULL, 18271719899202054484ULL), "Layer"});
  definition.m_Interface.PushBack({"RTFinalReflections", true, xiiUuid(11888925004933453672ULL, 16572323728904579559ULL), "Layer"});
  definition.m_Interface.PushBack({"HDRSceneColor", false, xiiUuid(12807320791531271412ULL, 3027488066675869735ULL), "Radiance"});
  definition.m_Interface.PushBack({"EnvironmentSpecular", true, xiiUuid(11888925004933453672ULL, 16572323728904579559ULL), "Background"});
  definition.m_Interface.PushBack({"EnvironmentSpecular", true, xiiUuid(12807320791531271412ULL, 3027488066675869735ULL), "PreviousContribution"});
  definition.m_Interface.PushBack({"SelectedReflection", false, xiiUuid(16586920730771583552ULL, 18271719899202054484ULL), "Radiance"});
  definition.m_Interface.PushBack({"HDRSceneColor", true, xiiUuid(12807320791531271412ULL, 3027488066675869735ULL), "Scene"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateReflectionSelectionTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ReflectionSelection";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(5478061005971027827ULL, 7063166405281416166ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("ConfidenceBlendPass 1", xiiRTTI::FindTypeByName("xiiConfidenceBlendPass"));
  definition.m_Passes[0].m_Id = xiiUuid(4556197879323637007ULL, 989935955054303816ULL);
  definition.AddPass("ConfidenceBlendPass 2", xiiRTTI::FindTypeByName("xiiConfidenceBlendPass"));
  definition.m_Passes[1].m_Id = xiiUuid(2447726942221121703ULL, 5179591225992370774ULL);
  definition.Connect(0, "Radiance", 1, "Background").AssertSuccess();
  definition.m_Interface.PushBack({"RTFinalReflections", true, xiiUuid(4556197879323637007ULL, 989935955054303816ULL), "Layer"});
  definition.m_Interface.PushBack({"SSRTerm", true, xiiUuid(2447726942221121703ULL, 5179591225992370774ULL), "Layer"});
  definition.m_Interface.PushBack({"SelectedReflection", false, xiiUuid(2447726942221121703ULL, 5179591225992370774ULL), "Radiance"});
  definition.m_Interface.PushBack({"EnvironmentSpecular", true, xiiUuid(4556197879323637007ULL, 989935955054303816ULL), "Background"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateRgbSensorTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/RgbSensor";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(10897487008026655459ULL, 6605048801064152480ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("RgbSensorPass 1", xiiRTTI::FindTypeByName("xiiRgbSensorPass"), Properties({{"UseViewProfile", true}}));
  definition.m_Passes[0].m_Id = xiiUuid(1889043740548167043ULL, 1107814470265882473ULL);
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(1889043740548167043ULL, 1107814470265882473ULL), "Radiance"});
  definition.m_Interface.PushBack({"Constants", false, xiiUuid(1889043740548167043ULL, 1107814470265882473ULL), "Constants"});
  definition.m_Interface.PushBack({"SensorOutput", false, xiiUuid(1889043740548167043ULL, 1107814470265882473ULL), "SensorOutput"});
  definition.m_Interface.PushBack({"PerFrameGlobal", true, xiiUuid(1889043740548167043ULL, 1107814470265882473ULL), "PerFrameGlobal"});
  definition.m_Interface.PushBack({"Velocity", true, xiiUuid(1889043740548167043ULL, 1107814470265882473ULL), "Velocity"});
  definition.m_Interface.PushBack({"PerFrameCamera", true, xiiUuid(1889043740548167043ULL, 1107814470265882473ULL), "PerFrameCamera"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(1889043740548167043ULL, 1107814470265882473ULL), "SceneDepth"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateSceneTargetsTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/SceneTargets";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(6655904598015543426ULL, 7860496177351265078ULL);
  definition.AddPass("ClearTexturePass 1", xiiRTTI::FindTypeByName("xiiClearTexturePass"), Properties({{"Color", xiiColor(0.00800000037997961f, 0.012000000104308128f, 0.02500000037252903f, 1.0f)}, {"Texture", Properties({{"Format", xiiInt64(29LL)}, {"Requirements", Properties({{"Dimension", xiiInt64(0LL)}, {"Height", xiiUInt32(0U)}, {"RequiredBindFlags", xiiInt64(0LL)}, {"Samples", xiiUInt32(0U)}, {"Width", xiiUInt32(0U)}})}, {"Resolution", Properties({{"Height", xiiUInt32(0U)}, {"Input", xiiString("")}, {"Mode", xiiInt64(1LL)}, {"Scale", 1.0f}, {"Width", xiiUInt32(0U)}})}})}}));
  definition.m_Passes[0].m_Id = xiiUuid(2106034439496507148ULL, 9930976772311898302ULL);
  definition.AddPass("DepthTargetPass 1", xiiRTTI::FindTypeByName("xiiDepthTargetPass"), Properties({{"DepthTarget", Properties({{"Format", xiiInt64(40LL)}, {"Requirements", Properties({{"Dimension", xiiInt64(0LL)}, {"Height", xiiUInt32(0U)}, {"RequiredBindFlags", xiiInt64(0LL)}, {"Samples", xiiUInt32(0U)}, {"Width", xiiUInt32(0U)}})}, {"Resolution", Properties({{"Height", xiiUInt32(0U)}, {"Input", xiiString("")}, {"Mode", xiiInt64(1LL)}, {"Scale", 1.0f}, {"Width", xiiUInt32(0U)}})}})}}));
  definition.m_Passes[1].m_Id = xiiUuid(16593442625342750333ULL, 14151768542942910819ULL);
  definition.m_Interface.PushBack({"Color", false, xiiUuid(2106034439496507148ULL, 9930976772311898302ULL), "Texture"});
  definition.m_Interface.PushBack({"Depth", false, xiiUuid(16593442625342750333ULL, 14151768542942910819ULL), "DepthTarget"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateScreenDiffuseLightingTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ScreenDiffuseLighting";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(10278261238104045071ULL, 17683428430908384669ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("EnvironmentLightConstantsPass 1", xiiRTTI::FindTypeByName("xiiEnvironmentLightConstantsPass"), Properties({{"AmbientRadiance", xiiVec3(0.0f, 0.0f, 0.0f)}}));
  definition.m_Passes[0].m_Id = xiiUuid(16639249855577324489ULL, 6213782259055210809ULL);
  definition.AddPass("DiffuseSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDiffuseSurfaceLightingPass"));
  definition.m_Passes[1].m_Id = xiiUuid(2275812571344115705ULL, 9000230713680815427ULL);
  definition.AddPass("ScreenIrradianceTracePass 1", xiiRTTI::FindTypeByName("xiiScreenIrradianceTracePass"));
  definition.m_Passes[2].m_Id = xiiUuid(16168116802015019809ULL, 5737190872992485015ULL);
  definition.Connect(0, "EnvironmentLight", 1, "EnvironmentLight").AssertSuccess();
  definition.Connect(2, "Irradiance", 1, "Irradiance").AssertSuccess();
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(16168116802015019809ULL, 5737190872992485015ULL), "Normal"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(2275812571344115705ULL, 9000230713680815427ULL), "Albedo"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(16168116802015019809ULL, 5737190872992485015ULL), "Depth"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(2275812571344115705ULL, 9000230713680815427ULL), "Depth"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(16168116802015019809ULL, 5737190872992485015ULL), "Camera"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(2275812571344115705ULL, 9000230713680815427ULL), "Material"});
  definition.m_Interface.PushBack({"Irradiance", false, xiiUuid(16168116802015019809ULL, 5737190872992485015ULL), "Irradiance"});
  definition.m_Interface.PushBack({"Lighting", false, xiiUuid(2275812571344115705ULL, 9000230713680815427ULL), "Lighting"});
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(16168116802015019809ULL, 5737190872992485015ULL), "Radiance"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(2275812571344115705ULL, 9000230713680815427ULL), "Emissive"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateScreenEffectsTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ScreenEffects";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(8199094342251595706ULL, 4251488879245364093ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("ConfidenceRadiancePass 1", xiiRTTI::FindTypeByName("xiiConfidenceRadiancePass"));
  definition.m_Passes[0].m_Id = xiiUuid(16043744743126432600ULL, 12041752158517615885ULL);
  definition.AddPass("RadianceAddPass 1", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[1].m_Id = xiiUuid(8805579378710562388ULL, 12082492896731865837ULL);
  definition.AddPass("ScreenRefractionTracePass 1", xiiRTTI::FindTypeByName("xiiScreenRefractionTracePass"));
  definition.m_Passes[2].m_Id = xiiUuid(8801869399431990474ULL, 13285975441210724081ULL);
  definition.AddPass("AtmosphereApplyPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereApplyPass"));
  definition.m_Passes[3].m_Id = xiiUuid(15674601982841584127ULL, 14492813330792820568ULL);
  definition.AddPass("RadianceAddPass 2", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[4].m_Id = xiiUuid(16547146576860885855ULL, 5183929136225262611ULL);
  definition.AddPass("TextureSelectionPass 1", xiiRTTI::FindTypeByName("xiiTextureSelectionPass"));
  definition.m_Passes[5].m_Id = xiiUuid(11010367291055903129ULL, 6960238543887886190ULL);
  definition.AddPass("AtmosphereConstantsPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereConstantsPass"), Properties({{"UseWorldAtmosphere", true}}));
  definition.m_Passes[6].m_Id = xiiUuid(9826015577700533508ULL, 9696918723530808323ULL);
  definition.AddPass("EnvironmentLightConstantsPass 1", xiiRTTI::FindTypeByName("xiiEnvironmentLightConstantsPass"), Properties({{"AmbientRadiance", xiiVec3(0.0f, 0.0f, 0.0f)}}));
  definition.m_Passes[7].m_Id = xiiUuid(11457162838411182568ULL, 8404597486490718394ULL);
  definition.AddPass("DiffuseSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDiffuseSurfaceLightingPass"));
  definition.m_Passes[8].m_Id = xiiUuid(11197482510196373326ULL, 13741472510094062395ULL);
  definition.AddPass("ScreenIrradianceTracePass 1", xiiRTTI::FindTypeByName("xiiScreenIrradianceTracePass"));
  definition.m_Passes[9].m_Id = xiiUuid(9011583596168081882ULL, 8037772720359688992ULL);
  definition.AddPass("TextureSelectionPass 2", xiiRTTI::FindTypeByName("xiiTextureSelectionPass"));
  definition.m_Passes[10].m_Id = xiiUuid(15637113781696150445ULL, 14319455098585107005ULL);
  definition.Connect(0, "Radiance", 1, "B").AssertSuccess();
  definition.Connect(2, "Refraction", 0, "Signal").AssertSuccess();
  definition.Connect(7, "EnvironmentLight", 8, "EnvironmentLight").AssertSuccess();
  definition.Connect(9, "Irradiance", 8, "Irradiance").AssertSuccess();
  definition.Connect(1, "Radiance", 3, "Radiance").AssertSuccess();
  definition.Connect(10, "Texture", 4, "B").AssertSuccess();
  definition.Connect(8, "Lighting", 10, "Fallback").AssertSuccess();
  definition.Connect(4, "Radiance", 1, "A").AssertSuccess();
  definition.Connect(4, "Radiance", 5, "Fallback").AssertSuccess();
  definition.Connect(5, "Texture", 2, "Radiance").AssertSuccess();
  definition.Connect(6, "AtmosphereConstants", 3, "Atmosphere").AssertSuccess();
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(8801869399431990474ULL, 13285975441210724081ULL), "Depth"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(15674601982841584127ULL, 14492813330792820568ULL), "Depth"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(9011583596168081882ULL, 8037772720359688992ULL), "Depth"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(11197482510196373326ULL, 13741472510094062395ULL), "Depth"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(11197482510196373326ULL, 13741472510094062395ULL), "Emissive"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(8801869399431990474ULL, 13285975441210724081ULL), "Emissive"});
  definition.m_Interface.PushBack({"ScreenLighting", true, xiiUuid(15637113781696150445ULL, 14319455098585107005ULL), "Primary"});
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(9011583596168081882ULL, 8037772720359688992ULL), "Radiance"});
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(16547146576860885855ULL, 5183929136225262611ULL), "A"});
  definition.m_Interface.PushBack({"RefractionBackground", true, xiiUuid(11010367291055903129ULL, 6960238543887886190ULL), "Primary"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(11197482510196373326ULL, 13741472510094062395ULL), "Albedo"});
  definition.m_Interface.PushBack({"RefractionSignal", false, xiiUuid(8801869399431990474ULL, 13285975441210724081ULL), "Refraction"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(8801869399431990474ULL, 13285975441210724081ULL), "Normal"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(9011583596168081882ULL, 8037772720359688992ULL), "Normal"});
  definition.m_Interface.PushBack({"ScreenIrradiance", false, xiiUuid(9011583596168081882ULL, 8037772720359688992ULL), "Irradiance"});
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(15674601982841584127ULL, 14492813330792820568ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"Transmittance", true, xiiUuid(15674601982841584127ULL, 14492813330792820568ULL), "Transmittance"});
  definition.m_Interface.PushBack({"TransmittedRadiance", false, xiiUuid(16043744743126432600ULL, 12041752158517615885ULL), "Radiance"});
  definition.m_Interface.PushBack({"MultiScatter", true, xiiUuid(15674601982841584127ULL, 14492813330792820568ULL), "MultiScatter"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(11197482510196373326ULL, 13741472510094062395ULL), "Material"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(8801869399431990474ULL, 13285975441210724081ULL), "Material"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(9011583596168081882ULL, 8037772720359688992ULL), "Camera"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(15674601982841584127ULL, 14492813330792820568ULL), "Camera"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(8801869399431990474ULL, 13285975441210724081ULL), "Camera"});
  definition.m_Interface.PushBack({"Radiance", false, xiiUuid(15674601982841584127ULL, 14492813330792820568ULL), "AtmosphereRadiance"});
  definition.m_Interface.PushBack({"ScreenLighting", false, xiiUuid(15637113781696150445ULL, 14319455098585107005ULL), "Texture"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateScreenIrradianceTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ScreenIrradiance";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(1210659510881776681ULL, 14605735366065831716ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("ScreenIrradianceTracePass 1", xiiRTTI::FindTypeByName("xiiScreenIrradianceTracePass"));
  definition.m_Passes[0].m_Id = xiiUuid(2039938884667676837ULL, 14397473519547250869ULL);
  definition.m_Interface.PushBack({"Irradiance", false, xiiUuid(2039938884667676837ULL, 14397473519547250869ULL), "Irradiance"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(2039938884667676837ULL, 14397473519547250869ULL), "Depth"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(2039938884667676837ULL, 14397473519547250869ULL), "Normal"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(2039938884667676837ULL, 14397473519547250869ULL), "Camera"});
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(2039938884667676837ULL, 14397473519547250869ULL), "Radiance"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateScreenRefractionTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ScreenRefraction";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(7469825329607710489ULL, 857795769080121327ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("ConfidenceRadiancePass 1", xiiRTTI::FindTypeByName("xiiConfidenceRadiancePass"));
  definition.m_Passes[0].m_Id = xiiUuid(6752151343517858170ULL, 1485645550504798878ULL);
  definition.AddPass("RadianceAddPass 1", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[1].m_Id = xiiUuid(16498187493160506195ULL, 11100420922680318278ULL);
  definition.AddPass("ScreenRefractionTracePass 1", xiiRTTI::FindTypeByName("xiiScreenRefractionTracePass"));
  definition.m_Passes[2].m_Id = xiiUuid(17689637447638201447ULL, 9455274675761552041ULL);
  definition.Connect(0, "Radiance", 1, "B").AssertSuccess();
  definition.Connect(2, "Refraction", 0, "Signal").AssertSuccess();
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(16498187493160506195ULL, 11100420922680318278ULL), "A"});
  definition.m_Interface.PushBack({"BackgroundRadiance", true, xiiUuid(17689637447638201447ULL, 9455274675761552041ULL), "Radiance"});
  definition.m_Interface.PushBack({"RefractionSignal", false, xiiUuid(17689637447638201447ULL, 9455274675761552041ULL), "Refraction"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(17689637447638201447ULL, 9455274675761552041ULL), "Depth"});
  definition.m_Interface.PushBack({"Radiance", false, xiiUuid(16498187493160506195ULL, 11100420922680318278ULL), "Radiance"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(17689637447638201447ULL, 9455274675761552041ULL), "Emissive"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(17689637447638201447ULL, 9455274675761552041ULL), "Camera"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(17689637447638201447ULL, 9455274675761552041ULL), "Normal"});
  definition.m_Interface.PushBack({"TransmittedRadiance", false, xiiUuid(6752151343517858170ULL, 1485645550504798878ULL), "Radiance"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(17689637447638201447ULL, 9455274675761552041ULL), "Material"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateScreenRefractionTraceTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ScreenRefractionTrace";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(2808386831484902341ULL, 17969721319958000229ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("ScreenRefractionTracePass 1", xiiRTTI::FindTypeByName("xiiScreenRefractionTracePass"));
  definition.m_Passes[0].m_Id = xiiUuid(15495635550598458295ULL, 14541305067020335710ULL);
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(15495635550598458295ULL, 14541305067020335710ULL), "Depth"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(15495635550598458295ULL, 14541305067020335710ULL), "Material"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(15495635550598458295ULL, 14541305067020335710ULL), "Camera"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(15495635550598458295ULL, 14541305067020335710ULL), "Normal"});
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(15495635550598458295ULL, 14541305067020335710ULL), "Radiance"});
  definition.m_Interface.PushBack({"Refraction", false, xiiUuid(15495635550598458295ULL, 14541305067020335710ULL), "Refraction"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(15495635550598458295ULL, 14541305067020335710ULL), "Emissive"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateShadowCascadesTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ShadowCascades";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(8346192775779191171ULL, 912969906211384136ULL);
  definition.AddPass("ShadowCascadePass 1", xiiRTTI::FindTypeByName("xiiShadowCascadePass"), Properties({{"Settings", Properties({{"CascadeCount", xiiUInt32(1U)}, {"DepthPadding", 30.0f}, {"MaximumShadowDistance", 120.0f}, {"ShadowMapResolution", xiiUInt32(2048U)}, {"SplitLambda", 0.65f}})}}));
  definition.m_Passes[0].m_Id = xiiUuid(8772185613539285549ULL, 7180326415700839937ULL);
  definition.m_Interface.PushBack({"CascadeConstants", false, xiiUuid(8772185613539285549ULL, 7180326415700839937ULL), "CascadeConstants"});
  definition.m_Interface.PushBack({"CascadeSnapshot", false, xiiUuid(8772185613539285549ULL, 7180326415700839937ULL), "CascadeSnapshot"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateShadowDepthTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ShadowDepth";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(12758566675379179091ULL, 10293399023355432947ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("DepthRasterPass 1", xiiRTTI::FindTypeByName("xiiDepthRasterPass"));
  definition.m_Passes[0].m_Id = xiiUuid(8087599706283587493ULL, 564743142538400482ULL);
  definition.AddPass("DepthRegionClearPass 1", xiiRTTI::FindTypeByName("xiiDepthRegionClearPass"));
  definition.m_Passes[1].m_Id = xiiUuid(1394359072038984754ULL, 5035227907553095771ULL);
  definition.AddPass("MatrixRasterConstantsPass 1", xiiRTTI::FindTypeByName("xiiMatrixRasterConstantsPass"));
  definition.m_Passes[2].m_Id = xiiUuid(12327173657533241552ULL, 15225347229162418934ULL);
  definition.Connect(1, "Depth", 0, "DepthTarget").AssertSuccess();
  definition.Connect(2, "RasterConstants", 0, "RasterConstants").AssertSuccess();
  definition.m_Interface.PushBack({"GeometryMetadata", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"DrawCommands", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "DrawCommands"});
  definition.m_Interface.PushBack({"SkinningMatrices", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"SceneInstances", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"DepthTarget", true, xiiUuid(1394359072038984754ULL, 5035227907553095771ULL), "DepthTarget"});
  definition.m_Interface.PushBack({"DrawCommandCount", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "DrawCommandCount"});
  definition.m_Interface.PushBack({"MeshletMetadata", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"ShadowDepth", false, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"VisibleMeshlets", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"SurfaceMaterials", true, xiiUuid(8087599706283587493ULL, 564743142538400482ULL), "SurfaceMaterials"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateShadowMeshDepthTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ShadowMeshDepth";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(898886094910907332ULL, 11543773326575457922ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::MeshShaders | xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("DepthRasterPass 1", xiiRTTI::FindTypeByName("xiiDepthRasterPass"), Properties({{"UseMeshShaders", true}}));
  definition.m_Passes[0].m_Id = xiiUuid(7135927616787475380ULL, 8459184706934267196ULL);
  definition.AddPass("DepthRegionClearPass 1", xiiRTTI::FindTypeByName("xiiDepthRegionClearPass"));
  definition.m_Passes[1].m_Id = xiiUuid(442333012724228276ULL, 13386837829042985644ULL);
  definition.AddPass("MatrixRasterConstantsPass 1", xiiRTTI::FindTypeByName("xiiMatrixRasterConstantsPass"));
  definition.m_Passes[2].m_Id = xiiUuid(10721935061896042681ULL, 4951923109858173982ULL);
  definition.Connect(1, "Depth", 0, "DepthTarget").AssertSuccess();
  definition.Connect(2, "RasterConstants", 0, "RasterConstants").AssertSuccess();
  definition.m_Interface.PushBack({"GeometryMetadata", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"DrawCommands", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "DrawCommands"});
  definition.m_Interface.PushBack({"SkinningMatrices", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"VisibleMeshlets", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"DrawCommandCount", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "DrawCommandCount"});
  definition.m_Interface.PushBack({"MeshletMetadata", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"DepthTarget", true, xiiUuid(442333012724228276ULL, 13386837829042985644ULL), "DepthTarget"});
  definition.m_Interface.PushBack({"ShadowDepth", false, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"SceneInstances", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"SurfaceMaterials", true, xiiUuid(7135927616787475380ULL, 8459184706934267196ULL), "SurfaceMaterials"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateSparseVoxelEnvironmentTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/SparseVoxelEnvironment";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(13478433150145651245ULL, 16151431799847988004ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"), Properties({{"AsyncCopy", false}}));
  definition.m_Passes[0].m_Id = xiiUuid(16443663611145148037ULL, 1726062294788236571ULL);
  definition.AddPass("SparseVoxelSchedulePass 1", xiiRTTI::FindTypeByName("xiiSparseVoxelSchedulePass"));
  definition.m_Passes[1].m_Id = xiiUuid(18311225480467063427ULL, 2050368264844360488ULL);
  definition.AddPass("SparseVoxelPoolPreparePass 1", xiiRTTI::FindTypeByName("xiiSparseVoxelPoolPreparePass"));
  definition.m_Passes[2].m_Id = xiiUuid(5863483018823202501ULL, 5771720668921387451ULL);
  definition.AddPass("SparseVoxelEnvironmentSeedPass 1", xiiRTTI::FindTypeByName("xiiSparseVoxelEnvironmentSeedPass"));
  definition.m_Passes[3].m_Id = xiiUuid(8228422041330662194ULL, 9160661232478108226ULL);
  definition.AddPass("SparseVoxelUploadPass 1", xiiRTTI::FindTypeByName("xiiSparseVoxelUploadPass"));
  definition.m_Passes[4].m_Id = xiiUuid(11095925058933268874ULL, 13000914763089096466ULL);
  definition.Connect(1, "Schedule", 3, "Schedule").AssertSuccess();
  definition.Connect(1, "Schedule", 4, "Schedule").AssertSuccess();
  definition.Connect(3, "RadiancePool", 0, "Buffer").AssertSuccess();
  definition.Connect(4, "LevelData", 3, "LevelData").AssertSuccess();
  definition.Connect(4, "BrickUpdates", 3, "BrickUpdates").AssertSuccess();
  definition.Connect(1, "Schedule", 2, "Schedule").AssertSuccess();
  definition.Connect(2, "RadiancePool", 3, "InitialPool").AssertSuccess();
  definition.Connect(4, "PageTable", 3, "InitialPageTable").AssertSuccess();
  definition.Connect(0, "PreviousBuffer", 1, "PreviousPool").AssertSuccess();
  definition.Connect(0, "PreviousBuffer", 2, "PreviousPool").AssertSuccess();
  definition.Connect(4, "Constants", 3, "Constants").AssertSuccess();
  definition.m_Interface.PushBack({"LevelData", false, xiiUuid(11095925058933268874ULL, 13000914763089096466ULL), "LevelData"});
  definition.m_Interface.PushBack({"Constants", false, xiiUuid(11095925058933268874ULL, 13000914763089096466ULL), "Constants"});
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(8228422041330662194ULL, 9160661232478108226ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"Schedule", false, xiiUuid(18311225480467063427ULL, 2050368264844360488ULL), "Schedule"});
  definition.m_Interface.PushBack({"RadiancePool", false, xiiUuid(8228422041330662194ULL, 9160661232478108226ULL), "RadiancePool"});
  definition.m_Interface.PushBack({"PageTable", false, xiiUuid(8228422041330662194ULL, 9160661232478108226ULL), "PageTable"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}



xiiRenderPipelineResourceHandle xiiCreateSurfaceIndirectLightingTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/SurfaceIndirectLighting";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(18328257549683386989ULL, 2181397918252551660ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("DiffuseSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDiffuseSurfaceLightingPass"));
  definition.m_Passes[0].m_Id = xiiUuid(15553189407506446903ULL, 8910246306911337264ULL);
  definition.AddPass("IrradianceLayerPass 1", xiiRTTI::FindTypeByName("xiiIrradianceLayerPass"));
  definition.m_Passes[1].m_Id = xiiUuid(1680317133558954808ULL, 18211958737554928610ULL);
  definition.AddPass("RadianceAddPass 1", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[2].m_Id = xiiUuid(15721232926978022293ULL, 3866737091163516374ULL);
  definition.AddPass("EnvironmentSpecularPass 1", xiiRTTI::FindTypeByName("xiiEnvironmentSpecularPass"));
  definition.m_Passes[3].m_Id = xiiUuid(221609753716089224ULL, 3286556801357769157ULL);
  definition.Connect(1, "Irradiance", 0, "Irradiance").AssertSuccess();
  definition.Connect(0, "Lighting", 2, "A").AssertSuccess();
  definition.Connect(3, "Lighting", 2, "B").AssertSuccess();
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "Albedo"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(15553189407506446903ULL, 8910246306911337264ULL), "Albedo"});
  definition.m_Interface.PushBack({"DiffuseLighting", false, xiiUuid(15553189407506446903ULL, 8910246306911337264ULL), "Lighting"});
  definition.m_Interface.PushBack({"CoarseIrradiance", true, xiiUuid(1680317133558954808ULL, 18211958737554928610ULL), "CoarseIrradiance"});
  definition.m_Interface.PushBack({"Occlusion", true, xiiUuid(15553189407506446903ULL, 8910246306911337264ULL), "Occlusion"});
  definition.m_Interface.PushBack({"Occlusion", true, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "Occlusion"});
  definition.m_Interface.PushBack({"IndirectLighting", false, xiiUuid(15721232926978022293ULL, 3866737091163516374ULL), "Radiance"});
  definition.m_Interface.PushBack({"DetailIrradiance", true, xiiUuid(1680317133558954808ULL, 18211958737554928610ULL), "DetailIrradiance"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(15553189407506446903ULL, 8910246306911337264ULL), "Depth"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "Depth"});
  definition.m_Interface.PushBack({"EnvironmentRadiance", true, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "EnvironmentRadiance"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(15553189407506446903ULL, 8910246306911337264ULL), "Emissive"});
  definition.m_Interface.PushBack({"EnvironmentSpecular", false, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "Lighting"});
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(15553189407506446903ULL, 8910246306911337264ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "Material"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(15553189407506446903ULL, 8910246306911337264ULL), "Material"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "Camera"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "Normal"});
  definition.m_Interface.PushBack({"BRDFLut", true, xiiUuid(221609753716089224ULL, 3286556801357769157ULL), "BRDFLut"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateSurfaceQueryTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/SurfaceQuery";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(5522121605785416492ULL, 5819896883166302448ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("MaterialUploadPass 1", xiiRTTI::FindTypeByName("xiiMaterialUploadPass"));
  definition.m_Passes[0].m_Id = xiiUuid(13159913979427388027ULL, 6203092451594754910ULL);
  definition.AddPass("SurfaceRasterConstantsPass 1", xiiRTTI::FindTypeByName("xiiSurfaceRasterConstantsPass"));
  definition.m_Passes[1].m_Id = xiiUuid(4665516052437595103ULL, 9384123229878499927ULL);
  definition.AddPass("VertexPullDrawCommandsPass 1", xiiRTTI::FindTypeByName("xiiVertexPullDrawCommandsPass"));
  definition.m_Passes[2].m_Id = xiiUuid(14234033585344164455ULL, 15831913377647909108ULL);
  definition.AddPass("MeshletDispatchBoundsPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchBoundsPass"));
  definition.m_Passes[3].m_Id = xiiUuid(9390150324821450698ULL, 6326127761306537877ULL);
  definition.AddPass("MeshletCullPass 1", xiiRTTI::FindTypeByName("xiiMeshletCullPass"));
  definition.m_Passes[4].m_Id = xiiUuid(447244371370987885ULL, 2218271992469067425ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[5].m_Id = xiiUuid(15716503801982877330ULL, 10564283837447394634ULL);
  definition.AddPass("CounterResetPass 2", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[6].m_Id = xiiUuid(11509739097762478085ULL, 3437200844590326423ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[7].m_Id = xiiUuid(6384832625874666310ULL, 8490914447879996976ULL);
  definition.AddPass("VisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiVisibilityViewPass"));
  definition.m_Passes[8].m_Id = xiiUuid(13658267384422468459ULL, 766539519500845559ULL);
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[9].m_Id = xiiUuid(17769000208239220942ULL, 1487766629593459564ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[10].m_Id = xiiUuid(13303369425498969123ULL, 2436230639067847094ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[11].m_Id = xiiUuid(17403563653345625494ULL, 17001979974840264849ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[12].m_Id = xiiUuid(9907796431123631610ULL, 9110354856760991925ULL);
  definition.AddPass("CounterResetPass 3", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[13].m_Id = xiiUuid(1251416029658928725ULL, 4600690344129171862ULL);
  definition.AddPass("CounterResetPass 4", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[14].m_Id = xiiUuid(1490879722725036542ULL, 4089041455308399374ULL);
  definition.AddPass("MeshletDispatchPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchPass"));
  definition.m_Passes[15].m_Id = xiiUuid(17186733254381955230ULL, 93278822389636380ULL);
  definition.AddPass("SurfaceQueryPass 1", xiiRTTI::FindTypeByName("xiiSurfaceQueryPass"));
  definition.m_Passes[16].m_Id = xiiUuid(13054756792291900119ULL, 13631396051060494278ULL);
  definition.Connect(7, "GeometryMetadata", 12, "GeometryMetadata").AssertSuccess();
  definition.Connect(10, "LodHistory", 12, "InitializedLodHistory").AssertSuccess();
  definition.Connect(11, "SceneInstances", 12, "SceneInstances").AssertSuccess();
  definition.Connect(11, "SceneInstances", 10, "SceneInstances").AssertSuccess();
  definition.Connect(13, "Counter", 12, "Counter").AssertSuccess();
  definition.Connect(8, "Candidates", 12, "Candidates").AssertSuccess();
  definition.Connect(9, "PreviousBuffer", 12, "PreviousLodHistory").AssertSuccess();
  definition.Connect(12, "LodHistory", 9, "Buffer").AssertSuccess();
  definition.Connect(8, "VisibilityView", 12, "VisibilityView").AssertSuccess();
  definition.Connect(7, "GeometryMetadata", 4, "GeometryMetadata").AssertSuccess();
  definition.Connect(12, "LodHistory", 3, "LodHistory").AssertSuccess();
  definition.Connect(7, "MeshletMetadata", 15, "MeshletMetadata").AssertSuccess();
  definition.Connect(11, "SceneInstances", 3, "SceneInstances").AssertSuccess();
  definition.Connect(12, "VisibleInstanceCount", 4, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(5, "Counter", 4, "InitialCount").AssertSuccess();
  definition.Connect(8, "VisibilityView", 3, "VisibilityView").AssertSuccess();
  definition.Connect(12, "VisibleInstanceCount", 15, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(7, "GeometryMetadata", 3, "GeometryMetadata").AssertSuccess();
  definition.Connect(6, "Counter", 3, "InitializedMaximum").AssertSuccess();
  definition.Connect(11, "SceneInstances", 4, "SceneInstances").AssertSuccess();
  definition.Connect(12, "VisibleInstances", 4, "VisibleInstances").AssertSuccess();
  definition.Connect(3, "MaximumMeshletGroups", 15, "MaximumMeshletGroups").AssertSuccess();
  definition.Connect(12, "VisibleInstances", 15, "VisibleInstances").AssertSuccess();
  definition.Connect(12, "VisibleInstances", 3, "VisibleInstances").AssertSuccess();
  definition.Connect(14, "Counter", 4, "InitialOverflow").AssertSuccess();
  definition.Connect(15, "DispatchArguments", 4, "DispatchArguments").AssertSuccess();
  definition.Connect(7, "MeshletMetadata", 4, "MeshletMetadata").AssertSuccess();
  definition.Connect(12, "LodHistory", 4, "LodHistory").AssertSuccess();
  definition.Connect(12, "VisibleInstanceCount", 3, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(7, "MeshletMetadata", 3, "MeshletMetadata").AssertSuccess();
  definition.Connect(8, "VisibilityView", 4, "VisibilityView").AssertSuccess();
  definition.Connect(7, "GeometryMetadata", 2, "GeometryMetadata").AssertSuccess();
  definition.Connect(4, "VisibleMeshlets", 2, "VisibleMeshlets").AssertSuccess();
  definition.Connect(4, "VisibleMeshletCount", 2, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(8, "VisibilityView", 2, "VisibilityView").AssertSuccess();
  definition.Connect(7, "MeshletMetadata", 2, "MeshletMetadata").AssertSuccess();
  definition.Connect(11, "SceneInstances", 2, "SceneInstances").AssertSuccess();
  definition.Connect(4, "VisibleMeshletCount", 16, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(7, "GeometryMetadata", 16, "GeometryMetadata").AssertSuccess();
  definition.Connect(0, "SurfaceMaterials", 16, "SurfaceMaterials").AssertSuccess();
  definition.Connect(4, "VisibleMeshlets", 16, "VisibleMeshlets").AssertSuccess();
  definition.Connect(2, "DrawCommandCount", 16, "DrawCommandCount").AssertSuccess();
  definition.Connect(7, "MeshletMetadata", 16, "MeshletMetadata").AssertSuccess();
  definition.Connect(1, "RasterConstants", 16, "RasterConstants").AssertSuccess();
  definition.Connect(11, "SceneInstances", 16, "SceneInstances").AssertSuccess();
  definition.Connect(2, "DrawCommands", 16, "DrawCommands").AssertSuccess();
  definition.Connect(11, "SkinningMatrices", 16, "SkinningMatrices").AssertSuccess();
  definition.m_Interface.PushBack({"LodHistory", false, xiiUuid(9907796431123631610ULL, 9110354856760991925ULL), "LodHistory"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(9907796431123631610ULL, 9110354856760991925ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", false, xiiUuid(447244371370987885ULL, 2218271992469067425ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"GeometryMetadata", false, xiiUuid(6384832625874666310ULL, 8490914447879996976ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"DispatchArguments", false, xiiUuid(17186733254381955230ULL, 93278822389636380ULL), "DispatchArguments"});
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(13658267384422468459ULL, 766539519500845559ULL), "InputCandidates"});
  definition.m_Interface.PushBack({"SkinningMatrices", false, xiiUuid(17403563653345625494ULL, 17001979974840264849ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"VisibleMeshletOverflow", false, xiiUuid(447244371370987885ULL, 2218271992469067425ULL), "VisibleMeshletOverflow"});
  definition.m_Interface.PushBack({"SceneInstances", false, xiiUuid(17403563653345625494ULL, 17001979974840264849ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"MeshletMetadata", false, xiiUuid(6384832625874666310ULL, 8490914447879996976ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"DrawCommands", false, xiiUuid(14234033585344164455ULL, 15831913377647909108ULL), "DrawCommands"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(9907796431123631610ULL, 9110354856760991925ULL), "VisibleInstances"});
  definition.m_Interface.PushBack({"DrawCommandCount", false, xiiUuid(14234033585344164455ULL, 15831913377647909108ULL), "DrawCommandCount"});
  definition.m_Interface.PushBack({"SceneDepth", false, xiiUuid(13054756792291900119ULL, 13631396051060494278ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"VisibleMeshlets", false, xiiUuid(447244371370987885ULL, 2218271992469067425ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"Surface", false, xiiUuid(13054756792291900119ULL, 13631396051060494278ULL), "Surface"});
  definition.m_Interface.PushBack({"VisibilityView", false, xiiUuid(13658267384422468459ULL, 766539519500845559ULL), "VisibilityView"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateSurfaceRadianceCompositionTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/SurfaceRadianceComposition";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(14962297587497638470ULL, 14004134725023127058ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("RadianceAddPass 1", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[0].m_Id = xiiUuid(3344487892727338868ULL, 82751780182916904ULL);
  definition.AddPass("RadianceAddPass 2", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[1].m_Id = xiiUuid(9512673872611069333ULL, 2608263885320456148ULL);
  definition.AddPass("ConfidenceRadiancePass 1", xiiRTTI::FindTypeByName("xiiConfidenceRadiancePass"));
  definition.m_Passes[2].m_Id = xiiUuid(945329417232521185ULL, 8054977367612668687ULL);
  definition.AddPass("RadianceAddPass 3", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[3].m_Id = xiiUuid(11398539004707477827ULL, 17087643005466540303ULL);
  definition.AddPass("VolumetricCompositePass 1", xiiRTTI::FindTypeByName("xiiVolumetricCompositePass"));
  definition.m_Passes[4].m_Id = xiiUuid(3348026251763784655ULL, 17123352771865612237ULL);
  definition.Connect(0, "Radiance", 3, "A").AssertSuccess();
  definition.Connect(3, "Radiance", 4, "SceneRadiance").AssertSuccess();
  definition.Connect(1, "Radiance", 0, "A").AssertSuccess();
  definition.Connect(2, "Radiance", 3, "B").AssertSuccess();
  definition.m_Interface.PushBack({"RTFinalGI", true, xiiUuid(945329417232521185ULL, 8054977367612668687ULL), "Signal"});
  definition.m_Interface.PushBack({"VolumetricScattering", true, xiiUuid(3348026251763784655ULL, 17123352771865612237ULL), "ScatteringOpacity"});
  definition.m_Interface.PushBack({"DirectLighting", true, xiiUuid(9512673872611069333ULL, 2608263885320456148ULL), "A"});
  definition.m_Interface.PushBack({"IndirectLighting", true, xiiUuid(9512673872611069333ULL, 2608263885320456148ULL), "B"});
  definition.m_Interface.PushBack({"HDRSceneColor", false, xiiUuid(3348026251763784655ULL, 17123352771865612237ULL), "Radiance"});
  definition.m_Interface.PushBack({"GBufferEmissive", true, xiiUuid(3344487892727338868ULL, 82751780182916904ULL), "B"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateTransparencyResolveTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/TransparencyResolve";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(9140756755650582779ULL, 7293809961247343899ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("TextureSelectionPass 1", xiiRTTI::FindTypeByName("xiiTextureSelectionPass"));
  definition.m_Passes[0].m_Id = xiiUuid(205952724351118527ULL, 5029053626508819207ULL);
  definition.AddPass("ParticleIntegrationConstantsPass 1", xiiRTTI::FindTypeByName("xiiParticleIntegrationConstantsPass"), Properties({{"UseWorldTime", true}}));
  definition.m_Passes[1].m_Id = xiiUuid(100429923195266464ULL, 5840082953450722579ULL);
  definition.AddPass("TextureSelectionPass 2", xiiRTTI::FindTypeByName("xiiTextureSelectionPass"));
  definition.m_Passes[2].m_Id = xiiUuid(9382240603925904584ULL, 6294007472402013166ULL);
  definition.AddPass("TextureForwardPass 1", xiiRTTI::FindTypeByName("xiiTextureForwardPass"));
  definition.m_Passes[3].m_Id = xiiUuid(10912220079710318220ULL, 7996978082116805660ULL);
  definition.AddPass("ClearTexturePass 1", xiiRTTI::FindTypeByName("xiiClearTexturePass"), Properties({{"Color", xiiColor(1.0f, 1.0f, 1.0f, 1.0f)}, {"Texture", Properties({{"Format", xiiInt64(61LL)}, {"Requirements", Properties({{"Dimension", xiiInt64(0LL)}, {"Height", xiiUInt32(0U)}, {"RequiredBindFlags", xiiInt64(0LL)}, {"Samples", xiiUInt32(0U)}, {"Width", xiiUInt32(0U)}})}, {"Resolution", Properties({{"Height", xiiUInt32(0U)}, {"Input", xiiString("Reference")}, {"Mode", xiiInt64(2LL)}, {"Scale", 1.0f}, {"Width", xiiUInt32(0U)}})}})}}));
  definition.m_Passes[4].m_Id = xiiUuid(17319885497844757021ULL, 8085170117999541262ULL);
  definition.AddPass("ClearTexturePass 2", xiiRTTI::FindTypeByName("xiiClearTexturePass"), Properties({{"Texture", Properties({{"Format", xiiInt64(10LL)}, {"Requirements", Properties({{"Dimension", xiiInt64(0LL)}, {"Height", xiiUInt32(0U)}, {"RequiredBindFlags", xiiInt64(0LL)}, {"Samples", xiiUInt32(0U)}, {"Width", xiiUInt32(0U)}})}, {"Resolution", Properties({{"Height", xiiUInt32(0U)}, {"Input", xiiString("Reference")}, {"Mode", xiiInt64(2LL)}, {"Scale", 1.0f}, {"Width", xiiUInt32(0U)}})}})}}));
  definition.m_Passes[5].m_Id = xiiUuid(12787364006276062479ULL, 9318975939464809592ULL);
  definition.AddPass("WeightedBlendedOITResolvePass 1", xiiRTTI::FindTypeByName("xiiWeightedBlendedOITResolvePass"));
  definition.m_Passes[6].m_Id = xiiUuid(17179449470672113368ULL, 9366517099025615460ULL);
  definition.AddPass("BufferForwardPass 1", xiiRTTI::FindTypeByName("xiiBufferForwardPass"));
  definition.m_Passes[7].m_Id = xiiUuid(8931986233501286658ULL, 11858121379403392365ULL);
  definition.Connect(0, "Texture", 6, "OITReveal").AssertSuccess();
  definition.Connect(2, "Texture", 6, "OITAccumulate").AssertSuccess();
  definition.Connect(4, "Texture", 0, "Fallback").AssertSuccess();
  definition.Connect(5, "Texture", 2, "Fallback").AssertSuccess();
  definition.m_Interface.PushBack({"DrawIndirectCommands", false, xiiUuid(8931986233501286658ULL, 11858121379403392365ULL), "Output"});
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(10912220079710318220ULL, 7996978082116805660ULL), "Input"});
  definition.m_Interface.PushBack({"HDRSceneColor", true, xiiUuid(12787364006276062479ULL, 9318975939464809592ULL), "Reference"});
  definition.m_Interface.PushBack({"HDRSceneColor", true, xiiUuid(17179449470672113368ULL, 9366517099025615460ULL), "HDRSceneColor"});
  definition.m_Interface.PushBack({"HDRSceneColor", true, xiiUuid(17319885497844757021ULL, 8085170117999541262ULL), "Reference"});
  definition.m_Interface.PushBack({"HDRSceneColor", false, xiiUuid(17179449470672113368ULL, 9366517099025615460ULL), "HDRSceneColor"});
  definition.m_Interface.PushBack({"SceneDepth", false, xiiUuid(10912220079710318220ULL, 7996978082116805660ULL), "Output"});
  definition.m_Interface.PushBack({"DrawIndirectCommands", true, xiiUuid(8931986233501286658ULL, 11858121379403392365ULL), "Input"});
  definition.m_Interface.PushBack({"OITAccumulate", true, xiiUuid(9382240603925904584ULL, 6294007472402013166ULL), "Primary"});
  definition.m_Interface.PushBack({"OITAccumulate", false, xiiUuid(9382240603925904584ULL, 6294007472402013166ULL), "Texture"});
  definition.m_Interface.PushBack({"OITReveal", false, xiiUuid(205952724351118527ULL, 5029053626508819207ULL), "Texture"});
  definition.m_Interface.PushBack({"OITReveal", true, xiiUuid(205952724351118527ULL, 5029053626508819207ULL), "Primary"});
  definition.m_Interface.PushBack({"ParticleSimConstants", false, xiiUuid(100429923195266464ULL, 5840082953450722579ULL), "ParticleSimConstants"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateVertexPullDrawsTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/VertexPullDraws";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(10495193538914833609ULL, 8081627444317147616ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("VertexPullDrawCommandsPass 1", xiiRTTI::FindTypeByName("xiiVertexPullDrawCommandsPass"));
  definition.m_Passes[0].m_Id = xiiUuid(3998350846093271118ULL, 6430642709239132843ULL);
  definition.AddPass("MeshletDispatchBoundsPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchBoundsPass"));
  definition.m_Passes[1].m_Id = xiiUuid(3812277770322118039ULL, 3558784225102125228ULL);
  definition.AddPass("MeshletCullPass 1", xiiRTTI::FindTypeByName("xiiMeshletCullPass"));
  definition.m_Passes[2].m_Id = xiiUuid(17873406116144979268ULL, 697161022033429140ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[3].m_Id = xiiUuid(8629650771364907620ULL, 12332564964426701209ULL);
  definition.AddPass("CounterResetPass 2", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[4].m_Id = xiiUuid(11385722726912795003ULL, 15600600853983187984ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[5].m_Id = xiiUuid(9377212253660506865ULL, 10395870279428089353ULL);
  definition.AddPass("VisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiVisibilityViewPass"));
  definition.m_Passes[6].m_Id = xiiUuid(2413363169345151480ULL, 2389237115156446162ULL);
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[7].m_Id = xiiUuid(17394493049661256259ULL, 4259046681903504103ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[8].m_Id = xiiUuid(1820737342838217218ULL, 11072104466016388336ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[9].m_Id = xiiUuid(16603445847795554451ULL, 7233746400167487575ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[10].m_Id = xiiUuid(1189863757643074559ULL, 3391912767021457355ULL);
  definition.AddPass("CounterResetPass 3", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[11].m_Id = xiiUuid(8787859718949211279ULL, 5012324838327698973ULL);
  definition.AddPass("CounterResetPass 4", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[12].m_Id = xiiUuid(13382509603083973078ULL, 11958279060954420251ULL);
  definition.AddPass("MeshletDispatchPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchPass"));
  definition.m_Passes[13].m_Id = xiiUuid(10280723777075182635ULL, 888325081946379559ULL);
  definition.Connect(5, "GeometryMetadata", 10, "GeometryMetadata").AssertSuccess();
  definition.Connect(8, "LodHistory", 10, "InitializedLodHistory").AssertSuccess();
  definition.Connect(9, "SceneInstances", 10, "SceneInstances").AssertSuccess();
  definition.Connect(9, "SceneInstances", 8, "SceneInstances").AssertSuccess();
  definition.Connect(11, "Counter", 10, "Counter").AssertSuccess();
  definition.Connect(6, "Candidates", 10, "Candidates").AssertSuccess();
  definition.Connect(7, "PreviousBuffer", 10, "PreviousLodHistory").AssertSuccess();
  definition.Connect(10, "LodHistory", 7, "Buffer").AssertSuccess();
  definition.Connect(6, "VisibilityView", 10, "VisibilityView").AssertSuccess();
  definition.Connect(5, "GeometryMetadata", 2, "GeometryMetadata").AssertSuccess();
  definition.Connect(10, "LodHistory", 1, "LodHistory").AssertSuccess();
  definition.Connect(5, "MeshletMetadata", 13, "MeshletMetadata").AssertSuccess();
  definition.Connect(9, "SceneInstances", 1, "SceneInstances").AssertSuccess();
  definition.Connect(10, "VisibleInstanceCount", 2, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(3, "Counter", 2, "InitialCount").AssertSuccess();
  definition.Connect(6, "VisibilityView", 1, "VisibilityView").AssertSuccess();
  definition.Connect(10, "VisibleInstanceCount", 13, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(5, "GeometryMetadata", 1, "GeometryMetadata").AssertSuccess();
  definition.Connect(4, "Counter", 1, "InitializedMaximum").AssertSuccess();
  definition.Connect(9, "SceneInstances", 2, "SceneInstances").AssertSuccess();
  definition.Connect(10, "VisibleInstances", 2, "VisibleInstances").AssertSuccess();
  definition.Connect(1, "MaximumMeshletGroups", 13, "MaximumMeshletGroups").AssertSuccess();
  definition.Connect(10, "VisibleInstances", 13, "VisibleInstances").AssertSuccess();
  definition.Connect(10, "VisibleInstances", 1, "VisibleInstances").AssertSuccess();
  definition.Connect(12, "Counter", 2, "InitialOverflow").AssertSuccess();
  definition.Connect(13, "DispatchArguments", 2, "DispatchArguments").AssertSuccess();
  definition.Connect(5, "MeshletMetadata", 2, "MeshletMetadata").AssertSuccess();
  definition.Connect(10, "LodHistory", 2, "LodHistory").AssertSuccess();
  definition.Connect(10, "VisibleInstanceCount", 1, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(5, "MeshletMetadata", 1, "MeshletMetadata").AssertSuccess();
  definition.Connect(6, "VisibilityView", 2, "VisibilityView").AssertSuccess();
  definition.Connect(5, "GeometryMetadata", 0, "GeometryMetadata").AssertSuccess();
  definition.Connect(2, "VisibleMeshlets", 0, "VisibleMeshlets").AssertSuccess();
  definition.Connect(2, "VisibleMeshletCount", 0, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(6, "VisibilityView", 0, "VisibilityView").AssertSuccess();
  definition.Connect(5, "MeshletMetadata", 0, "MeshletMetadata").AssertSuccess();
  definition.Connect(9, "SceneInstances", 0, "SceneInstances").AssertSuccess();
  definition.m_Interface.PushBack({"VisibleMeshlets", false, xiiUuid(17873406116144979268ULL, 697161022033429140ULL), "VisibleMeshlets"});
  definition.m_Interface.PushBack({"LodHistory", false, xiiUuid(1189863757643074559ULL, 3391912767021457355ULL), "LodHistory"});
  definition.m_Interface.PushBack({"SkinningMatrices", false, xiiUuid(16603445847795554451ULL, 7233746400167487575ULL), "SkinningMatrices"});
  definition.m_Interface.PushBack({"GeometryMetadata", false, xiiUuid(9377212253660506865ULL, 10395870279428089353ULL), "GeometryMetadata"});
  definition.m_Interface.PushBack({"VisibleInstances", false, xiiUuid(1189863757643074559ULL, 3391912767021457355ULL), "VisibleInstances"});
  definition.m_Interface.PushBack({"DrawCommandCount", false, xiiUuid(3998350846093271118ULL, 6430642709239132843ULL), "DrawCommandCount"});
  definition.m_Interface.PushBack({"VisibleMeshletOverflow", false, xiiUuid(17873406116144979268ULL, 697161022033429140ULL), "VisibleMeshletOverflow"});
  definition.m_Interface.PushBack({"DrawCommands", false, xiiUuid(3998350846093271118ULL, 6430642709239132843ULL), "DrawCommands"});
  definition.m_Interface.PushBack({"VisibleInstanceCount", false, xiiUuid(1189863757643074559ULL, 3391912767021457355ULL), "VisibleInstanceCount"});
  definition.m_Interface.PushBack({"VisibleMeshletCount", false, xiiUuid(17873406116144979268ULL, 697161022033429140ULL), "VisibleMeshletCount"});
  definition.m_Interface.PushBack({"VisibilityView", false, xiiUuid(2413363169345151480ULL, 2389237115156446162ULL), "VisibilityView"});
  definition.m_Interface.PushBack({"SceneInstances", false, xiiUuid(16603445847795554451ULL, 7233746400167487575ULL), "SceneInstances"});
  definition.m_Interface.PushBack({"MeshletMetadata", false, xiiUuid(9377212253660506865ULL, 10395870279428089353ULL), "MeshletMetadata"});
  definition.m_Interface.PushBack({"DispatchArguments", false, xiiUuid(10280723777075182635ULL, 888325081946379559ULL), "DispatchArguments"});
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(2413363169345151480ULL, 2389237115156446162ULL), "InputCandidates"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateViewPresentationTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/ViewPresentation";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(702527708468444643ULL, 12014198249725170238ULL);
  definition.AddPass("PresentPass 1", xiiRTTI::FindTypeByName("xiiPresentPass"));
  definition.m_Passes[0].m_Id = xiiUuid(1362939878103602832ULL, 11210879711243855241ULL);
  definition.m_Interface.PushBack({"Color", true, xiiUuid(1362939878103602832ULL, 11210879711243855241ULL), "GradedColor"});
  definition.m_Interface.PushBack({"Constants", false, xiiUuid(1362939878103602832ULL, 11210879711243855241ULL), "Final Blit Constants"});
  definition.m_Interface.PushBack({"ViewOutput", false, xiiUuid(1362939878103602832ULL, 11210879711243855241ULL), "ViewOutput"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateVirtualShadowFeedbackTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/VirtualShadowFeedback";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(9761984823693813761ULL, 13871228405223312139ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("VirtualShadowFeedbackReadbackPass 1", xiiRTTI::FindTypeByName("xiiVirtualShadowFeedbackReadbackPass"));
  definition.m_Passes[0].m_Id = xiiUuid(2111981296291797461ULL, 8061551403931206249ULL);
  definition.AddPass("VirtualShadowFeedbackPass 1", xiiRTTI::FindTypeByName("xiiVirtualShadowFeedbackPass"));
  definition.m_Passes[1].m_Id = xiiUuid(9903077955267097982ULL, 15662885014069666512ULL);
  definition.AddPass("VirtualShadowFeedbackResetPass 1", xiiRTTI::FindTypeByName("xiiVirtualShadowFeedbackResetPass"));
  definition.m_Passes[2].m_Id = xiiUuid(8420112275830098311ULL, 16520920777319685130ULL);
  definition.Connect(2, "Requests", 1, "InitializedRequests").AssertSuccess();
  definition.Connect(1, "Requests", 0, "Requests").AssertSuccess();
  definition.m_Interface.PushBack({"SceneDepth", true, xiiUuid(9903077955267097982ULL, 15662885014069666512ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"Requests", false, xiiUuid(9903077955267097982ULL, 15662885014069666512ULL), "Requests"});
  definition.m_Interface.PushBack({"CascadeConstants", true, xiiUuid(9903077955267097982ULL, 15662885014069666512ULL), "CascadeConstants"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateVirtualShadowPagePublishTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/VirtualShadowPagePublish";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(422676072324900442ULL, 12833371818747944887ULL);
  definition.AddPass("VirtualShadowPagePublishPass 1", xiiRTTI::FindTypeByName("xiiVirtualShadowPagePublishPass"));
  definition.m_Passes[0].m_Id = xiiUuid(11776095908427198256ULL, 14256066996967535457ULL);
  definition.m_Interface.PushBack({"VirtualPageTable", true, xiiUuid(11776095908427198256ULL, 14256066996967535457ULL), "VirtualPageTable"});
  definition.m_Interface.PushBack({"RasterPages", true, xiiUuid(11776095908427198256ULL, 14256066996967535457ULL), "RasterPages"});
  definition.m_Interface.PushBack({"ReadyAtlas", false, xiiUuid(11776095908427198256ULL, 14256066996967535457ULL), "ReadyAtlas"});
  definition.m_Interface.PushBack({"ReadyVirtualPageTable", false, xiiUuid(11776095908427198256ULL, 14256066996967535457ULL), "ReadyVirtualPageTable"});
  definition.m_Interface.PushBack({"PhysicalPageTable", true, xiiUuid(11776095908427198256ULL, 14256066996967535457ULL), "PhysicalPageTable"});
  definition.m_Interface.PushBack({"RasterAtlas", true, xiiUuid(11776095908427198256ULL, 14256066996967535457ULL), "RasterAtlas"});
  definition.m_Interface.PushBack({"ReadyPhysicalPageTable", false, xiiUuid(11776095908427198256ULL, 14256066996967535457ULL), "ReadyPhysicalPageTable"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateVirtualShadowRasterWorkTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/VirtualShadowRasterWork";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(9775465998978115672ULL, 16013991888604243040ULL);
  definition.AddPass("VirtualShadowRasterWorkPass 1", xiiRTTI::FindTypeByName("xiiVirtualShadowRasterWorkPass"));
  definition.m_Passes[0].m_Id = xiiUuid(3976963678630744476ULL, 3249767873243787674ULL);
  definition.m_Interface.PushBack({"Residency", true, xiiUuid(3976963678630744476ULL, 3249767873243787674ULL), "Residency"});
  definition.m_Interface.PushBack({"RasterPages", false, xiiUuid(3976963678630744476ULL, 3249767873243787674ULL), "RasterPages"});
  definition.m_Interface.PushBack({"Cascades", true, xiiUuid(3976963678630744476ULL, 3249767873243787674ULL), "Cascades"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateVirtualShadowTablesTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/VirtualShadowTables";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(17991256421285488753ULL, 14175841292953872087ULL);
  definition.AddPass("VirtualShadowTablesPass 1", xiiRTTI::FindTypeByName("xiiVirtualShadowTablesPass"));
  definition.m_Passes[0].m_Id = xiiUuid(15404431372640444430ULL, 5451618701604004685ULL);
  definition.m_Interface.PushBack({"VirtualPageTable", false, xiiUuid(15404431372640444430ULL, 5451618701604004685ULL), "VirtualPageTable"});
  definition.m_Interface.PushBack({"PhysicalAtlas", false, xiiUuid(15404431372640444430ULL, 5451618701604004685ULL), "PhysicalAtlas"});
  definition.m_Interface.PushBack({"PhysicalPageTable", false, xiiUuid(15404431372640444430ULL, 5451618701604004685ULL), "PhysicalPageTable"});
  definition.m_Interface.PushBack({"Residency", false, xiiUuid(15404431372640444430ULL, 5451618701604004685ULL), "Residency"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateWorldAtmosphereViewTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/WorldAtmosphereView";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(13103297635133322595ULL, 5162776158125067692ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("AtmosphereMultiScatterPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereMultiScatterPass"));
  definition.m_Passes[0].m_Id = xiiUuid(15914205062591417258ULL, 3738127387500946377ULL);
  definition.AddPass("AtmosphereTransmittancePass 1", xiiRTTI::FindTypeByName("xiiAtmosphereTransmittancePass"));
  definition.m_Passes[1].m_Id = xiiUuid(4286394839289817351ULL, 4086310046199052071ULL);
  definition.AddPass("AtmosphereConstantsPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereConstantsPass"), Properties({{"UseWorldAtmosphere", true}}));
  definition.m_Passes[2].m_Id = xiiUuid(9075040090643084954ULL, 10435129865958525888ULL);
  definition.AddPass("AtmosphereApplyPass 1", xiiRTTI::FindTypeByName("xiiAtmosphereApplyPass"));
  definition.m_Passes[3].m_Id = xiiUuid(8226243601622878002ULL, 15078970099146345410ULL);
  definition.Connect(1, "Transmittance", 0, "Transmittance").AssertSuccess();
  definition.Connect(2, "AtmosphereConstants", 1, "Atmosphere").AssertSuccess();
  definition.Connect(2, "AtmosphereConstants", 0, "Atmosphere").AssertSuccess();
  definition.Connect(1, "Transmittance", 3, "Transmittance").AssertSuccess();
  definition.Connect(2, "AtmosphereConstants", 3, "Atmosphere").AssertSuccess();
  definition.Connect(0, "MultiScatter", 3, "MultiScatter").AssertSuccess();
  definition.m_Interface.PushBack({"Transmittance", false, xiiUuid(4286394839289817351ULL, 4086310046199052071ULL), "Transmittance"});
  definition.m_Interface.PushBack({"EnvironmentLight", true, xiiUuid(8226243601622878002ULL, 15078970099146345410ULL), "EnvironmentLight"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(8226243601622878002ULL, 15078970099146345410ULL), "Camera"});
  definition.m_Interface.PushBack({"Radiance", false, xiiUuid(8226243601622878002ULL, 15078970099146345410ULL), "AtmosphereRadiance"});
  definition.m_Interface.PushBack({"MultiScatter", false, xiiUuid(15914205062591417258ULL, 3738127387500946377ULL), "MultiScatter"});
  definition.m_Interface.PushBack({"Atmosphere", false, xiiUuid(9075040090643084954ULL, 10435129865958525888ULL), "AtmosphereConstants"});
  definition.m_Interface.PushBack({"Radiance", true, xiiUuid(8226243601622878002ULL, 15078970099146345410ULL), "Radiance"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(8226243601622878002ULL, 15078970099146345410ULL), "Depth"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateWorldDeferredLightingTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/WorldDeferredLighting";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(6538553556571000757ULL, 3224509834575800349ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("RadianceAddPass 1", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[0].m_Id = xiiUuid(5477411076574998343ULL, 4289490620101449408ULL);
  definition.AddPass("ViewConstantsPass 1", xiiRTTI::FindTypeByName("xiiViewConstantsPass"));
  definition.m_Passes[1].m_Id = xiiUuid(5185066245865150186ULL, 8706648560490958339ULL);
  definition.AddPass("WorldDirectionalLightPass 1", xiiRTTI::FindTypeByName("xiiWorldDirectionalLightPass"));
  definition.m_Passes[2].m_Id = xiiUuid(15940683418819554144ULL, 12138266194960652399ULL);
  definition.AddPass("EnvironmentLightConstantsPass 1", xiiRTTI::FindTypeByName("xiiEnvironmentLightConstantsPass"));
  definition.m_Passes[3].m_Id = xiiUuid(10363511905609479855ULL, 14002394366992002504ULL);
  definition.AddPass("WorldLightsUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldLightsUploadPass"));
  definition.m_Passes[4].m_Id = xiiUuid(3171634100679029670ULL, 14439686896884281772ULL);
  definition.AddPass("ClusteredLocalSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiClusteredLocalSurfaceLightingPass"));
  definition.m_Passes[5].m_Id = xiiUuid(7719474710416432033ULL, 5211185321607326265ULL);
  definition.AddPass("LightClusterViewPass 1", xiiRTTI::FindTypeByName("xiiLightClusterViewPass"));
  definition.m_Passes[6].m_Id = xiiUuid(17848176564460824250ULL, 18177203475927815264ULL);
  definition.AddPass("ClusterListClearPass 1", xiiRTTI::FindTypeByName("xiiClusterListClearPass"));
  definition.m_Passes[7].m_Id = xiiUuid(6142836303037878887ULL, 11979892810603512174ULL);
  definition.AddPass("ClusterBoundsPass 1", xiiRTTI::FindTypeByName("xiiClusterBoundsPass"));
  definition.m_Passes[8].m_Id = xiiUuid(7578420234482186574ULL, 12373888981850975937ULL);
  definition.AddPass("ClusterLightSamplingPass 1", xiiRTTI::FindTypeByName("xiiClusterLightSamplingPass"));
  definition.m_Passes[9].m_Id = xiiUuid(7211545140057419760ULL, 2898284240708576885ULL);
  definition.AddPass("ClusterLightAssignmentPass 1", xiiRTTI::FindTypeByName("xiiClusterLightAssignmentPass"));
  definition.m_Passes[10].m_Id = xiiUuid(10206399329399147984ULL, 17421228393966591884ULL);
  definition.AddPass("DirectionalSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDirectionalSurfaceLightingPass"));
  definition.m_Passes[11].m_Id = xiiUuid(16293980103712428951ULL, 8571080486287472301ULL);
  definition.AddPass("RadianceAddPass 2", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[12].m_Id = xiiUuid(17096503718580127458ULL, 9321650475080323977ULL);
  definition.AddPass("MaterialUploadPass 1", xiiRTTI::FindTypeByName("xiiMaterialUploadPass"));
  definition.m_Passes[13].m_Id = xiiUuid(9733758830144807697ULL, 16147448920409080164ULL);
  definition.AddPass("DepthRasterPass 1", xiiRTTI::FindTypeByName("xiiDepthRasterPass"));
  definition.m_Passes[14].m_Id = xiiUuid(12416354358818154928ULL, 499563762721050332ULL);
  definition.AddPass("GBufferRasterPass 1", xiiRTTI::FindTypeByName("xiiGBufferRasterPass"));
  definition.m_Passes[15].m_Id = xiiUuid(13275141206278314661ULL, 10492493429547014563ULL);
  definition.AddPass("SurfaceRasterConstantsPass 1", xiiRTTI::FindTypeByName("xiiSurfaceRasterConstantsPass"));
  definition.m_Passes[16].m_Id = xiiUuid(14181680113114492483ULL, 852648553493322375ULL);
  definition.AddPass("DepthTargetPass 1", xiiRTTI::FindTypeByName("xiiDepthTargetPass"), Properties({{"DepthTarget", Properties({{"Format", xiiInt64(40LL)}, {"Requirements", Properties({{"Dimension", xiiInt64(0LL)}, {"Height", xiiUInt32(0U)}, {"RequiredBindFlags", xiiInt64(0LL)}, {"Samples", xiiUInt32(0U)}, {"Width", xiiUInt32(0U)}})}, {"Resolution", Properties({{"Height", xiiUInt32(0U)}, {"Input", xiiString("")}, {"Mode", xiiInt64(1LL)}, {"Scale", 1.0f}, {"Width", xiiUInt32(0U)}})}})}}));
  definition.m_Passes[17].m_Id = xiiUuid(2911010405087331611ULL, 12733524781791108692ULL);
  definition.AddPass("VertexPullDrawCommandsPass 1", xiiRTTI::FindTypeByName("xiiVertexPullDrawCommandsPass"));
  definition.m_Passes[18].m_Id = xiiUuid(10269434197024419784ULL, 4036604311031070274ULL);
  definition.AddPass("MeshletDispatchBoundsPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchBoundsPass"));
  definition.m_Passes[19].m_Id = xiiUuid(16578175431111836935ULL, 16004929866390590561ULL);
  definition.AddPass("MeshletCullPass 1", xiiRTTI::FindTypeByName("xiiMeshletCullPass"));
  definition.m_Passes[20].m_Id = xiiUuid(10993476076163345223ULL, 8300012352774010332ULL);
  definition.AddPass("CounterResetPass 1", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[21].m_Id = xiiUuid(7652295761447284549ULL, 1041418901650981914ULL);
  definition.AddPass("CounterResetPass 2", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[22].m_Id = xiiUuid(12461066611857244060ULL, 15471186104970878926ULL);
  definition.AddPass("GeometryUploadPass 1", xiiRTTI::FindTypeByName("xiiGeometryUploadPass"));
  definition.m_Passes[23].m_Id = xiiUuid(11813050977863171363ULL, 4693241604933286161ULL);
  definition.AddPass("VisibilityViewPass 1", xiiRTTI::FindTypeByName("xiiVisibilityViewPass"));
  definition.m_Passes[24].m_Id = xiiUuid(5888887906387321399ULL, 1446360893955185960ULL);
  definition.AddPass("BufferHistoryPass 1", xiiRTTI::FindTypeByName("xiiBufferHistoryPass"));
  definition.m_Passes[25].m_Id = xiiUuid(8181022584001049134ULL, 11788360774404958271ULL);
  definition.AddPass("LodHistoryResetPass 1", xiiRTTI::FindTypeByName("xiiLodHistoryResetPass"));
  definition.m_Passes[26].m_Id = xiiUuid(9765820011100469053ULL, 4241434545776314152ULL);
  definition.AddPass("WorldInstancesUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldInstancesUploadPass"));
  definition.m_Passes[27].m_Id = xiiUuid(10562151821271187847ULL, 13720399590009735127ULL);
  definition.AddPass("InstanceCullPass 1", xiiRTTI::FindTypeByName("xiiInstanceCullPass"));
  definition.m_Passes[28].m_Id = xiiUuid(2689651741018322995ULL, 17403516736494923768ULL);
  definition.AddPass("CounterResetPass 3", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[29].m_Id = xiiUuid(12367218102350278841ULL, 18074187468169114169ULL);
  definition.AddPass("CounterResetPass 4", xiiRTTI::FindTypeByName("xiiCounterResetPass"));
  definition.m_Passes[30].m_Id = xiiUuid(9198870708929059524ULL, 16025062023187649193ULL);
  definition.AddPass("MeshletDispatchPass 1", xiiRTTI::FindTypeByName("xiiMeshletDispatchPass"));
  definition.m_Passes[31].m_Id = xiiUuid(16549408489489698951ULL, 3254476280185645961ULL);
  definition.AddPass("DiffuseSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDiffuseSurfaceLightingPass"));
  definition.m_Passes[32].m_Id = xiiUuid(15456436784775397419ULL, 12676553589604815211ULL);
  definition.AddPass("RadianceAddPass 3", xiiRTTI::FindTypeByName("xiiRadianceAddPass"));
  definition.m_Passes[33].m_Id = xiiUuid(2468229430883449617ULL, 13242236703208982650ULL);
  definition.Connect(6, "Layout", 8, "Layout").AssertSuccess();
  definition.Connect(7, "LightGrid", 10, "InitializedGrid").AssertSuccess();
  definition.Connect(8, "Clusters", 10, "Clusters").AssertSuccess();
  definition.Connect(7, "LightIndices", 10, "InitializedIndices").AssertSuccess();
  definition.Connect(6, "Layout", 7, "Layout").AssertSuccess();
  definition.Connect(6, "Layout", 10, "Layout").AssertSuccess();
  definition.Connect(10, "LightGrid", 9, "LightGrid").AssertSuccess();
  definition.Connect(6, "Layout", 9, "Layout").AssertSuccess();
  definition.Connect(7, "Overflow", 10, "InitializedOverflow").AssertSuccess();
  definition.Connect(10, "LightIndices", 9, "LightIndices").AssertSuccess();
  definition.Connect(6, "Layout", 5, "Layout").AssertSuccess();
  definition.Connect(2, "DirectionalLight", 3, "DirectionalLight").AssertSuccess();
  definition.Connect(3, "EnvironmentLight", 11, "EnvironmentLight").AssertSuccess();
  definition.Connect(10, "Overflow", 5, "Overflow").AssertSuccess();
  definition.Connect(10, "LightGrid", 5, "LightGrid").AssertSuccess();
  definition.Connect(4, "Lights", 9, "Lights").AssertSuccess();
  definition.Connect(4, "Lights", 10, "Lights").AssertSuccess();
  definition.Connect(4, "Lights", 5, "Lights").AssertSuccess();
  definition.Connect(4, "Candidates", 5, "Candidates").AssertSuccess();
  definition.Connect(4, "IESProfiles", 5, "IESProfiles").AssertSuccess();
  definition.Connect(4, "Candidates", 10, "Candidates").AssertSuccess();
  definition.Connect(10, "LightIndices", 5, "LightIndices").AssertSuccess();
  definition.Connect(4, "Counts", 5, "Counts").AssertSuccess();
  definition.Connect(23, "GeometryMetadata", 28, "GeometryMetadata").AssertSuccess();
  definition.Connect(26, "LodHistory", 28, "InitializedLodHistory").AssertSuccess();
  definition.Connect(27, "SceneInstances", 28, "SceneInstances").AssertSuccess();
  definition.Connect(27, "SceneInstances", 26, "SceneInstances").AssertSuccess();
  definition.Connect(29, "Counter", 28, "Counter").AssertSuccess();
  definition.Connect(24, "Candidates", 28, "Candidates").AssertSuccess();
  definition.Connect(25, "PreviousBuffer", 28, "PreviousLodHistory").AssertSuccess();
  definition.Connect(28, "LodHistory", 25, "Buffer").AssertSuccess();
  definition.Connect(24, "VisibilityView", 28, "VisibilityView").AssertSuccess();
  definition.Connect(23, "GeometryMetadata", 20, "GeometryMetadata").AssertSuccess();
  definition.Connect(28, "LodHistory", 19, "LodHistory").AssertSuccess();
  definition.Connect(23, "MeshletMetadata", 31, "MeshletMetadata").AssertSuccess();
  definition.Connect(27, "SceneInstances", 19, "SceneInstances").AssertSuccess();
  definition.Connect(28, "VisibleInstanceCount", 20, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(21, "Counter", 20, "InitialCount").AssertSuccess();
  definition.Connect(24, "VisibilityView", 19, "VisibilityView").AssertSuccess();
  definition.Connect(28, "VisibleInstanceCount", 31, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(23, "GeometryMetadata", 19, "GeometryMetadata").AssertSuccess();
  definition.Connect(22, "Counter", 19, "InitializedMaximum").AssertSuccess();
  definition.Connect(27, "SceneInstances", 20, "SceneInstances").AssertSuccess();
  definition.Connect(28, "VisibleInstances", 20, "VisibleInstances").AssertSuccess();
  definition.Connect(19, "MaximumMeshletGroups", 31, "MaximumMeshletGroups").AssertSuccess();
  definition.Connect(28, "VisibleInstances", 31, "VisibleInstances").AssertSuccess();
  definition.Connect(28, "VisibleInstances", 19, "VisibleInstances").AssertSuccess();
  definition.Connect(30, "Counter", 20, "InitialOverflow").AssertSuccess();
  definition.Connect(31, "DispatchArguments", 20, "DispatchArguments").AssertSuccess();
  definition.Connect(23, "MeshletMetadata", 20, "MeshletMetadata").AssertSuccess();
  definition.Connect(28, "LodHistory", 20, "LodHistory").AssertSuccess();
  definition.Connect(28, "VisibleInstanceCount", 19, "VisibleInstanceCount").AssertSuccess();
  definition.Connect(23, "MeshletMetadata", 19, "MeshletMetadata").AssertSuccess();
  definition.Connect(24, "VisibilityView", 20, "VisibilityView").AssertSuccess();
  definition.Connect(23, "GeometryMetadata", 18, "GeometryMetadata").AssertSuccess();
  definition.Connect(20, "VisibleMeshlets", 18, "VisibleMeshlets").AssertSuccess();
  definition.Connect(20, "VisibleMeshletCount", 18, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(24, "VisibilityView", 18, "VisibilityView").AssertSuccess();
  definition.Connect(23, "MeshletMetadata", 18, "MeshletMetadata").AssertSuccess();
  definition.Connect(27, "SceneInstances", 18, "SceneInstances").AssertSuccess();
  definition.Connect(17, "DepthTarget", 14, "DepthTarget").AssertSuccess();
  definition.Connect(16, "RasterConstants", 14, "RasterConstants").AssertSuccess();
  definition.Connect(27, "SceneInstances", 15, "SceneInstances").AssertSuccess();
  definition.Connect(20, "VisibleMeshletCount", 14, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(18, "DrawCommandCount", 15, "DrawCommandCount").AssertSuccess();
  definition.Connect(27, "SkinningMatrices", 15, "SkinningMatrices").AssertSuccess();
  definition.Connect(13, "SurfaceMaterials", 15, "SurfaceMaterials").AssertSuccess();
  definition.Connect(23, "GeometryMetadata", 14, "GeometryMetadata").AssertSuccess();
  definition.Connect(20, "VisibleMeshlets", 14, "VisibleMeshlets").AssertSuccess();
  definition.Connect(23, "GeometryMetadata", 15, "GeometryMetadata").AssertSuccess();
  definition.Connect(27, "SkinningMatrices", 14, "SkinningMatrices").AssertSuccess();
  definition.Connect(20, "VisibleMeshletCount", 15, "VisibleMeshletCount").AssertSuccess();
  definition.Connect(18, "DrawCommands", 14, "DrawCommands").AssertSuccess();
  definition.Connect(14, "SceneDepth", 15, "SceneDepth").AssertSuccess();
  definition.Connect(18, "DrawCommandCount", 14, "DrawCommandCount").AssertSuccess();
  definition.Connect(18, "DrawCommands", 15, "DrawCommands").AssertSuccess();
  definition.Connect(16, "RasterConstants", 15, "RasterConstants").AssertSuccess();
  definition.Connect(27, "SceneInstances", 14, "SceneInstances").AssertSuccess();
  definition.Connect(23, "MeshletMetadata", 14, "MeshletMetadata").AssertSuccess();
  definition.Connect(23, "MeshletMetadata", 15, "MeshletMetadata").AssertSuccess();
  definition.Connect(13, "SurfaceMaterials", 14, "SurfaceMaterials").AssertSuccess();
  definition.Connect(20, "VisibleMeshlets", 15, "VisibleMeshlets").AssertSuccess();
  definition.Connect(15, "Albedo", 5, "Albedo").AssertSuccess();
  definition.Connect(15, "Albedo", 11, "Albedo").AssertSuccess();
  definition.Connect(15, "Emissive", 32, "Emissive").AssertSuccess();
  definition.Connect(15, "Material", 32, "Material").AssertSuccess();
  definition.Connect(5, "Lighting", 33, "B").AssertSuccess();
  definition.Connect(14, "SceneDepth", 32, "Depth").AssertSuccess();
  definition.Connect(33, "Radiance", 12, "A").AssertSuccess();
  definition.Connect(15, "Emissive", 0, "B").AssertSuccess();
  definition.Connect(15, "Albedo", 32, "Albedo").AssertSuccess();
  definition.Connect(11, "Lighting", 33, "A").AssertSuccess();
  definition.Connect(3, "EnvironmentLight", 32, "EnvironmentLight").AssertSuccess();
  definition.Connect(1, "PerFrameCamera", 5, "Camera").AssertSuccess();
  definition.Connect(1, "PerFrameCamera", 11, "Camera").AssertSuccess();
  definition.Connect(32, "Lighting", 12, "B").AssertSuccess();
  definition.Connect(15, "Material", 11, "Material").AssertSuccess();
  definition.Connect(15, "Material", 5, "Material").AssertSuccess();
  definition.Connect(14, "SceneDepth", 5, "Depth").AssertSuccess();
  definition.Connect(14, "SceneDepth", 11, "Depth").AssertSuccess();
  definition.Connect(15, "Emissive", 5, "Emissive").AssertSuccess();
  definition.Connect(15, "Emissive", 11, "Emissive").AssertSuccess();
  definition.Connect(12, "Radiance", 0, "A").AssertSuccess();
  definition.Connect(15, "Normal", 5, "Normal").AssertSuccess();
  definition.Connect(15, "Normal", 11, "Normal").AssertSuccess();
  definition.m_Interface.PushBack({"ObjectId", false, xiiUuid(13275141206278314661ULL, 10492493429547014563ULL), "ObjectId"});
  definition.m_Interface.PushBack({"Material", false, xiiUuid(13275141206278314661ULL, 10492493429547014563ULL), "Material"});
  definition.m_Interface.PushBack({"InputCandidates", true, xiiUuid(5888887906387321399ULL, 1446360893955185960ULL), "InputCandidates"});
  definition.m_Interface.PushBack({"Velocity", false, xiiUuid(13275141206278314661ULL, 10492493429547014563ULL), "Velocity"});
  definition.m_Interface.PushBack({"DirectLighting", false, xiiUuid(2468229430883449617ULL, 13242236703208982650ULL), "Radiance"});
  definition.m_Interface.PushBack({"Emissive", false, xiiUuid(13275141206278314661ULL, 10492493429547014563ULL), "Emissive"});
  definition.m_Interface.PushBack({"LightListOverflow", false, xiiUuid(10206399329399147984ULL, 17421228393966591884ULL), "Overflow"});
  definition.m_Interface.PushBack({"VisibleMeshletOverflow", false, xiiUuid(10993476076163345223ULL, 8300012352774010332ULL), "VisibleMeshletOverflow"});
  definition.m_Interface.PushBack({"HDRSceneColor", false, xiiUuid(5477411076574998343ULL, 4289490620101449408ULL), "Radiance"});
  definition.m_Interface.PushBack({"SceneDepth", false, xiiUuid(12416354358818154928ULL, 499563762721050332ULL), "SceneDepth"});
  definition.m_Interface.PushBack({"Albedo", false, xiiUuid(13275141206278314661ULL, 10492493429547014563ULL), "Albedo"});
  definition.m_Interface.PushBack({"AmbientLighting", false, xiiUuid(15456436784775397419ULL, 12676553589604815211ULL), "Lighting"});
  definition.m_Interface.PushBack({"Normal", false, xiiUuid(13275141206278314661ULL, 10492493429547014563ULL), "Normal"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateWorldDirectionalLightTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/WorldDirectionalLight";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(14251665419917275177ULL, 4295872589748721808ULL);
  definition.AddPass("WorldDirectionalLightPass 1", xiiRTTI::FindTypeByName("xiiWorldDirectionalLightPass"));
  definition.m_Passes[0].m_Id = xiiUuid(7565203138859174581ULL, 15289613082172290497ULL);
  definition.m_Interface.PushBack({"DirectionalLight", false, xiiUuid(7565203138859174581ULL, 15289613082172290497ULL), "DirectionalLight"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateWorldLightsUploadTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/WorldLightsUpload";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id = xiiUuid(4636350184676495845ULL, 8987891191670048097ULL);
  definition.AddPass("WorldLightsUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldLightsUploadPass"));
  definition.m_Passes[0].m_Id = xiiUuid(15269828565987023880ULL, 18391294450165003178ULL);
  definition.m_Interface.PushBack({"Candidates", false, xiiUuid(15269828565987023880ULL, 18391294450165003178ULL), "Candidates"});
  definition.m_Interface.PushBack({"IESProfiles", false, xiiUuid(15269828565987023880ULL, 18391294450165003178ULL), "IESProfiles"});
  definition.m_Interface.PushBack({"Counts", false, xiiUuid(15269828565987023880ULL, 18391294450165003178ULL), "Counts"});
  definition.m_Interface.PushBack({"Lights", false, xiiUuid(15269828565987023880ULL, 18391294450165003178ULL), "Lights"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}


xiiRenderPipelineResourceHandle xiiCreateWorldRayProbeTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/WorldRayProbe";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(16184759034937217507ULL, 15606785076350165742ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::RayTracing | xiiRenderPipelineDeviceFeature::BindlessResources;
  definition.AddPass("SceneRayProbePass 1", xiiRTTI::FindTypeByName("xiiSceneRayProbePass"));
  definition.m_Passes[0].m_Id = xiiUuid(1695846674966500877ULL, 8043895497244262800ULL);
  definition.AddPass("BufferExportPass 1", xiiRTTI::FindTypeByName("xiiBufferExportPass"));
  definition.m_Passes[1].m_Id = xiiUuid(7356440336819582616ULL, 10560683887652315558ULL);
  definition.AddPass("RayTracingScenePass 1", xiiRTTI::FindTypeByName("xiiRayTracingScenePass"), Properties({{"RequireHardwareRayTracing", true}, {"UseViewSelection", false}}));
  definition.m_Passes[2].m_Id = xiiUuid(11670277782495461018ULL, 17610966797425055894ULL);
  definition.Connect(2, "Materials", 0, "Materials").AssertSuccess();
  definition.Connect(2, "Scene", 0, "Scene").AssertSuccess();
  definition.Connect(0, "Result", 1, "Input").AssertSuccess();
  definition.m_Interface.PushBack({"Result", false, xiiUuid(7356440336819582616ULL, 10560683887652315558ULL), "Output"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}

xiiRenderPipelineResourceHandle xiiCreateWorldSurfaceLightingTestResource()
{
  constexpr xiiStringView identity = "UnitTests/Pipeline/WorldSurfaceLighting";
  if (auto existing = xiiResourceManager::GetExistingResource<xiiRenderPipelineResource>(identity); existing.IsValid())
    return existing;
  xiiRenderPipelineResourceDescriptor definition;
  definition.m_Id                                = xiiUuid(12680156662554852282ULL, 11321362737852015695ULL);
  definition.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
  definition.AddPass("WorldDirectionalLightPass 1", xiiRTTI::FindTypeByName("xiiWorldDirectionalLightPass"));
  definition.m_Passes[0].m_Id = xiiUuid(7780200389710980364ULL, 15494507136995808670ULL);
  definition.AddPass("EnvironmentLightConstantsPass 1", xiiRTTI::FindTypeByName("xiiEnvironmentLightConstantsPass"));
  definition.m_Passes[1].m_Id = xiiUuid(10281800150827018988ULL, 2795480540299796321ULL);
  definition.AddPass("WorldLightsUploadPass 1", xiiRTTI::FindTypeByName("xiiWorldLightsUploadPass"));
  definition.m_Passes[2].m_Id = xiiUuid(15125569569716004367ULL, 18190304551918696538ULL);
  definition.AddPass("ClusteredLocalSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiClusteredLocalSurfaceLightingPass"));
  definition.m_Passes[3].m_Id = xiiUuid(7675921542475334731ULL, 10539629826799664384ULL);
  definition.AddPass("LightClusterViewPass 1", xiiRTTI::FindTypeByName("xiiLightClusterViewPass"));
  definition.m_Passes[4].m_Id = xiiUuid(4606073668911545417ULL, 10048429954921695519ULL);
  definition.AddPass("ClusterListClearPass 1", xiiRTTI::FindTypeByName("xiiClusterListClearPass"));
  definition.m_Passes[5].m_Id = xiiUuid(7944343800119681004ULL, 10074563760791893192ULL);
  definition.AddPass("ClusterBoundsPass 1", xiiRTTI::FindTypeByName("xiiClusterBoundsPass"));
  definition.m_Passes[6].m_Id = xiiUuid(13304893978312641732ULL, 7604436106421520259ULL);
  definition.AddPass("ClusterLightSamplingPass 1", xiiRTTI::FindTypeByName("xiiClusterLightSamplingPass"));
  definition.m_Passes[7].m_Id = xiiUuid(14383063676178531874ULL, 1266420858731945380ULL);
  definition.AddPass("ClusterLightAssignmentPass 1", xiiRTTI::FindTypeByName("xiiClusterLightAssignmentPass"));
  definition.m_Passes[8].m_Id = xiiUuid(18286443631724066556ULL, 9283816026741481449ULL);
  definition.AddPass("DirectionalSurfaceLightingPass 1", xiiRTTI::FindTypeByName("xiiDirectionalSurfaceLightingPass"));
  definition.m_Passes[9].m_Id = xiiUuid(9098509417423990255ULL, 17040008419373292256ULL);
  definition.Connect(4, "Layout", 6, "Layout").AssertSuccess();
  definition.Connect(5, "LightGrid", 8, "InitializedGrid").AssertSuccess();
  definition.Connect(6, "Clusters", 8, "Clusters").AssertSuccess();
  definition.Connect(5, "LightIndices", 8, "InitializedIndices").AssertSuccess();
  definition.Connect(4, "Layout", 5, "Layout").AssertSuccess();
  definition.Connect(4, "Layout", 8, "Layout").AssertSuccess();
  definition.Connect(8, "LightGrid", 7, "LightGrid").AssertSuccess();
  definition.Connect(4, "Layout", 7, "Layout").AssertSuccess();
  definition.Connect(5, "Overflow", 8, "InitializedOverflow").AssertSuccess();
  definition.Connect(8, "LightIndices", 7, "LightIndices").AssertSuccess();
  definition.Connect(4, "Layout", 3, "Layout").AssertSuccess();
  definition.Connect(0, "DirectionalLight", 1, "DirectionalLight").AssertSuccess();
  definition.Connect(1, "EnvironmentLight", 9, "EnvironmentLight").AssertSuccess();
  definition.Connect(8, "Overflow", 3, "Overflow").AssertSuccess();
  definition.Connect(8, "LightGrid", 3, "LightGrid").AssertSuccess();
  definition.Connect(2, "Lights", 7, "Lights").AssertSuccess();
  definition.Connect(2, "Lights", 8, "Lights").AssertSuccess();
  definition.Connect(2, "Lights", 3, "Lights").AssertSuccess();
  definition.Connect(2, "Candidates", 3, "Candidates").AssertSuccess();
  definition.Connect(2, "IESProfiles", 3, "IESProfiles").AssertSuccess();
  definition.Connect(2, "Candidates", 8, "Candidates").AssertSuccess();
  definition.Connect(8, "LightIndices", 3, "LightIndices").AssertSuccess();
  definition.Connect(2, "Counts", 3, "Counts").AssertSuccess();
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(7675921542475334731ULL, 10539629826799664384ULL), "Normal"});
  definition.m_Interface.PushBack({"Normal", true, xiiUuid(9098509417423990255ULL, 17040008419373292256ULL), "Normal"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(7675921542475334731ULL, 10539629826799664384ULL), "Emissive"});
  definition.m_Interface.PushBack({"Emissive", true, xiiUuid(9098509417423990255ULL, 17040008419373292256ULL), "Emissive"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(7675921542475334731ULL, 10539629826799664384ULL), "Depth"});
  definition.m_Interface.PushBack({"Depth", true, xiiUuid(9098509417423990255ULL, 17040008419373292256ULL), "Depth"});
  definition.m_Interface.PushBack({"DirectionalLighting", false, xiiUuid(9098509417423990255ULL, 17040008419373292256ULL), "Lighting"});
  definition.m_Interface.PushBack({"Overflow", false, xiiUuid(18286443631724066556ULL, 9283816026741481449ULL), "Overflow"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(7675921542475334731ULL, 10539629826799664384ULL), "Camera"});
  definition.m_Interface.PushBack({"Camera", true, xiiUuid(9098509417423990255ULL, 17040008419373292256ULL), "Camera"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(7675921542475334731ULL, 10539629826799664384ULL), "Albedo"});
  definition.m_Interface.PushBack({"Albedo", true, xiiUuid(9098509417423990255ULL, 17040008419373292256ULL), "Albedo"});
  definition.m_Interface.PushBack({"Visibility", true, xiiUuid(9098509417423990255ULL, 17040008419373292256ULL), "Visibility"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(9098509417423990255ULL, 17040008419373292256ULL), "Material"});
  definition.m_Interface.PushBack({"Material", true, xiiUuid(7675921542475334731ULL, 10539629826799664384ULL), "Material"});
  definition.m_Interface.PushBack({"LocalLighting", false, xiiUuid(7675921542475334731ULL, 10539629826799664384ULL), "Lighting"});
  definition.m_Interface.PushBack({"EnvironmentLight", false, xiiUuid(10281800150827018988ULL, 2795480540299796321ULL), "EnvironmentLight"});
  return xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>(identity, std::move(definition));
}
