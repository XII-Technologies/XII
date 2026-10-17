/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineAssetDocument, 7, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class xiiPipelineCommandAccessor : public xiiNodeCommandAccessor
  {
  public:
    explicit xiiPipelineCommandAccessor(xiiCommandHistory* history) : xiiNodeCommandAccessor(history) {}
    xiiStatus AddObject(const xiiDocumentObject* parent, const xiiAbstractProperty* property, const xiiVariant& index, const xiiRTTI* type, xiiUuid& id) override
    {
      XII_SUCCEED_OR_RETURN(xiiNodeCommandAccessor::AddObject(parent, property, index, type, id));
      return Synchronize(GetObject(id));
    }
    xiiStatus SetValue(const xiiDocumentObject* object, const xiiAbstractProperty* property, const xiiVariant& value, xiiVariant index = {}) override
    {
      XII_SUCCEED_OR_RETURN(xiiNodeCommandAccessor::SetValue(object, property, value, index));
      return Synchronize(object);
    }

  private:
    bool      m_bSynchronizing = false;
    xiiStatus Synchronize(const xiiDocumentObject* object)
    {
      if (m_bSynchronizing || !object || !object->GetType()->IsDerivedFrom<xiiRenderPipelinePass>())
        return XII_SUCCESS;
      m_bSynchronizing = true;
      XII_SCOPE_EXIT(m_bSynchronizing = false);
      return static_cast<xiiRenderPipelineNodeManager*>(const_cast<xiiDocumentObjectManager*>(object->GetDocumentObjectManager()))->SynchronizePinConfigurations(object);
    }
  };

  xiiResult GetNativePins(const xiiDocumentObject* object, xiiDynamicArray<xiiRenderPipelinePin>& pins, bool configured)
  {
    const auto* type = xiiRTTI::FindTypeByName(object->GetType()->GetTypeName());
    if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate())
      return XII_FAILURE;
    xiiRenderPipelinePass* pass = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(pass));
    xiiAbstractObjectGraph           graph;
    xiiDocumentObjectConverterWriter writer(&graph, object->GetDocumentObjectManager());
    const auto*                      root = writer.AddObjectToGraph(object, "root");
    xiiRttiConverterContext          context;
    context.RegisterObject(root->GetGuid(), type, pass);
    xiiRttiConverterReader reader(&graph, &context);
    reader.ApplyPropertiesToObject(root, type, pass);
    // Preserve pins and connections while an authored property is temporarily invalid.
    // Configuration validation belongs to asset compilation, not editor pin discovery.
    if (configured && pass->GetConfiguredPins(pins).Succeeded())
      return XII_SUCCESS;
    // Keep invalid contracts visible so the inspector can repair them. The compiler
    // rejects them; native direction/kind/requiredness cannot be changed by assets.
    pass->GetPins(pins);
    return XII_SUCCESS;
  }

  class xiiPipelineEditorPin : public xiiPin
  {
  public:
    xiiPipelineEditorPin(const xiiRenderPipelinePin& pin, const xiiDocumentObject* object) :
      xiiPin(pin.m_bInput ? Type::Input : Type::Output, pin.m_sName, xiiColorScheme::DarkUI(pin.m_Type == xiiRenderPipelinePin::Texture ? xiiColorScheme::Blue : pin.m_Type == xiiRenderPipelinePin::Buffer ? xiiColorScheme::Green :
                                                                                              pin.m_Type == xiiRenderPipelinePin::AccelerationStructure                                                     ? xiiColorScheme::Violet :
                                                                                              pin.m_Type == xiiRenderPipelinePin::FrameData                                                                 ? xiiColorScheme::Orange :
                                                                                                                                                                                                              xiiColorScheme::Gray),
             object),
      m_Schema(pin)
    {
    }

    xiiRenderPipelinePin m_Schema;
    xiiStringView        GetDisplayName() const override { return m_Schema.m_sDisplayName.IsEmpty() ? m_Schema.m_sName.GetView() : m_Schema.m_sDisplayName.GetView(); }
  };
} // namespace

const xiiDocumentObject* xiiRenderPipelineNodeManager::GetPinPropertyObject(const xiiPin& pin) const
{
  for (const auto* child : pin.GetParent()->GetChildren())
  {
    if (child->GetParentProperty() != "PinConfigurations")
      continue;
    const auto  metadataId = child->GetTypeAccessor().GetValue("Metadata");
    const auto* metadata   = metadataId.IsA<xiiUuid>() ? GetObject(metadataId.Get<xiiUuid>()) : nullptr;
    if (metadata && metadata->GetTypeAccessor().GetValue("Name") == xiiVariant(pin.GetName()) &&
        metadata->GetTypeAccessor().GetValue("Input") == xiiVariant(pin.GetType() == xiiPin::Type::Input))
      return child;
  }
  return nullptr;
}

xiiStatus xiiRenderPipelineNodeManager::SynchronizePinConfigurations(const xiiDocumentObject* node)
{
  if (!node || !InternalIsNode(node))
    return XII_SUCCESS;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  if (GetNativePins(node, pins, false).Failed() || pins.GetCount() > 256)
    return xiiStatus("Cannot create persistent properties for the native pass pins.");
  auto*      accessor = GetDocument()->GetObjectAccessor();
  const auto set      = [&](const xiiDocumentObject* object, xiiStringView property, const xiiVariant& value) -> xiiStatus {
    if (object->GetTypeAccessor().GetValue(property) == value)
      return XII_SUCCESS;
    return accessor->SetValueByName(object, property, value);
  };
  xiiHashSet<xiiUuid> retained;
  for (const auto& pin : pins)
  {
    const xiiDocumentObject* configuration = nullptr;
    for (const auto* child : node->GetChildren())
    {
      if (child->GetParentProperty() != "PinConfigurations")
        continue;
      const auto* metadata = accessor->GetChildObjectByName(child, "Metadata", {});
      if (metadata && metadata->GetTypeAccessor().GetValue("Name") == xiiVariant(pin.m_sName) && metadata->GetTypeAccessor().GetValue("Input") == xiiVariant(pin.m_bInput))
      {
        configuration = child;
        break;
      }
    }
    if (!configuration)
    {
      xiiStringBuilder key(pin.m_bInput ? "Input/" : "Output/", pin.m_sName);
      auto             id = xiiUuid::MakeStableUuidFromString(key);
      id.HashCombine(node->GetGuid());
      XII_SUCCEED_OR_RETURN(accessor->AddObjectByName(node, "PinConfigurations", -1, xiiGetStaticRTTI<xiiRenderPipelinePinConfiguration>(), id));
      configuration = accessor->GetObject(id);
    }
    retained.Insert(configuration->GetGuid());
    XII_SUCCEED_OR_RETURN(set(configuration, "Id", configuration->GetGuid()));
    const auto  metadata = pin.GetMetadata();
    const auto* object   = accessor->GetChildObjectByName(configuration, "Metadata", {});
    if (!object)
      return xiiStatus("Pin configuration has no reflected schema object.");
    XII_SUCCEED_OR_RETURN(set(object, "Name", metadata.m_sName));
    XII_SUCCEED_OR_RETURN(set(object, "Input", metadata.m_bInput));
    XII_SUCCEED_OR_RETURN(set(object, "ResourceType", static_cast<xiiInt64>(metadata.m_Type.GetValue())));
    XII_SUCCEED_OR_RETURN(set(object, "Access", static_cast<xiiInt64>(metadata.m_Access.GetValue())));
    XII_SUCCEED_OR_RETURN(set(object, "Cardinality", static_cast<xiiInt64>(metadata.m_Cardinality.GetValue())));
    XII_SUCCEED_OR_RETURN(set(object, "Optional", metadata.m_bOptional));
    XII_SUCCEED_OR_RETURN(set(object, "TemporalHistory", metadata.m_bTemporalHistory));
    XII_SUCCEED_OR_RETURN(set(object, "DeclaredFormat", static_cast<xiiInt64>(metadata.m_DeclaredFormat.GetValue())));
    XII_SUCCEED_OR_RETURN(set(object, "DataType", metadata.m_sDataType));
  }
  xiiDynamicArray<const xiiDocumentObject*> obsolete;
  for (const auto* child : node->GetChildren())
    if (child->GetParentProperty() == "PinConfigurations" && !retained.Contains(child->GetGuid()))
      obsolete.PushBack(child);
  for (const auto* child : obsolete)
    XII_SUCCEED_OR_RETURN(accessor->RemoveObject(child));
  return XII_SUCCESS;
}

void xiiRenderPipelineNodeManager::GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const
{
  out_templates.Clear();
  xiiHybridArray<const xiiRTTI*, 32> types;
  GetCreateableTypes(types);
  for (const auto* type : types)
  {
    auto& entry       = out_templates.ExpandAndGetRef();
    entry.m_pType     = type;
    entry.m_sTypeName = type->GetTypeName();
    if (const auto* category = type->GetAttributeByType<xiiCategoryAttribute>())
      entry.m_sCategory.Assign(category->GetCategory());
  }
}

bool xiiRenderPipelineNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  return pObject->GetTypeAccessor().GetType()->IsDerivedFrom<xiiRenderPipelinePass>();
}

bool xiiRenderPipelineNodeManager::InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const
{
  // A reflected setting may change the native pass schema. The node manager handles pin recreation.
  return InternalIsNode(pObject);
}

void xiiRenderPipelineNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node)
{
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  if (GetNativePins(pObject, pins, true).Failed())
    return;
  for (const auto& pin : pins)
  {
    auto editorPin = XII_DEFAULT_NEW(xiiPipelineEditorPin, pin, pObject);
    if (pin.m_bInput)
      ref_node.m_Inputs.PushBack(std::move(editorPin));
    else
      ref_node.m_Outputs.PushBack(std::move(editorPin));
  }
}

void xiiRenderPipelineNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const
{
  xiiSet<const xiiRTTI*> types;
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiRenderPipelinePass>(), types);
  ref_types.Clear();
  for (const auto* type : types)
    if (!type->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract) && type->GetAllocator()->CanAllocate())
      ref_types.PushBack(type);
}

bool xiiRenderPipelineNodeManager::InternalCanConnectToSelf(const xiiPin& source, const xiiPin&) const
{
  return static_cast<const xiiPipelineEditorPin&>(source).m_Schema.m_bTemporalHistoryOutput;
}

xiiStatus xiiRenderPipelineNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  out_result         = CanConnectResult::ConnectNever;
  const auto& output = static_cast<const xiiPipelineEditorPin&>(source).m_Schema;
  const auto& input  = static_cast<const xiiPipelineEditorPin&>(target).m_Schema;
  if (!output.IsCompatibleWith(input))
    return xiiStatus("Render pipeline pin types, formats, dimensions or sample requirements do not match.");
  if (!output.m_bTemporalHistoryOutput)
  {
    xiiDynamicArray<const xiiDocumentObject*> pending;
    xiiHashSet<const xiiDocumentObject*>      visited;
    pending.PushBack(target.GetParent());
    while (!pending.IsEmpty())
    {
      const auto* node = pending.PeekBack();
      pending.PopBack();
      if (node == source.GetParent())
        return xiiStatus("Render pipeline contains an intra-frame dependency cycle. Use explicit temporal history output pins for previous-frame feedback.");
      if (visited.Contains(node))
        continue;
      visited.Insert(node);
      for (const auto& pin : GetOutputPins(node))
      {
        if (static_cast<const xiiPipelineEditorPin*>(pin.Borrow())->m_Schema.m_bTemporalHistoryOutput)
          continue;
        for (const auto* connection : GetConnections(*pin))
          pending.PushBack(connection->GetTargetPin().GetParent());
      }
    }
  }
  out_result = input.m_Type == xiiRenderPipelinePin::Order ? CanConnectResult::ConnectNtoN : CanConnectResult::ConnectNto1;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

xiiRenderPipelineAssetDocument::xiiRenderPipelineAssetDocument(xiiStringView sDocumentPath, xiiAssetDocEngineConnection connection) :
  xiiAssetDocument(sDocumentPath, XII_DEFAULT_NEW(xiiRenderPipelineNodeManager), connection)
{
  m_pObjectAccessor = XII_DEFAULT_NEW(xiiPipelineCommandAccessor, GetCommandHistory());
}

xiiRenderPipelineAssetDocument::~xiiRenderPipelineAssetDocument() = default;

xiiStatus xiiRenderPipelineAssetDocument::ExpandSubgraph(const xiiDocumentObject* object)
{
  if (!object || !object->GetType()->IsDerivedFrom<xiiRenderPipelineSubgraphPass>())
    return xiiStatus("Select a render pipeline subgraph reference.");
  xiiRenderPipelineSubgraphPass    instance;
  xiiAbstractObjectGraph           objectGraph;
  xiiDocumentObjectConverterWriter objectWriter(&objectGraph, GetObjectManager());
  const auto*                      objectNode = objectWriter.AddObjectToGraph(object, "root");
  xiiRttiConverterContext          objectContext;
  objectContext.RegisterObject(objectNode->GetGuid(), instance.GetDynamicRTTI(), &instance);
  xiiRttiConverterReader objectReader(&objectGraph, &objectContext);
  objectReader.ApplyPropertiesToObject(objectNode, instance.GetDynamicRTTI(), &instance);
  if (instance.m_sResource.IsEmpty())
    return xiiStatus("Subgraph has no definition resource.");
  const auto                                 resource = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(instance.m_sResource);
  xiiResourceLock<xiiRenderPipelineResource> acquired(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  const auto                                 authoring = acquired.IsValid() ? acquired->GetAuthoring() : xiiSharedPtr<const xiiRenderPipelineAuthoring>();
  if (!authoring)
    return xiiStatus("Subgraph definition is unavailable.");
  xiiRenderPipelineResourceDescriptor wrapper;
  wrapper.AddSubgraph("Expanded", authoring->m_Descriptor.m_Id);
  wrapper.m_Passes[0].m_Id         = object->GetGuid();
  wrapper.m_Passes[0].m_Overrides  = instance.m_Overrides;
  wrapper.m_Passes[0].m_Parameters = instance.m_Parameters;
  xiiVariantDictionary instanceProperties;
  if (instance.GetProperties(instanceProperties).Failed())
    return xiiStatus("Invalid subgraph pin properties.");
  if (const auto* pins = instanceProperties.GetValue("PinConfigurations"))
    wrapper.m_Passes[0].m_Properties.Insert("PinConfigurations", *pins);
  wrapper.m_References.PushBack({authoring->m_Descriptor.m_Id, instance.m_sResource});
  for (const auto& boundary : authoring->m_Descriptor.m_Interface)
  {
    bool found = false;
    for (const auto& existing : wrapper.m_Interface)
      found |= existing.m_sName == boundary.m_sName && existing.m_bInput == boundary.m_bInput;
    if (!found)
      wrapper.m_Interface.PushBack({boundary.m_sName, boundary.m_bInput, object->GetGuid(), boundary.m_sName});
  }
  xiiRenderPipelineResourceDescriptor resolved;
  xiiStringBuilder                    error;
  if (xiiRenderPipelineResource::ResolveDefinitions(wrapper, resolved, nullptr, &error).Failed())
    return xiiStatus(error.GetView());
  xiiGraphDefinition root;
  XII_SUCCEED_OR_RETURN(resolved.MakeGraphDefinition(root));
  const auto resolver = [&](const xiiUuid& id) -> const xiiGraphDefinition* {
    if (id == root.m_Id)
      return &root;
    for (const auto& definition : resolved.m_Definitions)
      if (definition.m_Id == id)
        return &definition;
    return nullptr;
  };
  xiiGraphExpansion           expansion;
  xiiGraphComposition::Limits limits;
  limits.m_uiNodes = 4096;
  limits.m_uiEdges = 65536;
  if (xiiGraphComposition::Expand(root, resolver, limits, expansion, &error).Failed())
    return xiiStatus(error.GetView());
  xiiDynamicArray<xiiRenderPipelinePass*> prototypes;
  XII_SCOPE_EXIT(for (auto* pass : prototypes) pass->GetDynamicRTTI()->GetAllocator()->Deallocate(pass));
  xiiDynamicArray<xiiDynamicArray<xiiRenderPipelinePin>> schemas;
  schemas.SetCount(expansion.m_Nodes.GetCount());
  xiiDynamicArray<xiiArrayPtr<xiiRenderPipelinePin>> pinSchemas;
  for (xiiUInt32 i = 0; i < expansion.m_Nodes.GetCount(); ++i)
  {
    const auto& leaf = expansion.m_Nodes[i];
    const auto* type = xiiRTTI::FindTypeByName(leaf.m_sType);
    if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate())
      return xiiStatus("Expanded node has no reflected render pass type.");
    xiiRenderPipelinePass* pass = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    prototypes.PushBack(pass);
    if (pass->Configure(leaf.m_Properties).Failed() || pass->GetConfiguredPins(schemas[i], &error).Failed())
      return xiiStatus("Invalid expanded pass or pin configuration.");
    pass->m_ParameterOverrides = leaf.m_Parameters;
    pinSchemas.PushBack(schemas[i].GetArrayPtr());
  }
  if (xiiApplyRenderPipelineInterfaceContracts(root, resolver, expansion, pinSchemas, &error).Failed())
    return xiiStatus(error.GetView());
  auto* manager = static_cast<xiiRenderPipelineNodeManager*>(GetObjectManager());
  struct ExternalConnection
  {
    xiiUuid   m_Other;
    xiiString m_sOtherPin, m_sBoundary;
    bool      m_bInput;
  };
  xiiDynamicArray<ExternalConnection> external;
  for (const auto& pin : manager->GetInputPins(object))
    for (const auto* connection : manager->GetConnections(*pin))
      external.PushBack({connection->GetSourcePin().GetParent()->GetGuid(), connection->GetSourcePin().GetName(), pin->GetName(), true});
  for (const auto& pin : manager->GetOutputPins(object))
    for (const auto* connection : manager->GetConnections(*pin))
    {
      if (connection->GetTargetPin().GetParent() == object)
        continue; // Captured once through the input.
      external.PushBack({connection->GetTargetPin().GetParent()->GetGuid(), connection->GetTargetPin().GetName(), pin->GetName(), false});
    }
  const auto position = manager->GetNodePos(object);
  auto*      history  = GetCommandHistory();
  auto*      accessor = GetObjectAccessor();
  history->StartTransaction("Expand Render Pipeline Subgraph");
  const auto apply = [&]() -> xiiStatus {
    xiiRemoveNodeCommand remove;
    remove.m_Object = object->GetGuid();
    XII_SUCCEED_OR_RETURN(history->AddCommand(remove));
    xiiDynamicArray<xiiUuid> nodes;
    for (xiiUInt32 i = 0; i < expansion.m_Nodes.GetCount(); ++i)
    {
      auto*               pass = prototypes[i];
      const auto*         type = pass->GetDynamicRTTI();
      xiiAddObjectCommand add;
      add.m_pType         = type;
      add.m_NewObjectGuid = xiiUuid::MakeUuid();
      add.m_Index         = -1;
      XII_SUCCEED_OR_RETURN(accessor->AddObject(nullptr, nullptr, -1, type, add.m_NewObjectGuid));
      nodes.PushBack(add.m_NewObjectGuid);
      const auto* added = accessor->GetObject(add.m_NewObjectGuid);
      for (auto* configuration : pass->m_PinConfigurations)
        configuration->GetDynamicRTTI()->GetAllocator()->Deallocate(configuration);
      pass->m_PinConfigurations.Clear();
      for (const auto& pin : schemas[i])
      {
        xiiRenderPipelinePinConfiguration* configuration = XII_DEFAULT_NEW(xiiRenderPipelinePinConfiguration);
        configuration->m_Metadata                        = pin.GetMetadata();
        configuration->m_Format                          = pin.m_Format;
        configuration->m_Constraints                     = pin.m_Constraints;
        configuration->m_sDisplayName                    = pin.m_sDisplayName;
        pass->m_PinConfigurations.PushBack(configuration);
      }
      // Apply the native object graph through the existing diff/command serializer;
      // member-only SetValue cannot restore owned pin arrays or structured settings.
      xiiAbstractObjectGraph           current, configured;
      xiiDocumentObjectConverterWriter currentWriter(&current, GetObjectManager());
      const auto*                      currentRoot = currentWriter.AddObjectToGraph(added, "root");
      xiiRttiConverterContext          nativeContext;
      nativeContext.RegisterObject(add.m_NewObjectGuid, type, pass);
      xiiRttiConverterWriter nativeWriter(&configured, &nativeContext, false, true);
      auto*                  configuredRoot = nativeWriter.AddObjectToGraph(type, pass, "root");
      configured.ReMapNodeGuidsToMatchGraph(configuredRoot, current, currentRoot);
      xiiDeque<xiiAbstractGraphDiffOperation> diff;
      configured.CreateDiffWithBaseGraph(current, diff);
      xiiDocumentObjectConverterReader::ApplyDiffToObject(accessor, added, diff);
      XII_SUCCEED_OR_RETURN(manager->SynchronizePinConfigurations(added));
      xiiMoveNodeCommand move;
      move.m_Object = add.m_NewObjectGuid;
      move.m_NewPos = position + xiiVec2(static_cast<float>(i % 4) * 240.0f, static_cast<float>(i / 4) * 140.0f);
      XII_SUCCEED_OR_RETURN(history->AddCommand(move));
    }
    const auto connect = [&](const xiiUuid& source, xiiStringView output, const xiiUuid& target, xiiStringView input) -> xiiStatus {
      const auto* a = manager->GetOutputPinByName(accessor->GetObject(source), output);
      const auto* b = manager->GetInputPinByName(accessor->GetObject(target), input);
      if (!a || !b)
        return xiiStatus("Expanded connection references an unknown reflected pin.");
      return xiiNodeCommands::AddAndConnectCommand(history, manager->GetConnectionType(), *a, *b);
    };
    for (const auto& edge : expansion.m_Edges)
      XII_SUCCEED_OR_RETURN(connect(nodes[expansion.m_Graph.GetVertexIndex(expansion.m_Graph.GetSource(edge.m_hEdge))], edge.m_sSourcePort, nodes[expansion.m_Graph.GetVertexIndex(expansion.m_Graph.GetTarget(edge.m_hEdge))], edge.m_sTargetPort));
    for (const auto& connection : external)
    {
      bool found = false;
      if (connection.m_Other == wrapper.m_Passes[0].m_Id)
      {
        for (const auto& output : expansion.m_Boundary)
          if (output.m_Source.m_InstancePath.IsEmpty() && !output.m_bInput && output.m_sName == connection.m_sOtherPin)
            for (const auto& input : expansion.m_Boundary)
              if (input.m_Source.m_InstancePath.IsEmpty() && input.m_bInput && input.m_sName == connection.m_sBoundary)
              {
                XII_SUCCEED_OR_RETURN(connect(nodes[expansion.m_Graph.GetVertexIndex(output.m_hVertex)], output.m_sPort, nodes[expansion.m_Graph.GetVertexIndex(input.m_hVertex)], input.m_sPort));
                found = true;
              }
        if (!found)
          return xiiStatus("Expanded temporal feedback has no matching interface mapping.");
        continue;
      }
      for (const auto& boundary : expansion.m_Boundary)
      {
        if (!boundary.m_Source.m_InstancePath.IsEmpty() || boundary.m_sName != connection.m_sBoundary || boundary.m_bInput != connection.m_bInput)
          continue;
        const auto id = nodes[expansion.m_Graph.GetVertexIndex(boundary.m_hVertex)];
        if (connection.m_bInput)
          XII_SUCCEED_OR_RETURN(connect(connection.m_Other, connection.m_sOtherPin, id, boundary.m_sPort));
        else
          XII_SUCCEED_OR_RETURN(connect(id, boundary.m_sPort, connection.m_Other, connection.m_sOtherPin));
        found = true;
      }
      if (!found)
        return xiiStatus("Expanded interface no longer contains a connected external pin.");
    }
    return XII_SUCCESS;
  };
  const auto status = apply();
  if (status.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
  return status;
}

xiiTransformStatus xiiRenderPipelineAssetDocument::InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  return xiiAssetDocument::RemoteExport(AssetHeader, sTargetFile);
}

xiiTransformStatus xiiRenderPipelineAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  XII_REPORT_FAILURE("Should not be called");
  return xiiTransformStatus();
}

void xiiRenderPipelineAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);

  auto* pManager = static_cast<xiiRenderPipelineNodeManager*>(GetObjectManager());

  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
  // Upgrade documents authored before persistent pin objects. Normal commands make
  // the addition visible and undoable; no mutation occurs in the const pin getter.
  GetCommandHistory()->StartTransaction("Initialize Render Pipeline Properties");
  xiiStatus  result  = EnsureSettingsObject();
  const auto objects = GetObjectManager()->GetRootObject()->GetChildren();
  for (const auto* object : objects)
    if (result.Failed() || (pManager->IsNode(object) && (result = pManager->SynchronizePinConfigurations(object)).Failed()))
      break;
  if (result.Succeeded())
    GetCommandHistory()->FinishTransaction();
  else
  {
    GetCommandHistory()->CancelTransaction();
    result.LogFailure();
  }
}

const xiiDocumentObject* xiiRenderPipelineAssetDocument::GetSettingsObject() const
{
  for (const auto* object : GetObjectManager()->GetRootObject()->GetChildren())
    if (object->GetType()->IsDerivedFrom<xiiRenderPipelineSettings>())
      return object;
  return nullptr;
}

xiiStatus xiiRenderPipelineAssetDocument::EnsureSettingsObject()
{
  if (GetSettingsObject())
    return XII_SUCCESS;
  auto id = xiiUuid::MakeStableUuidFromString("RenderPipeline/Settings");
  id.HashCombine(GetGuid());
  return GetObjectAccessor()->AddObject(nullptr, nullptr, -1, xiiGetStaticRTTI<xiiRenderPipelineSettings>(), id);
}

void xiiRenderPipelineAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  GetCommandHistory()->StartTransaction("Initialize Render Pipeline Settings");
  auto result = EnsureSettingsObject();
  if (result.Succeeded())
    GetCommandHistory()->FinishTransaction();
  else
  {
    GetCommandHistory()->CancelTransaction();
    result.LogFailure();
  }
}

void xiiRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.RenderPipelineGraph");
}

bool xiiRenderPipelineAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.RenderPipelineGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());

  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiRenderPipelineAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());

  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
