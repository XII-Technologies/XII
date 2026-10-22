/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>

class xiiContractPipelinePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiContractPipelinePass, xiiRenderPipelinePass);

public:
  bool      m_bInput           = false;
  bool      m_bPublishDirectly = false;
  xiiUInt32 m_uiWidth          = 16;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Texture", xiiRenderPipelinePin::Texture, m_bInput});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    if (m_bInput)
      return context.GetInputTexture("Texture").IsValid() ? XII_SUCCESS : XII_FAILURE;
    struct Data
    {
      xiiRenderGraphTextureHandle m_Texture;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [this](Data& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription description;
        description.m_Type = xiiGALResourceDimension::Texture2D;
        description.m_Size = {m_uiWidth, 16};
        description.m_Format = xiiGALResourceFormat::RGBA16Float;
        description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
        data.m_Texture = builder.WriteTexture("ContractTexture", description, xiiGALResourceStateFlags::UnorderedAccess); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    if (m_bPublishDirectly)
    {
      context.m_Bindings[0].m_hTexture = emitted.first->m_Texture;
      return XII_SUCCESS;
    }
    return context.SetOutput("Texture", emitted.first->m_Texture);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiContractPipelinePass, 1, xiiRTTIDefaultAllocator<xiiContractPipelinePass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Input", m_bInput),
      XII_MEMBER_PROPERTY("Width", m_uiWidth),
      XII_MEMBER_PROPERTY("PublishDirectly", m_bPublishDirectly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  xiiRenderPipelinePinConfiguration* AddConfiguration(xiiRenderPipelinePass& pass, xiiStringView name = "Texture", bool input = false)
  {
    xiiRenderPipelinePinConfiguration* configuration = XII_DEFAULT_NEW(xiiRenderPipelinePinConfiguration);
    configuration->m_Id                              = xiiUuid::MakeUuid();
    configuration->m_Metadata.m_sName                = name;
    configuration->m_Metadata.m_bInput               = input;
    configuration->m_Format                          = xiiGALResourceFormat::RGBA16Float;
    configuration->m_Constraints.m_Dimension         = xiiGALResourceDimension::Texture2D;
    configuration->m_Constraints.m_uiWidth           = 16;
    configuration->m_Constraints.m_uiHeight          = 16;
    configuration->m_Constraints.m_uiSamples         = 1;
    configuration->m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::ShaderResource;
    pass.m_PinConfigurations.PushBack(configuration);
    return configuration;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Pipeline, DefaultedPipelineInput)
{
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Default resource", xiiGetStaticRTTI<xiiContractPipelinePass>());
  xiiVariantDictionary properties;
  properties.Insert("Format", static_cast<xiiInt64>(xiiGALResourceFormat::RGBA16Float));
  definition.AddPass("Input boundary", xiiGetStaticRTTI<xiiRenderPipelineInputDefaultPass>(), properties);
  XII_TEST_RESULT(definition.Connect(0, "Texture", 1, "Default"));
  definition.m_Interface.PushBack({"Color", true, definition.m_Passes[1].m_Id, "Override"});
  definition.m_Interface.PushBack({"Color", false, definition.m_Passes[1].m_Id, "Value"});
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  if (!XII_TEST_RESULT(xiiRenderPipeline::Compile(definition, compiled)))
    return;
  xiiRenderPipelineInstance instance;
  XII_TEST_RESULT(instance.Initialize(compiled));
  const auto input  = compiled->FindInterface("Color", true);
  const auto output = compiled->FindInterface("Color", false);
  XII_TEST_BOOL(compiled->GetInterface()[input].m_Pin.m_bOptional);
  xiiRenderGraph           graph;
  xiiRenderGraphBlackboard blackboard;
  for (xiiUInt32 frame = 0; frame < 3; ++frame)
  {
    graph.BeginSetup(frame);
    xiiHybridArray<xiiRenderPipelineInputBinding, 1> inputs;
    xiiRenderGraphTextureHandle                      external;
    if (frame == 1)
    {
      struct Data
      {
        xiiRenderGraphTextureHandle m_Texture;
      };
      const auto emitted = graph.AddPass<Data>("Parent resource", xiiGALCommandQueueFlags::Compute, [](Data& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription description;
        description.m_Type = xiiGALResourceDimension::Texture2D;
        description.m_Size = {32, 16};
        description.m_Format = xiiGALResourceFormat::RGBA16Float;
        description.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
        data.m_Texture = builder.WriteTexture("Parent", description, xiiGALResourceStateFlags::UnorderedAccess); }, [](const Data&, xiiRenderGraphPassContext&) {});
      external           = emitted.first->m_Texture;
      inputs.PushBack({graph, input, external});
    }
    XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard, {}, inputs));
    const auto* binding = instance.GetInterfaceBinding(output);
    if (XII_TEST_BOOL(binding && graph.GetTextureDescription(binding->m_hTexture)))
    {
      XII_TEST_INT(graph.GetTextureDescription(binding->m_hTexture)->m_Size.width, frame == 1 ? 32 : 16);
      if (frame == 1)
        XII_TEST_BOOL(binding->m_hTexture == external);
    }
    graph.EndSetup();
  }

  const auto                          childId = definition.m_Id;
  const auto                          child   = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("DefaultInputChild", std::move(definition));
  xiiRenderPipelineResourceDescriptor middle;
  middle.AddSubgraph("Nested", childId);
  middle.m_References.PushBack({childId, "UnresolvedChildAsset"});
  middle.m_Interface.PushBack({"Color", true, middle.m_Passes[0].m_Id, "Color"});
  middle.m_Interface.PushBack({"Color", false, middle.m_Passes[0].m_Id, "Color"});
  const auto                    nested = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("DefaultInputNested", std::move(middle));
  xiiRenderPipelineSubgraphPass occurrence;
  occurrence.m_sResource = "UnresolvedNestedAsset";
  const auto resolve     = [&](xiiStringView id) -> xiiString {
    if (id == "UnresolvedChildAsset")
      return child.GetResourceID();
    if (id == "UnresolvedNestedAsset")
      return nested.GetResourceID();
    return xiiString(id);
  };
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  occurrence.GetPins(pins, resolve);
  XII_TEST_INT(pins.GetCount(), 2);
}

XII_CREATE_SIMPLE_TEST(Pipeline, ReflectedPinContracts)
{
  xiiContractPipelinePass source;
  auto*                   configuration = AddConfiguration(source);
  const auto              sourceId      = configuration->m_Id;
  configuration->m_sDisplayName         = "HDR scene";
  xiiVariantDictionary properties;
  XII_TEST_RESULT(source.GetProperties(properties));
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Producer", xiiGetStaticRTTI<xiiContractPipelinePass>(), properties);
  xiiVariantDictionary input;
  input.Insert("Input", true);
  definition.AddPass("Consumer", xiiGetStaticRTTI<xiiContractPipelinePass>(), input);
  XII_TEST_RESULT(definition.Connect(0, "Texture", 1, "Texture"));
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  xiiStringBuilder                      error;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(definition, compiled, &error));
  if (!XII_TEST_BOOL(compiled != nullptr))
    return;
  const auto& pin = compiled->GetPins(0)[0];
  XII_TEST_BOOL(pin.m_SourceId == sourceId && pin.m_Id.IsValid() && pin.m_Id != sourceId);
  XII_TEST_STRING(pin.m_sDisplayName, "HDR scene");
  XII_TEST_INT(pin.m_Constraints.m_uiWidth, 16);
  xiiSharedPtr<const xiiRenderPipeline> repeated;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(definition, repeated));
  XII_TEST_BOOL(repeated->GetPins(0)[0].m_Id == pin.m_Id);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Resource descriptions and direct binding publication obey compiled requirements")
  {
    for (bool direct : {false, true})
    {
      auto local = definition;
      local.m_Passes[0].m_Properties.Insert("PublishDirectly", direct);
      XII_TEST_RESULT(xiiRenderPipeline::Compile(local, repeated));
      xiiRenderPipelineInstance instance;
      XII_TEST_RESULT(instance.Initialize(repeated));
      XII_TEST_BOOL(instance.GetPass(0)->m_PinConfigurations.IsEmpty());
      xiiRenderGraph           graph("Pin contract");
      xiiRenderGraphBlackboard blackboard;
      graph.BeginSetup(0);
      XII_TEST_RESULT(instance.Build(nullptr, graph, blackboard));
      XII_TEST_BOOL(instance.GetLastBuildError().IsEmpty());
      graph.EndSetup();
      local.m_Passes[0].m_Properties.Insert("Width", 32U);
      XII_TEST_RESULT(xiiRenderPipeline::Compile(local, repeated));
      XII_TEST_RESULT(instance.Initialize(repeated));
      xiiRenderGraph mismatch("Pin contract mismatch");
      mismatch.BeginSetup(0);
      XII_TEST_BOOL(instance.Build(nullptr, mismatch, blackboard).Failed());
      XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("Producer") != nullptr);
      if (direct)
        XII_TEST_BOOL(instance.GetLastBuildError().FindSubString("Texture") != nullptr);
      mismatch.EndSetup();
    }
    xiiGALTextureCreationDescription texture;
    texture.m_Type      = xiiGALResourceDimension::Texture2D;
    texture.m_Size      = {16, 16};
    texture.m_Format    = xiiGALResourceFormat::RGBA16Float;
    texture.m_BindFlags = xiiGALBindFlags::ShaderResource;
    XII_TEST_BOOL(pin.Accepts(texture));
    texture.m_uiSampleCount = 4;
    XII_TEST_BOOL(!pin.Accepts(texture));
    texture.m_uiSampleCount = 1;
    texture.m_BindFlags     = {};
    XII_TEST_BOOL(!pin.Accepts(texture));
    xiiRenderPipelinePin buffer{"Buffer", xiiRenderPipelinePin::Buffer};
    buffer.m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UnorderedAccess;
    xiiGALBufferCreationDescription description;
    XII_TEST_BOOL(!buffer.Accepts(description));
    description.m_BindFlags = xiiGALBindFlags::UnorderedAccess;
    XII_TEST_BOOL(buffer.Accepts(description));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid or weakened native contracts fail with pin diagnostics")
  {
    const auto validate = [&]() {
      xiiVariantDictionary invalid;
      XII_TEST_RESULT(source.GetProperties(invalid));
      auto local                     = definition;
      local.m_Passes[0].m_Properties = invalid;
      error.Clear();
      XII_TEST_BOOL(xiiRenderPipeline::Compile(local, repeated, &error).Failed());
      XII_TEST_BOOL(error.FindSubString("Pin") != nullptr && error.FindSubString("Producer") != nullptr);
    };
    configuration->m_Constraints.m_uiSamples = 3;
    validate();
    configuration->m_Constraints.m_uiSamples = 1;
    configuration->m_Metadata.m_sName        = "Missing";
    validate();
    configuration->m_Metadata.m_sName = "Texture";
    AddConfiguration(source);
    validate();
    source.m_PinConfigurations.PeekBack()->GetDynamicRTTI()->GetAllocator()->Deallocate(source.m_PinConfigurations.PeekBack());
    source.m_PinConfigurations.PopBack();
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nested repeated definitions lower per-instance boundary contracts and persistent identities")
  {
    xiiGraphDefinition leaf;
    definition.m_Interface.PushBack({"Output", false, definition.m_Passes[0].m_Id, "Texture"});
    XII_TEST_RESULT(definition.MakeGraphDefinition(leaf));
    xiiRenderPipelineResourceDescriptor middle;
    middle.AddSubgraph("Nested", leaf.m_Id);
    middle.m_Interface.PushBack({"Output", false, middle.m_Passes[0].m_Id, "Output"});
    xiiGraphDefinition nested;
    XII_TEST_RESULT(middle.MakeGraphDefinition(nested));
    xiiRenderPipelineResourceDescriptor root;
    root.m_Definitions.PushBack(leaf);
    root.m_Definitions.PushBack(nested);
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      const auto                    index = root.AddSubgraph(i ? "Second" : "First", nested.m_Id);
      xiiRenderPipelineSubgraphPass occurrence;
      auto*                         boundary = AddConfiguration(occurrence, "Output");
      boundary->m_sDisplayName               = i ? "Second output" : "First output";
      xiiVariantDictionary captured;
      XII_TEST_RESULT(occurrence.GetProperties(captured));
      root.m_Passes[index].m_Properties.Insert("PinConfigurations", *captured.GetValue("PinConfigurations"));
      root.m_Interface.PushBack({i ? "Second" : "First", false, root.m_Passes[index].m_Id, "Output"});
    }
    XII_TEST_RESULT(xiiRenderPipeline::Compile(root, repeated, &error));
    if (!XII_TEST_BOOL(repeated != nullptr && repeated->GetProvenance().GetCount() == 4))
      return;
    XII_TEST_BOOL(repeated->GetPins(0)[0].m_Id != repeated->GetPins(2)[0].m_Id);
    xiiDynamicArray<xiiRenderPipelinePin> external;
    XII_TEST_RESULT(xiiGetRenderPipelineInterface(root, external, &error));
    XII_TEST_INT(external.GetCount(), 2);
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_RESULT(root.Serialize(writer));
    xiiMemoryStreamReader               reader(&storage);
    xiiRenderPipelineResourceDescriptor restored;
    XII_TEST_RESULT(restored.Deserialize(reader));
    xiiSharedPtr<const xiiRenderPipeline> roundtrip;
    XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, roundtrip));
    XII_TEST_BOOL(roundtrip->GetPins(0)[0].m_Id == repeated->GetPins(0)[0].m_Id);
    auto pins         = root.m_Passes[1].m_Properties.GetValue("PinConfigurations")->Get<xiiVariantArray>();
    auto fields       = pins[0].Get<xiiVariantDictionary>();
    auto requirements = fields.GetValue("Requirements")->Get<xiiVariantDictionary>();
    requirements.Insert("Width", 32U);
    fields.Insert("Requirements", requirements);
    pins[0] = fields;
    root.m_Passes[1].m_Properties.Insert("PinConfigurations", pins);
    XII_TEST_BOOL(xiiRenderPipeline::Compile(root, roundtrip, &error).Failed());
    XII_TEST_BOOL(error.FindSubString("Output") != nullptr && error.FindSubString("instance") != nullptr);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Boundary fan-out intersects every consumer requirement rather than only the first schema")
  {
    xiiRenderPipelineResourceDescriptor fanout;
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      xiiContractPipelinePass consumer;
      consumer.m_bInput = true;
      if (i)
        AddConfiguration(consumer, "Texture", true)->m_Constraints.m_uiWidth = i == 1 ? 16U : 32U;
      xiiVariantDictionary captured;
      XII_TEST_RESULT(consumer.GetProperties(captured));
      xiiStringBuilder name;
      name.SetFormat("Consumer{}", i);
      const auto index = fanout.AddPass(name, xiiGetStaticRTTI<xiiContractPipelinePass>(), captured);
      fanout.m_Interface.PushBack({"Input", true, fanout.m_Passes[index].m_Id, "Texture"});
    }
    XII_TEST_BOOL(xiiRenderPipeline::Compile(fanout, repeated, &error).Failed());
    XII_TEST_BOOL(error.FindSubString("fan-out") != nullptr);
    xiiDynamicArray<xiiRenderPipelinePin> external;
    XII_TEST_BOOL(xiiGetRenderPipelineInterface(fanout, external, &error).Failed());
  }
}
