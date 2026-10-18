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
#include <GraphicsCore/Pipeline/Passes/RayTracing/RayTracingScenePass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadePass.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowRasterWorkPass.h>
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
  bool                              m_bExtra = false;
  bool                              m_bValid = true;
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin m_Output;
  xiiRenderPipelineTextureInputPin  m_Extra{xiiGALResourceFormat::Unknown, true};
  xiiResult                         Configure(const xiiVariantDictionary& properties) override
  {
    XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
    return m_bValid ? XII_SUCCESS : XII_FAILURE;
  }
  xiiResult Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorPinTestPass, 1, xiiRTTIDefaultAllocator<xiiEditorPinTestPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Extra", m_bExtra),
      XII_MEMBER_PROPERTY("Valid", m_bValid)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("Input", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Output", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("ExtraPin", m_Extra)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Extra"), new xiiRenderPipelinePinEnabledAttribute("Extra")),
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
  XII_TEST_STATUS(document.EnsureSettingsObject());
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
  const auto*             pipelineSettings = document.GetSettingsObject();
  if (!XII_TEST_BOOL(pipelineSettings != nullptr))
    return;
  document.GetSelectionManager()->SetSelection(pipelineSettings);
  bool outputWidget = false;
  for (const auto* widget : grid.findChildren<xiiQtPropertyWidget*>())
    outputWidget |= widget->GetProperty() && widget->GetProperty()->GetPropertyName() == "OutputFormat";
  XII_TEST_BOOL(outputWidget);
  accessor->StartTransaction("Configure pipeline output");
  XII_TEST_STATUS(accessor->SetValueByName(pipelineSettings, "OutputFormat", static_cast<xiiInt64>(xiiGALResourceFormat::RGBA8UNormalized)));
  XII_TEST_STATUS(accessor->SetValueByName(pipelineSettings, "OutputSamples", 1U));
  accessor->FinishTransaction();
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  XII_TEST_INT(pipelineSettings->GetTypeAccessor().GetValue("OutputSamples").Get<xiiUInt32>(), 0);
  XII_TEST_STATUS(document.GetCommandHistory()->Redo());
  document.GetSelectionManager()->SetSelection(target);
  bool nodePinProperties = false;
  for (const auto* widget : grid.findChildren<xiiQtPropertyWidget*>())
    nodePinProperties |= widget->GetProperty() && widget->GetProperty()->GetPropertyName() == "PinConfigurations";
  XII_TEST_BOOL(nodePinProperties);
  const auto* pinProperty = target->GetType()->FindPropertyByName("PinConfigurations");
  XII_TEST_BOOL(pinProperty && !pinProperty->GetAttributeByType<xiiHiddenAttribute>());
  const auto* pin = manager->GetInputPinByName(target, "Input");
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
  XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *manager->GetOutputPinByName(source, "Output"), *manager->GetInputPinByName(target, "Input")));
  accessor->FinishTransaction();
  const auto connectionId = manager->GetConnections(*manager->GetInputPinByName(target, "Input"))[0]->GetParent()->GetGuid();
  accessor->StartTransaction("Temporarily invalidate pass configuration");
  XII_TEST_STATUS(accessor->SetValueByName(target, "Valid", false));
  accessor->FinishTransaction();
  XII_TEST_BOOL(manager->GetPinPropertyObject(*manager->GetInputPinByName(target, "Input"))->GetGuid() == settingsId);
  XII_TEST_BOOL(manager->GetConnections(*manager->GetInputPinByName(target, "Input"))[0]->GetParent()->GetGuid() == connectionId);
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
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
  XII_TEST_BOOL(manager->CanConnect(manager->GetConnectionType(), *manager->GetOutputPinByName(source, "Output"), *manager->GetInputPinByName(target, "Input"), canConnect).Failed());
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
  XII_TEST_BOOL(resource->GetPipeline()->GetDescriptor().m_Settings.m_OutputFormat == xiiGALResourceFormat::RGBA8UNormalized);
  XII_TEST_INT(resource->GetPipeline()->GetDescriptor().m_Settings.m_uiOutputSamples, 1);
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
    XII_TEST_STATUS(loadedAccessor->SetValueByName(loaded.GetSettingsObject(), "OutputFormat", static_cast<xiiInt64>(xiiGALResourceFormat::BGRA8UNormalized)));
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
    XII_TEST_BOOL(first.GetPipeline()->GetDescriptor().m_Settings.m_OutputFormat == xiiGALResourceFormat::BGRA8UNormalized);
    XII_TEST_BOOL(second.GetPipeline()->GetDescriptor().m_Settings.m_OutputFormat == xiiGALResourceFormat::RGBA8UNormalized);
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
      if (auto* children = node->FindProperty("Children"); children && children->m_Value.IsA<xiiVariantArray>())
      {
        auto entries = children->m_Value.Get<xiiVariantArray>();
        for (xiiUInt32 i = entries.GetCount(); i > 0; --i)
          if (entries[i - 1] == xiiVariant(pipelineSettings->GetGuid()))
            entries.RemoveAtAndCopy(i - 1);
        node->ChangeProperty("Children", entries);
      }
      if (node->GetType() == "xiiRenderPipelineSettings")
        obsolete.PushBack(node->GetGuid());
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
    XII_TEST_BOOL(legacy.GetSettingsObject() && legacy.GetSettingsObject()->GetGuid() == pipelineSettings->GetGuid());
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

class xiiEditorRayPinConsumer : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorRayPinConsumer, xiiRenderPipelinePass);

public:
  xiiRenderPipelineAccelerationStructureInputPin m_Scene{true};
  xiiRenderPipelineTextureInputPin               m_Texture{xiiGALResourceFormat::Unknown, true};
  xiiResult                                      Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorRayPinConsumer, 1, xiiRTTIDefaultAllocator<xiiEditorRayPinConsumer>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Scene", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Texture", m_Texture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(EditorDocuments, AccelerationStructurePinProperties)
{
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "RayPinEditorTest", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("RayPinEditorTest"));
  xiiStringBuilder path(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/RayPins.xiiRenderPipelineAsset");
  PinDocument      document(path);
  auto*            manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
  auto*            accessor = document.GetObjectAccessor();
  accessor->StartTransaction("Create reflected ray scene nodes");
  XII_TEST_STATUS(document.EnsureSettingsObject());
  xiiUuid sourceId, targetId;
  XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiRayTracingScenePass>(), sourceId));
  XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiEditorRayPinConsumer>(), targetId));
  accessor->FinishTransaction();
  const auto* source = accessor->GetObject(sourceId);
  const auto* target = accessor->GetObject(targetId);
  PinScene    scene;
  scene.InitScene(manager);
  xiiQtPropertyGridWidget grid(nullptr, &document);
  document.GetSelectionManager()->SetSelection(source);
  bool selectionProperty = false, pinProperties = false;
  for (const auto* widget : grid.findChildren<xiiQtPropertyWidget*>())
    if (const auto* property = widget->GetProperty())
    {
      selectionProperty |= property->GetPropertyName() == "UseViewSelection";
      pinProperties |= property->GetPropertyName() == "PinConfigurations";
    }
  XII_TEST_BOOL(selectionProperty && pinProperties);
  const auto* output  = manager->GetOutputPinByName(source, "Scene");
  const auto* input   = manager->GetInputPinByName(target, "Scene");
  const auto* texture = manager->GetInputPinByName(target, "Texture");
  if (!XII_TEST_BOOL(output && input && texture))
    return;
  const auto* settings = manager->GetPinPropertyObject(*output);
  if (!XII_TEST_BOOL(settings != nullptr))
    return;
  const auto* metadata = accessor->GetChildObjectByName(settings, "Metadata", {});
  if (!XII_TEST_BOOL(metadata != nullptr))
    return;
  XII_TEST_INT(metadata->GetTypeAccessor().GetValue("ResourceType").ConvertTo<xiiUInt32>(), xiiRenderPipelinePin::AccelerationStructure);
  const auto settingsId = settings->GetGuid();
  scene.Click(*output);
  XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == settings);
  accessor->StartTransaction("Name acceleration structure pin");
  XII_TEST_STATUS(accessor->SetValueByName(settings, "DisplayName", "World ray structure"));
  accessor->FinishTransaction();
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  XII_TEST_BOOL(settings->GetTypeAccessor().GetValue("DisplayName").Get<xiiString>().IsEmpty());
  XII_TEST_STATUS(document.GetCommandHistory()->Redo());
  XII_TEST_STRING(settings->GetTypeAccessor().GetValue("DisplayName").Get<xiiString>(), "World ray structure");
  output  = manager->GetOutputPinByName(source, "Scene");
  input   = manager->GetInputPinByName(target, "Scene");
  texture = manager->GetInputPinByName(target, "Texture");
  if (!XII_TEST_BOOL(output && input && texture))
    return;
  xiiDocumentNodeManager::CanConnectResult canConnect;
  XII_TEST_STATUS(manager->CanConnect(manager->GetConnectionType(), *output, *input, canConnect));
  XII_TEST_BOOL(manager->CanConnect(manager->GetConnectionType(), *output, *texture, canConnect).Failed());
  const auto childCount = manager->GetRootObject()->GetChildren().GetCount();
  accessor->StartTransaction("Reject incompatible typed connection");
  XII_TEST_BOOL(xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *output, *texture).Failed());
  accessor->FinishTransaction();
  XII_TEST_INT(manager->GetRootObject()->GetChildren().GetCount(), childCount);
  accessor->StartTransaction("Connect typed ray scene");
  const auto connected = xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *output, *input);
  XII_TEST_STATUS(connected);
  if (connected.Failed())
  {
    accessor->CancelTransaction();
    return;
  }
  accessor->FinishTransaction();
  const auto saved = document.SaveDocument(true);
  XII_TEST_STATUS(saved);
  if (saved.Failed())
    return;
  PinDocument restored(path);
  const auto  loaded = restored.LoadDocument();
  XII_TEST_STATUS(loaded);
  if (loaded.Failed())
    return;
  auto*       objects     = static_cast<xiiRenderPipelineNodeManager*>(restored.GetObjectManager());
  const auto* restoredPin = objects->GetOutputPinByName(objects->GetObject(sourceId), "Scene");
  if (!XII_TEST_BOOL(restoredPin != nullptr))
    return;
  const auto* restoredSettings = objects->GetPinPropertyObject(*restoredPin);
  XII_TEST_BOOL(restoredSettings && restoredSettings->GetGuid() == settingsId);
  XII_TEST_STRING(restoredPin->GetDisplayName(), "World ray structure");
  PinCompiler compiler;
  if (!XII_TEST_RESULT(compiler.Import(restored)))
    return;
  xiiExportDocumentMsgToEngine message;
  message.m_sOutputFile = xiiStringBuilder(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/RayPins.xiiBinRenderPipeline");
  message.m_uiAssetHash = 1;
  message.m_uiVersion   = 7;
  const auto exported   = compiler.ExportDocument(&message);
  XII_TEST_STATUS(exported);
  if (exported.Failed())
    return;
  const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(message.m_sOutputFile);
  xiiResourceLock<xiiRenderPipelineResource> acquired(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(acquired.IsValid() && acquired->GetPipeline()))
    return;
  bool preserved = false;
  for (xiiUInt32 i = 0; i < acquired->GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
    for (const auto& pin : acquired->GetPipeline()->GetPins(i))
      if (pin.m_SourceId == settingsId)
      {
        preserved = true;
        XII_TEST_BOOL(pin.m_Type == xiiRenderPipelinePin::AccelerationStructure);
        XII_TEST_STRING(pin.m_sDisplayName, "World ray structure");
      }
  XII_TEST_BOOL(preserved);
}

XII_CREATE_SIMPLE_TEST(EditorDocuments, FrameDataPinInspector)
{
  const auto directory = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(directory, "FrameDataPinInspector", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("FrameDataPinInspector"));
  const xiiStringBuilder path(directory, "/FrameDataPins.xiiRenderPipelineAsset");
  PinDocument            document(path);
  auto*                  accessor = document.GetObjectAccessor();
  auto*                  manager  = static_cast<xiiRenderPipelineNodeManager*>(document.GetObjectManager());
  xiiUuid                sourceId, targetId;
  accessor->StartTransaction("Create typed CPU snapshot nodes");
  XII_TEST_STATUS(document.EnsureSettingsObject());
  XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiShadowCascadePass>(), sourceId));
  XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiVirtualShadowRasterWorkPass>(), targetId));
  accessor->FinishTransaction();
  const auto* source = accessor->GetObject(sourceId);
  const auto* target = accessor->GetObject(targetId);
  if (!XII_TEST_BOOL(source && target))
    return;
  const auto* output = manager->GetOutputPinByName(source, "CascadeSnapshot");
  const auto* input  = manager->GetInputPinByName(target, "Cascades");
  const auto* other  = manager->GetInputPinByName(target, "Residency");
  if (!XII_TEST_BOOL(output && input && other))
    return;
  const auto* configuration = manager->GetPinPropertyObject(*output);
  if (!XII_TEST_BOOL(configuration != nullptr))
    return;
  const auto* metadata = accessor->GetChildObjectByName(configuration, "Metadata", {});
  if (!XII_TEST_BOOL(configuration && metadata))
    return;
  XII_TEST_STRING(metadata->GetTypeAccessor().GetValue("DataType").Get<xiiString>(), "xiiShadowCascadeSnapshot");
  const auto* typeProperty = metadata->GetType()->FindPropertyByName("DataType");
  XII_TEST_BOOL(typeProperty && typeProperty->GetAttributeByType<xiiReadOnlyAttribute>());
  PinScene scene;
  scene.InitScene(manager);
  xiiQtPropertyGridWidget grid(nullptr, &document);
  document.GetSelectionManager()->SetSelection(source);
  bool pinProperties = false;
  for (const auto* widget : grid.findChildren<xiiQtPropertyWidget*>())
    pinProperties |= widget->GetProperty() && widget->GetProperty()->GetPropertyName() == "PinConfigurations";
  XII_TEST_BOOL(pinProperties);
  scene.Click(*output);
  XII_TEST_BOOL(document.GetSelectionManager()->GetCurrentObject() == configuration);
  xiiDocumentNodeManager::CanConnectResult result;
  XII_TEST_BOOL(manager->CanConnect(manager->GetConnectionType(), *output, *other, result).Failed());
  const auto id = configuration->GetGuid();
  accessor->StartTransaction("Name a typed snapshot pin");
  XII_TEST_STATUS(accessor->SetValueByName(configuration, "DisplayName", "Immutable cascade snapshot"));
  accessor->FinishTransaction();
  XII_TEST_STATUS(document.GetCommandHistory()->Undo());
  XII_TEST_STATUS(document.GetCommandHistory()->Redo());
  // Pin objects are recreated after property edits and undo/redo; document IDs persist.
  output = manager->GetOutputPinByName(source, "CascadeSnapshot");
  input  = manager->GetInputPinByName(target, "Cascades");
  if (!XII_TEST_BOOL(output && input))
    return;
  accessor->StartTransaction("Connect typed snapshots");
  XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document.GetCommandHistory(), manager->GetConnectionType(), *output, *input));
  accessor->FinishTransaction();
  XII_TEST_STATUS(document.SaveDocument(true));
  PinDocument restored(path);
  const auto  loaded = restored.LoadDocument();
  XII_TEST_STATUS(loaded);
  if (loaded.Failed())
    return;
  auto*       restoredManager = static_cast<xiiRenderPipelineNodeManager*>(restored.GetObjectManager());
  const auto* restoredOutput  = restoredManager->GetOutputPinByName(restoredManager->GetObject(sourceId), "CascadeSnapshot");
  if (!XII_TEST_BOOL(restoredOutput != nullptr))
    return;
  const auto* restoredConfig = restoredManager->GetPinPropertyObject(*restoredOutput);
  XII_TEST_BOOL(restoredConfig && restoredConfig->GetGuid() == id);
  XII_TEST_STRING(restoredOutput->GetDisplayName(), "Immutable cascade snapshot");
  XII_TEST_INT(restoredManager->GetConnections(*restoredOutput).GetCount(), 1);
  const auto* restoredMetadata = restored.GetObjectAccessor()->GetChildObjectByName(restoredConfig, "Metadata", {});
  XII_TEST_STRING(restoredMetadata->GetTypeAccessor().GetValue("DataType").Get<xiiString>(), "xiiShadowCascadeSnapshot");
}
