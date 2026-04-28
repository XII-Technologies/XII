/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRemoveNodeCommand, 1, xiiRTTIDefaultAllocator<xiiRemoveNodeCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMoveNodeCommand, 1, xiiRTTIDefaultAllocator<xiiMoveNodeCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ObjectGuid", m_Object),
    XII_MEMBER_PROPERTY("NewPos", m_NewPos),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConnectNodePinsCommand, 1, xiiRTTIDefaultAllocator<xiiConnectNodePinsCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ConnectionGuid", m_ConnectionObject),
    XII_MEMBER_PROPERTY("SourceGuid", m_ObjectSource),
    XII_MEMBER_PROPERTY("TargetGuid", m_ObjectTarget),
    XII_MEMBER_PROPERTY("SourcePin", m_sSourcePin),
    XII_MEMBER_PROPERTY("TargetPin", m_sTargetPin),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDisconnectNodePinsCommand, 1, xiiRTTIDefaultAllocator<xiiDisconnectNodePinsCommand>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ConnectionGuid", m_ConnectionObject),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiRemoveNodeCommand
////////////////////////////////////////////////////////////////////////

xiiRemoveNodeCommand::xiiRemoveNodeCommand() = default;

xiiStatus xiiRemoveNodeCommand::DoInternal(bool bRedo)
{
  xiiDocument*            pDocument = GetDocument();
  xiiDocumentNodeManager* pManager  = static_cast<xiiDocumentNodeManager*>(pDocument->GetObjectManager());

  auto RemoveConnections = [&](const xiiPin& pin) -> xiiStatus {
    while (true)
    {
      auto connections = pManager->GetConnections(pin);

      if (connections.IsEmpty())
        break;

      xiiDisconnectNodePinsCommand cmd;
      cmd.m_ConnectionObject = connections[0]->GetParent()->GetGuid();
      xiiStatus res          = AddSubCommand(cmd);
      if (res.Succeeded())
      {
        xiiRemoveObjectCommand remove;
        remove.m_Object = cmd.m_ConnectionObject;
        res             = AddSubCommand(remove);
      }

      XII_SUCCEED_OR_RETURN(res);
    }
    return XII_SUCCESS;
  };

  if (!bRedo)
  {
    m_pObject = pManager->GetObject(m_Object);
    if (m_pObject == nullptr)
      return xiiStatus("Remove Node: The given object does not exist!");

    auto inputs = pManager->GetInputPins(m_pObject);
    for (auto& pPinTarget : inputs)
    {
      XII_SUCCEED_OR_RETURN(RemoveConnections(*pPinTarget));
    }

    auto outputs = pManager->GetOutputPins(m_pObject);
    for (auto& pPinSource : outputs)
    {
      XII_SUCCEED_OR_RETURN(RemoveConnections(*pPinSource));
    }

    xiiRemoveObjectCommand cmd;
    cmd.m_Object = m_Object;
    auto res     = AddSubCommand(cmd);
    if (res.Failed())
    {
      return res;
    }
  }
  return XII_SUCCESS;
}

xiiStatus xiiRemoveNodeCommand::UndoInternal(bool bFireEvents)
{
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  return XII_SUCCESS;
}

void xiiRemoveNodeCommand::CleanupInternal(CommandState state) {}


////////////////////////////////////////////////////////////////////////
// xiiMoveObjectCommand
////////////////////////////////////////////////////////////////////////

xiiMoveNodeCommand::xiiMoveNodeCommand() = default;

xiiStatus xiiMoveNodeCommand::DoInternal(bool bRedo)
{
  xiiDocument*            pDocument = GetDocument();
  xiiDocumentNodeManager* pManager  = static_cast<xiiDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
    if (m_pObject == nullptr)
      return xiiStatus("Move Node: The given object does not exist!");

    m_vOldPos = pManager->GetNodePos(m_pObject);
    XII_SUCCEED_OR_RETURN(pManager->CanMoveNode(m_pObject, m_NewPos));
  }

  pManager->MoveNode(m_pObject, m_NewPos);
  return XII_SUCCESS;
}

xiiStatus xiiMoveNodeCommand::UndoInternal(bool bFireEvents)
{
  xiiDocument*            pDocument = GetDocument();
  xiiDocumentNodeManager* pManager  = static_cast<xiiDocumentNodeManager*>(pDocument->GetObjectManager());
  XII_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  XII_SUCCEED_OR_RETURN(pManager->CanMoveNode(m_pObject, m_vOldPos));

  pManager->MoveNode(m_pObject, m_vOldPos);

  return XII_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// xiiConnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

xiiConnectNodePinsCommand::xiiConnectNodePinsCommand() = default;

xiiStatus xiiConnectNodePinsCommand::DoInternal(bool bRedo)
{
  xiiDocument*            pDocument = GetDocument();
  xiiDocumentNodeManager* pManager  = static_cast<xiiDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pConnectionObject = pManager->GetObject(m_ConnectionObject);
    if (!pManager->IsConnection(m_pConnectionObject))
      return xiiStatus("Connect Node Pins: The given connection object is not valid connection!");

    m_pObjectSource = pManager->GetObject(m_ObjectSource);
    if (m_pObjectSource == nullptr)
      return xiiStatus("Connect Node Pins: The given node does not exist!");
    m_pObjectTarget = pManager->GetObject(m_ObjectTarget);
    if (m_pObjectTarget == nullptr)
      return xiiStatus("Connect Node Pins: The given node does not exist!");
  }

  const xiiPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return xiiStatus("Connect Node Pins: The given pin does not exist!");

  const xiiPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return xiiStatus("Connect Node Pins: The given pin does not exist!");

  xiiDocumentNodeManager::CanConnectResult res;
  XII_SUCCEED_OR_RETURN(pManager->CanConnect(m_pConnectionObject->GetType(), *pOutput, *pInput, res));

  pManager->Connect(m_pConnectionObject, *pOutput, *pInput);
  return XII_SUCCESS;
}

xiiStatus xiiConnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  xiiDocument*            pDocument = GetDocument();
  xiiDocumentNodeManager* pManager  = static_cast<xiiDocumentNodeManager*>(pDocument->GetObjectManager());

  XII_SUCCEED_OR_RETURN(pManager->CanDisconnect(m_pConnectionObject));

  pManager->Disconnect(m_pConnectionObject);
  return XII_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// xiiDisconnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

xiiDisconnectNodePinsCommand::xiiDisconnectNodePinsCommand() = default;

xiiStatus xiiDisconnectNodePinsCommand::DoInternal(bool bRedo)
{
  xiiDocument*            pDocument = GetDocument();
  xiiDocumentNodeManager* pManager  = static_cast<xiiDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pConnectionObject = pManager->GetObject(m_ConnectionObject);
    if (!pManager->IsConnection(m_pConnectionObject))
      return xiiStatus("Disconnect Node Pins: The given connection object is not valid connection!");

    XII_SUCCEED_OR_RETURN(pManager->CanRemove(m_pConnectionObject));

    const xiiConnection& connection = pManager->GetConnection(m_pConnectionObject);
    const xiiPin&        pinSource  = connection.GetSourcePin();
    const xiiPin&        pinTarget  = connection.GetTargetPin();

    m_pObjectSource = pinSource.GetParent();
    m_pObjectTarget = pinTarget.GetParent();
    m_sSourcePin    = pinSource.GetName();
    m_sTargetPin    = pinTarget.GetName();
  }

  XII_SUCCEED_OR_RETURN(pManager->CanDisconnect(m_pConnectionObject));

  pManager->Disconnect(m_pConnectionObject);

  return XII_SUCCESS;
}

xiiStatus xiiDisconnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  xiiDocument*            pDocument = GetDocument();
  xiiDocumentNodeManager* pManager  = static_cast<xiiDocumentNodeManager*>(pDocument->GetObjectManager());

  const xiiPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return xiiStatus("Connect Node: The given pin does not exist!");

  const xiiPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return xiiStatus("Connect Node: The given pin does not exist!");

  xiiDocumentNodeManager::CanConnectResult res;
  XII_SUCCEED_OR_RETURN(pManager->CanConnect(m_pConnectionObject->GetType(), *pOutput, *pInput, res));

  pManager->Connect(m_pConnectionObject, *pOutput, *pInput);
  return XII_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// xiiNodeCommands
////////////////////////////////////////////////////////////////////////

// static
xiiStatus xiiNodeCommands::AddAndConnectCommand(xiiCommandHistory* pHistory, const xiiRTTI* pConnectionType, const xiiPin& sourcePin, const xiiPin& targetPin)
{
  xiiAddObjectCommand addCmd;
  addCmd.m_pType         = pConnectionType;
  addCmd.m_NewObjectGuid = xiiUuid::MakeUuid();
  addCmd.m_Index         = -1;

  XII_SUCCEED_OR_RETURN(pHistory->AddCommand(addCmd));

  constexpr xiiStringView propertyNames[] = {
    "Source"_xiisv,
    "Target"_xiisv,
    "SourcePin"_xiisv,
    "TargetPin"_xiisv,
  };
  xiiVariant propertyValues[] = {
    sourcePin.GetParent()->GetGuid(),
    targetPin.GetParent()->GetGuid(),
    sourcePin.GetName(),
    targetPin.GetName(),
  };
  static_assert(XII_ARRAY_SIZE(propertyNames) == XII_ARRAY_SIZE(propertyValues));

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(propertyNames); ++i)
  {
    xiiSetObjectPropertyCommand propCmd;
    propCmd.m_Object    = addCmd.m_NewObjectGuid;
    propCmd.m_sProperty = propertyNames[i];
    propCmd.m_NewValue  = propertyValues[i];

    XII_SUCCEED_OR_RETURN(pHistory->AddCommand(propCmd));
  }

  xiiConnectNodePinsCommand connectCmd;
  connectCmd.m_ConnectionObject = addCmd.m_NewObjectGuid;
  connectCmd.m_ObjectSource     = sourcePin.GetParent()->GetGuid();
  connectCmd.m_ObjectTarget     = targetPin.GetParent()->GetGuid();
  connectCmd.m_sSourcePin       = sourcePin.GetName();
  connectCmd.m_sTargetPin       = targetPin.GetName();

  return pHistory->AddCommand(connectCmd);
}

// static
xiiStatus xiiNodeCommands::DisconnectAndRemoveCommand(xiiCommandHistory* pHistory, const xiiUuid& connectionObject)
{
  xiiDisconnectNodePinsCommand cmd;
  cmd.m_ConnectionObject = connectionObject;

  xiiStatus res = pHistory->AddCommand(cmd);
  if (res.Succeeded())
  {
    xiiRemoveObjectCommand remove;
    remove.m_Object = cmd.m_ConnectionObject;

    res = pHistory->AddCommand(remove);
  }

  return res;
}
