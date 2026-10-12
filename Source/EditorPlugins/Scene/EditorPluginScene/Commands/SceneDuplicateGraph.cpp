/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  xiiStatus ReadParents(xiiStringView text, xiiMap<xiiUuid, xiiUuid>& out_parents)
  {
    xiiHybridArray<xiiStringView, 16> assignments;
    text.Split(false, assignments, ";");
    for (const auto assignment : assignments)
    {
      xiiHybridArray<xiiStringView, 2> pair;
      assignment.Split(false, pair, "=");
      if (pair.GetCount() != 2 || !xiiConversionUtils::IsStringUuid(pair[0]) || !xiiConversionUtils::IsStringUuid(pair[1]))
        return xiiStatus("Duplicate Objects: invalid source parent mapping.");
      out_parents.Insert(xiiConversionUtils::ConvertStringToUuid(pair[0]), xiiConversionUtils::ConvertStringToUuid(pair[1]));
    }
    return XII_SUCCESS;
  }
} // namespace

xiiStatus xiiDuplicateObjectsCommand::DeserializeGraph(xiiAbstractObjectGraph& graph)
{
  xiiRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
  XII_SUCCEED_OR_RETURN(xiiAbstractGraphDdlSerializer::Read(memoryReader, &graph));
  xiiMap<xiiUuid, xiiUuid> parents;
  XII_SUCCEED_OR_RETURN(ReadParents(m_sParentNodes, parents));
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
    if (it.Value().IsValid() && GetDocument()->GetObjectManager()->GetObject(it.Value()) == nullptr)
      return xiiStatus("Duplicate Objects: source parent no longer exists.");
  return XII_SUCCESS;
}

xiiStatus xiiDuplicateObjectsCommand::GroupRootObjects(xiiAbstractObjectGraph& graph)
{
  xiiMap<xiiUuid, xiiUuid> parents;
  XII_SUCCEED_OR_RETURN(ReadParents(m_sParentNodes, parents));
  xiiHybridArray<const xiiAbstractObjectNode*, 16> roots;
  for (auto it = graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
    if (it.Value()->GetNodeName() == "root")
      roots.PushBack(it.Value());
  if (roots.IsEmpty())
    return xiiStatus("Duplicate Objects: no source objects to group.");
  roots.Sort([](const auto* a, const auto* b) {
    const auto* orderA = a->FindProperty("__SelectionOrder");
    const auto* orderB = b->FindProperty("__SelectionOrder");
    return (orderA ? orderA->m_Value.template ConvertTo<xiiUInt32>() : 0) <
      (orderB ? orderB->m_Value.template ConvertTo<xiiUInt32>() : 0);
  });

  xiiUuid commonParent = parents[roots[0]->GetGuid()];
  xiiVec3 center       = xiiVec3::MakeZero();
  for (const auto* root : roots)
  {
    if (root->GetType() != "xiiGameObject")
      return xiiStatus("Duplicate Objects: only game objects can be grouped.");
    const auto* transform = root->FindProperty("__GlobalTransform");
    if (transform == nullptr || !transform->m_Value.IsA<xiiTransform>() || !transform->m_Value.Get<xiiTransform>().IsValid())
      return xiiStatus("Duplicate Objects: source transform is invalid.");
    center += transform->m_Value.Get<xiiTransform>().m_vPosition / static_cast<float>(roots.GetCount());
    if (parents[root->GetGuid()] != commonParent)
      commonParent = xiiUuid::MakeInvalid();
  }
  auto*              document        = static_cast<xiiSceneDocument*>(GetDocument());
  const auto*        parent          = document->GetObjectManager()->GetObject(commonParent);
  const xiiTransform parentTransform = parent ? document->GetGlobalTransform(parent) : xiiTransform::MakeIdentity();
  xiiTransform       groupTransform  = parentTransform;
  groupTransform.m_vPosition         = center;
  if (!groupTransform.IsValid() || groupTransform.m_vScale.x == 0 || groupTransform.m_vScale.y == 0 || groupTransform.m_vScale.z == 0)
    return xiiStatus("Duplicate Objects: group parent transform is singular or invalid.");

  xiiAbstractObjectGraph grouped;
  for (auto it = graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    const auto* source = it.Value();
    auto*       node   = grouped.AddNode(source->GetGuid(), source->GetType(), source->GetTypeVersion(), source->GetNodeName() == "root" ? xiiStringView() : source->GetNodeName());
    for (const auto& property : source->GetProperties())
      node->AddProperty(property.m_sPropertyName, property.m_Value);
  }
  xiiVariantArray children;
  for (const auto* root : roots)
  {
    children.PushBack(root->GetGuid());
    const auto local = xiiTransform::MakeLocalTransform(groupTransform, root->FindProperty("__GlobalTransform")->m_Value.Get<xiiTransform>());
    if (!local.IsValid())
      return xiiStatus("Duplicate Objects: grouped local transform is invalid.");
    auto* node = grouped.GetNode(root->GetGuid());
    for (const char* property : {"LocalPosition", "LocalRotation", "LocalScaling", "LocalUniformScaling"})
      node->RemoveProperty(property);
    node->AddProperty("LocalPosition", local.m_vPosition);
    node->AddProperty("LocalRotation", local.m_qRotation);
    node->AddProperty("LocalScaling", local.m_vScale);
    node->AddProperty("LocalUniformScaling", 1.0f);
  }
  const xiiUuid groupId = xiiUuid::MakeUuid();
  auto*         group   = grouped.AddNode(groupId, "xiiGameObject", xiiGetStaticRTTI<xiiGameObject>()->GetTypeVersion(), "root");
  group->AddProperty("Name", "Duplicates");
  group->AddProperty("Children", children);
  group->AddProperty("__GlobalTransform", groupTransform);
  group->AddProperty("__SelectionOrder", xiiUInt32(0));
  graph.Clear();
  grouped.Clone(graph);
  xiiStringBuilder objectId, parentId;
  m_sParentNodes = xiiStringBuilder(xiiConversionUtils::ToString(groupId, objectId), "=", xiiConversionUtils::ToString(commonParent, parentId), ";");
  return XII_SUCCESS;
}

void xiiDuplicateObjectsCommand::CreateOneDuplicate(xiiAbstractObjectGraph& graph, xiiHybridArray<xiiDocument::PasteInfo, 16>& ToBePasted)
{
  xiiSceneDocument* pDocument = static_cast<xiiSceneDocument*>(GetDocument());

  // Remap
  const xiiUuid seed = xiiUuid::MakeUuid();
  graph.ReMapNodeGuids(seed);
  XII_SCOPE_EXIT(graph.ReMapNodeGuids(seed, true));

  xiiDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateOnly);


  xiiMap<xiiUuid, xiiUuid> ParentGuids;
  xiiMap<xiiUuid, xiiUuid> sourceParents;
  ReadParents(m_sParentNodes, sourceParents).AssertSuccess();
  for (auto it = sourceParents.GetIterator(); it.IsValid(); ++it)
  {
    xiiUuid guidObj = it.Key();
    guidObj.CombineWithSeed(seed);
    ParentGuids.Insert(guidObj, it.Value());
  }

  xiiHybridArray<xiiAbstractObjectNode*, 16> roots;
  for (auto it = graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
    if (it.Value()->GetNodeName() == "root")
      roots.PushBack(it.Value());
  roots.Sort([](const xiiAbstractObjectNode* a, const xiiAbstractObjectNode* b) {
    const auto* orderA = a->FindProperty("__SelectionOrder");
    const auto* orderB = b->FindProperty("__SelectionOrder");
    return (orderA ? orderA->m_Value.ConvertTo<xiiUInt32>() : 0) < (orderB ? orderB->m_Value.ConvertTo<xiiUInt32>() : 0);
  });
  for (auto* pNode : roots)
  {
    auto* pNewObject = reader.CreateObjectFromNode(pNode);

    if (pNewObject)
    {
      reader.ApplyPropertiesToObject(pNode, pNewObject);

      auto& ref     = ToBePasted.ExpandAndGetRef();
      ref.m_pObject = pNewObject;
      ref.m_pParent = nullptr;

      const xiiUuid guidParent = ParentGuids[pNode->GetGuid()];

      if (guidParent.IsValid())
        ref.m_pParent = pDocument->GetObjectManager()->GetObject(guidParent);
    }
  }

  if (pDocument->DuplicateSelectedObjects(ToBePasted, graph, false))
  {
    for (xiiUInt32 i = 0; i < ToBePasted.GetCount(); ++i)
    {
      const auto& item = ToBePasted[i];

      auto& po             = m_DuplicatedObjects.ExpandAndGetRef();
      po.m_pObject         = item.m_pObject;
      po.m_Index           = item.m_pObject->GetPropertyIndex();
      po.m_pParent         = item.m_pParent;
      po.m_sParentProperty = item.m_pObject->GetParentProperty();
    }
  }
  else
  {
    for (const auto& item : ToBePasted)
    {
      pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
    }
  }
}
