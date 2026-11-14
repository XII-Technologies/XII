/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <GraphicsCore/Lighting/DDGIRenderSettings.h>
#include <GraphicsCore/Lighting/PostProcessRenderSettings.h>
#include <GraphicsCore/Pipeline/DynamicResolutionSettings.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeSchedulePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Streaming/RenderDeviceProfileConfig.h>

class xiiContributedRenderSettings final : public xiiRenderSubsystemSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiContributedRenderSettings, xiiRenderSubsystemSettings);

public:
  float m_fLower = 0.2f;
  float m_fUpper = 0.8f;
};

class xiiContributedSettingsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiContributedSettingsPass, xiiRenderPipelinePass);

public:
  float     m_fLower                 = 0.2f;
  float     m_fUpper                 = 0.8f;
  xiiUInt32 m_uiHistoryInvalidations = 0;
  xiiUInt32 m_uiImplementationState  = 42;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  xiiResult Configure(const xiiVariantDictionary& properties) override
  {
    XII_SUCCEED_OR_RETURN(SUPER::Configure(properties));
    return m_fLower <= m_fUpper ? XII_SUCCESS : XII_FAILURE;
  }
  void      InvalidateTemporalHistory() override { ++m_uiHistoryInvalidations; }
  xiiResult Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiContributedRenderSettings, 1, xiiRTTIDefaultAllocator<xiiContributedRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Lower", m_fLower)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiRenderSettingsBindingAttribute("xiiContributedSettingsPass", "Lower", xiiRenderSettingsChange::History)),
    XII_MEMBER_PROPERTY("Upper", m_fUpper)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiRenderSettingsBindingAttribute("xiiContributedSettingsPass", "Upper")),
  } XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiContributedSettingsPass, 1, xiiRTTIDefaultAllocator<xiiContributedSettingsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Lower", m_fLower),
    XII_MEMBER_PROPERTY("Upper", m_fUpper),
  } XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_CREATE_SIMPLE_TEST(Pipeline, DDGIReflectedDefaults)
{
  xiiDDGIProbeSchedulePass pass;
  const auto*              pProperty = pass.GetDynamicRTTI()->FindPropertyByName("TraceDistanceInProbeSpacings");
  if (!XII_TEST_BOOL(pProperty != nullptr))
    return;

  const xiiVariant defaultValue = xiiReflectionUtils::GetDefaultValue(pProperty);
  XII_TEST_FLOAT(defaultValue.ConvertTo<float>(), pass.m_fTraceDistanceInProbeSpacings, 0.001f);

  xiiVariantDictionary properties;
  properties.Insert("TraceDistanceInProbeSpacings", defaultValue);
  XII_TEST_RESULT(pass.Configure(properties));
  xiiRenderPipelinePass* passes[] = {&pass};
  xiiRenderConfiguration defaults;
  XII_TEST_RESULT(defaults.InitializeSubsystemDefaults(xiiMakeArrayPtr(passes)));
  const auto* pSettings = defaults.FindSubsystem<xiiDDGIRenderSettings>();
  if (XII_TEST_BOOL(pSettings != nullptr))
    XII_TEST_FLOAT(pSettings->m_fTraceDistanceInProbeSpacings, 4.0f, 0.001f);
}

XII_CREATE_SIMPLE_TEST(Pipeline, TypedSubsystemInheritance)
{
  xiiRenderConfiguration project, pipeline, scene, camera, viewport;
  XII_TEST_RESULT(project.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeSpacing", 6.0f));
  XII_TEST_RESULT(project.SetOverride<xiiDynamicResolutionSettings>("TargetFrameTimeMs", 20.0f));
  XII_TEST_RESULT(pipeline.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeUpdateBudget", 32U));
  XII_TEST_RESULT(scene.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeSpacing", 3.0f));
  XII_TEST_RESULT(camera.SetOverride<xiiDynamicResolutionSettings>("RenderScale", 0.7f));
  XII_TEST_RESULT(viewport.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeUpdateBudget", 64U));
  auto resolve = [&]() {
    xiiRenderConfiguration effective;
    effective.ApplyOverrides(project, xiiRenderSettingScope::Project);
    effective.ApplyOverrides(pipeline, xiiRenderSettingScope::Pipeline);
    effective.ApplyOverrides(scene, xiiRenderSettingScope::Scene);
    effective.ApplyOverrides(camera, xiiRenderSettingScope::Camera);
    effective.ApplyOverrides(viewport, xiiRenderSettingScope::Viewport);
    return effective;
  };
  auto effective = resolve();
  XII_TEST_FLOAT(effective.FindSubsystem<xiiDDGIRenderSettings>()->m_Probes.m_fProbeSpacing, 3.0f, 0.001f);
  XII_TEST_INT(effective.FindSubsystem<xiiDDGIRenderSettings>()->m_Probes.m_uiProbeUpdateBudget, 64U);
  XII_TEST_INT(effective.GetSource(xiiGetStaticRTTI<xiiDDGIRenderSettings>(), "Probes/ProbeSpacing"), xiiRenderSettingScope::Scene);
  XII_TEST_INT(effective.GetSource(xiiGetStaticRTTI<xiiDynamicResolutionSettings>(), "RenderScale"), xiiRenderSettingScope::Camera);
  XII_TEST_INT(effective.GetSource(xiiGetStaticRTTI<xiiDynamicResolutionSettings>(), "TargetFrameTimeMs"), xiiRenderSettingScope::Project);
  viewport.RemoveOverride(xiiGetStaticRTTI<xiiDDGIRenderSettings>(), "Probes/ProbeUpdateBudget");
  effective = resolve();
  XII_TEST_INT(effective.FindSubsystem<xiiDDGIRenderSettings>()->m_Probes.m_uiProbeUpdateBudget, 32U);
  XII_TEST_INT(effective.GetSource(xiiGetStaticRTTI<xiiDDGIRenderSettings>(), "Probes/ProbeUpdateBudget"), xiiRenderSettingScope::Pipeline);
  auto independent = effective;
  XII_TEST_RESULT(independent.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeSpacing", 10.0f));
  XII_TEST_FLOAT(effective.FindSubsystem<xiiDDGIRenderSettings>()->m_Probes.m_fProbeSpacing, 3.0f, 0.001f);
  const auto before = independent;
  XII_TEST_BOOL(independent.SetOverride<xiiDDGIRenderSettings>("Probes/ProbeSpacing", 0.0f).Failed());
  XII_TEST_BOOL(independent == before);
  XII_TEST_BOOL(independent.SetOverride<xiiDDGIRenderSettings>("UnknownProperty", 2.0f).Failed());
  XII_TEST_BOOL(independent == before);
  XII_TEST_BOOL(independent.GetChanges(effective).IsSet(xiiRenderSettingsChange::Resources));

  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  XII_TEST_RESULT(independent.Serialize(writer));
  xiiMemoryStreamReader  reader(&storage);
  xiiRenderConfiguration restored;
  XII_TEST_RESULT(restored.Deserialize(reader, xiiRenderConfiguration::s_uiSerializationVersion));
  XII_TEST_BOOL(restored == independent);
  XII_TEST_BOOL(restored.Deserialize(reader, xiiRenderConfiguration::s_uiSerializationVersion).Failed());
  XII_TEST_BOOL(restored == independent);
}

XII_CREATE_SIMPLE_TEST(Pipeline, ContributedSubsystemPublication)
{
  xiiRenderPipelineResourceDescriptor descriptor;
  xiiVariantDictionary                authored;
  authored.Insert("Lower", 0.3f);
  descriptor.AddPass("Contributed", xiiGetStaticRTTI<xiiContributedSettingsPass>(), authored);
  xiiSharedPtr<const xiiRenderPipeline> pipeline;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, pipeline));
  if (!pipeline)
    return;
  auto        effective   = pipeline->GetSubsystemDefaults();
  const auto* contributed = effective.FindSubsystem<xiiContributedRenderSettings>();
  if (!XII_TEST_BOOL(contributed != nullptr))
    return;
  XII_TEST_FLOAT(contributed->m_fLower, 0.3f, 0.001f);
  XII_TEST_BOOL(!contributed->IsOverridden("Lower"));
  xiiRenderPipelineInstance instance, independent;
  xiiStringBuilder          error;
  XII_TEST_RESULT(instance.Initialize(pipeline, effective, error));
  XII_TEST_RESULT(independent.Initialize(pipeline, effective, error));
  const auto* original = static_cast<const xiiContributedSettingsPass*>(instance.GetPass(0));
  XII_TEST_RESULT(effective.SetOverride<xiiContributedRenderSettings>("Lower", 0.5f));
  XII_TEST_RESULT(instance.Initialize(pipeline, effective, error));
  XII_TEST_BOOL(instance.GetPass(0) == original);
  XII_TEST_FLOAT(original->m_fLower, 0.5f, 0.001f);
  XII_TEST_INT(original->m_uiImplementationState, 42U);
  XII_TEST_INT(original->m_uiHistoryInvalidations, 1U);
  XII_TEST_FLOAT(static_cast<const xiiContributedSettingsPass*>(independent.GetPass(0))->m_fLower, 0.3f, 0.001f);
  XII_TEST_RESULT(instance.Initialize(pipeline, effective, error));
  XII_TEST_INT(original->m_uiHistoryInvalidations, 1U);
  auto invalid = effective;
  XII_TEST_RESULT(invalid.SetOverride<xiiContributedRenderSettings>("Lower", 0.9f));
  XII_TEST_BOOL(instance.Initialize(pipeline, invalid, error).Failed());
  XII_TEST_FLOAT(original->m_fLower, 0.5f, 0.001f);
  XII_TEST_BOOL(instance.GetRenderConfiguration() == effective);
  effective.RemoveOverride(xiiGetStaticRTTI<xiiContributedRenderSettings>(), "Lower");
  XII_TEST_RESULT(instance.Initialize(pipeline, effective, error));
  XII_TEST_FLOAT(original->m_fLower, 0.3f, 0.001f);
}


XII_CREATE_SIMPLE_TEST(Pipeline, SubsystemSettingsGraphMigration)
{
  xiiAbstractObjectGraph legacy;
  auto*                  root   = legacy.AddNode(xiiUuid::MakeUuid(), "xiiRenderConfiguration", 4, "root");
  auto*                  probes = legacy.AddNode(xiiUuid::MakeUuid(), "xiiDDGIProbeOverrides", 1);
  root->AddProperty("GlobalIllumination", "xiiRenderGlobalIllumination::DDGI");
  root->AddProperty("DDGI", probes->GetGuid());
  root->AddProperty("BloomStrength", 0.0f);
  root->AddProperty("RenderScale", 0.0f);
  probes->AddProperty("ProbeUpdateBudget", 48U);

  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  xiiAbstractGraphDdlSerializer::Write(writer, &legacy);
  xiiMemoryStreamReader  reader(&storage);
  xiiAbstractObjectGraph migrated;
  XII_TEST_RESULT(xiiAbstractGraphDdlSerializer::Read(reader, &migrated));

  xiiRenderConfiguration  configuration;
  xiiRttiConverterContext context;
  xiiRttiConverterReader  converter(&migrated, &context);
  converter.ApplyPropertiesToObject(migrated.GetNodeByName("root"), xiiGetStaticRTTI<xiiRenderConfiguration>(), &configuration);
  XII_TEST_BOOL(configuration.IsValid());
  const auto* settings = configuration.FindSubsystem<xiiDDGIRenderSettings>();
  if (!XII_TEST_BOOL(settings != nullptr))
    return;
  XII_TEST_INT(settings->m_Probes.m_uiProbeUpdateBudget, 48);
  XII_TEST_BOOL(settings->IsOverridden("Probes/ProbeUpdateBudget"));
  XII_TEST_BOOL(!settings->IsOverridden("Probes/ProbeSpacing"));
  XII_TEST_BOOL(configuration.GetPipelineVariant().m_GlobalIllumination == xiiRenderGlobalIllumination::DDGI);
  XII_TEST_BOOL(configuration.FindSubsystem<xiiDynamicResolutionSettings>() == nullptr);
}

XII_CREATE_SIMPLE_TEST(Pipeline, DeviceStreamingProfile)
{
  xiiRenderDeviceProfileConfig authored;
  authored.m_Memory.m_bUseNativeBudget                 = false;
  authored.m_Memory.m_uiBudgetMiB                      = 768;
  authored.m_Residency.m_Streaming.m_iTextureUploadMiB = 12;
  authored.m_Residency.m_Streaming.m_fTextureMipBias   = 1.25f;
  XII_TEST_RESULT(authored.Validate());

  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  xiiChunkStreamWriter             chunks(writer);
  chunks.BeginStream(1);
  authored.SaveRuntimeData(chunks);
  chunks.EndStream();
  xiiMemoryStreamReader reader(&storage);
  xiiChunkStreamReader  stream(reader);
  stream.BeginStream();
  xiiRenderDeviceProfileConfig restored;
  restored.LoadRuntimeData(stream);
  XII_TEST_BOOL(restored.m_Memory == authored.m_Memory);
  XII_TEST_BOOL(restored.m_Residency == authored.m_Residency);

  authored.m_Memory.m_uiPressureLowPercent = authored.m_Memory.m_uiPressureHighPercent;
  XII_TEST_BOOL(authored.Validate().Failed());
}
