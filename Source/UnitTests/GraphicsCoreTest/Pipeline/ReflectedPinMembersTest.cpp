/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusion/AmbientOcclusionTemporalPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIEnvironmentSeedPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeGatherPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbeSchedulePass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelEnvironmentSeedPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelGatherPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelSchedulePass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/ClusteredLocalSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/DirectionalSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/EnvironmentLightConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/WorldDirectionalLightPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/DepthSensorPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalAntiAliasingPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
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
  descriptor.AddPass("Source", xiiGetStaticRTTI<xiiMemberPinSource>());
  descriptor.AddPass("Sink", xiiGetStaticRTTI<xiiMemberPinSink>());
  XII_TEST_RESULT(descriptor.Connect(0, "Color", 1, "Color"));
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
  if (!compiled)
    return;
  XII_TEST_INT(compiled->GetPins(0).GetCount(), 1);
  XII_TEST_INT(compiled->GetPins(1).GetCount(), 1);
  XII_TEST_BOOL(!compiled->GetPins(0)[0].m_bInput && compiled->GetPins(1)[0].m_bInput);
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
  XII_TEST_INT(pins.GetCount(), 2);
  XII_TEST_BOOL(bloom.GetDynamicRTTI()->FindPropertyByName("Before") == nullptr);
  XII_TEST_BOOL(bloom.GetDynamicRTTI()->FindPropertyByName("After") == nullptr);
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
  XII_TEST_INT(bloom.m_InputUpscaledColor.GetBindingIndex(), 0);
  XII_TEST_INT(bloom.m_OutputBloomTexture.GetBindingIndex(), 1);
  auto changed       = pins;
  changed[0].m_sName = "Stale schema";
  XII_TEST_BOOL(bloom.InitializePinBindings(changed).Failed());
  XII_TEST_INT(bloom.m_InputUpscaledColor.GetBindingIndex(), xiiInvalidIndex);

  xiiTemporalAntiAliasingPass temporal;
  pins.Clear();
  temporal.GetPins(pins);
  XII_TEST_RESULT(temporal.InitializePinBindings(pins));
  const auto& history = pins[temporal.m_OutputPreviousColor.GetBindingIndex()];
  XII_TEST_BOOL(!history.m_bInput && history.m_bTemporalHistoryOutput && history.m_bOptional);
  xiiDepthSensorPass sensor;
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
    XII_TEST_BOOL(compiled->GetPins(0)[0].m_Id == repeated->GetPins(0)[0].m_Id);
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
  properties.Clear();
  properties.Insert("UseHistory", false);
  properties.Insert("HistoryWeight", 0.92f);
  XII_TEST_RESULT(temporal.Configure(properties));
  pins.Clear();
  temporal.GetPins(pins);
  XII_TEST_RESULT(temporal.InitializePinBindings(pins));
  XII_TEST_INT(temporal.m_Velocity.GetBindingIndex(), xiiInvalidIndex);
  XII_TEST_BOOL(pins[temporal.m_Raw.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[temporal.m_Output.GetBindingIndex()].m_bOptional);
  properties.Clear();
  properties.Insert("Enabled", false);
  XII_TEST_RESULT(generation.Configure(properties));
  XII_TEST_BOOL(generation.GetRequiredDeviceFeatures().IsNoFlagSet());

  xiiRenderPipelineResourceDescriptor leaf;
  properties.Clear();
  properties.Insert("UseRayTracing", false);
  leaf.AddPass("Visibility", xiiGetStaticRTTI<xiiAmbientOcclusionPass>(), properties);
  leaf.AddPass("Filter", xiiGetStaticRTTI<xiiAmbientOcclusionTemporalPass>());
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

XII_CREATE_SIMPLE_TEST(Pipeline, AuthoredAmbientOcclusion)
{
  const auto                                 resource = xiiCreateAmbientOcclusionTestResource();
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetPipeline() && loaded->GetAuthoring()))
    return;
  XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 2);
  XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet() && loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
}

XII_CREATE_SIMPLE_TEST(Pipeline, IrradianceGatherMemberPins)
{
  xiiDDGIProbeGatherPass   probe;
  xiiSparseVoxelGatherPass sparse;
  xiiVariantDictionary     properties;
  properties.Insert("Intensity", 2.0f);
  XII_TEST_RESULT(probe.Configure(properties));
  properties.Clear();
  XII_TEST_RESULT(probe.GetProperties(properties));
  XII_TEST_FLOAT(properties["Intensity"].Get<float>(), 2, 0);
  XII_TEST_BOOL(!properties.Contains("EnvironmentLight") && !properties.Contains("DDGIProbeStates"));
  xiiDDGIProbeGatherPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  properties.Insert("Intensity", -0.01f);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  XII_TEST_BOOL(sparse.Configure(properties).Failed());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  probe.GetPins(pins);
  XII_TEST_RESULT(probe.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[probe.m_Environment.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UniformBuffer));
  XII_TEST_BOOL(pins[probe.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA16Float);
  pins.Clear();
  sparse.GetPins(pins);
  XII_TEST_RESULT(sparse.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[sparse.m_PageTable.GetBindingIndex()].m_Type == xiiRenderPipelinePin::Buffer);
  XII_TEST_BOOL(sparse.GetExtractionRequirements().IsNoFlagSet() && sparse.GetRuntimeFeatures().IsNoFlagSet());
}


class xiiMemberPinForward : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMemberPinForward, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::RGBA16Float, false, xiiGALBindFlags::ShaderResource, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override { return context.SetOutput(m_Output, context.GetInputTexture(m_Input)); }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMemberPinForward, 1, xiiRTTIDefaultAllocator<xiiMemberPinForward>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Pipeline, CompiledRootBoundaryBindings)
{
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Forward", xiiGetStaticRTTI<xiiMemberPinForward>());
  descriptor.AddPass("Sink", xiiGetStaticRTTI<xiiMemberPinSink>());
  descriptor.m_Interface.PushBack({"Color", true, descriptor.m_Passes[0].m_Id, "Input"});
  descriptor.m_Interface.PushBack({"Color", true, descriptor.m_Passes[1].m_Id, "Color"});
  descriptor.m_Interface.PushBack({"Result", false, descriptor.m_Passes[0].m_Id, "Output"});
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  if (!XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled)))
    return;
  const auto inputIndex = compiled->FindInterface("Color", true), outputIndex = compiled->FindInterface("Result", false);
  XII_TEST_BOOL(inputIndex != xiiInvalidIndex && outputIndex != xiiInvalidIndex);
  XII_TEST_INT(compiled->GetInterface()[inputIndex].m_Bindings.GetCount(), 2);
  XII_TEST_INT(compiled->GetInterface()[outputIndex].m_Bindings.GetCount(), 1);
  XII_TEST_BOOL(!compiled->GetInterface()[inputIndex].m_Pin.m_bOptional);
  xiiRenderPipelineInstance instance;
  XII_TEST_RESULT(instance.Initialize(compiled));
  xiiRenderGraph           graph("Explicit resource input");
  xiiRenderGraphBlackboard blackboard;
  const auto               emit = [&](xiiGALResourceFormat::Enum format = xiiGALResourceFormat::RGBA16Float) {
    const auto pass = graph.AddPass<xiiRenderGraphTextureHandle>("Caller image", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription description; description.m_Type = xiiGALResourceDimension::Texture2D; description.m_Size = {8, 8};
      description.m_Format = format; description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data = builder.WriteTexture("Caller color", description, xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {});
    return *pass.first;
  };
  graph.BeginSetup(7);
  XII_TEST_RESULT(instance.BeginFrame(nullptr, graph, blackboard));
  XII_TEST_RESULT(instance.BeginFrame(nullptr, graph, blackboard));
  const auto                          image = emit();
  const xiiRenderPipelineInputBinding binding(graph, inputIndex, image);
  XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(&binding, 1)));
  XII_TEST_BOOL(instance.GetInterfaceBinding(outputIndex)->m_hTexture == image);
  XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(&binding, 1)).Failed());
  graph.EndSetup();
  // The same frame number and allocation names do not authorize reuse of bindings from an old setup.
  graph.BeginSetup(7);
  emit();
  XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(&binding, 1)).Failed());
  XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("another setup") != nullptr);
  graph.EndSetup();
  for (xiiUInt32 failure = 0; failure < 5; ++failure)
  {
    graph.BeginSetup(7);
    const auto                          current = emit(failure == 3 ? xiiGALResourceFormat::RGBA8UNormalized : xiiGALResourceFormat::RGBA16Float);
    const xiiRenderPipelineInputBinding input(graph, failure == 2 ? outputIndex : failure == 4 ? xiiInvalidIndex :
                                                                                                 inputIndex,
                                              current);
    const xiiRenderPipelineInputBinding duplicate[2] = {input, input};
    const auto                          inputs       = failure == 0 ? xiiArrayPtr<const xiiRenderPipelineInputBinding>{} : failure == 1 ? xiiMakeArrayPtr(duplicate) :
                                                                                                                                          xiiMakeArrayPtr(&input, 1);
    XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard, {}, inputs).Failed());
    XII_TEST_BOOL(!instance.GetLastBuildError().IsEmpty());
    graph.EndSetup();
  }
  graph.BeginSetup(7);
  XII_TEST_RESULT(instance.BeginFrame(nullptr, graph, blackboard));
  xiiVariantDictionary changed;
  changed.Insert("Changed", true);
  XII_TEST_BOOL(instance.BeginFrame(nullptr, graph, blackboard, changed).Failed());
  graph.EndSetup();
  graph.BeginSetup(7);
  const xiiRenderPipelineInputBinding current(graph, inputIndex, emit());
  XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard, changed, xiiMakeArrayPtr(&current, 1)));
  graph.EndSetup();
  xiiDefaultMemoryStreamStorage bytes;
  xiiMemoryStreamWriter         writer(&bytes);
  XII_TEST_RESULT(descriptor.Serialize(writer));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> recompiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, recompiled));
  XII_TEST_INT(recompiled->GetInterface().GetCount(), compiled->GetInterface().GetCount());
  XII_TEST_INT(recompiled->GetInterface()[recompiled->FindInterface("Color", true)].m_Bindings.GetCount(), 2);
}

XII_CREATE_SIMPLE_TEST(Pipeline, EnvironmentLightMemberPins)
{
  xiiEnvironmentLightConstantsPass environment;
  xiiVariantDictionary             properties;
  properties.Insert("Illuminance", 125.0f);
  properties.Insert("AmbientRadiance", xiiVec3(0.25f, 0.5f, 1.0f));
  XII_TEST_RESULT(environment.Configure(properties));
  properties.Clear();
  XII_TEST_RESULT(environment.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("DirectionalLight") && !properties.Contains("EnvironmentLight"));
  xiiEnvironmentLightConstantsPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_FLOAT(copy.m_fIlluminance, 125, 0);
  XII_TEST_BOOL(copy.m_Ambient == xiiVec3(0.25f, 0.5f, 1));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  copy.GetPins(pins);
  XII_TEST_RESULT(copy.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[copy.m_Directional.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[copy.m_Directional.GetBindingIndex()].m_pDataType == xiiGetStaticRTTI<xiiDirectionalLightFrameData>());
  XII_TEST_BOOL(pins[copy.m_Constants.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UniformBuffer));
  xiiWorldDirectionalLightPass source;
  XII_TEST_BOOL(source.GetRuntimeFeatures().IsNoFlagSet() && source.GetExtractionRequirements() == xiiRenderExtractionCategory::Lights);
  for (const xiiStringView property : {"Illuminance", "AngularRadius"})
  {
    xiiEnvironmentLightConstantsPass rejected;
    xiiVariantDictionary             invalid;
    invalid.Insert(property, -1.0f);
    XII_TEST_BOOL(rejected.Configure(invalid).Failed());
  }
  xiiEnvironmentLightConstantsPass rejectedDirection;
  xiiVariantDictionary             invalid;
  invalid.Insert("Direction", xiiVec3::MakeZero());
  XII_TEST_BOOL(rejectedDirection.Configure(invalid).Failed());
}

XII_CREATE_SIMPLE_TEST(Pipeline, DDGIIndependentOperationSchemas)
{
  xiiDDGIProbeSchedulePass schedule;
  xiiVariantDictionary     properties;
  properties.Insert("ProbeCountX", 4U);
  properties.Insert("ProbeCountY", 2U);
  properties.Insert("ProbeCountZ", 4U);
  properties.Insert("UpdateBudget", 8U);
  properties.Insert("UseViewCamera", false);
  properties.Insert("Hysteresis", 0.5f);
  XII_TEST_RESULT(schedule.Configure(properties));
  properties.Clear();
  XII_TEST_RESULT(schedule.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("Schedule") && !properties.Contains("PreviousIrradiance"));
  xiiDDGIProbeSchedulePass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_CountX, 4);
  XII_TEST_FLOAT(copy.m_fHysteresis, 0.5f, 0);
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  copy.GetPins(pins);
  XII_TEST_RESULT(copy.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[copy.m_PreviousIrradiance.GetBindingIndex()].m_bOptional && pins[copy.m_PreviousDistance.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[copy.m_Schedule.GetBindingIndex()].m_pDataType == xiiGetStaticRTTI<xiiDDGIProbeFrameData>());
  XII_TEST_BOOL(copy.GetExtractionRequirements().IsNoFlagSet() && copy.GetRuntimeFeatures().IsNoFlagSet());
  xiiDDGIProbeSchedulePass rejected;
  xiiVariantDictionary     invalid;
  invalid.Insert("ProbeCountX", 0U);
  XII_TEST_BOOL(rejected.Configure(invalid).Failed());
  invalid.Clear();
  invalid.Insert("ProbeCountX", 256U);
  invalid.Insert("ProbeCountY", 256U);
  invalid.Insert("ProbeCountZ", 256U);
  XII_TEST_BOOL(rejected.Configure(invalid).Failed());
  invalid.Clear();
  invalid.Insert("Hysteresis", 1.0f);
  xiiDDGIProbeSchedulePass invalidHysteresis;
  XII_TEST_BOOL(invalidHysteresis.Configure(invalid).Failed());
  xiiDDGIEnvironmentSeedPass seed;
  pins.Clear();
  seed.GetPins(pins);
  XII_TEST_RESULT(seed.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[seed.m_StatesInput.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UnorderedAccess));
  XII_TEST_BOOL(seed.GetRequiredDeviceFeatures().IsSet(xiiRenderPipelineDeviceFeature::Compute) && seed.GetRuntimeFeatures().IsNoFlagSet());
}

XII_CREATE_SIMPLE_TEST(Pipeline, SparseVoxelIndependentSchemasAndBudgets)
{
  xiiSparseVoxelSchedulePass schedule;
  xiiVariantDictionary       values;
  values.Insert("ClipmapLevels", 2U);
  values.Insert("BrickGridResolution", 2U);
  values.Insert("BrickVoxelResolution", 2U);
  values.Insert("ResidentBricks", 16U);
  values.Insert("UpdateBudget", 4U);
  values.Insert("UseViewCamera", false);
  XII_TEST_RESULT(schedule.Configure(values));
  values.Clear();
  XII_TEST_RESULT(schedule.GetProperties(values));
  XII_TEST_BOOL(!values.Contains("Schedule") && !values.Contains("PreviousPool"));
  xiiSparseVoxelSchedulePass copy;
  XII_TEST_RESULT(copy.Configure(values));
  XII_TEST_INT(copy.m_Resident, 16);
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  copy.GetPins(pins);
  XII_TEST_RESULT(copy.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[copy.m_PreviousPool.GetBindingIndex()].m_bOptional && pins[copy.m_Schedule.GetBindingIndex()].m_pDataType == xiiGetStaticRTTI<xiiSparseVoxelFrameData>());
  XII_TEST_BOOL(copy.GetRuntimeFeatures().IsNoFlagSet() && copy.GetExtractionRequirements().IsNoFlagSet());
  xiiVariantDictionary invalid;
  invalid.Insert("MaximumPoolBytes", 16U);
  xiiSparseVoxelSchedulePass rejected;
  XII_TEST_BOOL(rejected.Configure(invalid).Failed());
  invalid.Clear();
  invalid.Insert("ResidentBricks", 1U);
  xiiSparseVoxelSchedulePass insufficient;
  XII_TEST_BOOL(insufficient.Configure(invalid).Failed());
  invalid.Clear();
  invalid.Insert("Hysteresis", 1.0f);
  xiiSparseVoxelSchedulePass badWeight;
  XII_TEST_BOOL(badWeight.Configure(invalid).Failed());
  xiiSparseVoxelEnvironmentSeedPass seed;
  pins.Clear();
  seed.GetPins(pins);
  XII_TEST_RESULT(seed.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[seed.m_InitialPages.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UnorderedAccess));
}

XII_CREATE_SIMPLE_TEST(Pipeline, IndependentSurfaceLightingMemberSchemas)
{
  xiiDirectionalSurfaceLightingPass sun;
  xiiVariantDictionary              properties;
  properties.Insert("Scale", 2.0f);
  XII_TEST_RESULT(sun.Configure(properties));
  properties.Clear();
  XII_TEST_RESULT(sun.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("EnvironmentLight") && !properties.Contains("Camera"));
  xiiDirectionalSurfaceLightingPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_FLOAT(copy.m_fScale, 2, 0);
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  copy.GetPins(pins);
  XII_TEST_RESULT(copy.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[copy.m_Visibility.GetBindingIndex()].m_bOptional && pins[copy.m_Environment.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UniformBuffer));
  xiiClusteredLocalSurfaceLightingPass local;
  pins.Clear();
  local.GetPins(pins);
  XII_TEST_RESULT(local.InitializePinBindings(pins));
  XII_TEST_BOOL(!pins[local.m_Candidates.GetBindingIndex()].m_bOptional && !pins[local.m_Overflow.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[local.m_Layout.GetBindingIndex()].m_pDataType == xiiGetStaticRTTI<xiiLightClusterFrameData>());
  XII_TEST_BOOL(local.GetExtractionRequirements().IsNoFlagSet() && local.GetRuntimeFeatures().IsNoFlagSet() && sun.GetExtractionRequirements().IsNoFlagSet());
  xiiVariantDictionary invalid;
  invalid.Insert("Scale", -1.0f);
  XII_TEST_BOOL(copy.Configure(invalid).Failed());
  XII_TEST_BOOL(local.Configure(invalid).Failed());
}
