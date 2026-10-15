/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/BuiltinRenderPipelinePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineAssetDocument, 5, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class xiiPipelineEditorPin : public xiiPin
  {
  public:
    xiiPipelineEditorPin(const xiiRenderPipelinePin& pin, const xiiDocumentObject* object) :
      xiiPin(pin.m_bInput ? Type::Input : Type::Output, pin.m_sName, xiiColorScheme::DarkUI(pin.m_Type == xiiRenderPipelinePin::Texture ? xiiColorScheme::Blue : pin.m_Type == xiiRenderPipelinePin::Buffer ? xiiColorScheme::Green :
                                                                                                                                                                                                              xiiColorScheme::Gray),
             object),
      m_Schema(pin)
    {
    }

    xiiRenderPipelinePin m_Schema;
  };
} // namespace

void xiiRenderPipelineNodeManager::GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const
{
  out_templates.Clear();
  xiiHybridArray<const xiiRTTI*, 32> types;
  GetCreateableTypes(types);
  for (const auto* type : types)
  {
    if (type == xiiGetStaticRTTI<xiiBuiltinRenderPipelinePass>())
      continue;
    auto& entry       = out_templates.ExpandAndGetRef();
    entry.m_pType     = type;
    entry.m_sTypeName = type->GetTypeName();
  }
  const auto stages = xiiBuiltinRenderPipelinePass::GetStages();
  m_BuiltinProperties.Clear();
  m_BuiltinProperties.Reserve(stages.GetCount());
  for (const auto stage : stages)
  {
    auto& setting = m_BuiltinProperties.ExpandAndGetRef();
    setting.m_sPropertyName.Assign("Stage");
    setting.m_Value   = stage;
    auto& entry       = out_templates.ExpandAndGetRef();
    entry.m_pType     = xiiGetStaticRTTI<xiiBuiltinRenderPipelinePass>();
    entry.m_sTypeName = stage;
    entry.m_sCategory.Assign("Built-in Rendering");
    entry.m_PropertyValues = xiiArrayPtr<const xiiNodePropertyValue>(&setting, 1);
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
  const auto* type = xiiRTTI::FindTypeByName(pObject->GetTypeAccessor().GetType()->GetTypeName());
  if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>() || !type->GetAllocator()->CanAllocate())
    return;
  xiiRenderPipelinePass* pass = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
  XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(pass));
  // Materialize the reflected object, including instance parameter maps and nested override arrays.
  xiiAbstractObjectGraph           graph;
  xiiDocumentObjectConverterWriter writer(&graph, pObject->GetDocumentObjectManager());
  const auto*                      root = writer.AddObjectToGraph(pObject, "root");
  xiiRttiConverterContext          context;
  context.RegisterObject(root->GetGuid(), type, pass);
  xiiRttiConverterReader reader(&graph, &context);
  reader.ApplyPropertiesToObject(root, type, pass);
  if (pass->Configure({}).Failed())
    return;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  pass->GetPins(pins);
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
  if (output.m_Type != input.m_Type || (output.m_Type == xiiRenderPipelinePin::Texture && output.m_Format != xiiGALResourceFormat::Unknown && input.m_Format != xiiGALResourceFormat::Unknown && output.m_Format != input.m_Format))
    return xiiStatus("Render pipeline pin resource types or formats do not match.");
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

xiiRenderPipelineAssetDocument::xiiRenderPipelineAssetDocument(xiiStringView sDocumentPath) :
  xiiAssetDocument(sDocumentPath, XII_DEFAULT_NEW(xiiRenderPipelineNodeManager), xiiAssetDocEngineConnection::FullObjectMirroring)
{
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
      const auto& leaf = expansion.m_Nodes[i];
      const auto* type = xiiRTTI::FindTypeByName(leaf.m_sType);
      if (!type || !type->IsDerivedFrom<xiiRenderPipelinePass>())
        return xiiStatus("Expanded node has no reflected render pass type.");
      xiiAddObjectCommand add;
      add.m_pType         = type;
      add.m_NewObjectGuid = xiiUuid::MakeUuid();
      add.m_Index         = -1;
      XII_SUCCEED_OR_RETURN(history->AddCommand(add));
      nodes.PushBack(add.m_NewObjectGuid);
      const auto* added = accessor->GetObject(add.m_NewObjectGuid);
      for (auto it = leaf.m_Properties.GetIterator(); it.IsValid(); ++it)
        XII_SUCCEED_OR_RETURN(accessor->SetValueByName(added, it.Key(), it.Value()));
      for (auto it = leaf.m_Parameters.GetIterator(); it.IsValid(); ++it)
        XII_SUCCEED_OR_RETURN(accessor->InsertValueByName(added, "ParameterOverrides", it.Value(), it.Key()));
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

  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());

  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
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
