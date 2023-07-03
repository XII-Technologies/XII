#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiObjectChangeType, 1)
  XII_ENUM_CONSTANTS(xiiObjectChangeType::NodeAdded, xiiObjectChangeType::NodeRemoved)
  XII_ENUM_CONSTANTS(xiiObjectChangeType::PropertySet, xiiObjectChangeType::PropertyInserted, xiiObjectChangeType::PropertyRemoved)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAbstractObjectNode, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAbstractObjectNode>)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiDiffOperation, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiDiffOperation>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Operation", xiiObjectChangeType, m_Operation),
    XII_MEMBER_PROPERTY("Node", m_Node),
    XII_MEMBER_PROPERTY("Property", m_sProperty),
    XII_MEMBER_PROPERTY("Index", m_Index),
    XII_MEMBER_PROPERTY("Value", m_Value),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiAbstractObjectGraph::~xiiAbstractObjectGraph()
{
  Clear();
}

void xiiAbstractObjectGraph::Clear()
{
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    XII_DEFAULT_DELETE(it.Value());
  }
  m_Nodes.Clear();
  m_NodesByName.Clear();
  m_Strings.Clear();
}


xiiAbstractObjectNode* xiiAbstractObjectGraph::Clone(xiiAbstractObjectGraph& ref_cloneTarget, const xiiAbstractObjectNode* pRootNode, FilterFunction filter) const
{
  ref_cloneTarget.Clear();

  if (pRootNode == nullptr)
  {
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (filter.IsValid())
      {
        ref_cloneTarget.CopyNodeIntoGraph(it.Value(), filter);
      }
      else
      {
        ref_cloneTarget.CopyNodeIntoGraph(it.Value());
      }
    }
    return nullptr;
  }
  else
  {
    XII_ASSERT_DEV(pRootNode->GetOwner() == this, "The given root node must be part of this document");
    xiiSet<xiiUuid> reachableNodes;
    FindTransitiveHull(pRootNode->GetGuid(), reachableNodes);

    for (const xiiUuid& guid : reachableNodes)
    {
      if (auto* pNode = GetNode(guid))
      {
        if (filter.IsValid())
        {
          ref_cloneTarget.CopyNodeIntoGraph(pNode, filter);
        }
        else
        {
          ref_cloneTarget.CopyNodeIntoGraph(pNode);
        }
      }
    }

    return ref_cloneTarget.GetNode(pRootNode->GetGuid());
  }
}

xiiStringView xiiAbstractObjectGraph::RegisterString(xiiStringView sString)
{
  auto it = m_Strings.Insert(sString);
  XII_ASSERT_DEV(it.IsValid(), "");
  return it.Key();
}

xiiAbstractObjectNode* xiiAbstractObjectGraph::GetNode(const xiiUuid& guid)
{
  return m_Nodes.GetValueOrDefault(guid, nullptr);
}

const xiiAbstractObjectNode* xiiAbstractObjectGraph::GetNode(const xiiUuid& guid) const
{
  return const_cast<xiiAbstractObjectGraph*>(this)->GetNode(guid);
}

const xiiAbstractObjectNode* xiiAbstractObjectGraph::GetNodeByName(xiiStringView sName) const
{
  return const_cast<xiiAbstractObjectGraph*>(this)->GetNodeByName(sName);
}

xiiAbstractObjectNode* xiiAbstractObjectGraph::GetNodeByName(xiiStringView sName)
{
  return m_NodesByName.GetValueOrDefault(sName, nullptr);
}

xiiAbstractObjectNode* xiiAbstractObjectGraph::AddNode(const xiiUuid& guid, xiiStringView sType, xiiUInt32 uiTypeVersion, xiiStringView sNodeName)
{
  XII_ASSERT_DEV(!m_Nodes.Contains(guid), "object {0} must not yet exist", guid);
  if (!sNodeName.IsEmpty())
  {
    sNodeName = RegisterString(sNodeName);
  }
  else
  {
    sNodeName = {};
  }

  xiiAbstractObjectNode* pNode = XII_DEFAULT_NEW(xiiAbstractObjectNode);
  pNode->m_Guid                = guid;
  pNode->m_pOwner              = this;
  pNode->m_sType               = RegisterString(sType);
  pNode->m_uiTypeVersion       = uiTypeVersion;
  pNode->m_sNodeName           = sNodeName;

  m_Nodes[guid] = pNode;

  if (!sNodeName.IsEmpty())
  {
    m_NodesByName[sNodeName] = pNode;
  }

  return pNode;
}

void xiiAbstractObjectGraph::RemoveNode(const xiiUuid& guid)
{
  auto it = m_Nodes.Find(guid);

  if (it.IsValid())
  {
    xiiAbstractObjectNode* pNode = it.Value();
    if (!pNode->m_sNodeName.IsEmpty())
      m_NodesByName.Remove(pNode->m_sNodeName);

    m_Nodes.Remove(guid);
    XII_DEFAULT_DELETE(pNode);
  }
}

void xiiAbstractObjectNode::AddProperty(xiiStringView sName, const xiiVariant& value)
{
  auto& prop           = m_Properties.ExpandAndGetRef();
  prop.m_sPropertyName = m_pOwner->RegisterString(sName);
  prop.m_Value         = value;
}

void xiiAbstractObjectNode::ChangeProperty(xiiStringView sName, const xiiVariant& value)
{
  for (xiiUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      m_Properties[i].m_Value = value;
      return;
    }
  }

  XII_REPORT_FAILURE("Property '{0}' is unknown", sName);
}

void xiiAbstractObjectNode::RenameProperty(xiiStringView sOldName, xiiStringView sNewName)
{
  for (xiiUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sOldName)
    {
      m_Properties[i].m_sPropertyName = m_pOwner->RegisterString(sNewName);
      return;
    }
  }
}

void xiiAbstractObjectNode::ClearProperties()
{
  m_Properties.Clear();
}

xiiResult xiiAbstractObjectNode::InlineProperty(xiiStringView sName)
{
  for (xiiUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    Property& prop = m_Properties[i];
    if (prop.m_sPropertyName == sName)
    {
      if (!prop.m_Value.IsA<xiiUuid>())
        return XII_FAILURE;

      xiiUuid                guid  = prop.m_Value.Get<xiiUuid>();
      xiiAbstractObjectNode* pNode = m_pOwner->GetNode(guid);
      if (!pNode)
        return XII_FAILURE;

      class InlineContext : public xiiRttiConverterContext
      {
      public:
        void RegisterObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject) override
        {
          m_SubTree.PushBack(guid);
        }
        xiiHybridArray<xiiUuid, 1> m_SubTree;
      };

      InlineContext          context;
      xiiRttiConverterReader reader(m_pOwner, &context);
      void*                  pObject = reader.CreateObjectFromNode(pNode);
      if (!pObject)
        return XII_FAILURE;

      prop.m_Value.MoveTypedObject(pObject, xiiRTTI::FindTypeByName(pNode->GetType()));

      // Delete old objects.
      for (xiiUuid& uuid : context.m_SubTree)
      {
        m_pOwner->RemoveNode(uuid);
      }
      return XII_SUCCESS;
    }
  }
  return XII_FAILURE;
}

void xiiAbstractObjectNode::RemoveProperty(xiiStringView sName)
{
  for (xiiUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      m_Properties.RemoveAtAndSwap(i);
      return;
    }
  }
}

void xiiAbstractObjectNode::SetType(xiiStringView sType)
{
  m_sType = m_pOwner->RegisterString(sType);
}

const xiiAbstractObjectNode::Property* xiiAbstractObjectNode::FindProperty(xiiStringView sName) const
{
  for (xiiUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

xiiAbstractObjectNode::Property* xiiAbstractObjectNode::FindProperty(xiiStringView sName)
{
  for (xiiUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

void xiiAbstractObjectGraph::ReMapNodeGuids(const xiiUuid& seedGuid, bool bRemapInverse /*= false*/)
{
  xiiHybridArray<xiiAbstractObjectNode*, 16> nodes;
  nodes.Reserve(m_Nodes.GetCount());
  xiiHashTable<xiiUuid, xiiUuid> guidMap;
  guidMap.Reserve(m_Nodes.GetCount());

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    xiiUuid newGuid = it.Key();

    if (bRemapInverse)
      newGuid.RevertCombinationWithSeed(seedGuid);
    else
      newGuid.CombineWithSeed(seedGuid);

    guidMap[it.Key()] = newGuid;

    nodes.PushBack(it.Value());
  }

  m_Nodes.Clear();

  // go through all nodes to remap guids
  for (auto* pNode : nodes)
  {
    pNode->m_Guid = guidMap[pNode->m_Guid];

    // check every property
    for (auto& prop : pNode->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);
    }
    m_Nodes[pNode->m_Guid] = pNode;
  }
}


void xiiAbstractObjectGraph::ReMapNodeGuidsToMatchGraph(xiiAbstractObjectNode* pRoot, const xiiAbstractObjectGraph& rhsGraph, const xiiAbstractObjectNode* pRhsRoot)
{
  xiiHashTable<xiiUuid, xiiUuid> guidMap;
  XII_ASSERT_DEV(pRoot->GetType() == pRhsRoot->GetType(), "Roots must have the same type to be able re-map guids!");

  ReMapNodeGuidsToMatchGraphRecursive(guidMap, pRoot, rhsGraph, pRhsRoot);

  // go through all nodes to remap remaining occurrences of remapped guids
  for (auto it : m_Nodes)
  {
    // check every property
    for (auto& prop : it.Value()->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);
    }
    m_Nodes[it.Value()->m_Guid] = it.Value();
  }
}

void xiiAbstractObjectGraph::ReMapNodeGuidsToMatchGraphRecursive(xiiHashTable<xiiUuid, xiiUuid>& guidMap, xiiAbstractObjectNode* lhs, const xiiAbstractObjectGraph& rhsGraph, const xiiAbstractObjectNode* rhs)
{
  if (!lhs->GetType() == rhs->GetType())
  {
    // Types differ, remapping ends as this is a removal and add of a new object.
    return;
  }

  if (lhs->GetGuid() != rhs->GetGuid())
  {
    guidMap[lhs->GetGuid()] = rhs->GetGuid();
    m_Nodes.Remove(lhs->GetGuid());
    lhs->m_Guid = rhs->GetGuid();
    m_Nodes.Insert(rhs->GetGuid(), lhs);
  }

  for (xiiAbstractObjectNode::Property& prop : lhs->m_Properties)
  {
    if (prop.m_Value.IsA<xiiUuid>() && prop.m_Value.Get<xiiUuid>().IsValid())
    {
      // if the guid is an owned object in the graph, remap to rhs.
      auto it = m_Nodes.Find(prop.m_Value.Get<xiiUuid>());
      if (it.IsValid())
      {
        if (const xiiAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
        {
          if (rhsProp->m_Value.IsA<xiiUuid>() && rhsProp->m_Value.Get<xiiUuid>().IsValid())
          {
            if (const xiiAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsProp->m_Value.Get<xiiUuid>()))
            {
              ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
            }
          }
        }
      }
    }
    // Arrays may be of owner guids and could be remapped.
    else if (prop.m_Value.IsA<xiiVariantArray>())
    {
      const xiiVariantArray& values = prop.m_Value.Get<xiiVariantArray>();
      for (xiiUInt32 i = 0; i < values.GetCount(); i++)
      {
        auto& subValue = values[i];
        if (subValue.IsA<xiiUuid>() && subValue.Get<xiiUuid>().IsValid())
        {
          // if the guid is an owned object in the graph, remap to array element.
          auto it = m_Nodes.Find(subValue.Get<xiiUuid>());
          if (it.IsValid())
          {
            if (const xiiAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
            {
              if (rhsProp->m_Value.IsA<xiiVariantArray>())
              {
                const xiiVariantArray& rhsValues = rhsProp->m_Value.Get<xiiVariantArray>();
                if (i < rhsValues.GetCount())
                {
                  const auto& rhsElemValue = rhsValues[i];
                  if (rhsElemValue.IsA<xiiUuid>() && rhsElemValue.Get<xiiUuid>().IsValid())
                  {
                    if (const xiiAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsElemValue.Get<xiiUuid>()))
                    {
                      ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    // Maps may be of owner guids and could be remapped.
    else if (prop.m_Value.IsA<xiiVariantDictionary>())
    {
      const xiiVariantDictionary& values = prop.m_Value.Get<xiiVariantDictionary>();
      for (auto lhsIt = values.GetIterator(); lhsIt.IsValid(); ++lhsIt)
      {
        auto& subValue = lhsIt.Value();
        if (subValue.IsA<xiiUuid>() && subValue.Get<xiiUuid>().IsValid())
        {
          // if the guid is an owned object in the graph, remap to map element.
          auto it = m_Nodes.Find(subValue.Get<xiiUuid>());
          if (it.IsValid())
          {
            if (const xiiAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
            {
              if (rhsProp->m_Value.IsA<xiiVariantDictionary>())
              {
                const xiiVariantDictionary& rhsValues = rhsProp->m_Value.Get<xiiVariantDictionary>();
                if (rhsValues.Contains(lhsIt.Key()))
                {
                  const auto& rhsElemValue = *rhsValues.GetValue(lhsIt.Key());
                  if (rhsElemValue.IsA<xiiUuid>() && rhsElemValue.Get<xiiUuid>().IsValid())
                  {
                    if (const xiiAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsElemValue.Get<xiiUuid>()))
                    {
                      ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}


void xiiAbstractObjectGraph::FindTransitiveHull(const xiiUuid& rootGuid, xiiSet<xiiUuid>& ref_reachableNodes) const
{
  ref_reachableNodes.Clear();
  xiiSet<xiiUuid> inProgress;
  inProgress.Insert(rootGuid);

  while (!inProgress.IsEmpty())
  {
    xiiUuid current = *inProgress.GetIterator();
    auto    it      = m_Nodes.Find(current);
    if (it.IsValid())
    {
      const xiiAbstractObjectNode* pNode = it.Value();
      for (auto& prop : pNode->m_Properties)
      {
        if (prop.m_Value.IsA<xiiUuid>())
        {
          const xiiUuid& guid = prop.m_Value.Get<xiiUuid>();
          if (!ref_reachableNodes.Contains(guid))
          {
            inProgress.Insert(guid);
          }
        }
        // Arrays may be of uuids
        else if (prop.m_Value.IsA<xiiVariantArray>())
        {
          const xiiVariantArray& values = prop.m_Value.Get<xiiVariantArray>();
          for (auto& subValue : values)
          {
            if (subValue.IsA<xiiUuid>())
            {
              const xiiUuid& guid = subValue.Get<xiiUuid>();
              if (!ref_reachableNodes.Contains(guid))
              {
                inProgress.Insert(guid);
              }
            }
          }
        }
        else if (prop.m_Value.IsA<xiiVariantDictionary>())
        {
          const xiiVariantDictionary& values = prop.m_Value.Get<xiiVariantDictionary>();
          for (auto& subValue : values)
          {
            if (subValue.Value().IsA<xiiUuid>())
            {
              const xiiUuid& guid = subValue.Value().Get<xiiUuid>();
              if (!ref_reachableNodes.Contains(guid))
              {
                inProgress.Insert(guid);
              }
            }
          }
        }
      }
    }
    // Even if 'current' is not in the graph add it anyway to early out if it is found again.
    ref_reachableNodes.Insert(current);
    inProgress.Remove(current);
  }
}

void xiiAbstractObjectGraph::PruneGraph(const xiiUuid& rootGuid)
{
  xiiSet<xiiUuid> reachableNodes;
  FindTransitiveHull(rootGuid, reachableNodes);

  // Determine nodes to be removed by subtracting valid ones from all nodes.
  xiiSet<xiiUuid> removeSet;
  for (auto it = GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    removeSet.Insert(it.Key());
  }
  removeSet.Difference(reachableNodes);

  // Remove nodes.
  for (const xiiUuid& guid : removeSet)
  {
    RemoveNode(guid);
  }
}

void xiiAbstractObjectGraph::ModifyNodeViaNativeCounterpart(xiiAbstractObjectNode* pRootNode, xiiDelegate<void(void*, const xiiRTTI*)> callback)
{
  XII_ASSERT_DEV(pRootNode->GetOwner() == this, "Node must be from this graph.");

  // Clone sub graph
  xiiAbstractObjectGraph origGraph;
  xiiAbstractObjectNode* pOrigRootNode = nullptr;
  {
    pOrigRootNode = Clone(origGraph, pRootNode);
  }

  // Create native object
  xiiRttiConverterContext context;
  xiiRttiConverterReader  convRead(&origGraph, &context);
  void*                   pNativeRoot = convRead.CreateObjectFromNode(pOrigRootNode);
  const xiiRTTI*          pType       = xiiRTTI::FindTypeByName(pOrigRootNode->GetType());
  XII_SCOPE_EXIT(pType->GetAllocator()->Deallocate(pNativeRoot););

  // Make changes to native object
  if (callback.IsValid())
  {
    callback(pNativeRoot, pType);
  }

  // Create native object graph
  xiiAbstractObjectGraph graph;
  xiiAbstractObjectNode* pRootNode2 = nullptr;
  {
    // The xiiApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those
    // of the object manager.
    xiiApplyNativePropertyChangesContext nativeChangesContext(context, origGraph);
    xiiRttiConverterWriter               rttiConverter(&graph, &nativeChangesContext, true, true);
    nativeChangesContext.RegisterObject(pOrigRootNode->GetGuid(), pType, pNativeRoot);
    pRootNode2 = rttiConverter.AddObjectToGraph(pType, pNativeRoot, "Object");
  }

  // Create diff from native to cloned sub-graph and then apply the diff to the original graph.
  xiiDeque<xiiAbstractGraphDiffOperation> diffResult;
  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  ApplyDiff(diffResult);
}

xiiAbstractObjectNode* xiiAbstractObjectGraph::CopyNodeIntoGraph(const xiiAbstractObjectNode* pNode)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetTypeVersion(), pNode->GetNodeName());

  for (const auto& props : pNode->GetProperties())
  {
    pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
  }

  return pNewNode;
}

xiiAbstractObjectNode* xiiAbstractObjectGraph::CopyNodeIntoGraph(const xiiAbstractObjectNode* pNode, FilterFunction& ref_filter)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetTypeVersion(), pNode->GetNodeName());

  if (ref_filter.IsValid())
  {
    for (const auto& props : pNode->GetProperties())
    {
      if (!ref_filter(pNode, &props))
        continue;

      pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
    }
  }
  else
  {
    for (const auto& props : pNode->GetProperties())
    {
      pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
    }
  }

  return pNewNode;
}

void xiiAbstractObjectGraph::CreateDiffWithBaseGraph(const xiiAbstractObjectGraph& base, xiiDeque<xiiAbstractGraphDiffOperation>& out_diffResult) const
{
  out_diffResult.Clear();

  // check whether any nodes have been deleted
  {
    for (auto itNodeBase = base.GetAllNodes().GetIterator(); itNodeBase.IsValid(); ++itNodeBase)
    {
      if (GetNode(itNodeBase.Key()) == nullptr)
      {
        // does not exist in this graph -> has been deleted from base
        xiiAbstractGraphDiffOperation op;
        op.m_Node      = itNodeBase.Key();
        op.m_Operation = xiiAbstractGraphDiffOperation::Op::NodeRemoved;
        op.m_sProperty = itNodeBase.Value()->m_sType;
        op.m_Value     = itNodeBase.Value()->m_sNodeName;

        out_diffResult.PushBack(op);
      }
    }
  }

  // check whether any nodes have been added
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      if (base.GetNode(itNodeThis.Key()) == nullptr)
      {
        // does not exist in base graph -> has been added
        xiiAbstractGraphDiffOperation op;
        op.m_Node      = itNodeThis.Key();
        op.m_Operation = xiiAbstractGraphDiffOperation::Op::NodeAdded;
        op.m_sProperty = itNodeThis.Value()->m_sType;
        op.m_Value     = itNodeThis.Value()->m_sNodeName;

        out_diffResult.PushBack(op);

        // set all properties
        for (const auto& prop : itNodeThis.Value()->GetProperties())
        {
          op.m_Operation = xiiAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_sPropertyName;
          op.m_Value     = prop.m_Value;

          out_diffResult.PushBack(op);
        }
      }
    }
  }

  // check whether any properties have been modified
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      const auto pBaseNode = base.GetNode(itNodeThis.Key());

      if (pBaseNode == nullptr)
        continue;

      for (const xiiAbstractObjectNode::Property& prop : itNodeThis.Value()->GetProperties())
      {
        bool bDifferent = true;

        for (const xiiAbstractObjectNode::Property& baseProp : pBaseNode->GetProperties())
        {
          if (xiiStringUtils::IsEqual(baseProp.m_sPropertyName, prop.m_sPropertyName))
          {
            if (baseProp.m_Value == prop.m_Value)
            {
              bDifferent = false;
              break;
            }

            bDifferent = true;
            break;
          }
        }

        if (bDifferent)
        {
          xiiAbstractGraphDiffOperation op;
          op.m_Node      = itNodeThis.Key();
          op.m_Operation = xiiAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_sPropertyName;
          op.m_Value     = prop.m_Value;

          out_diffResult.PushBack(op);
        }
      }
    }
  }
}


void xiiAbstractObjectGraph::ApplyDiff(xiiDeque<xiiAbstractGraphDiffOperation>& ref_diff)
{
  for (const auto& op : ref_diff)
  {
    switch (op.m_Operation)
    {
      case xiiAbstractGraphDiffOperation::Op::NodeAdded:
      {
        AddNode(op.m_Node, op.m_sProperty, op.m_uiTypeVersion, op.m_Value.Get<xiiString>());
      }
      break;

      case xiiAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        RemoveNode(op.m_Node);
      }
      break;

      case xiiAbstractGraphDiffOperation::Op::PropertyChanged:
      {
        auto* pNode = GetNode(op.m_Node);
        if (pNode)
        {
          auto* pProp = pNode->FindProperty(op.m_sProperty);

          if (!pProp)
          {
            pNode->AddProperty(op.m_sProperty, op.m_Value);
          }
          else
          {
            pProp->m_Value = op.m_Value;
          }
        }
      }
      break;
    }
  }
}


void xiiAbstractObjectGraph::MergeDiffs(const xiiDeque<xiiAbstractGraphDiffOperation>& lhs, const xiiDeque<xiiAbstractGraphDiffOperation>& rhs, xiiDeque<xiiAbstractGraphDiffOperation>& ref_out) const
{
  struct Prop
  {
    Prop() = default;
    Prop(xiiUuid node, xiiStringView sProperty) :
      m_Node(node), m_sProperty(sProperty)
    {
    }

    xiiUuid       m_Node;
    xiiStringView m_sProperty;

    bool operator<(const Prop& rhs) const
    {
      if (m_Node == rhs.m_Node)
      {
        return m_sProperty < rhs.m_sProperty;
      }

      return m_Node < rhs.m_Node;
    }

    bool operator==(const Prop& rhs) const { return m_Node == rhs.m_Node && m_sProperty == rhs.m_sProperty; }
  };

  xiiMap<Prop, xiiHybridArray<const xiiAbstractGraphDiffOperation*, 2>> propChanges;
  xiiSet<xiiUuid>                                                       removed;
  xiiMap<xiiUuid, xiiUInt32>                                            added;
  for (const xiiAbstractGraphDiffOperation& op : lhs)
  {
    if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      removed.Insert(op.m_Node);
      ref_out.PushBack(op);
    }
    else if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::NodeAdded)
    {
      added[op.m_Node] = ref_out.GetCount();
      ref_out.PushBack(op);
    }
    else if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }
  for (const xiiAbstractGraphDiffOperation& op : rhs)
  {
    if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      if (!removed.Contains(op.m_Node))
        ref_out.PushBack(op);
    }
    else if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::NodeAdded)
    {
      if (added.Contains(op.m_Node))
      {
        xiiAbstractGraphDiffOperation& leftOp = ref_out[added[op.m_Node]];
        leftOp.m_sProperty                    = op.m_sProperty; // Take type from rhs.
      }
      else
      {
        ref_out.PushBack(op);
      }
    }
    else if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }

  for (auto it = propChanges.GetIterator(); it.IsValid(); ++it)
  {
    const Prop&                                                    key   = it.Key();
    const xiiHybridArray<const xiiAbstractGraphDiffOperation*, 2>& value = it.Value();

    if (value.GetCount() == 1)
    {
      ref_out.PushBack(*value[0]);
    }
    else
    {
      const xiiAbstractGraphDiffOperation& leftProp  = *value[0];
      const xiiAbstractGraphDiffOperation& rightProp = *value[1];

      if (leftProp.m_Value.GetType() == xiiVariantType::VariantArray && rightProp.m_Value.GetType() == xiiVariantType::VariantArray)
      {
        const xiiVariantArray& leftArray  = leftProp.m_Value.Get<xiiVariantArray>();
        const xiiVariantArray& rightArray = rightProp.m_Value.Get<xiiVariantArray>();

        const xiiAbstractObjectNode* pNode = GetNode(key.m_Node);
        if (pNode)
        {
          xiiStringBuilder                       sTemp(key.m_sProperty);
          const xiiAbstractObjectNode::Property* pProperty = pNode->FindProperty(sTemp);
          if (pProperty && pProperty->m_Value.GetType() == xiiVariantType::VariantArray)
          {
            // Do 3-way array merge
            const xiiVariantArray& baseArray = pProperty->m_Value.Get<xiiVariantArray>();
            xiiVariantArray        res;
            MergeArrays(baseArray, leftArray, rightArray, res);
            ref_out.PushBack(rightProp);
            ref_out.PeekBack().m_Value = res;
          }
          else
          {
            ref_out.PushBack(rightProp);
          }
        }
        else
        {
          ref_out.PushBack(rightProp);
        }
      }
      else
      {
        ref_out.PushBack(rightProp);
      }
    }
  }
}

void xiiAbstractObjectGraph::RemapVariant(xiiVariant& value, const xiiHashTable<xiiUuid, xiiUuid>& guidMap)
{
  xiiStringBuilder tmp;

  // if the property is a guid, we check if we need to remap it
  if (value.IsA<xiiUuid>())
  {
    const xiiUuid& guid = value.Get<xiiUuid>();

    // if we find the guid in our map, replace it by the new guid
    if (auto* found = guidMap.GetValue(guid))
    {
      value = *found;
    }
  }
  else if (value.IsA<xiiString>() && xiiConversionUtils::IsStringUuid(value.Get<xiiString>()))
  {
    const xiiUuid guid = xiiConversionUtils::ConvertStringToUuid(value.Get<xiiString>());

    // if we find the guid in our map, replace it by the new guid
    if (auto* found = guidMap.GetValue(guid))
    {
      value = xiiConversionUtils::ToString(*found, tmp).GetData();
    }
  }
  // Arrays may be of uuids
  else if (value.IsA<xiiVariantArray>())
  {
    const xiiVariantArray& values       = value.Get<xiiVariantArray>();
    bool                   bNeedToRemap = false;
    for (auto& subValue : values)
    {
      if (subValue.IsA<xiiUuid>() && guidMap.Contains(subValue.Get<xiiUuid>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<xiiString>() && xiiConversionUtils::IsStringUuid(subValue.Get<xiiString>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<xiiVariantArray>())
      {
        bNeedToRemap = true;
        break;
      }
    }

    if (bNeedToRemap)
    {
      xiiVariantArray newValues = values;
      for (auto& subValue : newValues)
      {
        RemapVariant(subValue, guidMap);
      }
      value = newValues;
    }
  }
  // Maps may be of uuids
  else if (value.IsA<xiiVariantDictionary>())
  {
    const xiiVariantDictionary& values       = value.Get<xiiVariantDictionary>();
    bool                        bNeedToRemap = false;
    for (auto it = values.GetIterator(); it.IsValid(); ++it)
    {
      const xiiVariant& subValue = it.Value();

      if (subValue.IsA<xiiUuid>() && guidMap.Contains(subValue.Get<xiiUuid>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<xiiString>() && xiiConversionUtils::IsStringUuid(subValue.Get<xiiString>()))
      {
        bNeedToRemap = true;
        break;
      }
    }

    if (bNeedToRemap)
    {
      xiiVariantDictionary newValues = values;
      for (auto it = newValues.GetIterator(); it.IsValid(); ++it)
      {
        RemapVariant(it.Value(), guidMap);
      }
      value = newValues;
    }
  }
}

void xiiAbstractObjectGraph::MergeArrays(const xiiDynamicArray<xiiVariant>& baseArray, const xiiDynamicArray<xiiVariant>& leftArray, const xiiDynamicArray<xiiVariant>& rightArray, xiiDynamicArray<xiiVariant>& out) const
{
  // Find element type.
  xiiVariantType::Enum type = xiiVariantType::Invalid;
  if (!baseArray.IsEmpty())
    type = baseArray[0].GetType();
  if (type != xiiVariantType::Invalid && !leftArray.IsEmpty())
    type = leftArray[0].GetType();
  if (type != xiiVariantType::Invalid && !rightArray.IsEmpty())
    type = rightArray[0].GetType();

  if (type == xiiVariantType::Invalid)
    return;

  // For now, assume non-uuid types are arrays, uuids are sets.
  if (type != xiiVariantType::Uuid)
  {
    // Any size changes?
    xiiUInt32 uiSize = baseArray.GetCount();
    if (leftArray.GetCount() != baseArray.GetCount())
      uiSize = leftArray.GetCount();
    if (rightArray.GetCount() != baseArray.GetCount())
      uiSize = rightArray.GetCount();

    out.SetCount(uiSize);
    for (xiiUInt32 i = 0; i < uiSize; i++)
    {
      if (i < baseArray.GetCount())
        out[i] = baseArray[i];
    }

    xiiUInt32 uiCountLeft = xiiMath::Min(uiSize, leftArray.GetCount());
    for (xiiUInt32 i = 0; i < uiCountLeft; i++)
    {
      if (leftArray[i] != baseArray[i])
        out[i] = leftArray[i];
    }

    xiiUInt32 uiCountRight = xiiMath::Min(uiSize, rightArray.GetCount());
    for (xiiUInt32 i = 0; i < uiCountRight; i++)
    {
      if (rightArray[i] != baseArray[i])
        out[i] = rightArray[i];
    }
    return;
  }

  // Move distance is NP-complete so try greedy algorithm
  struct Element
  {
    Element(const xiiVariant* pValue = nullptr, xiiInt32 iBaseIndex = -1, xiiInt32 iLeftIndex = -1, xiiInt32 iRightIndex = -1) :
      m_pValue(pValue), m_iBaseIndex(iBaseIndex), m_iLeftIndex(iLeftIndex), m_iRightIndex(iRightIndex), m_fIndex(xiiMath::MaxValue<float>())
    {
    }
    bool IsDeleted() const { return m_iBaseIndex != -1 && (m_iLeftIndex == -1 || m_iRightIndex == -1); }
    bool operator<(const Element& rhs) const { return m_fIndex < rhs.m_fIndex; }

    const xiiVariant* m_pValue;
    xiiInt32          m_iBaseIndex;
    xiiInt32          m_iLeftIndex;
    xiiInt32          m_iRightIndex;
    float             m_fIndex;
  };
  xiiDynamicArray<Element> baseOrder;
  baseOrder.Reserve(leftArray.GetCount() + rightArray.GetCount());

  // First, add up all unique elements and their position in each array.
  for (xiiInt32 i = 0; i < (xiiInt32)baseArray.GetCount(); i++)
  {
    baseOrder.PushBack(Element(&baseArray[i], i));
    baseOrder.PeekBack().m_fIndex = (float)i;
  }

  xiiDynamicArray<xiiInt32> leftOrder;
  leftOrder.SetCountUninitialized(leftArray.GetCount());
  for (xiiInt32 i = 0; i < (xiiInt32)leftArray.GetCount(); i++)
  {
    const xiiVariant& val    = leftArray[i];
    bool              bFound = false;
    for (xiiInt32 j = 0; j < (xiiInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iLeftIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iLeftIndex = i;
        leftOrder[i]      = j;
        bFound            = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      leftOrder[i] = (xiiInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&leftArray[i], -1, i));
    }
  }

  xiiDynamicArray<xiiInt32> rightOrder;
  rightOrder.SetCountUninitialized(rightArray.GetCount());
  for (xiiInt32 i = 0; i < (xiiInt32)rightArray.GetCount(); i++)
  {
    const xiiVariant& val    = rightArray[i];
    bool              bFound = false;
    for (xiiInt32 j = 0; j < (xiiInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iRightIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iRightIndex = i;
        rightOrder[i]      = j;
        bFound             = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      rightOrder[i] = (xiiInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&rightArray[i], -1, -1, i));
    }
  }

  // Re-order greedy
  float fLastElement = -0.5f;
  for (xiiInt32 i = 0; i < (xiiInt32)leftOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[leftOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = xiiMath::MaxValue<float>();
    for (xiiInt32 j = i + 1; j < (xiiInt32)leftOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[leftOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }

  fLastElement = -0.5f;
  for (xiiInt32 i = 0; i < (xiiInt32)rightOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[rightOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = xiiMath::MaxValue<float>();
    for (xiiInt32 j = i + 1; j < (xiiInt32)rightOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[rightOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }


  // Sort
  baseOrder.Sort();
  out.Reserve(baseOrder.GetCount());
  for (const Element& elem : baseOrder)
  {
    if (!elem.IsDeleted())
    {
      out.PushBack(*elem.m_pValue);
    }
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_AbstractObjectGraph);
