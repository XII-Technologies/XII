#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>

xiiNodeCommandAccessor::xiiNodeCommandAccessor(xiiCommandHistory* pHistory) :
  xiiObjectCommandAccessor(pHistory)
{
}

xiiNodeCommandAccessor::~xiiNodeCommandAccessor() = default;

xiiStatus xiiNodeCommandAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  if (m_pHistory->InTemporaryTransaction() == false && IsDynamicPinProperty(pObject, pProp))
  {
    xiiHybridArray<ConnectionInfo, 16> oldConnections;
    XII_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    // TODO: remap oldConnections

    XII_SUCCEED_OR_RETURN(xiiObjectCommandAccessor::SetValue(pObject, pProp, newValue, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return xiiObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
  }
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

bool xiiNodeCommandAccessor::IsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const
{
  auto pManager = static_cast<const xiiDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsDynamicPinProperty(pObject, pProp);
}

xiiStatus xiiNodeCommandAccessor::DisconnectAllPins(const xiiDocumentObject* pObject, xiiDynamicArray<ConnectionInfo>& out_oldConnections)
{
  auto pManager = static_cast<const xiiDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  auto Disconnect = [&](xiiArrayPtr<const xiiConnection* const> connections) -> xiiStatus {
    for (const xiiConnection* pConnection : connections)
    {
      auto& connectionInfo        = out_oldConnections.ExpandAndGetRef();
      connectionInfo.m_pSource    = pConnection->GetSourcePin().GetParent();
      connectionInfo.m_pTarget    = pConnection->GetTargetPin().GetParent();
      connectionInfo.m_sSourcePin = pConnection->GetSourcePin().GetName();
      connectionInfo.m_sTargetPin = pConnection->GetTargetPin().GetName();

      XII_SUCCEED_OR_RETURN(xiiNodeCommands::DisconnectAndRemoveCommand(m_pHistory, pConnection->GetParent()->GetGuid()));
    }

    return xiiStatus(XII_SUCCESS);
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

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiNodeCommandAccessor::TryReconnectAllPins(const xiiDocumentObject* pObject, const xiiDynamicArray<ConnectionInfo>& oldConnections)
{
  auto          pManager        = static_cast<const xiiDocumentNodeManager*>(pObject->GetDocumentObjectManager());
  const xiiRTTI* pConnectionType = pManager->GetConnectionType();

  for (auto& connectionInfo : oldConnections)
  {
    const xiiPin* pSourcePin = pManager->GetOutputPinByName(connectionInfo.m_pSource, connectionInfo.m_sSourcePin);
    const xiiPin* pTargetPin = pManager->GetInputPinByName(connectionInfo.m_pTarget, connectionInfo.m_sTargetPin);

    // This connection can't be restored because a pin doesn't exist anymore, which is ok in this case.
    if (pSourcePin == nullptr || pTargetPin == nullptr)
      continue;

    // This connection is not valid anymore after pins have changed.
    xiiDocumentNodeManager::CanConnectResult res;
    if (pManager->CanConnect(pConnectionType, *pSourcePin, *pTargetPin, res).Failed())
      continue;

    XII_SUCCEED_OR_RETURN(xiiNodeCommands::AddAndConnectCommand(m_pHistory, pConnectionType, *pSourcePin, *pTargetPin));
  }

  return xiiStatus(XII_SUCCESS);
}
