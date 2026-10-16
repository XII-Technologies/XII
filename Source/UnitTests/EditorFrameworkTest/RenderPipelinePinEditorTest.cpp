/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QGraphicsSceneMouseEvent>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class xiiEditorPinTestPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorPinTestPass, xiiRenderPipelinePass);

public:
  bool m_bExtra = false;
  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Input", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"Output", xiiRenderPipelinePin::Texture, false});
    if (m_bExtra)
      pins.PushBack({"Extra", xiiRenderPipelinePin::Texture, true, true});
  }
  xiiResult Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorPinTestPass, 1, xiiRTTIDefaultAllocator<xiiEditorPinTestPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Extra", m_bExtra),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class PinDocument : public xiiRenderPipelineAssetDocument
  {
  public:
    explicit PinDocument(xiiStringView path) : xiiRenderPipelineAssetDocument(path, xiiAssetDocEngineConnection::None)
    {
      m_Type.m_sDocumentTypeName  = "RenderPipeline";
      m_Type.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform;
      m_pTypeDescriptor           = &m_Type;
      m_pDocumentInfo             = CreateDocumentInfo();
    }

  protected:
    void InternalAfterSaveDocument() override {} // Offline fixture has no active project curator.
  private:
    xiiAssetDocumentTypeDescriptor m_Type;
  };
  class PinScene : public xiiQtNodeScene
  {
  public:
    void Click(const xiiPin& native)
    {
      xiiQtPin* visual = nullptr;
      for (auto* item : items())
        if (item->type() == xiiQtNodeScene::Pin && static_cast<xiiQtPin*>(item)->GetPin() == &native)
          visual = static_cast<xiiQtPin*>(item);
      if (!XII_TEST_BOOL(visual != nullptr))
        return;
      QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
      press.setButton(Qt::LeftButton);
      press.setScenePos(visual->GetPinPos());
      press.setScreenPos(QPoint(100, 100));
      mousePressEvent(&press);
      QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
      release.setButton(Qt::LeftButton);
      release.setScenePos(visual->GetPinPos());
      release.setScreenPos(QPoint(100, 100));
      release.setButtonDownScreenPos(Qt::LeftButton, QPoint(100, 100));
      mouseReleaseEvent(&release);
    }
  };
  class PinCompiler : public xiiRenderPipelineContext
  {
  public:
    ~PinCompiler()
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
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, RenderPinProperties)
{
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "RenderPinEditorTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("RenderPinEditorTest"));
  xiiStringBuilder path(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/Pins.xiiRenderPipelineAsset");
  PinDocument      document(path);
  auto*            manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
  auto*            accessor = document.GetObjectAccessor();
  xiiUuid          nodes[2];
  accessor->StartTransaction("Create reflected passes");
  for (auto& id : nodes)
  {
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiEditorPinTestPass>(), id));
  }
  accessor->FinishTransaction();
  const auto* source = accessor->GetObject(nodes[0]);
  const auto* target = accessor->GetObject(nodes[1]);
  PinScene    scene;
  scene.InitScene(manager);
  xiiQtPropertyGridWidget grid(nullptr, &document);
  const auto*             pin = manager->GetInputPinByName(target, "Input");
  if (!XII_TEST_BOOL(pin != nullptr))
    return;
  const auto* settings = manager->GetPinPropertyObject(*pin);
  if (!XII_TEST_BOOL(settings && settings->GetType()->IsDerivedFrom<xiiRenderPipelinePinConfiguration>()))
    return;
  const auto  settingsId   = settings->GetGuid();
  const auto* requirements = accessor->GetChildObjectByName(settings, "Requirements", {});
  if (!XII_TEST_BOOL(requirements != nullptr))
    return;
  scene.Click(*pin);
  XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == settings);
  bool formatWidget = false;
  for (const auto* widget : grid.findChildren<xiiQtPropertyWidget*>())
    formatWidget |= widget->GetProperty() && widget->GetProperty()->GetPropertyName() == "RequiredFormat";
  XII_TEST_BOOL(formatWidget);
  accessor->StartTransaction("Set reflected pin requirements");
  XII_TEST_STATUS(accessor->SetValueByName(settings, "RequiredFormat", static_cast<xiiInt64>(xiiGALResourceFormat::RGBA16Float)));
  XII_TEST_STATUS(accessor->SetValueByName(requirements, "Width", 16U));
  accessor->FinishTransaction();
  XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == settings);
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  XII_TEST_INT(requirements->GetTypeAccessor().GetValue("Width").Get<xiiUInt32>(), 0);
  XII_TEST_STATUS(document.GetCommandHistory()->Redo());
  XII_TEST_INT(requirements->GetTypeAccessor().GetValue("Width").Get<xiiUInt32>(), 16);
  accessor->StartTransaction("Connect reflected pass resources");
  XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(),
                                                        *manager->GetOutputPinByName(source, "Output"), *manager->GetInputPinByName(target, "Input")));
  accessor->FinishTransaction();
  const auto connectionId = manager->GetConnections(*manager->GetInputPinByName(target, "Input"))[0]->GetParent()->GetGuid();
  accessor->StartTransaction("Edit connected nested requirements");
  XII_TEST_STATUS(accessor->SetValueByName(requirements, "Samples", 1U));
  accessor->FinishTransaction();
  XII_TEST_BOOL(manager->GetConnections(*manager->GetInputPinByName(target, "Input"))[0]->GetParent()->GetGuid() == connectionId);
  const auto* sourceSettings     = manager->GetPinPropertyObject(*manager->GetOutputPinByName(source, "Output"));
  const auto* sourceRequirements = accessor->GetChildObjectByName(sourceSettings, "Requirements", {});
  accessor->StartTransaction("Reject incompatible connected extents");
  XII_TEST_STATUS(accessor->SetValueByName(sourceRequirements, "Width", 32U));
  accessor->FinishTransaction();
  XII_TEST_BOOL(manager->GetConnections(*manager->GetInputPinByName(target, "Input")).IsEmpty());
  xiiDocumentNodeManager::CanConnectResult canConnect;
  XII_TEST_BOOL(manager->CanConnect(manager->GetConnectionType(), *manager->GetOutputPinByName(source, "Output"),
                                    *manager->GetInputPinByName(target, "Input"), canConnect)
                  .Failed());
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  XII_TEST_BOOL(manager->GetConnections(*manager->GetInputPinByName(target, "Input"))[0]->GetParent()->GetGuid() == connectionId);
  accessor->StartTransaction("Add dynamic pin");
  XII_TEST_STATUS(accessor->SetValueByName(target, "Extra", true));
  accessor->FinishTransaction();
  const auto* extra = manager->GetInputPinByName(target, "Extra");
  if (!XII_TEST_BOOL(extra && manager->GetPinPropertyObject(*extra)))
    return;
  const auto extraId = manager->GetPinPropertyObject(*extra)->GetGuid();
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  XII_TEST_BOOL(!manager->GetInputPinByName(target, "Extra") && !manager->GetObject(extraId));
  XII_TEST_STATUS(document.GetCommandHistory()->Redo());
  XII_TEST_BOOL(manager->GetPinPropertyObject(*manager->GetInputPinByName(target, "Extra"))->GetGuid() == extraId);
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy and paste remap persistent pin objects with their authored requirements")
  {
    document.GetSelectionManager()->SetSelection(target);
    xiiAbstractObjectGraph graph;
    XII_TEST_BOOL(manager->CopySelectedObjects(graph));
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            writer(&storage);
    xiiAbstractGraphDdlSerializer::Write(writer, &graph);
    xiiPasteObjectsCommand paste;
    paste.m_sMimeType        = "application/xiiEditor.RenderPipelineGraph";
    paste.m_sGraphTextFormat = xiiStringView(reinterpret_cast<const char*>(storage.GetData()), storage.GetStorageSize32());
    accessor->StartTransaction("Duplicate configured pass");
    XII_TEST_STATUS(document.GetCommandHistory()->AddCommand(paste));
    accessor->FinishTransaction();
    const auto* copy = document.GetSelectionManager()->GetCurrentObject();
    if (!XII_TEST_BOOL(copy && copy != target))
      return;
    const auto* copied = manager->GetPinPropertyObject(*manager->GetInputPinByName(copy, "Input"));
    if (!XII_TEST_BOOL(copied && copied->GetGuid() != settingsId))
      return;
    XII_TEST_BOOL(copied->GetTypeAccessor().GetValue("Id").Get<xiiUuid>() == copied->GetGuid());
    const auto* copiedRequirements = accessor->GetChildObjectByName(copied, "Requirements", {});
    XII_TEST_INT(copiedRequirements->GetTypeAccessor().GetValue("Width").Get<xiiUInt32>(), 16);
  }
  XII_TEST_STATUS(document.SaveDocument(true));
  PinDocument loaded(path);
  XII_TEST_STATUS(loaded.LoadDocument());
  auto*       loadedManager  = static_cast<xiiRenderPipelineNodeManager*>(loaded.GetObjectManager());
  const auto* loadedSettings = loadedManager->GetPinPropertyObject(*loadedManager->GetInputPinByName(loadedManager->GetObject(nodes[1]), "Input"));
  XII_TEST_BOOL(loadedSettings && loadedSettings->GetGuid() == settingsId);
  XII_TEST_BOOL(loadedManager->GetObject(connectionId) != nullptr);
  PinCompiler compiler;
  XII_TEST_RESULT(compiler.Import(loaded));
  xiiExportDocumentMsgToEngine message;
  message.m_sOutputFile = xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/Pins.xiiBinRenderPipeline");
  message.m_uiAssetHash = 1;
  message.m_uiVersion   = 6;
  const auto exported   = compiler.ExportDocument(&message);
  XII_TEST_STATUS(exported);
  if (exported.Failed())
    return;
  const auto                                 handle = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
  xiiResourceLock<xiiRenderPipelineResource> resource(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(resource && resource->GetPipeline()))
    return;
  bool found = false;
  for (xiiUInt32 i = 0; i < resource->GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
    for (const auto& compiledPin : resource->GetPipeline()->GetPins(i))
      if (compiledPin.m_SourceId == settingsId)
      {
        XII_TEST_INT(compiledPin.m_Constraints.m_uiWidth, 16);
        XII_TEST_INT(compiledPin.m_Constraints.m_uiSamples, 1);
        XII_TEST_BOOL(compiledPin.m_Format == xiiGALResourceFormat::RGBA16Float);
        found = true;
      }
  XII_TEST_BOOL(found);
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pin contract hot reload replaces compiled snapshots at instance synchronization")
  {
    const auto                original = resource->GetPipeline();
    xiiRenderPipelineInstance first, second;
    XII_TEST_RESULT(first.Initialize(original));
    XII_TEST_RESULT(second.Initialize(original));
    const auto width = [&](const xiiRenderPipeline& pipeline) {
      for (xiiUInt32 i = 0; i < pipeline.GetDescriptor().m_Passes.GetCount(); ++i)
        for (const auto& compiledPin : pipeline.GetPins(i))
          if (compiledPin.m_SourceId == settingsId)
            return compiledPin.m_Constraints.m_uiWidth;
      return xiiInvalidIndex;
    };
    auto*       loadedAccessor     = loaded.GetObjectAccessor();
    const auto* loadedRequirements = loadedAccessor->GetChildObjectByName(loadedSettings, "Requirements", {});
    loadedAccessor->StartTransaction("Change loaded pin contract");
    XII_TEST_STATUS(loadedAccessor->SetValueByName(loadedRequirements, "Width", 24U));
    loadedAccessor->FinishTransaction();
    PinCompiler replacement;
    XII_TEST_RESULT(replacement.Import(loaded));
    const auto updated = replacement.ExportDocument(&message);
    XII_TEST_STATUS(updated);
    if (updated.Failed())
      return;
    XII_TEST_BOOL(xiiResourceManager::ReloadResource(handle, true));
    bool changed = false;
    XII_TEST_RESULT(first.Synchronize(handle, &changed));
    XII_TEST_BOOL(changed && first.GetPipeline() != original);
    XII_TEST_INT(width(*first.GetPipeline()), 24);
    XII_TEST_INT(width(*second.GetPipeline()), 16);
    XII_TEST_RESULT(second.Synchronize(handle));
    XII_TEST_BOOL(first.GetPipeline() == second.GetPipeline());
    XII_TEST_INT(width(*original), 16);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Documents without pin objects initialize stable identities without changing connections")
  {
    xiiUniquePtr<xiiAbstractObjectGraph> header, objects, types;
    XII_TEST_STATUS(xiiDocument::ReadDocument(path, header, objects, types));
    xiiDynamicArray<xiiUuid> obsolete;
    for (const auto& entry : objects->GetAllNodes())
    {
      auto* node = entry.Value();
      if (node->GetType() == "xiiEditorPinTestPass")
        node->RemoveProperty("PinConfigurations");
      if (node->GetType() == "xiiRenderPipelinePinConfiguration" || node->GetType() == "xiiRenderPipelinePinMetadata" || node->GetType() == "xiiRenderPipelinePinConstraints")
        obsolete.PushBack(node->GetGuid());
    }
    for (const auto& id : obsolete)
      objects->RemoveNode(id);
    const xiiStringBuilder legacyPath(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/LegacyPins.xiiRenderPipelineAsset");
    {
      xiiFileWriter writer;
      if (!XII_TEST_RESULT(writer.Open(legacyPath)))
        return;
      xiiAbstractGraphDdlSerializer::WriteDocument(writer, header.Borrow(), objects.Borrow(), types.Borrow());
    }
    PinDocument legacy(legacyPath);
    XII_TEST_STATUS(legacy.LoadDocument());
    auto*       legacyManager = static_cast<xiiRenderPipelineNodeManager*>(legacy.GetObjectManager());
    const auto* legacyPin     = legacyManager->GetInputPinByName(legacyManager->GetObject(nodes[1]), "Input");
    if (!XII_TEST_BOOL(legacyPin != nullptr))
      return;
    const auto* initialized = legacyManager->GetPinPropertyObject(*legacyPin);
    XII_TEST_BOOL(initialized && initialized->GetGuid() == settingsId);
    XII_TEST_BOOL(legacyManager->GetConnections(*legacyPin)[0]->GetParent()->GetGuid() == connectionId);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Subgraph interface properties survive expansion and undo without losing leaf settings")
  {
    xiiRenderPipelineResourceDescriptor child;
    xiiVariantDictionary                leafProperties;
    leafProperties.Insert("Extra", true);
    child.AddPass("Leaf", "xiiEditorPinTestPass", leafProperties);
    child.m_Interface.PushBack({"Output", false, child.m_Passes[0].m_Id, "Output"});
    const auto  reusable = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("RenderPinEditorReusable", std::move(child));
    PinDocument composition(xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/Composition.xiiRenderPipelineAsset"));
    auto*       objects  = static_cast<xiiRenderPipelineNodeManager*>(composition.GetObjectManager());
    auto*       commands = composition.GetObjectAccessor();
    xiiUuid     instanceId;
    commands->StartTransaction("Reference reusable graph");
    XII_TEST_STATUS(commands->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiRenderPipelineSubgraphPass>(), instanceId));
    const auto* instance = commands->GetObject(instanceId);
    XII_TEST_STATUS(commands->SetValueByName(instance, "Resource", reusable.GetResourceID()));
    const auto* boundary = objects->GetPinPropertyObject(*objects->GetOutputPinByName(instance, "Output"));
    if (!XII_TEST_BOOL(boundary != nullptr))
      return;
    const auto  boundaryId = boundary->GetGuid();
    const auto* limits     = commands->GetChildObjectByName(boundary, "Requirements", {});
    XII_TEST_STATUS(commands->SetValueByName(limits, "Width", 32U));
    commands->FinishTransaction();
    XII_TEST_STATUS(composition.ExpandSubgraph(instance));
    XII_TEST_BOOL(!objects->GetObject(instanceId));
    const xiiDocumentObject* expanded = nullptr;
    for (const auto* object : objects->GetRootObject()->GetChildren())
      if (object->GetType()->IsDerivedFrom<xiiEditorPinTestPass>())
        expanded = object;
    if (!XII_TEST_BOOL(expanded != nullptr))
      return;
    XII_TEST_BOOL(expanded->GetTypeAccessor().GetValue("Extra").Get<bool>());
    const auto* expandedSettings = objects->GetPinPropertyObject(*objects->GetOutputPinByName(expanded, "Output"));
    if (!XII_TEST_BOOL(expandedSettings != nullptr))
      return;
    const auto* expandedLimits = commands->GetChildObjectByName(expandedSettings, "Requirements", {});
    XII_TEST_INT(expandedLimits->GetTypeAccessor().GetValue("Width").Get<xiiUInt32>(), 32);
    XII_TEST_BOOL(expandedSettings->GetTypeAccessor().GetValue("Id").Get<xiiUuid>() == expandedSettings->GetGuid());
    XII_TEST_STATUS(composition.GetCommandHistory()->Undo());
    XII_TEST_BOOL(objects->GetObject(instanceId) && objects->GetObject(boundaryId));
    XII_TEST_STATUS(composition.GetCommandHistory()->Redo());
    XII_TEST_BOOL(!objects->GetObject(instanceId));
  }
}
