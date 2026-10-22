/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/RayTracing/RayTracingScenePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>

class xiiRayPinContractSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRayPinContractSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineAccelerationStructureOutputPin m_Output{true};
  bool                                            m_bPublishPartial = false;
  bool                                            m_bUseSetter      = false;
  xiiResult                                       Build(xiiRenderPipelinePassContext& context) override
  {
    if (!m_bPublishPartial)
      return XII_SUCCESS;
    const auto                                    emitted = context.m_Graph.AddPass<xiiRenderGraphBufferHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [](auto& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = 4;
      description.m_BindFlags = xiiGALBindFlags::RayTracing;
      data = builder.WriteBuffer("Invalid partial AS", description, xiiGALResourceStateFlags::BuildASWrite); }, [](const auto&, xiiRenderGraphPassContext&) {});
    xiiRenderPipelineAccelerationStructureBinding partial;
    partial.m_Dependency = *emitted.first;
    if (m_bUseSetter)
      return context.SetOutput(m_Output, partial);
    context.m_Bindings[m_Output.GetBindingIndex()].m_AccelerationStructure = partial;
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRayPinContractSource, 1, xiiRTTIDefaultAllocator<xiiRayPinContractSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Scene", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("PublishPartial", m_bPublishPartial),
      XII_MEMBER_PROPERTY("UseSetter", m_bUseSetter),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiRayPinContractConsumer : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRayPinContractConsumer, xiiRenderPipelinePass);

public:
  xiiRenderPipelineAccelerationStructureInputPin m_Input{true};
  xiiRenderPipelineBufferInputPin                m_Buffer{true};
  bool                                           m_bOptional = true;
  xiiResult                                      Build(xiiRenderPipelinePassContext& context) override
  {
    return context.GetInputAccelerationStructure(m_Input).IsEmpty() ? XII_SUCCESS : XII_FAILURE;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRayPinContractConsumer, 1, xiiRTTIDefaultAllocator<xiiRayPinContractConsumer>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Scene", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinOptionalAttribute("Optional")),
      XII_MEMBER_PROPERTY("Buffer", m_Buffer)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Optional", m_bOptional)->AddAttributes(new xiiDefaultValueAttribute(true)),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Pipeline, ReflectedAccelerationStructurePins)
{
  // Existing serialized resource kinds retain their numeric values.
  XII_TEST_INT(xiiRenderPipelinePin::Texture, 0);
  XII_TEST_INT(xiiRenderPipelinePin::Buffer, 1);
  XII_TEST_INT(xiiRenderPipelinePin::Order, 2);
  xiiRayPinContractSource               source;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  source.GetPins(pins);
  XII_TEST_INT(source.m_Output.GetBindingIndex(), xiiInvalidIndex);
  bool discovered = false;
  for (const auto& pin : pins)
    if (pin.m_sName == "Scene")
    {
      discovered = true;
      XII_TEST_BOOL(pin.m_Type == xiiRenderPipelinePin::AccelerationStructure && !pin.m_bInput && pin.m_bOptional);
      xiiGALBufferCreationDescription buffer;
      XII_TEST_BOOL(!pin.Accepts(buffer));
    }
  XII_TEST_BOOL(discovered);
  xiiVariantDictionary properties;
  XII_TEST_RESULT(source.GetProperties(properties));
  XII_TEST_BOOL(properties.GetValue("Scene") == nullptr);
  xiiRenderPipelineResourceDescriptor leaf;
  leaf.AddPass("Producer", xiiGetStaticRTTI<xiiRayPinContractSource>());
  leaf.AddPass("Consumer", xiiGetStaticRTTI<xiiRayPinContractConsumer>());
  XII_TEST_RESULT(leaf.Connect(0, "Scene", 1, "Scene"));
  leaf.m_Interface.PushBack({"Scene", false, leaf.m_Passes[0].m_Id, "Scene"});
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First", definition.m_Id);
  root.AddSubgraph("Second", definition.m_Id);
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  xiiStringBuilder                      error;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled, &error));
  if (!XII_TEST_BOOL(compiled != nullptr))
    return;
  XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 4);
  xiiRenderPipelineInstance instance;
  XII_TEST_RESULT(instance.Initialize(compiled));
  xiiRenderGraph           graph("Absent optional ray scenes");
  xiiRenderGraphBlackboard blackboard;
  graph.BeginSetup(1);
  XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard));
  graph.EndSetup();
  const auto* first  = instance.GetOutputBinding(0, "Scene");
  const auto* second = instance.GetOutputBinding(2, "Scene");
  XII_TEST_BOOL(first && second && first->m_AccelerationStructure.IsEmpty() && second->m_AccelerationStructure.IsEmpty());
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  XII_TEST_RESULT(root.Serialize(writer));
  xiiMemoryStreamReader               reader(&storage);
  xiiRenderPipelineResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> repeated;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, repeated));
  XII_TEST_BOOL(compiled->GetPins(0)[2].m_Id == repeated->GetPins(0)[2].m_Id);

  auto incompatible                      = leaf;
  incompatible.m_Connections[0].m_sInput = "Buffer";
  XII_TEST_BOOL(xiiRenderPipeline::Compile(incompatible, repeated, &error).Failed());
  for (bool setter : {false, true})
  {
    auto partial = leaf;
    partial.m_Passes[0].m_Properties.Insert("PublishPartial", true);
    partial.m_Passes[0].m_Properties.Insert("UseSetter", setter);
    XII_TEST_RESULT(xiiRenderPipeline::Compile(partial, repeated));
    XII_TEST_RESULT(instance.Initialize(repeated));
    xiiRenderGraph invalid("Partial ray scene");
    invalid.BeginSetup(2);
    XII_TEST_BOOL(instance.Build(nullptr, invalid, blackboard).Failed());
    XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("Producer") != nullptr);
    if (!setter)
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("acceleration-structure") != nullptr);
    invalid.EndSetup();
  }
  auto required = leaf;
  required.m_Passes[1].m_Properties.Insert("Optional", false);
  XII_TEST_RESULT(xiiRenderPipeline::Compile(required, repeated));
  XII_TEST_RESULT(instance.Initialize(repeated));
  xiiRenderGraph absent("Missing required ray scene");
  absent.BeginSetup(3);
  XII_TEST_BOOL(instance.Build(nullptr, absent, blackboard).Failed());
  XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("Consumer") != nullptr);
  absent.EndSetup();
  xiiRenderPipelinePinConfiguration* configuration = XII_DEFAULT_NEW(xiiRenderPipelinePinConfiguration);
  configuration->m_Id                              = xiiUuid::MakeUuid();
  configuration->m_Metadata.m_sName                = "Scene";
  configuration->m_Metadata.m_bInput               = false;
  configuration->m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  source.m_PinConfigurations.PushBack(configuration);
  XII_TEST_BOOL(source.GetConfiguredPins(pins, &error).Failed());
  configuration->m_Constraints.m_RequiredBindFlags = {};
  configuration->m_Format                          = xiiGALResourceFormat::RGBA16Float;
  XII_TEST_BOOL(source.GetConfiguredPins(pins, &error).Failed());
  xiiRenderPipelineInterfacePass boundary;
  boundary.m_sName          = "Scene";
  boundary.m_uiResourceType = xiiRenderPipelinePin::AccelerationStructure;
  boundary.GetPins(pins);
  XII_TEST_BOOL(pins.PeekBack().m_Type == xiiRenderPipelinePin::AccelerationStructure);
  xiiRayTracingScenePass authored;
  authored.m_bUseViewSelection          = false;
  authored.m_bRequireHardwareRayTracing = true;
  properties.Clear();
  XII_TEST_RESULT(authored.GetProperties(properties));
  xiiRayTracingScenePass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_BOOL(!copy.m_bUseViewSelection && copy.m_bRequireHardwareRayTracing);
  XII_TEST_BOOL(copy.GetExtractionRequirements().IsSet(xiiRenderExtractionCategory::Geometry));
  XII_TEST_BOOL(!copy.GetExtractionRequirements().IsSet(xiiRenderExtractionCategory::Lights));
}
