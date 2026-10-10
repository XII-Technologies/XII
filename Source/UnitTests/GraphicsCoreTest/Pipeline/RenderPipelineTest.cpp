/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

namespace PipelineResourceTest
{
  struct PassData
  {
    xiiRenderGraphBufferHandle m_hBuffer;
  };
} // namespace PipelineResourceTest

class xiiPipelineTestSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineTestSource, xiiRenderPipelinePass);

public:
  xiiUInt32 m_uiValue      = 1;
  xiiUInt32 m_uiBuildCount = 0;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Data", xiiRenderPipelinePin::Buffer, false});
    pins.PushBack({"Texture", xiiRenderPipelinePin::Texture, false, true, xiiGALResourceFormat::RGBA8UNormalized});
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
        data.m_hBuffer = builder.WriteBuffer("TestProducedBuffer", description, xiiGALResourceStateFlags::UnorderedAccess); }, [](const PipelineResourceTest::PassData&, xiiRenderGraphPassContext&) {});
    return context.SetOutput("Data", emitted.first->m_hBuffer);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineTestSource, 1, xiiRTTIDefaultAllocator<xiiPipelineTestSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Value", m_uiValue),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiPipelineTestSink : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineTestSink, xiiRenderPipelinePass);

public:
  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Input", xiiRenderPipelinePin::Buffer, true});
    pins.PushBack({"Texture", xiiRenderPipelinePin::Texture, true, true, xiiGALResourceFormat::RGBA16Float});
    pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    const auto input = context.GetInputBuffer("Input");
    if (!input.IsValid()) return XII_FAILURE;
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
    descriptor.AddPass("Consumer", "xiiPipelineTestSink");
    xiiVariantDictionary properties;
    properties.Insert("Value", 4U);
    descriptor.AddPass("Producer", "xiiPipelineTestSource", properties);
    descriptor.Connect(1, "Data", 0, "Input").IgnoreResult();
    descriptor.m_Parameters.Insert("Exposure", 1.0f);
    return descriptor;
  }
} // namespace PipelineResourceTest

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
    descriptor.m_Passes[0].m_sType = "MissingType";
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
    descriptor = PipelineResourceTest::MakeDefinition();
    descriptor.m_Connections.Clear();
    XII_TEST_BOOL(xiiRenderPipeline::Compile(descriptor, compiled).Failed());
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
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Resource manager sharing, reload and compilation snapshot lifetime")
  {
    auto                                      resource      = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("PipelineTest");
    xiiUniquePtr<xiiResourceLoaderFromMemory> initialLoader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     initialStream(&initialLoader->m_CustomData);
    initialStream << "PipelineTest";
    XII_TEST_RESULT(PipelineResourceTest::MakeDefinition().Serialize(initialStream));
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(initialLoader));
    xiiRenderPipelineInstance a, b;
    XII_TEST_RESULT(a.Synchronize(resource));
    XII_TEST_RESULT(b.Synchronize(resource));
    XII_TEST_BOOL(a.GetPipeline() == b.GetPipeline());
    const auto* original    = a.GetPipeline();
    auto        replacement = PipelineResourceTest::MakeDefinition();
    replacement.m_Passes[1].m_Properties.Insert("Value", 9U);
    xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
    stream << "PipelineTest";
    XII_TEST_RESULT(replacement.Serialize(stream));
    xiiResourceManager::UpdateResourceWithCustomLoader(resource, std::move(loader));
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
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default definition is a shared reusable resource with pin-level topology")
  {
    auto a = xiiRenderPipelineResource::GetDefaultPipeline();
    auto b = xiiRenderPipelineResource::GetDefaultPipeline();
    XII_TEST_BOOL(a == b);
    xiiResourceLock<xiiRenderPipelineResource> resource(a, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(resource.IsValid());
    const auto pipeline = resource->GetPipeline();
    XII_TEST_BOOL(pipeline != nullptr);
    if (pipeline)
    {
      XII_TEST_INT(pipeline->GetTopology().GetVertexCount(), 14);
      XII_TEST_BOOL(pipeline->GetTopology().GetEdgeCount() > 13);
      XII_TEST_INT(pipeline->GetExecutionOrder().GetCount(), 14);
      auto descriptor = pipeline->GetDescriptor();
      XII_TEST_RESULT(descriptor.RemovePass(11)); // Debug is a removable independent stage.
      xiiSharedPtr<const xiiRenderPipeline> edited;
      XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, edited));
    }
  }
}
