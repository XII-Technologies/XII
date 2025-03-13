#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct DocumentNodeManager_NodeMetaData
{
  xiiVec2 m_Pos = xiiVec2::MakeZero();
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, DocumentNodeManager_NodeMetaData);

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(DocumentNodeManager_NodeMetaData, xiiNoBase, 1, xiiRTTIDefaultAllocator<DocumentNodeManager_NodeMetaData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Node::Pos", m_Pos),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

struct DocumentNodeManager_ConnectionMetaData
{
  xiiUuid   m_Source;
  xiiUuid   m_Target;
  xiiString m_SourcePin;
  xiiString m_TargetPin;

  bool IsValid() const { return m_Source.IsValid() && m_Target.IsValid(); }
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, DocumentNodeManager_ConnectionMetaData);

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(DocumentNodeManager_ConnectionMetaData, xiiNoBase, 1, xiiRTTIDefaultAllocator<DocumentNodeManager_ConnectionMetaData>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Connection::Source", m_Source),
    XII_MEMBER_PROPERTY("Connection::Target", m_Target),
    XII_MEMBER_PROPERTY("Connection::SourcePin", m_SourcePin),
    XII_MEMBER_PROPERTY("Connection::TargetPin", m_TargetPin),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

class DocumentNodeManager_DefaultConnection : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(DocumentNodeManager_DefaultConnection, xiiReflectedClass);
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(DocumentNodeManager_DefaultConnection, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiDocumentObject_ConnectionBase
////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDocumentObject_ConnectionBase, 1, xiiRTTIDefaultAllocator<xiiDocumentObject_ConnectionBase>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Source", m_Source)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Target", m_Target)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("SourcePin", m_SourcePin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("TargetPin", m_TargetPin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiDocumentNodeManager
////////////////////////////////////////////////////////////////////////

xiiDocumentNodeManager::xiiDocumentNodeManager()
{
  m_ObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiDocumentNodeManager::ObjectHandler, this));
  m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiDocumentNodeManager::StructureEventHandler, this));
  m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiDocumentNodeManager::PropertyEventsHandler, this));
}

xiiDocumentNodeManager::~xiiDocumentNodeManager()
{
  m_ObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiDocumentNodeManager::ObjectHandler, this));
  m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiDocumentNodeManager::StructureEventHandler, this));
  m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiDocumentNodeManager::PropertyEventsHandler, this));
}

void xiiDocumentNodeManager::GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const
{
  xiiHybridArray<const xiiRTTI*, 32> types;
  GetCreateableTypes(types);

  for (auto pType : types)
  {
    auto& nodeTemplate   = out_templates.ExpandAndGetRef();
    nodeTemplate.m_pType = pType;
  }
}

const xiiRTTI* xiiDocumentNodeManager::GetConnectionType() const
{
  return xiiGetStaticRTTI<xiiDocumentObject_ConnectionBase>();
}

xiiVec2 xiiDocumentNodeManager::GetNodePos(const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  XII_ASSERT_DEV(it.IsValid(), "Can't get pos of objects that aren't nodes!");

  return it.Value().m_vPos;
}

const xiiConnection& xiiDocumentNodeManager::GetConnection(const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");

  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  XII_ASSERT_DEV(it.IsValid(), "Can't get connection for objects that aren't connections!");

  return *it.Value();
}

const xiiConnection* xiiDocumentNodeManager::GetConnectionIfExists(const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  return it.IsValid() ? it.Value().Borrow() : nullptr;
}

const xiiPin* xiiDocumentNodeManager::GetInputPinByName(const xiiDocumentObject* pObject, xiiStringView sName) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  XII_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");

  for (auto& pPin : it.Value().m_Inputs)
  {
    if (pPin->GetName() == sName)
      return pPin.Borrow();
  }
  return nullptr;
}

const xiiPin* xiiDocumentNodeManager::GetOutputPinByName(const xiiDocumentObject* pObject, xiiStringView sName) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  XII_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");

  for (auto& pPin : it.Value().m_Outputs)
  {
    if (pPin->GetName() == sName)
      return pPin.Borrow();
  }
  return nullptr;
}

xiiArrayPtr<const xiiUniquePtr<const xiiPin>> xiiDocumentNodeManager::GetInputPins(const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  XII_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");

  return xiiMakeArrayPtr((xiiUniquePtr<const xiiPin>*)it.Value().m_Inputs.GetData(), it.Value().m_Inputs.GetCount());
}

xiiArrayPtr<const xiiUniquePtr<const xiiPin>> xiiDocumentNodeManager::GetOutputPins(const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  XII_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");

  return xiiMakeArrayPtr((xiiUniquePtr<const xiiPin>*)it.Value().m_Outputs.GetData(), it.Value().m_Outputs.GetCount());
}

bool xiiDocumentNodeManager::IsNode(const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");

  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsNode(pObject);
}

bool xiiDocumentNodeManager::IsConnection(const xiiDocumentObject* pObject) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");

  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsConnection(pObject);
}

bool xiiDocumentNodeManager::IsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const
{
  if (IsNode(pObject) == false)
    return false;

  if (pProp == nullptr)
    return false;

  return InternalIsDynamicPinProperty(pObject, pProp);
}

xiiArrayPtr<const xiiConnection* const> xiiDocumentNodeManager::GetConnections(const xiiPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  if (it.IsValid())
  {
    return it.Value();
  }

  return xiiArrayPtr<const xiiConnection* const>();
}

bool xiiDocumentNodeManager::HasConnections(const xiiPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  return it.IsValid() && it.Value().IsEmpty() == false;
}

bool xiiDocumentNodeManager::IsConnected(const xiiPin& source, const xiiPin& target) const
{
  auto it = m_Connections.Find(&source);
  if (it.IsValid())
  {
    for (auto pConnection : it.Value())
    {
      if (&pConnection->GetTargetPin() == &target)
        return true;
    }
  }

  return false;
}

xiiStatus xiiDocumentNodeManager::CanConnect(const xiiRTTI* pObjectType, const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNever;

  if (pObjectType == nullptr || pObjectType->IsDerivedFrom(GetConnectionType()) == false)
    return xiiStatus("Invalid connection object type");

  if (source.m_Type != xiiPin::Type::Output)
    return xiiStatus("Source pin is not an output pin.");
  if (target.m_Type != xiiPin::Type::Input)
    return xiiStatus("Target pin is not an input pin.");

  if (source.m_pParent == target.m_pParent)
    return xiiStatus("Nodes cannot be connect with themselves.");

  if (IsConnected(source, target))
    return xiiStatus("Pins already connected.");

  return InternalCanConnect(source, target, out_result);
}

xiiStatus xiiDocumentNodeManager::CanDisconnect(const xiiConnection* pConnection) const
{
  if (pConnection == nullptr)
    return xiiStatus("Invalid connection");

  return InternalCanDisconnect(pConnection->GetSourcePin(), pConnection->GetTargetPin());
}

xiiStatus xiiDocumentNodeManager::CanDisconnect(const xiiDocumentObject* pObject) const
{
  if (!IsConnection(pObject))
    return xiiStatus("Invalid connection object");

  const xiiConnection& connection = GetConnection(pObject);
  return InternalCanDisconnect(connection.GetSourcePin(), connection.GetTargetPin());
}

xiiStatus xiiDocumentNodeManager::CanMoveNode(const xiiDocumentObject* pObject, const xiiVec2& vPos) const
{
  XII_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (!IsNode(pObject))
    return xiiStatus("The given object is not a node!");

  return InternalCanMoveNode(pObject, vPos);
}

void xiiDocumentNodeManager::Connect(const xiiDocumentObject* pObject, const xiiPin& source, const xiiPin& target)
{
  xiiDocumentNodeManager::CanConnectResult res = CanConnectResult::ConnectNever;
  XII_IGNORE_UNUSED(res);
  XII_ASSERT_DEBUG(CanConnect(pObject->GetType(), source, target, res).m_Result.Succeeded(), "Connect: Sanity check failed!");

  XII_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("Source") == source.GetParent()->GetGuid(), "Property should have been set at this point already");
  XII_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("Target") == target.GetParent()->GetGuid(), "Property should have been set at this point already");
  XII_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("SourcePin") == source.GetName(), "Property should have been set at this point already");
  XII_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("TargetPin") == target.GetName(), "Property should have been set at this point already");

  auto pConnection = XII_DEFAULT_NEW(xiiConnection, source, target, pObject);
  m_ObjectToConnection.Insert(pObject->GetGuid(), pConnection);

  m_Connections[&source].PushBack(pConnection);
  m_Connections[&target].PushBack(pConnection);

  {
    xiiDocumentNodeManagerEvent e(xiiDocumentNodeManagerEvent::Type::AfterPinsConnected, pObject);
    m_NodeEvents.Broadcast(e);
  }
}

void xiiDocumentNodeManager::Disconnect(const xiiDocumentObject* pObject)
{
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  XII_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");
  XII_ASSERT_DEBUG(CanDisconnect(pObject).m_Result.Succeeded(), "Disconnect: Sanity check failed!");

  {
    xiiDocumentNodeManagerEvent e(xiiDocumentNodeManagerEvent::Type::BeforePinsDisonnected, pObject);
    m_NodeEvents.Broadcast(e);
  }

  auto&         pConnection = it.Value();
  const xiiPin& source      = pConnection->GetSourcePin();
  const xiiPin& target      = pConnection->GetTargetPin();
  m_Connections[&source].RemoveAndCopy(pConnection.Borrow());
  m_Connections[&target].RemoveAndCopy(pConnection.Borrow());

  m_ObjectToConnection.Remove(it);
}

void xiiDocumentNodeManager::MoveNode(const xiiDocumentObject* pObject, const xiiVec2& vPos)
{
  XII_ASSERT_DEBUG(CanMoveNode(pObject, vPos).m_Result.Succeeded(), "MoveNode: Sanity check failed!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  XII_ASSERT_DEBUG(it.IsValid(), "Moveable node does not exist, CanMoveNode impl invalid!");
  it.Value().m_vPos = vPos;

  xiiDocumentNodeManagerEvent e(xiiDocumentNodeManagerEvent::Type::NodeMoved, pObject);
  m_NodeEvents.Broadcast(e);
}

void xiiDocumentNodeManager::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& ref_graph) const
{
  auto pNodeMetaDataType       = xiiGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = xiiGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  xiiRttiConverterContext context;
  xiiRttiConverterWriter  rttiConverter(&ref_graph, &context, true, true);

  for (auto it = ref_graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    auto*          pAbstractObject = it.Value();
    const xiiUuid& guid            = pAbstractObject->GetGuid();

    {
      auto it2 = m_ObjectToNode.Find(guid);
      if (it2.IsValid())
      {
        const NodeInternal& node = it2.Value();

        DocumentNodeManager_NodeMetaData nodeMetaData;
        nodeMetaData.m_Pos = node.m_vPos;
        rttiConverter.AddProperties(pAbstractObject, pNodeMetaDataType, &nodeMetaData);
      }
    }
  }
}

void xiiDocumentNodeManager::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  xiiCommandHistory* history = GetDocument()->GetCommandHistory();

  auto pNodeMetaDataType       = xiiGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = xiiGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  xiiRttiConverterContext context;
  xiiRttiConverterReader  rttiConverter(&graph, &context);

  // Ensure that all nodes have their pins created
  for (auto it : graph.GetAllNodes())
  {
    auto               pAbstractObject = it.Value();
    xiiDocumentObject* pObject         = GetObject(pAbstractObject->GetGuid());
    if (pObject != nullptr && IsNode(pObject))
    {
      auto& nodeInternal = m_ObjectToNode[pObject->GetGuid()];
      if (nodeInternal.m_Inputs.IsEmpty() && nodeInternal.m_Outputs.IsEmpty())
      {
        InternalCreatePins(pObject, nodeInternal);
      }
    }
  }

  for (auto it : graph.GetAllNodes())
  {
    auto               pAbstractObject = it.Value();
    xiiDocumentObject* pObject         = GetObject(pAbstractObject->GetGuid());
    if (pObject == nullptr)
      continue;

    if (IsNode(pObject))
    {
      DocumentNodeManager_NodeMetaData nodeMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pNodeMetaDataType, &nodeMetaData);

      if (CanMoveNode(pObject, nodeMetaData.m_Pos).m_Result.Succeeded())
      {
        if (bUndoable)
        {
          xiiMoveNodeCommand move;
          move.m_Object = pObject->GetGuid();
          move.m_NewPos = nodeMetaData.m_Pos;
          history->AddCommand(move).LogFailure();
        }
        else
        {
          MoveNode(pObject, nodeMetaData.m_Pos);
        }
      }

      XII_ASSERT_DEV(pAbstractObject->FindProperty("Node::Connections") == nullptr, "Old file format detected that is not supported anymore. Re-save the document with a previous version of xii. ({})", GetDocument()->GetDocumentPath());
    }
    else if (IsConnection(pObject))
    {
      xiiVariant sourceVar    = pObject->GetTypeAccessor().GetValue("Source");
      xiiVariant targetVar    = pObject->GetTypeAccessor().GetValue("Target");
      xiiVariant sourcePinVar = pObject->GetTypeAccessor().GetValue("SourcePin");
      xiiVariant targetPinVar = pObject->GetTypeAccessor().GetValue("TargetPin");
      XII_ASSERT_DEV(sourceVar.IsA<xiiUuid>() && targetVar.IsA<xiiUuid>() && sourcePinVar.IsA<xiiString>() && targetPinVar.IsA<xiiString>(), "Invalid connection object");

      xiiUuid       source    = sourceVar.Get<xiiUuid>();
      xiiUuid       target    = targetVar.Get<xiiUuid>();
      xiiStringView sourcePin = sourcePinVar.Get<xiiString>();
      xiiStringView targetPin = targetPinVar.Get<xiiString>();

      const xiiPin* pSourcePin = nullptr;
      const xiiPin* pTargetPin = nullptr;
      if (ResolveConnection(source, target, sourcePin, targetPin, pSourcePin, pTargetPin).Failed())
      {
        // Try to restore from metadata
        DocumentNodeManager_ConnectionMetaData connectionMetaData;
        rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);
        if (connectionMetaData.IsValid())
        {
          pObject->GetTypeAccessor().SetValue("Source", connectionMetaData.m_Source);
          pObject->GetTypeAccessor().SetValue("Target", connectionMetaData.m_Target);
          pObject->GetTypeAccessor().SetValue("SourcePin", connectionMetaData.m_SourcePin);
          pObject->GetTypeAccessor().SetValue("TargetPin", connectionMetaData.m_TargetPin);

          source    = connectionMetaData.m_Source;
          target    = connectionMetaData.m_Target;
          sourcePin = connectionMetaData.m_SourcePin;
          targetPin = connectionMetaData.m_TargetPin;
        }
      }

      if (ResolveConnection(source, target, sourcePin, targetPin, pSourcePin, pTargetPin).Succeeded())
      {
        if (bUndoable)
        {
          xiiConnectNodePinsCommand cmd;
          cmd.m_ConnectionObject = pObject->GetGuid();
          cmd.m_ObjectSource     = pSourcePin->GetParent()->GetGuid();
          cmd.m_ObjectTarget     = pTargetPin->GetParent()->GetGuid();
          cmd.m_sSourcePin       = pSourcePin->GetName();
          cmd.m_sTargetPin       = pTargetPin->GetName();
          history->AddCommand(cmd).LogFailure();
        }
        else
        {
          Connect(pObject, *pSourcePin, *pTargetPin);
        }
      }
      else
      {
        RemoveObject(pObject);
        DestroyObject(pObject);
      }
    }
    else
    {
      DocumentNodeManager_ConnectionMetaData connectionMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);

      if (connectionMetaData.IsValid() == false)
        continue;

      const xiiPin* pSourcePin = nullptr;
      const xiiPin* pTargetPin = nullptr;
      if (ResolveConnection(connectionMetaData.m_Source, connectionMetaData.m_Target, connectionMetaData.m_SourcePin, connectionMetaData.m_TargetPin, pSourcePin, pTargetPin).Succeeded())
      {
        xiiDocumentObject* pNewConnectionObject = CreateObject(GetConnectionType());
        pNewConnectionObject->GetTypeAccessor().SetValue("Source", connectionMetaData.m_Source);
        pNewConnectionObject->GetTypeAccessor().SetValue("Target", connectionMetaData.m_Target);
        pNewConnectionObject->GetTypeAccessor().SetValue("SourcePin", connectionMetaData.m_SourcePin);
        pNewConnectionObject->GetTypeAccessor().SetValue("TargetPin", connectionMetaData.m_TargetPin);
        AddObject(pNewConnectionObject, nullptr, "", -1);

        XII_ASSERT_DEV(bUndoable == false, "This code path should only be taken by document loading code");
        Connect(pNewConnectionObject, *pSourcePin, *pTargetPin);
      }

      RemoveObject(pObject);
      DestroyObject(pObject);
    }
  }
}

void xiiDocumentNodeManager::GetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  if (IsNode(pObject))
  {
    // The node position is not hashed here since the hash is only used for asset transform
    // and for that the node position is irrelevant.
  }
  else if (IsConnection(pObject))
  {
    const xiiConnection& connection = GetConnection(pObject);
    const xiiPin&        sourcePin  = connection.GetSourcePin();
    const xiiPin&        targetPin  = connection.GetTargetPin();

    inout_uiHash = xiiHashingUtils::xxHash64(&sourcePin.GetParent()->GetGuid(), sizeof(xiiUuid), inout_uiHash);
    inout_uiHash = xiiHashingUtils::xxHash64(&targetPin.GetParent()->GetGuid(), sizeof(xiiUuid), inout_uiHash);
    inout_uiHash = xiiHashingUtils::xxHash64String(sourcePin.GetName(), inout_uiHash);
    inout_uiHash = xiiHashingUtils::xxHash64String(targetPin.GetName(), inout_uiHash);
  }
}

bool xiiDocumentNodeManager::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph) const
{
  const auto& selection = GetDocument()->GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return false;

  xiiDocumentObjectConverterWriter writer(&out_objectGraph, this);

  xiiHashSet<const xiiDocumentObject*> copiedNodes;
  for (const xiiDocumentObject* pObject : selection)
  {
    // Only add nodes here, connections are then collected below to ensure
    // that we always include only valid connections within the copied subgraph no matter if they are selected or not.
    if (IsNode(pObject))
    {
      // objects are required to be named root but this is not enforced or obvious by the interface.
      writer.AddObjectToGraph(pObject, "root");
      copiedNodes.Insert(pObject);
    }
  }

  xiiHashSet<const xiiDocumentObject*> copiedConnections;
  for (const xiiDocumentObject* pNodeObject : selection)
  {
    if (IsNode(pNodeObject) == false)
      continue;

    auto outputs = GetOutputPins(pNodeObject);
    for (auto& pSourcePin : outputs)
    {
      auto connections = GetConnections(*pSourcePin);
      for (const xiiConnection* pConnection : connections)
      {
        const xiiDocumentObject* pConnectionObject = pConnection->GetParent();

        XII_ASSERT_DEV(pSourcePin == &pConnection->GetSourcePin(), "");
        if (copiedConnections.Contains(pConnectionObject) == false && copiedNodes.Contains(pConnection->GetTargetPin().GetParent()))
        {
          writer.AddObjectToGraph(pConnectionObject, "root");
          copiedConnections.Insert(pConnectionObject);
        }
      }
    }
  }

  AttachMetaDataBeforeSaving(out_objectGraph);

  return true;
}

bool xiiDocumentNodeManager::PasteObjects(const xiiArrayPtr<xiiDocument::PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, const xiiVec2& vPickedPosition, bool bAllowPickedPosition)
{
  bool                               bAddedAll = true;
  xiiDeque<const xiiDocumentObject*> AddedObjects;

  for (const auto& pi : info)
  {
    // only add nodes that are allowed to be added
    if (CanAdd(pi.m_pObject->GetTypeAccessor().GetType(), nullptr, "Children", pi.m_Index).m_Result.Succeeded())
    {
      AddedObjects.PushBack(pi.m_pObject);
      AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      bAddedAll = false;
    }
  }

  RestoreMetaDataAfterLoading(objectGraph, true);

  if (!AddedObjects.IsEmpty() && bAllowPickedPosition)
  {
    xiiCommandHistory* history = GetDocument()->GetCommandHistory();

    xiiVec2   vAvgPos(0);
    xiiUInt32 nodeCount = 0;
    for (const xiiDocumentObject* pObject : AddedObjects)
    {
      if (IsNode(pObject))
      {
        vAvgPos += GetNodePos(pObject);
        ++nodeCount;
      }
    }

    vAvgPos /= (float)nodeCount;
    const xiiVec2 vMoveNode = -vAvgPos + vPickedPosition;

    for (const xiiDocumentObject* pObject : AddedObjects)
    {
      if (IsNode(pObject))
      {
        xiiMoveNodeCommand move;
        move.m_Object = pObject->GetGuid();
        move.m_NewPos = GetNodePos(pObject) + vMoveNode;
        history->AddCommand(move).LogFailure();
      }
    }

    if (!bAddedAll)
    {
      xiiLog::Info("[EditorStatus]Not all nodes were allowed to be added to the document");
    }
  }

  GetDocument()->GetSelectionManager()->SetSelection(AddedObjects);
  return true;
}

bool xiiDocumentNodeManager::CanReachNode(const xiiDocumentObject* pSource, const xiiDocumentObject* pTarget, xiiSet<const xiiDocumentObject*>& Visited) const
{
  if (pSource == pTarget)
    return true;

  if (Visited.Contains(pSource))
    return false;

  Visited.Insert(pSource);

  auto outputs = GetOutputPins(pSource);
  for (auto& pSourcePin : outputs)
  {
    auto connections = GetConnections(*pSourcePin);
    for (const xiiConnection* pConnection : connections)
    {
      if (CanReachNode(pConnection->GetTargetPin().GetParent(), pTarget, Visited))
        return true;
    }
  }

  return false;
}


bool xiiDocumentNodeManager::WouldConnectionCreateCircle(const xiiPin& source, const xiiPin& target) const
{
  const xiiDocumentObject*         pSourceNode = source.GetParent();
  const xiiDocumentObject*         pTargetNode = target.GetParent();
  xiiSet<const xiiDocumentObject*> Visited;

  return CanReachNode(pTargetNode, pSourceNode, Visited);
}

xiiResult xiiDocumentNodeManager::ResolveConnection(const xiiUuid& sourceObject, const xiiUuid& targetObject, xiiStringView sourcePin, xiiStringView targetPin, const xiiPin*& out_pSourcePin, const xiiPin*& out_pTargetPin) const
{
  const xiiDocumentObject* pSource = GetObject(sourceObject);
  const xiiDocumentObject* pTarget = GetObject(targetObject);
  if (pSource == nullptr || pTarget == nullptr)
  {
    return XII_FAILURE;
  }

  const xiiPin* pSourcePin = GetOutputPinByName(pSource, sourcePin);
  if (pSourcePin == nullptr)
  {
    xiiLog::Error("Unknown output pin '{}' on '{}'. The connection has been removed.", sourcePin, pSource->GetType()->GetTypeName());
    return XII_FAILURE;
  }

  const xiiPin* pTargetPin = GetInputPinByName(pTarget, targetPin);
  if (pTargetPin == nullptr)
  {
    xiiLog::Error("Unknown input pin '{}' on '{}'. The connection has been removed.", targetPin, pTarget->GetType()->GetTypeName());
    return XII_FAILURE;
  }

  out_pSourcePin = pSourcePin;
  out_pTargetPin = pTargetPin;
  return XII_SUCCESS;
}

void xiiDocumentNodeManager::GetDynamicPinNames(const xiiDocumentObject* pObject, xiiStringView sPropertyName, xiiStringView sPinName, xiiDynamicArray<xiiString>& out_Names) const
{
  out_Names.Clear();

  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sPropertyName);
  if (pProp == nullptr)
  {
    xiiLog::Warning("Property '{0}' not found in type '{1}'", sPropertyName, pObject->GetType()->GetTypeName());
    return;
  }

  xiiStringBuilder sTemp;
  xiiVariant       value = pObject->GetTypeAccessor().GetValue(sPropertyName);

  if (pProp->GetCategory() == xiiPropertyCategory::Member)
  {
    if (value.CanConvertTo<xiiUInt32>())
    {
      xiiUInt32 uiCount = value.ConvertTo<xiiUInt32>();
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}[{}]", sPinName, i);
        out_Names.PushBack(sTemp);
      }
    }
  }
  else if (pProp->GetCategory() == xiiPropertyCategory::Array)
  {
    auto pArrayProp = static_cast<const xiiAbstractArrayProperty*>(pProp);

    auto&           a       = value.Get<xiiVariantArray>();
    const xiiUInt32 uiCount = a.GetCount();

    auto variantType = pArrayProp->GetSpecificType()->GetVariantType();
    if (variantType >= xiiVariantType::Int8 && variantType <= xiiVariantType::UInt64)
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}", a[i]);
        out_Names.PushBack(sTemp);
      }
    }
    else if (variantType == xiiVariantType::String || variantType == xiiVariantType::StringView || variantType == xiiVariantType::HashedString)
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        out_Names.PushBack(a[i].ConvertTo<xiiString>());
      }
    }
    else if (pArrayProp->GetSpecificType()->GetTypeFlags().IsSet(xiiTypeFlags::Class))
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        auto pInnerObject = GetObject(a[i].Get<xiiUuid>());
        if (pInnerObject == nullptr)
          continue;

        xiiVariant nameVar = pInnerObject->GetTypeAccessor().GetValue("Name");
        if (nameVar.IsString() || nameVar.IsHashedString())
        {
          out_Names.PushBack(nameVar.ConvertTo<xiiString>());
        }
        else
        {
          sTemp.SetFormat("{}[{}]", sPinName, i);
          out_Names.PushBack(sTemp);
        }
      }
    }
    else
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}[{}]", sPinName, i);
        out_Names.PushBack(sTemp);
      }
    }
  }
}

bool xiiDocumentNodeManager::TryRecreatePins(const xiiDocumentObject* pObject)
{
  if (!IsNode(pObject))
    return false;

  auto& nodeInternal = m_ObjectToNode[pObject->GetGuid()];

  for (auto& pPin : nodeInternal.m_Inputs)
  {
    if (HasConnections(*pPin))
    {
      xiiLog::Error("Can't re-create pins if they are still connected");
      return false;
    }
  }

  for (auto& pPin : nodeInternal.m_Outputs)
  {
    if (HasConnections(*pPin))
    {
      xiiLog::Error("Can't re-create pins if they are still connected");
      return false;
    }
  }

  {
    xiiDocumentNodeManagerEvent e(xiiDocumentNodeManagerEvent::Type::BeforePinsChanged, pObject);
    m_NodeEvents.Broadcast(e);
  }

  nodeInternal.m_Inputs.Clear();
  nodeInternal.m_Outputs.Clear();
  InternalCreatePins(pObject, nodeInternal);

  {
    xiiDocumentNodeManagerEvent e(xiiDocumentNodeManagerEvent::Type::AfterPinsChanged, pObject);
    m_NodeEvents.Broadcast(e);
  }

  return true;
}

bool xiiDocumentNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  return true;
}

bool xiiDocumentNodeManager::InternalIsConnection(const xiiDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom(GetConnectionType());
}

xiiStatus xiiDocumentNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return xiiStatus(XII_SUCCESS);
}

void xiiDocumentNodeManager::ObjectHandler(const xiiDocumentObjectEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiDocumentObjectEvent::Type::AfterObjectCreated:
    {
      if (IsNode(e.m_pObject))
      {
        XII_ASSERT_DEBUG(!m_ObjectToNode.Contains(e.m_pObject->GetGuid()), "Sanity check failed!");
        m_ObjectToNode[e.m_pObject->GetGuid()] = NodeInternal();
      }
      else if (IsConnection(e.m_pObject))
      {
        // Nothing to do here: Map entries are created in Connect method.
      }
    }
    break;
    case xiiDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      if (IsNode(e.m_pObject))
      {
        auto it = m_ObjectToNode.Find(e.m_pObject->GetGuid());
        XII_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");

        m_ObjectToNode.Remove(it);
      }
      else if (IsConnection(e.m_pObject))
      {
        // Nothing to do here: Map entries are removed in Disconnect method.
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiDocumentNodeManager::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        auto& nodeInternal = m_ObjectToNode[e.m_pObject->GetGuid()];
        if (nodeInternal.m_Inputs.IsEmpty() && nodeInternal.m_Outputs.IsEmpty())
        {
          InternalCreatePins(e.m_pObject, nodeInternal);
          // TODO: Sanity check pins (duplicate names etc).
        }

        xiiDocumentNodeManagerEvent e2(xiiDocumentNodeManagerEvent::Type::BeforeNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        xiiDocumentNodeManagerEvent e2(xiiDocumentNodeManagerEvent::Type::AfterNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
      else
      {
        HandlePotentialDynamicPinPropertyChanged(e.m_pNewParent, e.m_sParentProperty);
      }
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      if (IsNode(e.m_pObject))
      {
        xiiDocumentNodeManagerEvent e2(xiiDocumentNodeManagerEvent::Type::BeforeNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (IsNode(e.m_pObject))
      {
        xiiDocumentNodeManagerEvent e2(xiiDocumentNodeManagerEvent::Type::AfterNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
      else
      {
        HandlePotentialDynamicPinPropertyChanged(e.m_pPreviousParent, e.m_sParentProperty);
      }
    }
    break;

    default:
      break;
  }
}

void xiiDocumentNodeManager::PropertyEventsHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == nullptr)
    return;

  HandlePotentialDynamicPinPropertyChanged(e.m_pObject, e.m_sProperty);

  if (const xiiDocumentObject* pParent = e.m_pObject->GetParent())
  {
    HandlePotentialDynamicPinPropertyChanged(pParent, e.m_pObject->GetParentProperty());
  }
}

void xiiDocumentNodeManager::HandlePotentialDynamicPinPropertyChanged(const xiiDocumentObject* pObject, xiiStringView sPropertyName)
{
  if (pObject == nullptr)
    return;

  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sPropertyName);
  if (pProp == nullptr)
    return;

  if (IsDynamicPinProperty(pObject, pProp))
  {
    TryRecreatePins(pObject);
  }
}
