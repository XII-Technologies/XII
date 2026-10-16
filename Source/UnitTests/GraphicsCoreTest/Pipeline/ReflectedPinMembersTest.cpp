/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionTemporalPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorOutputPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalAntiAliasingPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

class xiiMemberPinSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMemberPinSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureOutputPin m_Color{xiiGALResourceFormat::RGBA16Float, false, {}, xiiGALResourceDimension::Texture2D, 1};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphTextureHandle m_Color;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [](Data& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription description;
        description.m_Type = xiiGALResourceDimension::Texture2D;
        description.m_Size = {8, 8};
        description.m_Format = xiiGALResourceFormat::RGBA16Float;
        description.m_BindFlags = xiiGALBindFlags::ShaderResource;
        data.m_Color = builder.WriteTexture("Member color", description, xiiGALResourceStateFlags::ShaderResource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    return context.SetOutput(m_Color, emitted.first->m_Color);
  }
};
class xiiMemberPinSink : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMemberPinSink, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin m_Color;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    return context.GetInputTexture(m_Color).IsValid() ? XII_SUCCESS : XII_FAILURE;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMemberPinSource, 1, xiiRTTIDefaultAllocator<xiiMemberPinSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Color", m_Color)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMemberPinSink, 1, xiiRTTIDefaultAllocator<xiiMemberPinSink>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Color", m_Color)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Pipeline, ReflectedPinMembers)
{
  // No GetPins implementation or parallel type table exists on either type.
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Source", "xiiMemberPinSource");
  descriptor.AddPass("Sink", "xiiMemberPinSink");
  XII_TEST_RESULT(descriptor.Connect(0, "Color", 1, "Color"));
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
  if (!compiled)
    return;
  xiiRenderPipelineInstance instance;
  XII_TEST_RESULT(instance.Initialize(compiled));
  xiiRenderGraph graph("Reflected member bindings");
  graph.BeginSetup(1);
  xiiRenderGraphBlackboard blackboard;
  XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard));
  graph.EndSetup();
  const auto* binding = instance.GetOutputBinding(0, "Color");
  XII_TEST_BOOL(binding && binding->m_hTexture.IsValid());

  xiiBloomPass                          bloom;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  bloom.GetPins(pins);
  XII_TEST_RESULT(bloom.InitializePinBindings(pins));
  XII_TEST_INT(pins.GetCount(), 4);
  XII_TEST_STRING(pins[bloom.m_InputUpscaledColor.GetBindingIndex()].m_sName, "UpscaledColor");
  XII_TEST_BOOL(pins[bloom.m_OutputBloomTexture.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA16Float);
  const auto* property = bloom.GetDynamicRTTI()->FindPropertyByName("BloomTexture");
  XII_TEST_BOOL(property && property->GetSpecificType() == xiiGetStaticRTTI<xiiRenderPipelineTextureOutputPin>());
  xiiVariantDictionary properties;
  XII_TEST_RESULT(bloom.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("BloomTexture") && !properties.Contains("UpscaledColor") && !properties.Contains("Before"));
  properties.Insert("BloomTexture", xiiVariantDictionary());
  XII_TEST_BOOL(bloom.Configure(properties).Failed()); // Runtime binding indices are never authorable.
  auto inspection = pins;
  bloom.GetPins(inspection); // Appending an editor query must not alter runtime indices.
  XII_TEST_INT(bloom.m_InputUpscaledColor.GetBindingIndex(), 2);
  XII_TEST_INT(bloom.m_OutputBloomTexture.GetBindingIndex(), 3);
  auto changed       = pins;
  changed[2].m_sName = "Stale schema";
  XII_TEST_BOOL(bloom.InitializePinBindings(changed).Failed());
  XII_TEST_INT(bloom.m_InputUpscaledColor.GetBindingIndex(), xiiInvalidIndex);

  xiiTemporalAntiAliasingPass temporal;
  pins.Clear();
  temporal.GetPins(pins);
  XII_TEST_RESULT(temporal.InitializePinBindings(pins));
  const auto& history = pins[temporal.m_OutputPreviousColor.GetBindingIndex()];
  XII_TEST_BOOL(!history.m_bInput && history.m_bTemporalHistoryOutput && history.m_bOptional);
  xiiSensorOutputPass sensor;
  pins.Clear();
  sensor.GetPins(pins);
  XII_TEST_RESULT(sensor.InitializePinBindings(pins));
  XII_TEST_BOOL(!pins[sensor.m_InputSceneDepth.GetBindingIndex()].m_bOptional);
  sensor.m_bUseViewProfile = true;
  pins.Clear();
  sensor.GetPins(pins);
  XII_TEST_RESULT(sensor.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[sensor.m_InputSceneDepth.GetBindingIndex()].m_bOptional);
  xiiSurfaceQueryPass surface;
  surface.m_bOnlyViewWireframe = true;
  pins.Clear();
  surface.GetPins(pins);
  XII_TEST_RESULT(surface.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[surface.m_InputSceneInstances.GetBindingIndex()].m_bOptional);

  // Names, authored identities and deterministic binary data survive the change
  // from procedural schema declarations to member RTTI.
  xiiContiguousMemoryStreamStorage bytes;
  xiiMemoryStreamWriter            writer(&bytes);
  XII_TEST_RESULT(descriptor.Serialize(writer));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor loaded;
  XII_TEST_RESULT(loaded.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> repeated;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(loaded, repeated));
  if (repeated)
  {
    XII_TEST_BOOL(compiled->GetDescriptor().m_Passes[0].m_Id == repeated->GetDescriptor().m_Passes[0].m_Id);
    XII_TEST_BOOL(compiled->GetPins(0)[2].m_Id == repeated->GetPins(0)[2].m_Id);
  }
}

XII_CREATE_SIMPLE_TEST(Pipeline, AmbientOcclusionMemberPins)
{
  xiiAmbientOcclusionPass generation;
  xiiVariantDictionary    properties;
  XII_TEST_RESULT(generation.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("SceneDepth") && !properties.Contains("RayTracingScene"));
  xiiAmbientOcclusionPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_FLOAT(copy.m_fRadius, 2.0f, 0);
  properties.Insert("DirectionCount", 0U);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  properties.Clear();
  properties.Insert("Radius", 0.001f);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  properties.Clear();
  properties.Insert("UseRayTracing", false);
  properties.Insert("AllowScreenSpace", false);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  properties.Insert("AllowScreenSpace", true);
  XII_TEST_RESULT(generation.Configure(properties));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  generation.GetPins(pins);
  XII_TEST_RESULT(generation.InitializePinBindings(pins));
  XII_TEST_INT(generation.m_Scene.GetBindingIndex(), xiiInvalidIndex);
  XII_TEST_INT(generation.m_Materials.GetBindingIndex(), xiiInvalidIndex);
  XII_TEST_INT(generation.m_Geometry.GetBindingIndex(), xiiInvalidIndex);
  XII_TEST_BOOL(pins[generation.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::R8UNormalized);
  generation.m_bUseRayTracing    = true;
  generation.m_bAllowScreenSpace = false;
  pins.Clear();
  generation.GetPins(pins);
  XII_TEST_RESULT(generation.InitializePinBindings(pins));
  XII_TEST_BOOL(!pins[generation.m_Scene.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(generation.GetRequiredDeviceFeatures().IsSet(xiiRenderPipelineDeviceFeature::RayTracing));
  xiiAmbientOcclusionTemporalPass temporal;
  pins.Clear();
  temporal.GetPins(pins);
  XII_TEST_RESULT(temporal.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[temporal.m_Previous.GetBindingIndex()].m_bTemporalHistoryOutput);
  properties.Clear();
  properties.Insert("HistoryWeight", 1.1f);
  XII_TEST_BOOL(temporal.Configure(properties).Failed());

  xiiRenderPipelineResourceDescriptor leaf;
  properties.Clear();
  properties.Insert("UseRayTracing", false);
  leaf.AddPass("Visibility", "xiiAmbientOcclusionPass", properties);
  leaf.AddPass("Filter", "xiiAmbientOcclusionTemporalPass");
  XII_TEST_RESULT(leaf.Connect(0, "RawAO", 1, "RawAO"));
  for (xiiUInt32 index = 0; index < 2; ++index)
  {
    xiiAmbientOcclusionPass         generationSchema;
    xiiAmbientOcclusionTemporalPass temporalSchema;
    xiiRenderPipelinePass*          pass = index == 0 ? static_cast<xiiRenderPipelinePass*>(&generationSchema) : static_cast<xiiRenderPipelinePass*>(&temporalSchema);
    XII_TEST_RESULT(pass->Configure(leaf.m_Passes[index].m_Properties));
    pins.Clear();
    pass->GetPins(pins);
    for (const auto& pin : pins)
      if (pin.m_bInput && pin.m_sName != "RawAO")
        leaf.m_Interface.PushBack({pin.m_sName, true, leaf.m_Passes[index].m_Id, pin.m_sName});
  }
  leaf.m_Interface.PushBack({"StableAO", false, leaf.m_Passes[1].m_Id, "StableAO"});
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First", definition.m_Id);
  root.AddSubgraph("Second", definition.m_Id);
  for (xiiUInt32 index = 0; index < 2; ++index)
    for (const auto& mapping : leaf.m_Interface)
      if (mapping.m_bInput)
      {
        bool exists = false;
        for (const auto& previous : root.m_Interface)
          exists |= previous.m_sName == mapping.m_sName && previous.m_Node == root.m_Passes[index].m_Id;
        if (!exists)
          root.m_Interface.PushBack({mapping.m_sName, true, root.m_Passes[index].m_Id, mapping.m_sName});
      }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (compiled)
  {
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 4);
    XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet() && compiled->GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_BOOL(compiled->GetProvenance()[0].m_InstancePath != compiled->GetProvenance()[2].m_InstancePath);
  }
  xiiContiguousMemoryStreamStorage first, second;
  xiiMemoryStreamWriter            writer(&first), repeatedWriter(&second);
  XII_TEST_RESULT(root.Serialize(writer));
  xiiMemoryStreamReader               reader(&first);
  xiiRenderPipelineResourceDescriptor loaded;
  XII_TEST_RESULT(loaded.Deserialize(reader));
  XII_TEST_RESULT(loaded.Serialize(repeatedWriter));
  XII_TEST_BOOL(xiiMakeArrayPtr(first.GetData(), first.GetStorageSize32()) == xiiMakeArrayPtr(second.GetData(), second.GetStorageSize32()));
}

XII_CREATE_SIMPLE_TEST(Pipeline, AmbientOcclusionAssetMigration)
{
  xiiRenderPipelineResourceDescriptor legacy;
  for (auto stage : {"PrepareFrame", "Visibility", "DepthAndGeometry", "LightingPreparation", "Lighting"})
  {
    xiiVariantDictionary properties;
    properties.Insert("Stage", xiiString(stage));
    legacy.AddPass(stage, "xiiBuiltinRenderPipelinePass", properties);
  }
  XII_TEST_RESULT(legacy.Connect(3, "RawAO", 4, "RawAO"));
  const auto edgeId = legacy.m_Connections[0].m_Id;
  legacy.m_Interface.PushBack({"Raw", false, legacy.m_Passes[3].m_Id, "RawAO"});
  xiiVariantDictionary metadata, configuration;
  metadata.Insert("Name", "RawAO");
  metadata.Insert("Input", false);
  configuration.Insert("Metadata", metadata);
  const auto configurationId = xiiUuid::MakeStableUuidFromString("Authored AO output contract");
  configuration.Insert("Id", configurationId);
  configuration.Insert("DisplayName", "Ambient Visibility");
  xiiVariantArray configurations;
  configurations.PushBack(configuration);
  legacy.m_Passes[3].m_Properties.Insert("PinConfigurations", configurations);
  xiiGraphDefinition nested;
  XII_TEST_RESULT(legacy.MakeGraphDefinition(nested));
  legacy.m_Definitions.PushBack(nested);
  xiiContiguousMemoryStreamStorage original;
  xiiMemoryStreamWriter            writer(&original);
  XII_TEST_RESULT(legacy.Serialize(writer));
  // Version 2 authoring documents predate independently authored AO operations.
  writer.SetWritePosition(0);
  const xiiUInt32 oldVersion = 2;
  XII_TEST_RESULT(writer.WriteDWordValue(&oldVersion));
  xiiMemoryStreamReader               reader(&original);
  xiiRenderPipelineResourceDescriptor migrated;
  XII_TEST_RESULT(migrated.Deserialize(reader));
  XII_TEST_INT(migrated.m_Passes.GetCount(), 6);
  XII_TEST_BOOL(migrated.m_Passes[5].m_Definition.IsValid());
  XII_TEST_BOOL(migrated.m_Connections[0].m_Id == edgeId && migrated.m_Connections[0].m_uiSource == 5);
  XII_TEST_BOOL(migrated.m_Interface[0].m_Node == migrated.m_Passes[5].m_Id);
  XII_TEST_BOOL(migrated.m_Passes[5].m_Properties.GetValue("PinConfigurations") != nullptr);
  XII_TEST_BOOL(migrated.m_Passes[5].m_Properties["PinConfigurations"].Get<xiiVariantArray>()[0].Get<xiiVariantDictionary>().GetValue("Id")->Get<xiiUuid>() == configurationId);
  XII_TEST_RESULT(migrated.m_Definitions[0].Validate());
  XII_TEST_INT(migrated.m_Definitions[0].m_Nodes.GetCount(), 6);
  XII_TEST_BOOL(migrated.m_Definitions[0].m_Boundary[0].m_Node == migrated.m_Definitions[0].m_Nodes[5].m_Id);
  xiiContiguousMemoryStreamStorage first, second;
  xiiMemoryStreamWriter            firstWriter(&first), secondWriter(&second);
  XII_TEST_RESULT(migrated.Serialize(firstWriter));
  xiiMemoryStreamReader               firstReader(&first);
  xiiRenderPipelineResourceDescriptor repeated;
  XII_TEST_RESULT(repeated.Deserialize(firstReader));
  XII_TEST_RESULT(repeated.Serialize(secondWriter));
  XII_TEST_BOOL(xiiMakeArrayPtr(first.GetData(), first.GetStorageSize32()) == xiiMakeArrayPtr(second.GetData(), second.GetStorageSize32()));
  auto malformed                        = legacy;
  malformed.m_Connections[0].m_uiTarget = xiiInvalidIndex;
  xiiContiguousMemoryStreamStorage invalidBytes;
  xiiMemoryStreamWriter            invalidWriter(&invalidBytes);
  XII_TEST_RESULT(malformed.Serialize(invalidWriter));
  invalidWriter.SetWritePosition(0);
  XII_TEST_RESULT(invalidWriter.WriteDWordValue(&oldVersion));
  xiiMemoryStreamReader               invalidReader(&invalidBytes);
  xiiRenderPipelineResourceDescriptor rejected;
  XII_TEST_RESULT(rejected.Deserialize(invalidReader));
  xiiSharedPtr<const xiiRenderPipeline> invalidPipeline;
  XII_TEST_BOOL(xiiRenderPipeline::Compile(rejected, invalidPipeline).Failed());
  const auto                                 resource = xiiLoadPipelineLibraryResource("AmbientOcclusion");
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetPipeline() && loaded->GetAuthoring()))
    return;
  XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 2);
  XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet() && loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
}
