/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>

#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Lighting/DisplayOutput.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineConfiguration.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <type_traits>

namespace PipelineResourceTest
{
  class MemoryLoader : public xiiResourceLoaderFromMemory
  {
  public:
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> m_pClosed;
    xiiRenderPipelineResourceHandle                       m_Target;
    xiiUniquePtr<xiiResourceTypeLoader>                   m_Next;
    xiiUniquePtr<xiiResourceTypeLoader>                   m_Latest;

    void CloseDataStream(const xiiResource* resource, const xiiResourceLoadData& data) override
    {
      xiiResourceLoaderFromMemory::CloseDataStream(resource, data);
      // Submit from the previous loader's closing phase to deterministically exercise
      // the loaded-but-still-busy window, without sleeps or scheduler timing assumptions.
      if (m_Next)
        xiiResourceManager::UpdateResourceWithCustomLoader(m_Target, std::move(m_Next));
      if (m_Latest)
        xiiResourceManager::UpdateResourceWithCustomLoader(m_Target, std::move(m_Latest));
      m_Target.Invalidate();
      if (m_pClosed)
        m_pClosed->m_Content.RaiseSignal();
    }
  };

  struct PassData
  {
    xiiRenderGraphBufferHandle  m_hBuffer;
    xiiRenderGraphTextureHandle m_hTexture;
  };
} // namespace PipelineResourceTest

class xiiPipelineTestSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineTestSource, xiiRenderPipelinePass);

public:
  xiiUInt32 m_uiValue               = 1;
  xiiUInt32 m_uiBuildCount          = 0;
  xiiUInt32 m_uiHistoryResets       = 0;
  bool      m_bEmitTexture          = false;
  bool      m_bUnknownTextureFormat = false;
  bool      m_bOmitBuffer           = false;
  void      InvalidateTemporalHistory() override { ++m_uiHistoryResets; }
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Data", xiiRenderPipelinePin::Buffer, false});
    pins.PushBack({"Texture", xiiRenderPipelinePin::Texture, false, true, m_bUnknownTextureFormat ? xiiGALResourceFormat::Unknown : xiiGALResourceFormat::RGBA8UNormalized});
    pins.PushBack({"Before", xiiRenderPipelinePin::Order, true, true});
    pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    ++m_uiBuildCount;
    auto emitted = context.m_Graph.AddPass<PipelineResourceTest::PassData>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [this](PipelineResourceTest::PassData& data, xiiRenderGraphBuilder& builder) {
        xiiGALBufferCreationDescription description;
        description.m_uiSize = m_uiValue * 16;
        description.m_BindFlags = xiiGALBindFlags::UnorderedAccess;
        data.m_hBuffer = builder.WriteBuffer("TestProducedBuffer", description, xiiGALResourceStateFlags::UnorderedAccess);
        if (m_bEmitTexture)
        {
          xiiGALTextureCreationDescription texture;
          texture.m_Type = xiiGALResourceDimension::Texture2D;
          texture.m_Size = xiiSizeU32(4, 4);
          texture.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
          texture.m_BindFlags = xiiGALBindFlags::UnorderedAccess;
          data.m_hTexture = builder.WriteTexture("TestProducedTexture", texture, xiiGALResourceStateFlags::UnorderedAccess);
        } }, [](const PipelineResourceTest::PassData&, xiiRenderGraphPassContext&) {});
    if (m_bEmitTexture)
      XII_SUCCEED_OR_RETURN(context.SetOutput("Texture", emitted.first->m_hTexture));
    return m_bOmitBuffer ? XII_SUCCESS : context.SetOutput("Data", emitted.first->m_hBuffer);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineTestSource, 1, xiiRTTIDefaultAllocator<xiiPipelineTestSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Value", m_uiValue),
      XII_MEMBER_PROPERTY("EmitTexture", m_bEmitTexture),
      XII_MEMBER_PROPERTY("UnknownTextureFormat", m_bUnknownTextureFormat),
      XII_MEMBER_PROPERTY("OmitBuffer", m_bOmitBuffer),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiPipelineTestSink : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineTestSink, xiiRenderPipelinePass);

public:
  xiiUInt32 m_uiBuildCount = 0;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Input", xiiRenderPipelinePin::Buffer, true});
    pins.PushBack({"Texture", xiiRenderPipelinePin::Texture, true, true, xiiGALResourceFormat::RGBA16Float});
    pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    ++m_uiBuildCount;
    const auto input = context.GetInputBuffer("Input");
    if (!input.IsValid())
      return XII_FAILURE;
    context.m_Graph.AddPass<PipelineResourceTest::PassData>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [input](PipelineResourceTest::PassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hBuffer = builder.ReadBuffer(input, xiiGALResourceStateFlags::UnorderedAccess);
        builder.SetPassSideEffects(true); }, [](const PipelineResourceTest::PassData&, xiiRenderGraphPassContext&) {});
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineTestSink, 1, xiiRTTIDefaultAllocator<xiiPipelineTestSink>)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace PipelineResourceTest
{
  xiiRenderPipelineResourceDescriptor MakeDefinition()
  {
    xiiRenderPipelineResourceDescriptor descriptor;
    // Insert consumer first to prove compilation follows dependencies rather than asset order.
    descriptor.AddPass("Consumer", xiiGetStaticRTTI<xiiPipelineTestSink>());
    xiiVariantDictionary properties;
    properties.Insert("Value", 4U);
    descriptor.AddPass("Producer", xiiGetStaticRTTI<xiiPipelineTestSource>(), properties);
    descriptor.Connect(1, "Data", 0, "Input").IgnoreResult();
    descriptor.m_Parameters.Insert("Exposure", 1.0f);
    return descriptor;
  }
} // namespace PipelineResourceTest

static_assert(!std::is_copy_constructible_v<xiiRenderPipelineInstance>);

class xiiPipelineTestHistory : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineTestHistory, xiiRenderPipelinePass);

public:
  bool      m_bTemporal      = true;
  xiiUInt32 m_uiHistoryCalls = 0;
  xiiUInt32 m_uiBuildCalls   = 0;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Previous", xiiRenderPipelinePin::Buffer, true, true});
    xiiRenderPipelinePin history{"History", xiiRenderPipelinePin::Buffer, false, true};
    history.m_bTemporalHistoryOutput = m_bTemporal;
    pins.PushBack(history);
  }
  xiiResult BuildTemporalHistory(xiiRenderPipelinePassContext& context) override
  {
    ++m_uiHistoryCalls;
    return context.m_bTemporalHistoryPhase ? XII_SUCCESS : XII_FAILURE;
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    ++m_uiBuildCalls;
    return !context.m_bTemporalHistoryPhase ? XII_SUCCESS : XII_FAILURE;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineTestHistory, 1, xiiRTTIDefaultAllocator<xiiPipelineTestHistory>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Temporal", m_bTemporal),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Pipeline, TemporalDependencyPolicy)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Unused interface fan-out still requires a consistent native resource schema")
  {
    auto invalid = PipelineResourceTest::MakeDefinition();
    invalid.m_Interface.PushBack({"Unused", true, invalid.m_Passes[0].m_Id, "Texture"});
    invalid.m_Interface.PushBack({"Unused", true, invalid.m_Passes[1].m_Id, "Before"});
    xiiSharedPtr<const xiiRenderPipeline> rejected;
    xiiStringBuilder                      diagnostic;
    XII_TEST_BOOL(xiiRenderPipeline::Compile(invalid, rejected, &diagnostic).Failed());
    XII_TEST_BOOL(diagnostic.FindSubString("fan-out") != nullptr);
  }
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Feedback", xiiGetStaticRTTI<xiiPipelineTestHistory>());
  XII_TEST_RESULT(descriptor.Connect(0, "History", 0, "Previous"));
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
  XII_TEST_INT(compiled->GetTopology().GetEdgeCount(), 0);
  XII_TEST_INT(compiled->GetDescriptor().m_Connections.GetCount(), 1);
  xiiRenderPipelineInstance instance;
  XII_TEST_RESULT(instance.Initialize(compiled));
  xiiRenderGraph           graph;
  xiiRenderGraphBlackboard blackboard;
  graph.BeginSetup(1);
  XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard));
  graph.EndSetup();
  const auto* pass = static_cast<const xiiPipelineTestHistory*>(instance.GetPass(0));
  XII_TEST_INT(pass->m_uiHistoryCalls, 1);
  XII_TEST_INT(pass->m_uiBuildCalls, 1);
  descriptor.m_Passes[0].m_Properties.Insert("Temporal", false);
  xiiStringBuilder error;
  XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("intra-frame") != nullptr);
}

XII_CREATE_SIMPLE_TEST(Pipeline, IncrementalRenderGraphCompilation)
{
  struct Data
  {
    xiiUInt32                  m_uiFrame;
    xiiRenderGraphBufferHandle m_Buffer;
  };
  xiiRenderGraph graph;
  const auto     setup = [&](xiiUInt32 frame, xiiUInt32 size, xiiStringView name, bool sideEffects) {
    graph.BeginSetup(frame);
    const auto pass = graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Transfer, [=](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_uiFrame = frame;
      xiiGALBufferCreationDescription description;
      description.m_uiSize = size;
      description.m_BindFlags = xiiGALBindFlags::VertexBuffer;
      data.m_Buffer = builder.WriteBuffer("Data", description, xiiGALResourceStateFlags::CopyDestination); }, [](const Data&, xiiRenderGraphPassContext&) {}, sideEffects);
    graph.EndSetup();
    return pass.first->m_Buffer;
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Topology reuse keeps current callbacks and resource descriptions")
  {
    setup(1, 16, "First label", true);
    XII_TEST_RESULT(graph.Compile());
    XII_TEST_BOOL(!graph.GetStatistics().m_bUsedCachedCompile);
    const auto buffer = setup(2, 32, "Presentation changed", true);
    XII_TEST_RESULT(graph.Compile());
    XII_TEST_BOOL(graph.GetStatistics().m_bUsedCachedCompile);
    XII_TEST_INT(graph.GetBufferDescription(buffer)->m_uiSize, 32);
    XII_TEST_INT(static_cast<Data*>(graph.GetCompiledPasses()[0].m_pPassData)->m_uiFrame, 2);
    setup(3, 32, "Presentation changed", false);
    XII_TEST_RESULT(graph.Compile());
    XII_TEST_BOOL(!graph.GetStatistics().m_bUsedCachedCompile);
    XII_TEST_INT(graph.GetStatistics().m_uiCompiledPassCount, 0);
    setup(4, 32, "Presentation changed", false);
    xiiRenderGraphCompileSettings settings;
    settings.m_bEnablePassCulling = false;
    XII_TEST_RESULT(graph.Compile(settings));
    XII_TEST_BOOL(!graph.GetStatistics().m_bUsedCachedCompile);
    XII_TEST_INT(graph.GetStatistics().m_uiCompiledPassCount, 1);
    setup(5, 32, "Presentation changed", false);
    XII_TEST_RESULT(graph.Compile(settings));
    XII_TEST_BOOL(graph.GetStatistics().m_bUsedCachedCompile);
    settings.m_uiCacheSalt = 7;
    setup(6, 32, "Presentation changed", false);
    XII_TEST_RESULT(graph.Compile(settings));
    XII_TEST_BOOL(!graph.GetStatistics().m_bUsedCachedCompile);
  }
}

XII_CREATE_SIMPLE_TEST(Pipeline, HundredsOfReflectedPasses)
{
  auto               shared = PipelineResourceTest::MakeDefinition();
  xiiGraphDefinition definition;
  XII_TEST_RESULT(shared.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  for (xiiUInt32 i = 0; i < 100; ++i)
  {
    xiiStringBuilder name;
    name.SetFormat("Instance {}", i);
    root.AddSubgraph(name, shared.m_Id);
  }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  XII_TEST_INT(compiled->GetTopology().GetVertexCount(), 200);
  xiiRenderPipelineInstance instance;
  XII_TEST_RESULT(instance.Initialize(compiled));
  xiiRenderGraph           graph;
  xiiRenderGraphBlackboard blackboard;
  graph.BeginSetup(1);
  XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard));
  graph.EndSetup();
  XII_TEST_RESULT(graph.Compile());
  XII_TEST_INT(graph.GetStatistics().m_uiCompiledPassCount, 200);
  XII_TEST_INT(graph.GetResourceDescriptions().GetCount(), 100);
  XII_TEST_INT(graph.GetQueueSubmissions().GetCount(), 1);
}

XII_CREATE_SIMPLE_TEST(Pipeline, HierarchicalRenderPipeline)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Independent asset definitions, selective dependencies and child hot reload")
  {
    auto                  leaf          = PipelineResourceTest::MakeDefinition();
    const auto            child         = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("HierarchyChild");
    auto                  initialLoader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter initialWriter(&initialLoader->m_CustomData);
    initialWriter << "HierarchyChild";
    XII_TEST_RESULT(leaf.Serialize(initialWriter));
    xiiResourceManager::UpdateResourceWithCustomLoader(child, std::move(initialLoader));
    xiiRenderPipelineResourceDescriptor root;
    root.m_References.PushBack({leaf.m_Id, "HierarchyChild"});
    root.m_References.PushBack({xiiUuid::MakeUuid(), "UnusedDefinitionMustNotLoad"});
    root.AddSubgraph("First", leaf.m_Id);
    root.AddSubgraph("Second", leaf.m_Id);
    const auto                                                       parent = xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>("HierarchyParent", xiiRenderPipelineResourceDescriptor(root));
    xiiRenderPipelineResourceDescriptor                              resolved;
    xiiDynamicArray<xiiRenderPipelineResource::DefinitionDependency> dependencies;
    XII_TEST_RESULT(xiiRenderPipelineResource::ResolveDefinitions(root, resolved, &dependencies));
    XII_TEST_INT(dependencies.GetCount(), 1);
    xiiRenderPipelineInstance instance;
    XII_TEST_RESULT(instance.Synchronize(parent));
    const auto*                                previous = instance.GetPipeline();
    xiiResourceLock<xiiRenderPipelineResource> parentLock(parent, xiiResourceAcquireMode::BlockTillLoaded);
    const auto                                 snapshot = parentLock->GetPipeline();
    XII_TEST_BOOL(snapshot.Borrow() == previous);
    leaf.m_Passes[1].m_Properties.Insert("Value", 12U);
    auto                                                  loader = XII_DEFAULT_NEW(PipelineResourceTest::MemoryLoader);
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> closed = XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>);
    loader->m_pClosed                                            = closed;
    xiiMemoryStreamWriter writer(&loader->m_CustomData);
    writer << "HierarchyChild";
    XII_TEST_RESULT(leaf.Serialize(writer));
    xiiResourceManager::UpdateResourceWithCustomLoader(child, std::move(loader));
    if (!XII_TEST_BOOL(closed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(5)) == xiiThreadSignal::WaitResult::Signaled))
      return;
    bool changed = false;
    XII_TEST_RESULT(instance.Synchronize(parent, &changed));
    XII_TEST_BOOL(changed && instance.GetPipeline() != previous);
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(instance.GetPass(1))->m_uiValue, 12);
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(instance.GetPass(3))->m_uiValue, 12);
    XII_TEST_INT(snapshot->GetDescriptor().m_Passes[1].m_Properties.GetValue("Value")->Get<xiiUInt32>(), 4);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nested repeated definitions, parameters, resource namespaces and independent history")
  {
    auto leaf = PipelineResourceTest::MakeDefinition();
    leaf.m_Interface.PushBack({"Output", false, leaf.m_Passes[1].m_Id, "Data"});
    xiiGraphDefinition leafGraph;
    XII_TEST_RESULT(leaf.MakeGraphDefinition(leafGraph));
    xiiRenderPipelineResourceDescriptor nested;
    nested.AddSubgraph("Inner", leaf.m_Id);
    nested.m_Interface.PushBack({"Output", false, nested.m_Passes[0].m_Id, "Output"});
    xiiGraphDefinition nestedGraph;
    XII_TEST_RESULT(nested.MakeGraphDefinition(nestedGraph));
    xiiRenderPipelineResourceDescriptor root;
    root.m_Definitions.PushBack(leafGraph);
    root.m_Definitions.PushBack(nestedGraph);
    root.AddSubgraph("A", nested.m_Id);
    root.AddSubgraph("B", nested.m_Id);
    root.AddPass("Sink A", xiiGetStaticRTTI<xiiPipelineTestSink>());
    root.AddPass("Sink B", xiiGetStaticRTTI<xiiPipelineTestSink>());
    XII_TEST_RESULT(root.Connect(0, "Output", 2, "Input"));
    XII_TEST_RESULT(root.Connect(1, "Output", 3, "Input"));
    root.m_Passes[1].m_Parameters.Insert("Exposure", 2.0f);
    auto& override = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
    override.m_Path.PushBack(nested.m_Passes[0].m_Id);
    override.m_Path.PushBack(leaf.m_Passes[1].m_Id);
    override.m_Properties.Insert("Value", 9U);
    xiiSharedPtr<const xiiRenderPipeline> compiled, again;
    xiiStringBuilder                      error;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled, &error));
    XII_TEST_RESULT(xiiRenderPipeline::Compile(root, again));
    XII_TEST_INT(compiled->GetTopology().GetVertexCount(), 6);
    XII_TEST_BOOL(compiled->GetExecutionOrder() == again->GetExecutionOrder());
    XII_TEST_INT(compiled->GetProvenance()[1].m_InstancePath.GetCount(), 2);
    XII_TEST_BOOL(compiled->GetProvenance()[1].m_InstancePath != compiled->GetProvenance()[3].m_InstancePath);
    XII_TEST_FLOAT(compiled->GetPassParameters(3).GetValue("Exposure")->Get<float>(), 2.0f, 0);
    xiiRenderPipelineInstance a, b;
    XII_TEST_RESULT(a.Initialize(compiled));
    XII_TEST_RESULT(b.Initialize(compiled));
    XII_TEST_BOOL(a.GetPass(1) != a.GetPass(3) && a.GetPass(1) != b.GetPass(1));
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(a.GetPass(1))->m_uiValue, 4);
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(a.GetPass(3))->m_uiValue, 9);
    a.InvalidateTemporalHistory();
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(a.GetPass(1))->m_uiHistoryResets, 1);
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(b.GetPass(1))->m_uiHistoryResets, 0);
    xiiRenderGraph           graph;
    xiiRenderGraphBlackboard blackboard;
    graph.BeginSetup(1);
    XII_TEST_RESULT(a.Build(nullptr, graph, blackboard));
    XII_TEST_BOOL(graph.GetResourceNamespace().IsEmpty());
    graph.EndSetup();
    XII_TEST_RESULT(graph.Compile());
    XII_TEST_INT(graph.GetResourceDescriptions().GetCount(), 2);
    XII_TEST_INT(graph.GetBufferDescription(graph.FindBuffer(graph.GetResourceDescriptions()[0].m_sName))->m_uiSize, 64);
    XII_TEST_INT(graph.GetBufferDescription(graph.FindBuffer(graph.GetResourceDescriptions()[1].m_sName))->m_uiSize, 144);
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_RESULT(root.Serialize(writer));
    xiiMemoryStreamContainerStorage<xiiDynamicArray<xiiUInt8>> firstBytes, secondBytes;
    xiiMemoryStreamWriter                                      firstWriter(&firstBytes), secondWriter(&secondBytes);
    XII_TEST_RESULT(root.Serialize(firstWriter));
    XII_TEST_RESULT(root.Serialize(secondWriter));
    XII_TEST_BOOL(xiiConstByteArrayPtr(firstBytes.GetData(), firstBytes.GetStorageSize32()) == xiiConstByteArrayPtr(secondBytes.GetData(), secondBytes.GetStorageSize32()));
    xiiMemoryStreamReader               reader(&storage);
    xiiRenderPipelineResourceDescriptor loaded;
    XII_TEST_RESULT(loaded.Deserialize(reader));
    XII_TEST_RESULT(xiiRenderPipeline::Compile(loaded, again));
    XII_TEST_BOOL(compiled->GetExecutionOrder() == again->GetExecutionOrder());
    XII_TEST_BOOL(loaded.m_Definitions[0].m_Id == leaf.m_Id);
    root.m_Definitions[0].m_Nodes[1].m_sType = "MissingReflectedPass";
    XII_TEST_BOOL(xiiRenderPipeline::Compile(root, again, &error).Failed());
    XII_TEST_BOOL(error.FindSubString("Pass/instance:") != nullptr);
    XII_TEST_BOOL(error.FindSubString("Producer") != nullptr);
  }
}

XII_CREATE_SIMPLE_TEST(Pipeline, RenderPipelineResource)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compilation, reflected settings and independent view instances")
  {
    auto                                  descriptor = PipelineResourceTest::MakeDefinition();
    xiiSharedPtr<const xiiRenderPipeline> compiled;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
    XII_TEST_INT(compiled->GetTopology().GetVertexCount(), 2);
    XII_TEST_INT(compiled->GetTopology().GetEdgeCount(), 1);
    XII_TEST_INT(compiled->GetExecutionOrder()[0], 1);
    xiiRenderPipelineInstance a, b;
    XII_TEST_RESULT(a.Initialize(compiled));
    XII_TEST_RESULT(b.Initialize(compiled));
    XII_TEST_BOOL(a.GetPipeline() == b.GetPipeline());
    XII_TEST_BOOL(a.GetPass(1) != b.GetPass(1));
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(a.GetPass(1))->m_uiValue, 4);
    xiiRenderGraph           graph;
    xiiRenderGraphBlackboard blackboard;
    graph.BeginSetup(1);
    XII_TEST_RESULT(a.Build(nullptr, graph, blackboard));
    graph.EndSetup();
    XII_TEST_RESULT(graph.Compile());
    XII_TEST_INT(graph.GetStatistics().m_uiCompiledPassCount, 2);
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(a.GetPass(1))->m_uiBuildCount, 1);
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(b.GetPass(1))->m_uiBuildCount, 0);
    XII_TEST_BOOL(!graph.GetResourceDescriptions()[0].m_bIsTexture);
    const auto* original = a.GetPass(1);
    XII_TEST_RESULT(a.Initialize(compiled));
    XII_TEST_BOOL(a.GetPass(1) == original);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pin validation, parallel dependencies, cycle detection and structural editing")
  {
    auto                                  descriptor = PipelineResourceTest::MakeDefinition();
    xiiSharedPtr<const xiiRenderPipeline> compiled;
    xiiStringBuilder                      error;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
    const auto original                  = compiled;
    descriptor.m_Connections[0].m_sInput = "Missing";
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled, &error).Failed());
    XII_TEST_BOOL(compiled == original && !error.IsEmpty());
    descriptor = PipelineResourceTest::MakeDefinition();
    descriptor.Connect(1, "Texture", 0, "Input").IgnoreResult();
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
    descriptor.m_Connections.PeekBack().m_sInput = "Texture";
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
    descriptor = PipelineResourceTest::MakeDefinition();
    descriptor.Connect(1, "Data", 0, "Input").IgnoreResult();
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
    XII_TEST_RESULT(descriptor.Disconnect(1));
    descriptor.Connect(0, "After", 1, "Before").IgnoreResult();
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
    XII_TEST_RESULT(descriptor.RemovePass(0));
    XII_TEST_INT(descriptor.m_Connections.GetCount(), 0);
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
    XII_TEST_BOOL(descriptor.RemovePass(9).Failed());
    descriptor.m_Passes[0].m_pType = nullptr;
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
    descriptor = PipelineResourceTest::MakeDefinition();
    descriptor.m_Connections.Clear();
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
    descriptor = PipelineResourceTest::MakeDefinition();
    descriptor.m_Passes[1].m_Properties.Insert("Value", "invalid numeric value");
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Late format resolution, required output and history invalidation")
  {
    auto descriptor = PipelineResourceTest::MakeDefinition();
    descriptor.m_Passes[1].m_Properties.Insert("EmitTexture", true);
    descriptor.m_Passes[1].m_Properties.Insert("UnknownTextureFormat", true);
    XII_TEST_RESULT(descriptor.Connect(1, "Texture", 0, "Texture"));
    xiiSharedPtr<const xiiRenderPipeline> compiled;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
    xiiRenderPipelineInstance instance;
    XII_TEST_RESULT(instance.Initialize(compiled));
    xiiRenderGraph           graph("Pipeline late format mismatch");
    xiiRenderGraphBlackboard blackboard;
    graph.BeginSetup(1);
    XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard).Failed());
    XII_TEST_INT(static_cast<const xiiPipelineTestSink*>(instance.GetPass(0))->m_uiBuildCount, 0);
    graph.EndSetup();
    instance.InvalidateTemporalHistory();
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(instance.GetPass(1))->m_uiHistoryResets, 1);
    descriptor = PipelineResourceTest::MakeDefinition();
    descriptor.m_Passes[1].m_Properties.Insert("OmitBuffer", true);
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
    XII_TEST_RESULT(instance.Initialize(compiled));
    graph.BeginSetup(2);
    XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard).Failed());
    XII_TEST_INT(static_cast<const xiiPipelineTestSink*>(instance.GetPass(0))->m_uiBuildCount, 0);
    graph.EndSetup();
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Scoped bindings preserve explicit source versions and validate descriptions")
  {
    xiiRenderGraph graph("Pipeline bindings");
    graph.BeginSetup(1);
    auto       source   = graph.AddPass<PipelineResourceTest::PassData>("Source", xiiGALCommandQueueFlags::Transfer, [](auto& data, xiiRenderGraphBuilder& builder) {
        xiiGALBufferCreationDescription description;
        description.m_uiSize = 16;
        data.m_hBuffer = builder.WriteBuffer("Physical", description, xiiGALResourceStateFlags::CopyDestination); }, [](const auto&, xiiRenderGraphPassContext&) {});
    const auto original = source.first->m_hBuffer;
    auto       newer    = graph.AddPass<PipelineResourceTest::PassData>("Newer version", xiiGALCommandQueueFlags::Transfer, [original](auto& data, xiiRenderGraphBuilder& builder) { data.m_hBuffer = builder.WriteBuffer(original, xiiGALResourceStateFlags::CopyDestination); }, [](const auto&, xiiRenderGraphPassContext&) {});
    graph.SetSetupBinding("Input", original);
    auto consumer = graph.AddPass<PipelineResourceTest::PassData>("Consumer", xiiGALCommandQueueFlags::Transfer, [](auto& data, xiiRenderGraphBuilder& builder) { data.m_hBuffer = builder.ReadBuffer("Input", xiiGALResourceStateFlags::CopySource); builder.SetPassSideEffects(true); }, [](const auto&, xiiRenderGraphPassContext&) {});
    XII_TEST_BOOL(consumer.first->m_hBuffer == original);
    XII_TEST_BOOL(consumer.first->m_hBuffer != newer.first->m_hBuffer);
    XII_TEST_BOOL(graph.GetBufferDescription(original) != nullptr);
    XII_TEST_BOOL(graph.GetBufferDescription({}) == nullptr);
    XII_TEST_BOOL(graph.GetTextureDescription({}) == nullptr);
    graph.ClearSetupBindings();
    graph.EndSetup();
    XII_TEST_RESULT(graph.Compile());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Versioned reflected serialization rejects every truncated envelope")
  {
    auto                          descriptor = PipelineResourceTest::MakeDefinition();
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_RESULT(descriptor.Serialize(writer));
    xiiMemoryStreamReader               reader(&storage);
    xiiRenderPipelineResourceDescriptor loaded;
    XII_TEST_RESULT(loaded.Deserialize(reader));
    XII_TEST_INT(loaded.m_Passes.GetCount(), 2);
    XII_TEST_INT(loaded.m_Connections.GetCount(), 1);
    XII_TEST_BOOL(loaded.m_Passes[0].m_pType == descriptor.m_Passes[0].m_pType);
    XII_TEST_BOOL(loaded.m_Passes[1].m_pType == descriptor.m_Passes[1].m_pType);
    XII_TEST_INT(loaded.m_Passes[1].m_Properties["Value"].ConvertTo<xiiUInt32>(), 4);
    xiiSharedPtr<const xiiRenderPipeline> pipeline;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(loaded, pipeline));
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(storage.GetStorageSize32());
    reader.SetReadPosition(0);
    reader.ReadBytes(bytes.GetData(), bytes.GetCount());
    for (xiiUInt32 length = 0; length < bytes.GetCount(); ++length)
    {
      xiiRawMemoryStreamReader truncated(bytes.GetData(), length);
      XII_TEST_BOOL(loaded.Deserialize(truncated).Failed());
      XII_TEST_INT(loaded.m_Passes.GetCount(), 2);
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dynamic pass RTTI is preserved and invalid serialized types do not replace descriptors")
  {
    xiiPipelineTestSource               source;
    const xiiRenderPipelinePass&        pass = source;
    xiiRenderPipelineResourceDescriptor descriptor;
    const auto                          index = descriptor.AddPass("Dynamic source", pass.GetDynamicRTTI());
    XII_TEST_BOOL(descriptor.m_Passes[index].m_pType == source.GetDynamicRTTI());
    xiiSharedPtr<const xiiRenderPipeline> compiled;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
    XII_TEST_BOOL(compiled->GetDescriptor().m_Passes[index].m_pType == pass.GetDynamicRTTI());

    for (const xiiRTTI* pInvalidType : {static_cast<const xiiRTTI*>(nullptr), xiiGetStaticRTTI<xiiRenderPipelineResourceDescriptor>()})
    {
      descriptor.m_Passes[index].m_pType = pInvalidType;
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      XII_TEST_RESULT(descriptor.Serialize(writer));
      auto                  loaded     = PipelineResourceTest::MakeDefinition();
      const auto            originalId = loaded.m_Id;
      xiiMemoryStreamReader reader(&storage);
      XII_TEST_BOOL(loaded.Deserialize(reader).Failed());
      XII_TEST_BOOL(loaded.m_Id == originalId);
      XII_TEST_INT(loaded.m_Passes.GetCount(), 2);
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Checksummed payload rejects complete corrupted definitions")
  {
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    auto                          descriptor = PipelineResourceTest::MakeDefinition();
    XII_TEST_RESULT(descriptor.Serialize(writer));
    xiiDynamicArray<xiiUInt8> bytes;
    bytes.SetCountUninitialized(storage.GetStorageSize32());
    xiiMemoryStreamReader reader(&storage);
    reader.ReadBytes(bytes.GetData(), bytes.GetCount());
    for (xiiUInt32 i = 8; i < bytes.GetCount(); ++i)
    {
      bytes[i] ^= 1;
      xiiRawMemoryStreamReader corrupt(bytes);
      XII_TEST_BOOL(descriptor.Deserialize(corrupt).Failed());
      XII_TEST_INT(descriptor.m_Passes.GetCount(), 2);
      bytes[i] ^= 1;
    }
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Resource manager sharing, reload and compilation snapshot lifetime")
  {
    auto                                      resource      = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("PipelineTest");
    xiiUniquePtr<xiiResourceLoaderFromMemory> initialLoader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     initialStream(&initialLoader->m_CustomData);
    initialStream << "PipelineTest.xiiBinRenderPipeline";
    xiiAssetFileHeader header;
    XII_TEST_RESULT(header.Write(initialStream));
    initialStream << static_cast<xiiUInt8>(3);
    XII_TEST_RESULT(PipelineResourceTest::MakeDefinition().Serialize(initialStream));
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(initialLoader));
    xiiRenderPipelineInstance a, b;
    XII_TEST_RESULT(a.Synchronize(resource));
    XII_TEST_RESULT(b.Synchronize(resource));
    XII_TEST_BOOL(a.GetPipeline() == b.GetPipeline());
    const auto* original    = a.GetPipeline();
    auto        replacement = PipelineResourceTest::MakeDefinition();
    replacement.m_Passes[1].m_Properties.Insert("Value", 9U);
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> replacementClosed = XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>);
    xiiUniquePtr<PipelineResourceTest::MemoryLoader>      loader            = XII_DEFAULT_NEW(PipelineResourceTest::MemoryLoader);
    loader->m_pClosed                                                       = replacementClosed;
    xiiMemoryStreamWriter stream(&loader->m_CustomData);
    stream << "PipelineTest.xiiBinRenderPipeline";
    XII_TEST_RESULT(header.Write(stream));
    stream << static_cast<xiiUInt8>(3);
    XII_TEST_RESULT(replacement.Serialize(stream));
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(loader));
    if (!XII_TEST_BOOL(replacementClosed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(5)) == xiiThreadSignal::WaitResult::Signaled))
      return;
    bool changed = false;
    XII_TEST_RESULT(a.Synchronize(resource, &changed));
    XII_TEST_BOOL(changed && a.GetPipeline() != original);
    XII_TEST_BOOL(b.GetPipeline() == original);
    XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(a.GetPass(1))->m_uiValue, 9);
    XII_TEST_RESULT(b.Synchronize(resource));
    XII_TEST_BOOL(a.GetPipeline() == b.GetPipeline());
    XII_TEST_BOOL(a.Synchronize({}, &changed).Failed());
    XII_TEST_BOOL(changed && a.GetPipeline() == nullptr);
    b.Clear();
    resource.Invalidate();
    xiiResourceManager::FreeAllUnusedResources();
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Latest custom pipeline payload survives an in-flight loader")
  {
    auto resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("PipelineBusyReloadTest");
    auto initial  = XII_DEFAULT_NEW(PipelineResourceTest::MemoryLoader);
    auto next     = XII_DEFAULT_NEW(PipelineResourceTest::MemoryLoader);
    auto latest   = XII_DEFAULT_NEW(PipelineResourceTest::MemoryLoader);
    auto write    = [](PipelineResourceTest::MemoryLoader& loader, xiiUInt32 value) -> xiiResult {
      auto definition = PipelineResourceTest::MakeDefinition();
      definition.m_Passes[1].m_Properties.Insert("Value", value);
      xiiMemoryStreamWriter writer(&loader.m_CustomData);
      writer << "PipelineBusyReloadTest.xiiBinRenderPipeline";
      xiiAssetFileHeader header;
      XII_SUCCEED_OR_RETURN(header.Write(writer));
      writer << static_cast<xiiUInt8>(3);
      return definition.Serialize(writer);
    };
    XII_TEST_RESULT(write(*initial, 4));
    XII_TEST_RESULT(write(*next, 9));
    XII_TEST_RESULT(write(*latest, 17));
    xiiSharedPtr<xiiRefCountedContainer<xiiThreadSignal>> latestClosed = XII_DEFAULT_NEW(xiiRefCountedContainer<xiiThreadSignal>);
    latest->m_pClosed                                                  = latestClosed;
    initial->m_Target                                                  = resource;
    initial->m_Next                                                    = std::move(next);
    initial->m_Latest                                                  = std::move(latest);
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(initial));
    xiiResourceManager::PreloadResource(resource);
    if (!XII_TEST_BOOL(latestClosed->m_Content.WaitForSignal(xiiTime::MakeFromSeconds(5)) == xiiThreadSignal::WaitResult::Signaled))
      return;
    xiiRenderPipelineInstance instance;
    XII_TEST_RESULT(instance.Synchronize(resource));
    if (XII_TEST_BOOL(instance.GetPipeline() != nullptr))
      XII_TEST_INT(static_cast<const xiiPipelineTestSource*>(instance.GetPass(1))->m_uiValue, 17);
    instance.Clear();
    resource.Invalidate();
    xiiResourceManager::FreeAllUnusedResources();
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native definitions are shared resources with pin-level topology")
  {
    const auto a = xiiLoadPipelineLibraryResource("ScreenEffects");
    const auto b = xiiLoadPipelineLibraryResource("ScreenEffects");
    XII_TEST_BOOL(a == b);
    xiiResourceLock<xiiRenderPipelineResource> resource(a, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(resource.IsValid()))
      return;
    const auto pipeline = resource->GetPipeline();
    if (!XII_TEST_BOOL(pipeline != nullptr))
      return;
    XII_TEST_INT(pipeline->GetTopology().GetVertexCount(), pipeline->GetDescriptor().m_Passes.GetCount());
    // Independent interface aliases are valid islands; pin connections, not graph
    // density or connectedness, determine this asset's intra-frame edge count.
    XII_TEST_INT(pipeline->GetTopology().GetEdgeCount(), pipeline->GetDescriptor().m_Connections.GetCount());
    XII_TEST_INT(pipeline->GetExecutionOrder().GetCount(), pipeline->GetTopology().GetVertexCount());
    XII_TEST_BOOL(pipeline->GetHeapMemoryUsage() > pipeline->GetDescriptor().m_Passes.GetHeapMemoryUsage());
    auto descriptor = resource->GetAuthoring()->m_Descriptor;
    descriptor.AddPass("Extra camera constants", xiiGetStaticRTTI<xiiViewConstantsPass>());
    descriptor.AddPass("Independent camera constants", xiiGetStaticRTTI<xiiViewConstantsPass>());
    xiiSharedPtr<const xiiRenderPipeline> edited;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, edited));
    xiiContiguousMemoryStreamStorage bytes;
    xiiMemoryStreamWriter            writer(&bytes);
    XII_TEST_RESULT(descriptor.Serialize(writer));
    xiiMemoryStreamReader               reader(&bytes);
    xiiRenderPipelineResourceDescriptor restored;
    XII_TEST_RESULT(restored.Deserialize(reader));
    XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, edited));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Runtime display parameters use their native resolver")
  {
    xiiVariantDictionary display, exposure, parameters;
    display.Insert("BloomStrength", 0.2f);
    exposure.Insert("ExposureCompensation", 1.25f);
    display.Insert("Exposure", exposure);
    parameters.Insert("DisplayOutput", display);
    xiiDisplayOutputSettings resolved;
    XII_TEST_RESULT(xiiResolvePipelineDisplaySettings(parameters, resolved));
    XII_TEST_FLOAT(resolved.m_fBloomStrength, 0.2f, 0);
    XII_TEST_FLOAT(resolved.m_Exposure.m_fExposureCompensation, 1.25f, 0);
    display.Insert("BloomRadius", "invalid numeric value");
    parameters.Insert("DisplayOutput", display);
    XII_TEST_BOOL(xiiResolvePipelineDisplaySettings(parameters, resolved).Failed());
    display.Insert("BloomRadius", -1.0f);
    parameters.Insert("DisplayOutput", display);
    XII_TEST_BOOL(xiiResolvePipelineDisplaySettings(parameters, resolved).Failed());
  }
}
