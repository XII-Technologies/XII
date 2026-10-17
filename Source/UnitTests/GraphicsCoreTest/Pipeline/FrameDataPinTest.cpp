/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

class xiiTestPipelineFrameValue final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestPipelineFrameValue, xiiRenderPipelineFrameData);

public:
  xiiTestPipelineFrameValue(xiiUInt64 setup, xiiInt32 value) : xiiRenderPipelineFrameData(setup), m_Value(value) { ++s_Live; }
  ~xiiTestPipelineFrameValue() override { --s_Live; }
  const xiiInt32          m_Value;
  inline static xiiUInt32 s_Live = 0;
};
class xiiTestPipelineOtherValue final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestPipelineOtherValue, xiiRenderPipelineFrameData);

public:
  explicit xiiTestPipelineOtherValue(xiiUInt64 setup) : xiiRenderPipelineFrameData(setup) {}
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestPipelineFrameValue, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestPipelineOtherValue, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiTestPipelineDataSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestPipelineDataSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataOutputPin m_Output{xiiGetStaticRTTI<xiiTestPipelineFrameValue>()};
  xiiInt32                       m_Value     = 3;
  bool                           m_WrongType = false, m_Stale = false, m_BypassSetter = false;
  xiiResult                      Build(xiiRenderPipelinePassContext& context) override
  {
    const auto                                     serial = context.m_Graph.GetSetupSerial() - (m_Stale ? 1U : 0U);
    xiiSharedPtr<const xiiRenderPipelineFrameData> value;
    if (m_WrongType)
      value = XII_DEFAULT_NEW(xiiTestPipelineOtherValue, serial);
    else
      value = XII_DEFAULT_NEW(xiiTestPipelineFrameValue, serial, m_Value);
    if (m_BypassSetter)
    {
      context.m_Bindings[m_Output.GetBindingIndex()].m_pFrameData = value;
      return XII_SUCCESS;
    }
    return context.SetOutput(m_Output, std::move(value));
  }
};
class xiiTestPipelineDataSink : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestPipelineDataSink, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin  m_Input{xiiGetStaticRTTI<xiiTestPipelineFrameValue>()};
  xiiRenderPipelineDataOutputPin m_Output{xiiGetStaticRTTI<xiiTestPipelineFrameValue>()};
  xiiInt32                       m_Expected = 3;
  xiiResult                      Build(xiiRenderPipelinePassContext& context) override
  {
    const auto value = context.GetInputData(m_Input);
    if (!value || value.Downcast<const xiiTestPipelineFrameValue>()->m_Value != m_Expected)
      return context.ReportFailure("Unexpected immutable frame value.");
    return context.SetOutput(m_Output, value);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestPipelineDataSource, 1, xiiRTTIDefaultAllocator<xiiTestPipelineDataSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Value", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Number", m_Value),
      XII_MEMBER_PROPERTY("WrongType", m_WrongType),
      XII_MEMBER_PROPERTY("Stale", m_Stale),
      XII_MEMBER_PROPERTY("BypassSetter", m_BypassSetter),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestPipelineDataSink, 1, xiiRTTIDefaultAllocator<xiiTestPipelineDataSink>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Expected", m_Expected),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Pipeline, ImmutableFrameDataComposition)
{
  XII_TEST_INT(xiiRenderPipelinePin::AccelerationStructure, 3);
  XII_TEST_INT(xiiRenderPipelinePin::FrameData, 4);
  xiiRenderPipelineResourceDescriptor leaf;
  leaf.AddPass("Snapshot source", "xiiTestPipelineDataSource");
  leaf.AddPass("Snapshot consumer", "xiiTestPipelineDataSink");
  XII_TEST_RESULT(leaf.Connect(0, "Value", 1, "Input"));
  leaf.m_Interface.PushBack({"Snapshot", false, leaf.m_Passes[1].m_Id, "Output"});
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto instance = root.AddSubgraph(i ? "Second" : "First", definition.m_Id);
    root.m_Interface.PushBack({i ? "Second" : "First", false, root.m_Passes[instance].m_Id, "Snapshot"});
    auto& source = root.m_Passes[instance].m_Overrides.ExpandAndGetRef();
    source.m_Path.PushBack(leaf.m_Passes[0].m_Id);
    source.m_Properties.Insert("Number", xiiInt32(i ? 22 : 11));
    auto& sink = root.m_Passes[instance].m_Overrides.ExpandAndGetRef();
    sink.m_Path.PushBack(leaf.m_Passes[1].m_Id);
    sink.m_Properties.Insert("Expected", xiiInt32(i ? 22 : 11));
  }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  xiiStringBuilder                      error;
  if (!XII_TEST_RESULT_MSG(xiiRenderPipeline::Compile(root, compiled, &error), "%s", error.GetData()))
    return;
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  XII_TEST_RESULT(root.Serialize(writer));
  xiiMemoryStreamReader               reader(&storage);
  xiiRenderPipelineResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> repeated;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, repeated));
  XII_TEST_BOOL(compiled->GetInterface()[0].m_Pin.m_pDataType == xiiGetStaticRTTI<xiiTestPipelineFrameValue>());
  XII_TEST_BOOL(repeated->GetInterface()[0].m_Pin.m_pDataType == compiled->GetInterface()[0].m_Pin.m_pDataType);
  XII_TEST_BOOL(compiled->GetInterface()[0].m_Pin.GetMetadata().m_sDataType == "xiiTestPipelineFrameValue");
  {
    xiiRenderPipelineInstance                      views[2];
    xiiRenderGraph                                 graphs[2];
    xiiRenderGraphBlackboard                       blackboards[2];
    xiiSharedPtr<const xiiRenderPipelineFrameData> held;
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      XII_TEST_RESULT(views[i].Initialize(compiled));
      graphs[i].BeginSetup(1);
      XII_TEST_RESULT(views[i].Build(nullptr, graphs[i], blackboards[i]));
      const auto first  = views[i].GetInterfaceBinding(compiled->FindInterface("First", false))->m_pFrameData;
      const auto second = views[i].GetInterfaceBinding(compiled->FindInterface("Second", false))->m_pFrameData;
      XII_TEST_BOOL(first && second && first != second && first != held);
      XII_TEST_INT(first.Downcast<const xiiTestPipelineFrameValue>()->m_Value, 11);
      XII_TEST_INT(second.Downcast<const xiiTestPipelineFrameValue>()->m_Value, 22);
      if (!held)
        held = first;
      graphs[i].EndSetup();
    }
    views[0].Clear();
    XII_TEST_INT(held.Downcast<const xiiTestPipelineFrameValue>()->m_Value, 11);
    XII_TEST_BOOL(xiiTestPipelineFrameValue::s_Live > 0);
  }
  XII_TEST_INT(xiiTestPipelineFrameValue::s_Live, 0);
}

XII_CREATE_SIMPLE_TEST(Pipeline, FrameDataContractsAndSetupLifetime)
{
  for (bool bypass : {false, true})
    for (const xiiStringView invalid : {"WrongType", "Stale"})
    {
      xiiRenderPipelineResourceDescriptor definition;
      definition.AddPass("Invalid snapshot", "xiiTestPipelineDataSource");
      definition.m_Passes[0].m_Properties.Insert(invalid, true);
      definition.m_Passes[0].m_Properties.Insert("BypassSetter", bypass);
      xiiSharedPtr<const xiiRenderPipeline> compiled;
      XII_TEST_RESULT(xiiRenderPipeline::Compile(definition, compiled));
      xiiRenderPipelineInstance instance;
      XII_TEST_RESULT(instance.Initialize(compiled));
      xiiRenderGraph           graph;
      xiiRenderGraphBlackboard blackboard;
      graph.BeginSetup(1);
      XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard).Failed());
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("Invalid snapshot") != nullptr);
      graph.EndSetup();
    }
  xiiRenderPipelineResourceDescriptor external;
  external.AddPass("External snapshot consumer", "xiiTestPipelineDataSink");
  external.m_Interface.PushBack({"Snapshot", true, external.m_Passes[0].m_Id, "Input"});
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(external, compiled));
  xiiRenderPipelineInstance instance;
  XII_TEST_RESULT(instance.Initialize(compiled));
  xiiRenderGraph           graph;
  xiiRenderGraphBlackboard blackboard;
  graph.BeginSetup(1);
  xiiRenderPipelineBinding binding;
  binding.m_pFrameData = XII_DEFAULT_NEW(xiiTestPipelineFrameValue, graph.GetSetupSerial(), 3);
  const xiiRenderPipelineInputBinding input(graph, compiled->FindInterface("Snapshot", true), binding);
  XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(&input, 1)));
  graph.EndSetup();
  graph.BeginSetup(2);
  // Retagging the external binding cannot make an older snapshot current.
  const xiiRenderPipelineInputBinding retagged(graph, input.m_uiInterface, binding);
  XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard, {}, xiiMakeArrayPtr(&retagged, 1)).Failed());
  XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("Snapshot") != nullptr);
  graph.EndSetup();
  xiiRenderPipelinePin wrong = compiled->GetInterface()[0].m_Pin;
  wrong.m_pDataType          = xiiGetStaticRTTI<xiiTestPipelineOtherValue>();
  XII_TEST_BOOL(!wrong.IsCompatibleWith(compiled->GetInterface()[0].m_Pin));
  xiiRenderPipelinePinConfiguration config;
  config.m_Id                              = xiiUuid::MakeUuid();
  config.m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
  xiiStringBuilder error;
  XII_TEST_BOOL(wrong.Apply(config, &error).Failed());
}
