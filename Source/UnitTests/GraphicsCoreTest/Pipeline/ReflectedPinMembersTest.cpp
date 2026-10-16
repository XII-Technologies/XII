/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorOutputPass.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalAntiAliasingPass.h>
#include <GraphicsCore/Pipeline/Passes/Visualization/SurfaceQueryPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

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
