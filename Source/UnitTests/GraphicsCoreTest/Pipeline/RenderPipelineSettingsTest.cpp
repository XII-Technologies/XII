/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Lighting/DDGIRenderSettings.h>
#include <GraphicsCore/Lighting/GlobalIlluminationRenderSettings.h>
#include <GraphicsCore/Lighting/PostProcessRenderSettings.h>
#include <GraphicsCore/Lighting/ShadowRenderSettings.h>
#include <GraphicsCore/Pipeline/DynamicResolutionSettings.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeSchedulePass.h>
#include <GraphicsCore/Pipeline/RenderConfiguration.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <bit>

class xiiSettingsPipelinePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSettingsPipelinePass, xiiRenderPipelinePass);

public:
  static xiiUInt32 s_uiBuilds;
  void             GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  xiiResult        Build(xiiRenderPipelinePassContext&) override
  {
    ++s_uiBuilds;
    return XII_SUCCESS;
  }
};
xiiUInt32 xiiSettingsPipelinePass::s_uiBuilds = 0;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSettingsPipelinePass, 1, xiiRTTIDefaultAllocator<xiiSettingsPipelinePass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Pipeline, LegacyRenderConfigurationMigration)
{
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  const xiiUInt32                  bloom = std::bit_cast<xiiUInt32>(0.0f), scale = std::bit_cast<xiiUInt32>(0.7f);
  XII_TEST_RESULT(writer.WriteDWordValue(&bloom));
  XII_TEST_RESULT(writer.WriteDWordValue(&scale));
  const xiiUInt8 gi = xiiRenderGlobalIllumination::DDGI;
  XII_TEST_RESULT(writer.WriteBytes(&gi, 1));
  const xiiUInt32 probes[] = {4U, 4U, 4U, 16U, std::bit_cast<xiiUInt32>(2.0f), std::bit_cast<xiiUInt32>(0.95f), std::bit_cast<xiiUInt32>(0.5f)};
  for (const auto value : probes)
    XII_TEST_RESULT(writer.WriteDWordValue(&value));
  const xiiUInt8 policies[] = {2, 1};
  XII_TEST_RESULT(writer.WriteBytes(policies, 2));
  xiiMemoryStreamReader  reader(&storage);
  xiiRenderConfiguration migrated;
  XII_TEST_RESULT(migrated.Deserialize(reader, 4));
  XII_TEST_FLOAT(migrated.FindSubsystem<xiiPostProcessRenderSettings>()->m_Display.m_fBloomStrength, 0.0f, 0.001f);
  XII_TEST_FLOAT(migrated.FindSubsystem<xiiDynamicResolutionSettings>()->m_fRenderScale, 0.7f, 0.001f);
  XII_TEST_INT(migrated.FindSubsystem<xiiDDGIRenderSettings>()->m_Probes.m_uiProbeCountX, 4);
  XII_TEST_INT(migrated.FindSubsystem<xiiDDGIRenderSettings>()->m_Probes.m_uiProbeUpdateBudget, 16);
  XII_TEST_BOOL(migrated.FindSubsystem<xiiDDGIRenderSettings>()->IsOverridden("Probes/ProbeSpacing"));
  XII_TEST_BOOL(migrated.GetPipelineVariant().m_Disabled.IsSet(xiiOptionalRenderFeature::Shadows));
  const auto before = migrated;
  XII_TEST_BOOL(migrated.Deserialize(reader, 4).Failed());
  XII_TEST_BOOL(migrated == before);
  XII_TEST_BOOL(xiiGetStaticRTTI<xiiRenderConfiguration>()->FindPropertyByName("BloomStrength") == nullptr);
  XII_TEST_BOOL(xiiGetStaticRTTI<xiiRenderConfiguration>()->FindPropertyByName("Subsystems") != nullptr);
}

XII_CREATE_SIMPLE_TEST(Pipeline, RenderConfigurationProfileMigration)
{
  xiiPlatformProfile platform;
  platform.AddMissingConfigs();
  const auto* pDefaults = platform.GetTypeConfig<xiiRenderPipelineProfileConfig>();
  if (!XII_TEST_BOOL(pDefaults != nullptr))
    return;
  XII_TEST_BOOL(pDefaults->m_RenderConfiguration == xiiRenderConfiguration{});
  XII_TEST_STRING(pDefaults->GetResourceReference(), xiiRenderPipelineProfileConfig::s_szScenePipeline);

  xiiRenderPipelineProfileConfig saved;
  saved.m_sMainRenderPipeline                              = "CustomPipeline";
  XII_TEST_RESULT(saved.m_RenderConfiguration.SetOverride<xiiGlobalIlluminationRenderSettings>("Technique", xiiInt64(xiiRenderGlobalIllumination::DDGI)));
  XII_TEST_RESULT(saved.m_RenderConfiguration.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeUpdateBudget", 64U));
  XII_TEST_RESULT(saved.m_RenderConfiguration.SetOverride<xiiShadowRenderSettings>("Enabled", false));
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  xiiChunkStreamWriter             chunks(writer);
  chunks.BeginStream(1);
  saved.SaveRuntimeData(chunks);
  chunks.EndStream();

  xiiMemoryStreamReader reader(&storage);
  xiiChunkStreamReader  stream(reader);
  stream.BeginStream();
  xiiRenderPipelineProfileConfig loaded;
  loaded.LoadRuntimeData(stream);
  XII_TEST_BOOL(loaded.IsRuntimeDataValid());
  XII_TEST_BOOL(loaded.GetResourceReference() == saved.GetResourceReference());
  XII_TEST_BOOL(loaded.m_RenderConfiguration == saved.m_RenderConfiguration);

  xiiContiguousMemoryStreamStorage legacy;
  xiiMemoryStreamWriter            legacyWriter(&legacy);
  xiiChunkStreamWriter             legacyChunks(legacyWriter);
  legacyChunks.BeginStream(1);
  legacyChunks.BeginChunk("RenderPipelineReferences", 3);
  legacyChunks << saved.m_sMainRenderPipeline;
  legacyChunks.EndChunk();
  legacyChunks.EndStream();
  xiiMemoryStreamReader legacyReader(&legacy);
  xiiChunkStreamReader  legacyStream(legacyReader);
  legacyStream.BeginStream();
  loaded.LoadRuntimeData(legacyStream);
  XII_TEST_BOOL(loaded.IsRuntimeDataValid());
  XII_TEST_BOOL(loaded.GetResourceReference() == saved.GetResourceReference());
  XII_TEST_BOOL(loaded.m_RenderConfiguration == xiiRenderConfiguration{});
}

XII_CREATE_SIMPLE_TEST(Pipeline, RenderConfigurationVariants)
{
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Shared", xiiGetStaticRTTI<xiiSettingsPipelinePass>());
  const auto uiBranch = descriptor.AddPass("Probe update", xiiGetStaticRTTI<xiiSettingsPipelinePass>());
  descriptor.m_Passes[uiBranch].m_Parameters.Insert("GlobalIlluminationBranches", xiiRenderGlobalIllumination::BranchBit(xiiRenderGlobalIllumination::DDGI));
  const auto                                 resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("PipelineSettings/Variants", std::move(descriptor));
  xiiResourceLock<xiiRenderPipelineResource> acquired(resource, xiiResourceAcquireMode::BlockTillLoaded);
  const auto                                 base = acquired->GetPipeline();
  XII_TEST_INT(base->GetExecutionOrder().GetCount(), 2);
  xiiSharedPtr<const xiiRenderPipeline> disabled, reused;
  xiiStringBuilder                      error;
  XII_TEST_RESULT(acquired->GetPipelineVariant(base, {xiiRenderGlobalIllumination::Disabled, {}}, disabled, error));
  XII_TEST_RESULT(acquired->GetPipelineVariant(base, {xiiRenderGlobalIllumination::Disabled, {}}, reused, error));
  XII_TEST_BOOL(disabled == reused);
  XII_TEST_INT(disabled->GetExecutionOrder().GetCount(), 1);
  XII_TEST_BOOL(acquired->GetPipelineVariant(base, {xiiRenderGlobalIllumination::ScreenSpace, {}}, reused, error).Failed());
  XII_TEST_BOOL(error.FindSubString("does not author") != nullptr);
  XII_TEST_BOOL(reused == disabled);

  xiiRenderPipelineInstance first, second;
  xiiRenderConfiguration    selection;
  XII_TEST_RESULT(selection.SetOverride<xiiGlobalIlluminationRenderSettings>("Technique", xiiInt64(xiiRenderGlobalIllumination::Disabled)));
  XII_TEST_RESULT(first.Synchronize(resource, nullptr, selection));
  XII_TEST_RESULT(second.Synchronize(resource));
  XII_TEST_BOOL(first.GetPipeline() == disabled.Borrow());
  XII_TEST_BOOL(second.GetPipeline() == base.Borrow());
  XII_TEST_RESULT(selection.SetOverride<xiiGlobalIlluminationRenderSettings>("Technique", xiiInt64(xiiRenderGlobalIllumination::ScreenSpace)));
  XII_TEST_RESULT(first.Synchronize(resource, nullptr, selection));
  XII_TEST_BOOL(first.GetPipeline() == disabled.Borrow());
  XII_TEST_BOOL(first.GetRenderConfiguration().GetPipelineVariant().m_GlobalIllumination == xiiRenderGlobalIllumination::Disabled);
  XII_TEST_BOOL(!first.GetConfigurationError().IsEmpty());
}

XII_CREATE_SIMPLE_TEST(Pipeline, DDGIConfigurationValidation)
{
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Probes", xiiGetStaticRTTI<xiiDDGIProbeSchedulePass>());
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("PipelineSettings/ProbeSettings", std::move(descriptor));
  xiiRenderPipelineInstance instance;
  xiiRenderConfiguration    requested;
  XII_TEST_RESULT(requested.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeUpdateBudget", 16U));
  XII_TEST_RESULT(instance.Synchronize(resource, nullptr, requested));
  const auto* pass = instance.GetPass(0);
  if (!XII_TEST_BOOL(pass != nullptr))
    return;
  const auto before = requested;
  XII_TEST_BOOL(requested.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeCountX", 257U).Failed());
  XII_TEST_BOOL(requested == before);
  XII_TEST_RESULT(requested.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeCountX", 8U));
  XII_TEST_RESULT(instance.Synchronize(resource, nullptr, requested));
  XII_TEST_BOOL(instance.GetPass(0) == pass);
  XII_TEST_INT(static_cast<const xiiDDGIProbeSchedulePass*>(pass)->m_CountX, 8U);
  XII_TEST_INT(static_cast<const xiiDDGIProbeSchedulePass*>(pass)->m_Budget, 16U);
}

XII_CREATE_SIMPLE_TEST(Pipeline, AssetSettings)
{
  xiiRenderPipelineSettings settings;
  xiiStringBuilder          error;
  XII_TEST_RESULT(settings.Validate());
  XII_TEST_RESULT(settings.ValidateDevice(nullptr));
  XII_TEST_RESULT(settings.ValidateOutput(nullptr));
  settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::RayTracing;
  xiiGALDeviceFeatures features;
  features.m_ComputeShaders = xiiGALDeviceFeatureState::Enabled;
  XII_TEST_BOOL(settings.ValidateDevice(&features, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("RayTracing") != nullptr);
  features.m_RayTracing = xiiGALDeviceFeatureState::Optional;
  XII_TEST_BOOL(settings.ValidateDevice(&features).Failed());
  features.m_RayTracing = xiiGALDeviceFeatureState::Enabled;
  XII_TEST_RESULT(settings.ValidateDevice(&features));
  settings.m_OutputFormat    = xiiGALResourceFormat::RGBA8UNormalized;
  settings.m_uiOutputSamples = 1;
  xiiGALTextureCreationDescription output;
  output.m_Type          = xiiGALResourceDimension::Texture2D;
  output.m_Format        = xiiGALResourceFormat::RGBA8UNormalized;
  output.m_uiSampleCount = 1;
  XII_TEST_RESULT(settings.ValidateOutput(&output));
  XII_TEST_BOOL(settings.ValidateOutput(nullptr, &error).Failed());
  output.m_uiSampleCount = 4;
  XII_TEST_BOOL(settings.ValidateOutput(&output, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("OutputSamples") != nullptr);
  output.m_uiSampleCount = 1;
  output.m_Format        = xiiGALResourceFormat::RGBA16Float;
  XII_TEST_BOOL(settings.ValidateOutput(&output, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("OutputFormat") != nullptr);

  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Operation", xiiGetStaticRTTI<xiiSettingsPipelinePass>());
  descriptor.m_Settings = settings;
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
  xiiContiguousMemoryStreamStorage bytes, repeated;
  xiiMemoryStreamWriter            writer(&bytes), repeatWriter(&repeated);
  XII_TEST_RESULT(descriptor.Serialize(writer));
  XII_TEST_RESULT(descriptor.Serialize(repeatWriter));
  XII_TEST_BOOL(bytes.GetStorageSize32() == repeated.GetStorageSize32() && xiiMemoryUtils::IsEqual(bytes.GetData(), repeated.GetData(), bytes.GetStorageSize32()));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  XII_TEST_BOOL(restored.m_Settings.m_RequiredDeviceFeatures == settings.m_RequiredDeviceFeatures);
  XII_TEST_BOOL(restored.m_Settings.m_OutputFormat == settings.m_OutputFormat);
  XII_TEST_INT(restored.m_Settings.m_uiOutputSamples, 1);
  restored.m_Settings.m_uiOutputSamples = 3;
  XII_TEST_BOOL(xiiRenderPipeline::Compile(restored, compiled, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("OutputSamples") != nullptr);
  restored.m_Settings.m_uiOutputSamples = 1;
  restored.m_Settings.m_RequiredDeviceFeatures.SetValue(128);
  XII_TEST_BOOL(xiiRenderPipeline::Compile(restored, compiled, &error).Failed());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rejected pipeline output never constructs algorithm work")
  {
    // No view/output is selected. A contract failure precedes every pass Build.
    descriptor.m_Settings.m_RequiredDeviceFeatures = {};
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
    xiiRenderPipelineInstance instance;
    XII_TEST_RESULT(instance.Initialize(compiled));
    xiiRenderGraph           graph;
    xiiRenderGraphBlackboard blackboard;
    const auto               builds = xiiSettingsPipelinePass::s_uiBuilds;
    xiiTestLogInterface      log;
    xiiLogSystemScope        scope(&log);
    log.ExpectMessage("Pipeline output contract requires a selected output texture", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard).Failed());
    XII_TEST_INT(xiiSettingsPipelinePass::s_uiBuilds, builds);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Referenced definition device requirements constrain the containing resource")
  {
    xiiRenderPipelineResourceDescriptor child;
    child.AddPass("Child operation", xiiGetStaticRTTI<xiiSettingsPipelinePass>());
    child.m_Settings.m_RequiredDeviceFeatures    = xiiRenderPipelineDeviceFeature::MeshShaders;
    const auto                          id       = child.m_Id;
    const auto                          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("PipelineSettings/SharedDefinition", std::move(child));
    xiiRenderPipelineResourceDescriptor root;
    root.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
    root.AddSubgraph("First", id);
    root.AddSubgraph("Second", id);
    root.m_References.PushBack({id, resource.GetResourceID()});
    XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
    const auto& requirements = compiled->GetDescriptor().m_Settings;
    XII_TEST_BOOL(requirements.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::Compute));
    XII_TEST_BOOL(requirements.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::MeshShaders));
    XII_TEST_BOOL(requirements.ValidateDevice(&features, &error).Failed());
    XII_TEST_BOOL(error.FindSubString("MeshShaders") != nullptr);
    XII_TEST_BOOL(!root.m_Settings.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::MeshShaders));
  }
}
