/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineProfileConfig.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QImage>
#include <QPainter>
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
      static xiiAssetDocumentTypeDescriptor descriptor;
      descriptor.m_sDocumentTypeName  = "RenderPipeline";
      descriptor.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform;
      m_pTypeDescriptor               = &descriptor;
    }

  protected:
    void InternalAfterSaveDocument() override {}
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

void xiiCheckAuthoredSceneContract(const xiiRenderPipelineResource& scene);

XII_CREATE_SIMPLE_TEST_GROUP(RendererAssets);
XII_CREATE_SIMPLE_TEST(RendererAssets, PipelineAssetLibrary)
{
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "PipelineLibraryTest", "PipelineLibrary", xiiDataDirUsage::ReadOnly)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base/AssetCache/Common", "PipelineLibraryTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PipelineLibraryTest"));
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "PipelineLibraryTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  // Export dependencies first so resource references resolve to the freshly rebuilt library in the output directory.
  xiiStringBuilder lookupTable;
  for (const xiiStringView name : {"SceneShadowCascade", "SceneDirectionalShadows", "SceneSpotShadows", "SceneAtmosphere", "SceneScreenSpaceReflections", "SceneTransparency", "SceneParticles", "SceneScreenSpaceIrradiance", "Scene", "Editor"})
  {
    xiiStringBuilder        source(xiiFileSystem::GetSdkRootDirectory(), "/Data/Base/RenderPipelines/", name, ".xiiRenderPipelineAsset");
    xiiTestFramework::Output(xiiTestOutput::Details, "Exporting %s", name.GetStartPointer());
    PipelineLibraryDocument storage(source);
    xiiDocument*            document = &storage;
    const auto              opened   = document->LoadDocument();
    XII_TEST_STATUS(opened);
    if (opened.Failed())
      return;
    const auto* info = static_cast<const xiiAssetDocumentInfo*>(document->GetDocumentInfo());
    XII_TEST_BOOL(info->GetMetaInfo<xiiRenderPipelineAssetInterface>() != nullptr);
    if (name == "Editor")
    {
      XII_TEST_BOOL(info->m_TransformDependencies.Contains(xiiRenderPipelineProfileConfig::s_szScenePipeline));
      XII_TEST_BOOL(info->m_PackageDependencies.Contains(xiiRenderPipelineProfileConfig::s_szScenePipeline));
    }
    if (name == "Scene")
    {
      for (const xiiStringView workload : {"SceneAtmosphere", "SceneDirectionalShadows", "SceneSpotShadows", "SceneScreenSpaceReflections", "SceneTransparency", "SceneParticles", "SceneScreenSpaceIrradiance"})
      {
        const auto dependency = xiiPipelineLibraryTestIdentity(workload);
        XII_TEST_BOOL(info->m_TransformDependencies.Contains(dependency));
        XII_TEST_BOOL(info->m_PackageDependencies.Contains(dependency));
      }
    }
    if (name == "SceneDirectionalShadows" || name == "SceneSpotShadows")
    {
      const auto dependency = xiiPipelineLibraryTestIdentity("SceneShadowCascade");
      XII_TEST_BOOL(info->m_TransformDependencies.Contains(dependency));
      XII_TEST_BOOL(info->m_PackageDependencies.Contains(dependency));
    }
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
            if (properties && properties->GetType()->IsDerivedFrom<xiiRenderPipelinePinConfiguration>())
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
    message.m_sOutputFile = xiiStringBuilder(xiiFileSystem::GetSdkRootDirectory(), "/Data/Base/AssetCache/Common/RenderPipelines/", name, ".xiiBinRenderPipeline");
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
    if (name == "Scene")
      xiiCheckAuthoredSceneContract(*loaded.operator->());
    const auto       identity = xiiPipelineLibraryTestIdentity(name);
    xiiStringBuilder documentIdentity;
    xiiConversionUtils::ToString(document->GetGuid(), documentIdentity);
    XII_TEST_STRING(documentIdentity, identity);
    xiiResourceManager::RegisterNamedResource(identity, message.m_sOutputFile);
    lookupTable.AppendFormat("{};RenderPipelines/{}.xiiBinRenderPipeline\n", identity, name);
    const auto authoring = loaded->GetAuthoring();
    XII_TEST_INT(authoring->m_Descriptor.m_Settings.m_uiOutputSamples, 1);
    XII_TEST_BOOL(authoring->m_Descriptor.m_Settings.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::Compute));
    XII_TEST_BOOL(authoring->m_Descriptor.m_Id == document->GetGuid());
    XII_TEST_INT(authoring->m_Descriptor.m_Passes.GetCount(), passCount);
    XII_TEST_INT(authoring->m_Descriptor.m_Connections.GetCount(), internalConnectionCount);
    for (const auto& pass : authoring->m_Descriptor.m_Passes)
      XII_TEST_BOOL(nodes->GetObject(pass.m_Id) != nullptr);
    xiiContiguousMemoryStreamStorage rebuilt, repeated;
    if (!ReadBytes(message.m_sOutputFile, rebuilt))
      return;
    // Exercise the Editor's save/load path, not just a second export of the same
    // in-memory graph. Interface refresh must retain typed resource connections.
    const xiiStringBuilder roundTripPath(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/RoundTrip/", name, ".xiiRenderPipelineAsset");
    document->DocumentRenamed(roundTripPath);
    const auto saved = document->SaveDocument(true);
    XII_TEST_STATUS(saved);
    if (saved.Failed())
      return;
    PipelineLibraryDocument restored(roundTripPath);
    const auto              reloaded = restored.LoadDocument();
    XII_TEST_STATUS(reloaded);
    if (reloaded.Failed())
      return;
    PipelineLibraryContext roundTripContext;
    if (!XII_TEST_RESULT(roundTripContext.Import(restored)))
      return;
    exported = roundTripContext.ExportDocument(&message);
    XII_TEST_STATUS(exported);
    if (exported.Failed() || !ReadBytes(message.m_sOutputFile, repeated))
      return;
    XII_TEST_BOOL(xiiMakeArrayPtr(rebuilt.GetData(), rebuilt.GetStorageSize32()) == xiiMakeArrayPtr(repeated.GetData(), repeated.GetStorageSize32()));
  }
  // Standalone applications use the normal file-system asset redirection format
  // to resolve this library's GUID references without test-process registrations.
  xiiFileWriter          lookup;
  const xiiStringBuilder path(xiiFileSystem::GetSdkRootDirectory(), "/Data/Base/AssetCache/Common/RenderPipelines/PipelineLibrary.xiiAidlt");
  if (XII_TEST_RESULT(lookup.Open(path)))
    XII_TEST_RESULT(lookup.WriteBytes(lookupTable.GetData(), lookupTable.GetElementCount()));
}

XII_CREATE_SIMPLE_TEST(NodeLayout, CanonicalScene)
{
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "PipelineLayoutTest", "PipelineLayout", xiiDataDirUsage::ReadOnly)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PipelineLayoutTest"));
  const auto                                 sceneResource = xiiLoadPipelineLibraryResource("Scene");
  xiiResourceLock<xiiRenderPipelineResource> sceneDefinition(sceneResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(sceneDefinition.IsValid() && sceneDefinition->GetPipeline()))
    return;
  PipelineLibraryDocument document(xiiStringBuilder(xiiFileSystem::GetSdkRootDirectory(), "/Data/Base/RenderPipelines/Scene.xiiRenderPipelineAsset"));
  XII_TEST_STATUS(document.LoadDocument());
  auto*          manager = static_cast<xiiDocumentNodeManager*>(document.GetObjectManager());
  xiiQtNodeScene scene;
  scene.InitScene(manager);
  XII_TEST_STATUS(scene.ArrangeNodes(xiiQtNodeScene::NodeArrangement::Format, true));
  xiiDynamicArray<QRectF>                   bounds;
  xiiDynamicArray<xiiVec2>                  positions;
  xiiDynamicArray<const xiiDocumentObject*> objects;
  for (auto* item : scene.items())
  {
    if (item->type() != xiiQtNodeScene::Node)
      continue;
    auto* node = static_cast<xiiQtNode*>(item);
    bounds.PushBack(node->mapRectToScene(node->boundingRect().united(node->childrenBoundingRect())));
    objects.PushBack(node->GetObject());
    positions.PushBack(manager->GetNodePos(node->GetObject()));
  }
  XII_TEST_BOOL(!bounds.IsEmpty());
  for (xiiUInt32 i = 0; i < bounds.GetCount(); ++i)
    for (xiiUInt32 j = 0; j < i; ++j)
      XII_TEST_BOOL(!bounds[i].intersects(bounds[j]));
  const auto count = document.GetCommandHistory()->GetUndoStackSize();
  XII_TEST_STATUS(scene.ArrangeNodes(xiiQtNodeScene::NodeArrangement::Format, true));
  XII_TEST_INT(document.GetCommandHistory()->GetUndoStackSize(), count);
  for (xiiUInt32 i = 0; i < objects.GetCount(); ++i)
    XII_TEST_BOOL(manager->GetNodePos(objects[i]).IsEqual(positions[i], 0.01f));
  QImage image(3000, 1800, QImage::Format_ARGB32_Premultiplied);
  image.fill(QColor(32, 34, 38));
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  scene.render(&painter, QRectF(20, 20, 2960, 1760), scene.itemsBoundingRect().adjusted(-30, -30, 30, 30), Qt::KeepAspectRatio);
  painter.end();
  XII_TEST_BOOL(image.save(xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/NodeLayout-Scene.png").GetData()));
}
