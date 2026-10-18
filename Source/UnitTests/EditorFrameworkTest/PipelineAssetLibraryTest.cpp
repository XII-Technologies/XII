/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  class PipelineLibraryDocument : public xiiRenderPipelineAssetDocument
  {
  public:
    explicit PipelineLibraryDocument(xiiStringView path) : xiiRenderPipelineAssetDocument(path, xiiAssetDocEngineConnection::None)
    {
      m_pDocumentInfo = CreateDocumentInfo();
    }
  };
  class PipelineLibraryContext : public xiiRenderPipelineContext
  {
  public:
    ~PipelineLibraryContext()
    {
      for (const auto& id : m_Objects)
        m_Context.DeleteObject(id);
    }
    xiiResult Import(const xiiDocument& document)
    {
      m_DocumentGuid = document.GetGuid();
      xiiAbstractObjectGraph           graph;
      xiiDocumentObjectConverterWriter writer(&graph, document.GetObjectManager());
      for (const auto* object : document.GetObjectManager()->GetRootObject()->GetChildren())
        writer.AddObjectToGraph(object);
      xiiRttiConverterReader reader(&graph, &m_Context);
      for (const auto* object : document.GetObjectManager()->GetRootObject()->GetChildren())
      {
        if (!reader.CreateObjectFromNode(graph.GetNode(object->GetGuid())))
          return XII_FAILURE;
        m_Objects.PushBack(object->GetGuid());
      }
      return XII_SUCCESS;
    }
    using xiiRenderPipelineContext::ExportDocument;

  private:
    xiiDynamicArray<xiiUuid> m_Objects;
  };

  bool ReadBytes(xiiStringView path, xiiContiguousMemoryStreamStorage& storage)
  {
    xiiFileReader file;
    if (!XII_TEST_RESULT(file.Open(path)))
      return false;
    storage.ReadAll(file);
    return true;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, PipelineAssetLibrary)
{
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "PipelineLibraryTest", "PipelineLibrary", xiiDataDirUsage::ReadOnly)) ||
      !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "PipelineLibraryTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PipelineLibraryTest"));
  for (const xiiStringView name : {"AmbientOcclusion", "TransparencyResolve", "OpaqueComposition", "DebugVisualization", "ScreenEffects", "InstanceVisibility", "MeshletVisibility", "VertexPullDraws", "SurfaceQuery", "EditorMeshPicking", "DepthOnly", "GBuffer", "DepthPyramid", "InstanceOcclusion", "MeshletOcclusion", "GeometryLodFeedback", "ShadowDepth", "MatrixInstanceVisibility", "MatrixMeshletVisibility", "WorldMeshDraws", "ShadowMap", "ShadowMeshDepth", "ShadowCascades", "VirtualShadowTables", "VirtualShadowRasterWork", "VirtualShadowPagePublish", "VirtualShadowFeedback", "TexturePreview", "TextureCubePreview", "SceneTargets", "ViewPresentation", "BufferExport", "DepthSensor", "RgbSensor", "InfraredSensor", "LidarSensor", "DepthSensorDiagnostic", "DirectionalMesh", "DirectionalMeshConstants", "VirtualShadowSamplingConstants", "SceneRayProbe", "WorldRayProbe", "LightClusterView", "ClusterBounds", "ClusterListClear", "ClusterLightAssignment", "ClusterLightLists", "ClusterLightSampling", "WorldLightsUpload", "WorldDirectionalLight", "EnvironmentLightConstants", "DDGIProbeSchedule", "DDGIProbeUpload", "DDGIAtlasPrepare", "DDGIEnvironmentSeed", "DDGIEnvironmentProbes"})
  {
    xiiStringBuilder        source(xiiFileSystem::GetSdkRootDirectory(), "/Data/Base/RenderPipelines/", name, ".xiiRenderPipelineAsset");
    PipelineLibraryDocument storage(source);
    xiiDocument*            document = &storage;
    const auto              opened   = document->LoadDocument();
    XII_TEST_STATUS(opened);
    if (opened.Failed())
      return;
    XII_TEST_BOOL(storage.GetSettingsObject() != nullptr);
    XII_TEST_BOOL(document->IsInstanceOf<xiiRenderPipelineAssetDocument>());
    auto*     nodes     = static_cast<const xiiDocumentNodeManager*>(document->GetObjectManager());
    xiiUInt32 nodeCount = 0, passCount = 0, connectionCount = 0, internalConnectionCount = 0;
    for (const auto* object : nodes->GetRootObject()->GetChildren())
    {
      nodeCount += nodes->IsNode(object) ? 1U : 0U;
      passCount += nodes->IsNode(object) && !object->GetType()->IsDerivedFrom<xiiRenderPipelineInterfacePass>() ? 1U : 0U;
      connectionCount += nodes->IsConnection(object) ? 1U : 0U;
      if (nodes->IsConnection(object))
      {
        const auto* sourceNode = nodes->GetObject(object->GetTypeAccessor().GetValue("Source").Get<xiiUuid>());
        const auto* targetNode = nodes->GetObject(object->GetTypeAccessor().GetValue("Target").Get<xiiUuid>());
        internalConnectionCount += sourceNode && targetNode && !sourceNode->GetType()->IsDerivedFrom<xiiRenderPipelineInterfacePass>() && !targetNode->GetType()->IsDerivedFrom<xiiRenderPipelineInterfacePass>() ? 1U : 0U;
      }
      if (nodes->IsNode(object))
      {
        const auto inspect = [&](const auto& pins) {
          for (const auto& pin : pins)
          {
            const auto* properties = nodes->GetPinPropertyObject(*pin);
            XII_TEST_BOOL(properties != nullptr);
            if (properties)
              XII_TEST_BOOL(properties->GetTypeAccessor().GetValue("Id").Get<xiiUuid>() == properties->GetGuid());
          }
        };
        inspect(nodes->GetInputPins(object));
        inspect(nodes->GetOutputPins(object));
      }
    }
    XII_TEST_BOOL(passCount > 0);
    {
      xiiQtNodeScene scene;
      scene.InitScene(nodes);
      xiiUInt32 visibleNodes = 0, visibleConnections = 0;
      for (const auto* item : scene.items())
      {
        visibleNodes += item->type() == xiiQtNodeScene::Node ? 1U : 0U;
        visibleConnections += item->type() == xiiQtNodeScene::Connection ? 1U : 0U;
      }
      XII_TEST_INT(visibleNodes, nodeCount);
      XII_TEST_INT(visibleConnections, connectionCount);
    }
    PipelineLibraryContext context;
    if (!XII_TEST_RESULT(context.Import(*document)))
      return;
    xiiExportDocumentMsgToEngine message;
    message.m_sOutputFile = xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/", name, ".xiiBinRenderPipeline");
    message.m_uiAssetHash = 1;
    message.m_uiVersion   = 7;
    auto exported         = context.ExportDocument(&message);
    XII_TEST_STATUS(exported);
    if (exported.Failed())
      return;
    const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
    xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
      return;
    const auto authoring = loaded->GetAuthoring();
    const bool preview   = name == "TexturePreview" || name == "TextureCubePreview";
    XII_TEST_INT(authoring->m_Descriptor.m_Settings.m_uiOutputSamples, preview ? 1 : 0);
    XII_TEST_BOOL(authoring->m_Descriptor.m_Settings.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::Compute) == (!preview && name != "ShadowDepth" && name != "ShadowCascades" && name != "VirtualShadowTables" && name != "VirtualShadowRasterWork" && name != "VirtualShadowPagePublish" && name != "SceneTargets" && name != "ViewPresentation" && name != "BufferExport" && name != "DirectionalMesh" && name != "DirectionalMeshConstants" && name != "VirtualShadowSamplingConstants" && name != "LightClusterView" && name != "WorldLightsUpload" && name != "WorldDirectionalLight" && name != "EnvironmentLightConstants" && name != "DDGIProbeSchedule" && name != "DDGIProbeUpload" && name != "DDGIAtlasPrepare"));
    XII_TEST_BOOL(authoring->m_Descriptor.m_Id == document->GetGuid());
    XII_TEST_INT(authoring->m_Descriptor.m_Passes.GetCount(), passCount);
    XII_TEST_INT(authoring->m_Descriptor.m_Connections.GetCount(), internalConnectionCount);
    for (const auto& pass : authoring->m_Descriptor.m_Passes)
      XII_TEST_BOOL(nodes->GetObject(pass.m_Id) != nullptr);
    xiiContiguousMemoryStreamStorage packaged, rebuilt, repeated;
    const xiiStringBuilder           package(":PipelineLibrary/RenderPipelines/", name, ".xiiBinRenderPipeline");
    if (!ReadBytes(package, packaged) || !ReadBytes(message.m_sOutputFile, rebuilt))
      return;
    XII_TEST_BOOL(xiiMakeArrayPtr(packaged.GetData(), packaged.GetStorageSize32()) == xiiMakeArrayPtr(rebuilt.GetData(), rebuilt.GetStorageSize32()));
    exported = context.ExportDocument(&message);
    XII_TEST_STATUS(exported);
    if (exported.Failed() || !ReadBytes(message.m_sOutputFile, repeated))
      return;
    XII_TEST_BOOL(xiiMakeArrayPtr(rebuilt.GetData(), rebuilt.GetStorageSize32()) == xiiMakeArrayPtr(repeated.GetData(), repeated.GetStorageSize32()));
  }
}
