/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiAssetPipelineSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetPipelineSource, xiiRenderPipelinePass);

public:
  xiiUInt32                  m_uiValue = 1;
  xiiDynamicArray<xiiUInt32> m_Samples;
  xiiMap<xiiString, float>   m_Weights;
  void                       GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override { pins.PushBack({"Data", xiiRenderPipelinePin::Buffer, false}); }
  xiiResult                  Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetPipelineSource, 1, xiiRTTIDefaultAllocator<xiiAssetPipelineSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Value", m_uiValue),
      XII_ARRAY_MEMBER_PROPERTY("Samples", m_Samples),
      XII_MAP_MEMBER_PROPERTY("Weights", m_Weights),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiAssetPipelineSink : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetPipelineSink, xiiRenderPipelinePass);

public:
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override { pins.PushBack({"Input", xiiRenderPipelinePin::Buffer, true}); }
  xiiResult Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetPipelineSink, 1, xiiRTTIDefaultAllocator<xiiAssetPipelineSink>)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class PipelineAssetTestContext : public xiiRenderPipelineContext
  {
  public:
    explicit PipelineAssetTestContext(const xiiUuid& id) { m_DocumentGuid = id; }
    ~PipelineAssetTestContext()
    {
      for (const auto& id : m_Objects)
        m_Context.UnregisterObject(id);
    }
    template <typename T> void Register(const xiiUuid& id, T& object)
    {
      m_Context.RegisterObject(id, object.GetDynamicRTTI(), &object);
      m_Objects.PushBack(id);
    }
    using xiiRenderPipelineContext::ExportDocument;

  private:
    xiiDynamicArray<xiiUuid> m_Objects;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, RenderPipelineAsset)
{
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "RenderPipelineAssetTest", "pipelineassets", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("RenderPipelineAssetTest"));
  const auto  childId    = xiiUuid::MakeStableUuidFromString("Pipeline Child Document");
  const auto  rootId     = xiiUuid::MakeStableUuidFromString("Pipeline Root Document");
  const auto  passId     = xiiUuid::MakeStableUuidFromInt(1);
  const auto  boundaryId = xiiUuid::MakeStableUuidFromInt(2);
  const char* childPath  = ":pipelineassets/Child.xiiBinRenderPipeline";
  const char* rootPath   = ":pipelineassets/Root.xiiBinRenderPipeline";
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native interface and repeated reference nodes export through the existing asset compiler")
  {
    xiiAssetPipelineSource source;
    source.m_Samples.PushBack(2);
    source.m_Samples.PushBack(4);
    source.m_Weights.Insert("Near", 0.25f);
    source.m_Weights.Insert("Far", 0.75f);
    xiiRenderPipelineInterfacePass boundary;
    boundary.m_sName          = "Output";
    boundary.m_bInput         = false;
    boundary.m_uiResourceType = xiiRenderPipelinePin::Buffer;
    xiiDocumentObject_ConnectionBase output;
    output.m_Source    = passId;
    output.m_Target    = boundaryId;
    output.m_SourcePin = "Data";
    output.m_TargetPin = "Value";
    PipelineAssetTestContext child(childId);
    child.Register(passId, source);
    child.Register(boundaryId, boundary);
    child.Register(xiiUuid::MakeStableUuidFromInt(3), output);
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = childPath;
    message.m_uiAssetHash = 123;
    message.m_uiVersion   = 1;
    XII_TEST_BOOL(child.ExportDocument(&message).Succeeded());
    const auto                                 childResource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(childPath);
    xiiResourceLock<xiiRenderPipelineResource> acquiredChild(childResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(acquiredChild.IsValid() && acquiredChild->GetAuthoring()))
      return;
    XII_TEST_BOOL(acquiredChild->GetAuthoring()->m_Descriptor.m_Id == childId);
    XII_TEST_BOOL(acquiredChild->GetAuthoring()->m_Descriptor.m_Interface[0].m_Node == passId);
    const auto& captured = acquiredChild->GetAuthoring()->m_Descriptor.m_Passes[0].m_Properties;
    const auto* samples  = captured.GetValue("Samples");
    const auto* weights  = captured.GetValue("Weights");
    if (!XII_TEST_BOOL(samples && weights))
      return;
    XII_TEST_INT(samples->Get<xiiVariantArray>().GetCount(), 2);
    const auto* nearWeight = weights->Get<xiiVariantDictionary>().GetValue("Near");
    if (XII_TEST_BOOL(nearWeight != nullptr))
      XII_TEST_FLOAT(nearWeight->Get<float>(), 0.25f, 0);
    xiiRenderPipelineSubgraphPass    instances[2];
    xiiAssetPipelineSink             sinks[2];
    xiiDocumentObject_ConnectionBase connections[2];
    PipelineAssetTestContext         root(rootId);
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      instances[i].m_sResource = childPath;
      auto& override           = instances[i].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(passId);
      override.m_Properties.Insert("Value", i == 0 ? 4U : 7U);
      xiiDynamicArray<xiiRenderPipelinePin> pins;
      instances[i].GetPins(pins);
      XII_TEST_INT(pins.GetCount(), 1);
      XII_TEST_BOOL(!pins[0].m_bInput && pins[0].m_Type == xiiRenderPipelinePin::Buffer && pins[0].m_sName == "Output");
      const auto instanceId = xiiUuid::MakeStableUuidFromInt(10 + i);
      const auto sinkId     = xiiUuid::MakeStableUuidFromInt(20 + i);
      root.Register(instanceId, instances[i]);
      root.Register(sinkId, sinks[i]);
      connections[i].m_Source    = instanceId;
      connections[i].m_Target    = sinkId;
      connections[i].m_SourcePin = "Output";
      connections[i].m_TargetPin = "Input";
      root.Register(xiiUuid::MakeStableUuidFromInt(30 + i), connections[i]);
    }
    message.m_sOutputFile = rootPath;
    XII_TEST_BOOL(root.ExportDocument(&message).Succeeded());
    const auto                rootResource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(rootPath);
    xiiRenderPipelineInstance runtime;
    XII_TEST_RESULT(runtime.Synchronize(rootResource));
    if (!XII_TEST_BOOL(runtime.GetPipeline() != nullptr))
      return;
    XII_TEST_INT(runtime.GetPipeline()->GetTopology().GetVertexCount(), 4);
    xiiUInt32 configured[2] = {};
    for (xiiUInt32 i = 0; i < 4; ++i)
      if (runtime.GetPass(i)->IsInstanceOf<xiiAssetPipelineSource>())
      {
        const auto* configuredSource = static_cast<const xiiAssetPipelineSource*>(runtime.GetPass(i));
        const auto  value            = configuredSource->m_uiValue;
        XII_TEST_BOOL(configuredSource->m_Samples == source.m_Samples);
        XII_TEST_BOOL(configuredSource->m_Weights == source.m_Weights);
        if (value == 4)
          ++configured[0];
        if (value == 7)
          ++configured[1];
        XII_TEST_BOOL(runtime.GetPipeline()->GetProvenance()[i].m_Element == passId);
        XII_TEST_INT(runtime.GetPipeline()->GetProvenance()[i].m_InstancePath.GetCount(), 1);
      }
    XII_TEST_INT(configured[0], 1);
    XII_TEST_INT(configured[1], 1);
    connections[0].m_TargetPin = "Missing";
    XII_TEST_BOOL(root.ExportDocument(&message).Failed());
    xiiFileReader preserved;
    XII_TEST_RESULT(preserved.Open(rootPath));
    xiiAssetFileHeader header;
    XII_TEST_RESULT(header.Read(preserved));
    xiiUInt8 version = 0;
    preserved >> version;
    XII_TEST_INT(version, 3);
    xiiRenderPipelineResourceDescriptor saved;
    XII_TEST_RESULT(saved.Deserialize(preserved));
    XII_TEST_BOOL(saved.m_Id == rootId);
    XII_TEST_INT(saved.m_References.GetCount(), 1);
    XII_TEST_BOOL(saved.m_References[0].m_Id == childId);
  }
}
