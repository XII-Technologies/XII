/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <type_traits>

namespace PipelineResourceTest
{
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
    descriptor.AddPass("Consumer", "xiiPipelineTestSink");
    xiiVariantDictionary properties;
    properties.Insert("Value", 4U);
    descriptor.AddPass("Producer", "xiiPipelineTestSource", properties);
    descriptor.Connect(1, "Data", 0, "Input").IgnoreResult();
    descriptor.m_Parameters.Insert("Exposure", 1.0f);
    return descriptor;
  }
} // namespace PipelineResourceTest

static_assert(!std::is_copy_constructible_v<xiiRenderPipelineInstance>);

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
    xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
    stream << "PipelineTest.xiiBinRenderPipeline";
    XII_TEST_RESULT(header.Write(stream));
    stream << static_cast<xiiUInt8>(3);
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
      XII_TEST_INT(pipeline->GetTopology().GetVertexCount(), 18);
      XII_TEST_BOOL(pipeline->GetTopology().GetEdgeCount() > 17);
      XII_TEST_INT(pipeline->GetExecutionOrder().GetCount(), 18);
      auto descriptor = pipeline->GetDescriptor();
      XII_TEST_RESULT(descriptor.RemovePass(15)); // Debug is a removable independent stage.
      XII_TEST_RESULT(descriptor.Connect(14, "DisplayLinearColor", 15, "DisplayLinearColor"));
      xiiSharedPtr<const xiiRenderPipeline> edited;
      XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, edited));
      XII_TEST_BOOL(edited->GetHeapMemoryUsage() > descriptor.m_Passes.GetHeapMemoryUsage() + descriptor.m_Connections.GetHeapMemoryUsage());
      auto incomplete = pipeline->GetDescriptor();
      XII_TEST_RESULT(incomplete.RemovePass(0));
      xiiStringBuilder error;
      XII_TEST_BOOL(xiiRenderPipeline::Compile(incomplete, edited, &error).Failed());
      XII_TEST_BOOL(!error.IsEmpty());
      auto duplicate = pipeline->GetDescriptor();
      duplicate.AddPass("Duplicate preparation", "xiiBuiltinRenderPipelinePass", duplicate.m_Passes[0].m_Properties);
      XII_TEST_BOOL(xiiRenderPipeline::Compile(duplicate, edited).Failed());
      auto                 configured = pipeline->GetDescriptor();
      xiiVariantDictionary display;
      display.Insert("BloomStrength", 0.2f);
      xiiVariantDictionary exposure;
      exposure.Insert("ExposureCompensation", 1.25f);
      display.Insert("Exposure", exposure);
      configured.m_Parameters.Insert("DisplayOutput", display);
      XII_TEST_RESULT(xiiRenderPipeline::Compile(configured, edited));
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         stream(&storage);
      XII_TEST_RESULT(configured.Serialize(stream));
      xiiMemoryStreamReader               reader(&storage);
      xiiRenderPipelineResourceDescriptor restored;
      XII_TEST_RESULT(restored.Deserialize(reader));
      XII_TEST_BOOL(restored.m_Parameters == configured.m_Parameters);
      XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, edited));
      display.Insert("BloomRadius", "invalid numeric value");
      configured.m_Parameters.Insert("DisplayOutput", display);
      XII_TEST_BOOL(xiiRenderPipeline::Compile(configured, edited).Failed());
      display.Insert("BloomRadius", -1.0f);
      configured.m_Parameters.Insert("DisplayOutput", display);
      XII_TEST_BOOL(xiiRenderPipeline::Compile(configured, edited).Failed());
    }
  }
}
