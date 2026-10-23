/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNodeCommandAccessor, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiNodeCommandAccessor::xiiNodeCommandAccessor(xiiCommandHistory* pHistory) :
  xiiObjectCommandAccessor(pHistory)
{
}

xiiNodeCommandAccessor::~xiiNodeCommandAccessor() = default;

void xiiNodeCommandAccessor::BeginTemporaryCommands(xiiStringView sDisplayString, bool /*bFireEventsWhenUndoingTempCommands*/)
{
  // Undoing a live edit must restore native pins before reconnecting the original connection commands.
  xiiObjectCommandAccessor::BeginTemporaryCommands(sDisplayString, true);
}

xiiStatus xiiNodeCommandAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  auto pNodeObject         = pObject;
  auto pDynamicPinProperty = pProp;

  while (pNodeObject && !IsNode(pNodeObject))
  {
    const auto* child = pNodeObject;
    pNodeObject       = child->GetParent();
    if (pNodeObject)
      pDynamicPinProperty = pNodeObject->GetType()->FindPropertyByName(child->GetParentProperty());
  }

  if (IsDynamicPinProperty(pNodeObject, pDynamicPinProperty))
  {
    xiiHybridArray<ConnectionInfo, 16> oldConnections;
    XII_SUCCEED_OR_RETURN(DisconnectAllPins(pNodeObject, oldConnections));

    // TODO: remap oldConnections

    XII_SUCCEED_OR_RETURN(xiiObjectCommandAccessor::SetValue(pObject, pProp, newValue, index));

    return TryReconnectAllPins(pNodeObject, oldConnections);
  }

  return xiiObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
}

xiiStatus xiiNodeCommandAccessor::RefreshPins(const xiiDocumentObject* pObject)
{
  XII_ASSERT_DEV(IsNode(pObject), "Only nodes have a dependency-derived pin schema.");
  XII_ASSERT_DEV(m_pHistory->IsInTransaction(), "Pin refresh requires an active transaction.");
  xiiHybridArray<ConnectionInfo, 16> connections;
  XII_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, connections));
  auto* manager = static_cast<xiiDocumentNodeManager*>(const_cast<xiiDocumentObjectManager*>(pObject->GetDocumentObjectManager()));
  if (!manager->TryRecreatePins(pObject))
    return xiiStatus("Could not refresh the node's dependency-derived pins.");
  return TryReconnectAllPins(pObject, connections);
}

xiiStatus xiiNodeCommandAccessor::InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    xiiHybridArray<ConnectionInfo, 16> oldConnections;
    XII_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    XII_SUCCEED_OR_RETURN(xiiObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return xiiObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

xiiStatus xiiNodeCommandAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index /*= xiiVariant()*/)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    xiiHybridArray<ConnectionInfo, 16> oldConnections;
    XII_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    XII_SUCCEED_OR_RETURN(xiiObjectCommandAccessor::RemoveValue(pObject, pProp, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return xiiObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

xiiStatus xiiNodeCommandAccessor::MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    xiiHybridArray<ConnectionInfo, 16> oldConnections;
    XII_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    // TODO: remap oldConnections

    XII_SUCCEED_OR_RETURN(xiiObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return xiiObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

xiiStatus xiiNodeCommandAccessor::AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid)
{
  if (IsDynamicPinProperty(pParent, pParentProp))
  {
    xiiHybridArray<ConnectionInfo, 16> oldConnections;
    XII_SUCCEED_OR_RETURN(DisconnectAllPins(pParent, oldConnections));

    // TODO: remap oldConnections

    XII_SUCCEED_OR_RETURN(xiiObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid));

    return TryReconnectAllPins(pParent, oldConnections);
  }
  else
  {
    return xiiObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

xiiStatus xiiNodeCommandAccessor::RemoveObject(const xiiDocumentObject* pObject)
{
  if (pObject == nullptr)
    return xiiStatus("Cannot remove a null document object.");
  if (IsNode(pObject))
  {
    xiiRemoveNodeCommand command;
    command.m_Object = pObject->GetGuid();
    return m_pHistory->AddCommand(command);
  }
  if (const xiiDocumentObject* pParent = pObject->GetParent())
  {
    const xiiAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(pObject->GetParentProperty());
    if (IsDynamicPinProperty(pParent, pProp))
    {
      xiiHybridArray<ConnectionInfo, 16> oldConnections;
      XII_SUCCEED_OR_RETURN(DisconnectAllPins(pParent, oldConnections));

      // TODO: remap oldConnections

      XII_SUCCEED_OR_RETURN(xiiObjectCommandAccessor::RemoveObject(pObject));

      return TryReconnectAllPins(pParent, oldConnections);
    }
  }

  return xiiObjectCommandAccessor::RemoveObject(pObject);
}


bool xiiNodeCommandAccessor::IsNode(const xiiDocumentObject* pObject) const
{
  if (pObject == nullptr)
    return false;
  auto pManager = static_cast<const xiiDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsNode(pObject);
}

bool xiiNodeCommandAccessor::IsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const
{
  if (pObject == nullptr || pProp == nullptr)
    return false;
  auto pManager = static_cast<const xiiDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsDynamicPinProperty(pObject, pProp);
}

xiiStatus xiiNodeCommandAccessor::DisconnectAllPins(const xiiDocumentObject* pObject, xiiDynamicArray<ConnectionInfo>& out_oldConnections)
{
  auto pManager = static_cast<const xiiDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  auto Disconnect = [&](xiiArrayPtr<const xiiConnection* const> connections) -> xiiStatus {
    // Disconnect compacts the adjacency array returned by the manager.
    xiiHybridArray<const xiiConnection*, 16> snapshot;
    snapshot = connections;
    for (const xiiConnection* pConnection : snapshot)
    {
      auto& connectionInfo              = out_oldConnections.ExpandAndGetRef();
      connectionInfo.m_ConnectionObject = pConnection->GetParent()->GetGuid();
      connectionInfo.m_pSource          = pConnection->GetSourcePin().GetParent();
      connectionInfo.m_pTarget          = pConnection->GetTargetPin().GetParent();
      connectionInfo.m_sSourcePin       = pConnection->GetSourcePin().GetName();
      connectionInfo.m_sTargetPin       = pConnection->GetTargetPin().GetName();

      xiiDisconnectNodePinsCommand disconnect;
      disconnect.m_ConnectionObject = connectionInfo.m_ConnectionObject;
      XII_SUCCEED_OR_RETURN(m_pHistory->AddCommand(disconnect));
    }

    return XII_SUCCESS;
  };

  auto inputs = pManager->GetInputPins(pObject);
  for (auto& pInputPin : inputs)
  {
    XII_SUCCEED_OR_RETURN(Disconnect(pManager->GetConnections(*pInputPin)));
  }

  auto outputs = pManager->GetOutputPins(pObject);
  for (auto& pOutputPin : outputs)
  {
    XII_SUCCEED_OR_RETURN(Disconnect(pManager->GetConnections(*pOutputPin)));
  }

  return XII_SUCCESS;
}

xiiStatus xiiNodeCommandAccessor::TryReconnectAllPins(const xiiDocumentObject* pObject, const xiiDynamicArray<ConnectionInfo>& oldConnections)
{
  auto pManager = static_cast<const xiiDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  for (auto& connectionInfo : oldConnections)
  {
    const xiiPin* pSourcePin = pManager->GetOutputPinByName(connectionInfo.m_pSource, connectionInfo.m_sSourcePin);
    const xiiPin* pTargetPin = pManager->GetInputPinByName(connectionInfo.m_pTarget, connectionInfo.m_sTargetPin);

    const auto* object = pManager->GetObject(connectionInfo.m_ConnectionObject);
    if (!object)
      return xiiStatus("The disconnected connection object disappeared during a pin edit.");
    xiiDocumentNodeManager::CanConnectResult res;
    if (!pSourcePin || !pTargetPin || pManager->CanConnect(object->GetType(), *pSourcePin, *pTargetPin, res).Failed())
    {
      // Remove only connections made invalid by the edit. Undo restores their
      // original object, identity and authored properties before reconnecting.
      xiiRemoveObjectCommand remove;
      remove.m_Object = connectionInfo.m_ConnectionObject;
      XII_SUCCEED_OR_RETURN(m_pHistory->AddCommand(remove));
      continue;
    }
    xiiConnectNodePinsCommand connect;
    connect.m_ConnectionObject = connectionInfo.m_ConnectionObject;
    connect.m_ObjectSource     = connectionInfo.m_pSource->GetGuid();
    connect.m_ObjectTarget     = connectionInfo.m_pTarget->GetGuid();
    connect.m_sSourcePin       = connectionInfo.m_sSourcePin;
    connect.m_sTargetPin       = connectionInfo.m_sTargetPin;
    XII_SUCCEED_OR_RETURN(m_pHistory->AddCommand(connect));
  }

  return XII_SUCCESS;
}
