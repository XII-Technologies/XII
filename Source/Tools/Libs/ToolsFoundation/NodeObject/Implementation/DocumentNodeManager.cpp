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
  xiiVec2 m_Pos = xiiVec2::ZeroVector();
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

const xiiRTTI* xiiDocumentNodeManager::GetConnectionType() const
{
  return xiiGetStaticRTTI<DocumentNodeManager_DefaultConnection>();
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

    {
      auto it2 = m_ObjectToConnection.Find(guid);
      if (it2.IsValid())
      {
        const xiiConnection& connection = *it2.Value();
        const xiiPin&        sourcePin  = connection.GetSourcePin();
        const xiiPin&        targetPin  = connection.GetTargetPin();

        DocumentNodeManager_ConnectionMetaData connectionMetaData;
        connectionMetaData.m_Source    = sourcePin.GetParent()->GetGuid();
        connectionMetaData.m_Target    = targetPin.GetParent()->GetGuid();
        connectionMetaData.m_SourcePin = sourcePin.GetName();
        connectionMetaData.m_TargetPin = targetPin.GetName();
        rttiConverter.AddProperties(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);
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

      // Backwards compatibility to old file format
      if (auto pOldConnections = pAbstractObject->FindProperty("Node::Connections"))
      {
        XII_ASSERT_DEV(bUndoable == false, "Undo not supported for old file format");
        RestoreOldMetaDataAfterLoading(graph, *pOldConnections, pObject);
      }
    }
    else if (IsConnection(pObject))
    {
      DocumentNodeManager_ConnectionMetaData connectionMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);

      xiiDocumentObject* pSource = GetObject(connectionMetaData.m_Source);
      xiiDocumentObject* pTarget = GetObject(connectionMetaData.m_Target);
      if (pSource == nullptr || pTarget == nullptr)
      {
        RemoveObject(pObject);
        DestroyObject(pObject);
        continue;
      }

      const xiiPin* pSourcePin = GetOutputPinByName(pSource, connectionMetaData.m_SourcePin);
      const xiiPin* pTargetPin = GetInputPinByName(pTarget, connectionMetaData.m_TargetPin);
      if (pSourcePin == nullptr || pTargetPin == nullptr)
      {
        RemoveObject(pObject);
        DestroyObject(pObject);
        continue;
      }

      xiiDocumentNodeManager::CanConnectResult res;
      if (CanConnect(pObject->GetType(), *pSourcePin, *pTargetPin, res).m_Result.Failed())
      {
        RemoveObject(pObject);
        DestroyObject(pObject);
        continue;
      }

      if (bUndoable)
      {
        xiiConnectNodePinsCommand cmd;
        cmd.m_ConnectionObject = pObject->GetGuid();
        cmd.m_ObjectSource     = connectionMetaData.m_Source;
        cmd.m_ObjectTarget     = connectionMetaData.m_Target;
        cmd.m_sSourcePin       = connectionMetaData.m_SourcePin;
        cmd.m_sTargetPin       = connectionMetaData.m_TargetPin;
        history->AddCommand(cmd).LogFailure();
      }
      else
      {
        Connect(pObject, *pSourcePin, *pTargetPin);
      }
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
        sTemp.Format("{}[{}]", sPinName, i);
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
        sTemp.Format("{}", a[i]);
        out_Names.PushBack(sTemp);
      }
    }
    else if (variantType == xiiVariantType::String || variantType == xiiVariantType::HashedString)
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        out_Names.PushBack(a[i].ConvertTo<xiiString>());
      }
    }
    else
    {
      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.Format("{}[{}]", sPinName, i);
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
      return false;
  }

  for (auto& pPin : nodeInternal.m_Outputs)
  {
    if (HasConnections(*pPin))
      return false;
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

  const xiiAbstractProperty* pProp = e.m_pObject->GetType()->FindPropertyByName(e.m_sProperty);
  if (pProp == nullptr)
    return;

  if (IsDynamicPinProperty(e.m_pObject, pProp))
  {
    TryRecreatePins(e.m_pObject);
  }
}

void xiiDocumentNodeManager::RestoreOldMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, const xiiAbstractObjectNode::Property& connectionsProperty, const xiiDocumentObject* pSourceObject)
{
  if (connectionsProperty.m_Value.IsA<xiiVariantArray>() == false)
    return;

  const xiiVariantArray& array = connectionsProperty.m_Value.Get<xiiVariantArray>();
  for (const xiiVariant& var : array)
  {
    if (var.IsA<xiiUuid>() == false)
      continue;

    auto pOldConnectionAbstractObject = graph.GetNode(var.Get<xiiUuid>());
    auto pTargetProperty              = pOldConnectionAbstractObject->FindProperty("Target");
    if (pTargetProperty == nullptr || pTargetProperty->m_Value.IsA<xiiUuid>() == false)
      continue;

    xiiDocumentObject* pTargetObject = GetObject(pTargetProperty->m_Value.Get<xiiUuid>());
    if (pTargetObject == nullptr)
      continue;

    auto pSourcePinProperty = pOldConnectionAbstractObject->FindProperty("SourcePin");
    if (pSourcePinProperty == nullptr || pSourcePinProperty->m_Value.IsA<xiiString>() == false)
      continue;

    auto pTargetPinProperty = pOldConnectionAbstractObject->FindProperty("TargetPin");
    if (pTargetPinProperty == nullptr || pTargetPinProperty->m_Value.IsA<xiiString>() == false)
      continue;

    const xiiPin* pSourcePin = GetOutputPinByName(pSourceObject, pSourcePinProperty->m_Value.Get<xiiString>());
    const xiiPin* pTargetPin = GetInputPinByName(pTargetObject, pTargetPinProperty->m_Value.Get<xiiString>());
    if (pSourcePin == nullptr || pTargetPin == nullptr)
      continue;

    const xiiRTTI*                           pConnectionType = GetConnectionType();
    xiiDocumentNodeManager::CanConnectResult res;
    if (CanConnect(pConnectionType, *pSourcePin, *pTargetPin, res).m_Result.Succeeded())
    {
      xiiUuid ObjectGuid;
      ObjectGuid.CreateNewUuid();
      xiiDocumentObject* pConnectionObject = CreateObject(pConnectionType, ObjectGuid);

      AddObject(pConnectionObject, nullptr, "", -1);

      Connect(pConnectionObject, *pSourcePin, *pTargetPin);
    }
  }
}
