#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/GameObject.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptBasicNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

xiiVisualScriptInstance::xiiVisualScriptInstance()
{
  SetupPinDataTypeConversions();
}

xiiVisualScriptInstance::~xiiVisualScriptInstance()
{
  Clear();
}

void xiiVisualScriptInstance::Clear()
{
  for (xiiUInt32 i = 0; i < m_Nodes.GetCount(); ++i)
  {
    m_Nodes[i]->GetDynamicRTTI()->GetAllocator()->Deallocate(m_Nodes[i]);
  }

  m_pWorld = nullptr;
  m_Nodes.Clear();
  m_ExecutionConnections.Clear();
  m_DataConnections.Clear();
  m_LocalVariables.Clear();
  m_hScriptResource.Invalidate();
}


void xiiVisualScriptInstance::ComputeNodeDependencies()
{
  m_NodeDependencies.SetCount(m_Nodes.GetCount());

  for (auto it = m_DataConnections.GetIterator(); it.IsValid(); ++it)
  {
    const xiiVisualScriptPinConnectionID src = it.Key();

    const xiiUInt16 uiSourceNode = (src >> 16) & 0xFFFF;

    if (m_Nodes[uiSourceNode]->IsManuallyStepped())
      continue;

    const xiiHybridArray<DataPinConnection, 2>& dst = it.Value();

    for (const DataPinConnection& target : dst)
    {
      m_NodeDependencies[target.m_uiTargetNode].PushBack(uiSourceNode);
    }
  }
}


void xiiVisualScriptInstance::ExecuteDependentNodes(xiiUInt16 uiNode)
{
  const auto& dep = m_NodeDependencies[uiNode];
  for (xiiUInt32 i = 0; i < dep.GetCount(); ++i)
  {
    const xiiUInt16 uiDependency = dep[i];
    auto*           pNode        = m_Nodes[uiDependency];

    // recurse to the most dependent nodes first
    ExecuteDependentNodes(uiDependency);

    // only nodes that are not manually stepped are in the dependency list
    // so we do not need to filter those out here
    pNode->Execute(this, 0);
    pNode->m_bInputValuesChanged = false;
  }
}

void xiiVisualScriptInstance::Configure(const xiiVisualScriptResourceHandle& hScript, xiiComponent* pOwnerComponent)
{
  Clear();

  xiiResourceLock<xiiVisualScriptResource> pScript(hScript, xiiResourceAcquireMode::BlockTillLoaded);
  const auto&                              resource = pScript->GetDescriptor();
  m_pMessageHandlers                                = &resource.m_MessageHandlers;

  m_hScriptResource = hScript;

  if (pOwnerComponent)
  {
    m_hOwnerObject    = pOwnerComponent->GetOwner()->GetHandle();
    m_hOwnerComponent = pOwnerComponent->GetHandle();
    m_pWorld          = pOwnerComponent->GetWorld();
  }

  m_Nodes.Reserve(resource.m_Nodes.GetCount());

  for (xiiUInt32 n = 0; n < resource.m_Nodes.GetCount(); ++n)
  {
    const auto& node = resource.m_Nodes[n];

    if (node.m_isFunctionCall)
    {
      CreateFunctionCallNode(n, resource);
    }
    else if (node.m_pType && node.m_pType->IsDerivedFrom<xiiMessage>())
    {
      if (node.m_isMsgSender)
      {
        CreateMessageSenderNode(n, resource);
      }
      else if (node.m_isMsgHandler)
      {
        CreateMessageHandlerNode(n, resource);
      }
    }
    else if (node.m_pType && node.m_pType->IsDerivedFrom<xiiVisualScriptNode>())
    {
      CreateVisualScriptNode(n, resource);
    }
    else
    {
      xiiLog::Error("Invalid node type '{0}' in visual script", node.m_sTypeName);
      Clear();
      return;
    }
  }

  m_ExecutionConnections.Reserve(resource.m_ExecutionPaths.GetCount());

  for (const auto& con : resource.m_ExecutionPaths)
  {
    ConnectExecutionPins(con.m_uiSourceNode, con.m_uiOutputPin, con.m_uiTargetNode, con.m_uiInputPin);
  }

  for (const auto& con : resource.m_DataPaths)
  {
    ConnectDataPins(con.m_uiSourceNode, con.m_uiOutputPin, (xiiVisualScriptDataPinType::Enum)con.m_uiOutputPinType, con.m_uiTargetNode,
                    con.m_uiInputPin, (xiiVisualScriptDataPinType::Enum)con.m_uiInputPinType);
  }

  ComputeNodeDependencies();

  // initialize local variables
  {
    for (const auto& p : resource.m_BoolParameters)
    {
      m_LocalVariables.StoreBool(p.m_sName, p.m_Value);
    }

    for (const auto& p : resource.m_NumberParameters)
    {
      m_LocalVariables.StoreDouble(p.m_sName, p.m_Value);
    }

    for (const auto& p : resource.m_StringParameters)
    {
      m_LocalVariables.StoreString(p.m_sName, p.m_sValue);
    }
  }
}


void xiiVisualScriptInstance::CreateVisualScriptNode(xiiUInt32 uiNodeIdx, const xiiVisualScriptResourceDescriptor& resource)
{
  XII_ASSERT_DEBUG(uiNodeIdx < xiiMath::MaxValue<xiiUInt16>(), "Max supported node index is 16 bit.");

  const auto& node = resource.m_Nodes[uiNodeIdx];

  xiiVisualScriptNode* pNode = node.m_pType->GetAllocator()->Allocate<xiiVisualScriptNode>();
  pNode->m_uiNodeID          = static_cast<xiiUInt16>(uiNodeIdx);

  resource.AssignNodeProperties(*pNode, node);

  m_Nodes.PushBack(pNode);
}

void xiiVisualScriptInstance::CreateMessageSenderNode(xiiUInt32 uiNodeIdx, const xiiVisualScriptResourceDescriptor& resource)
{
  XII_ASSERT_DEBUG(uiNodeIdx < xiiMath::MaxValue<xiiUInt16>(), "Max supported node index is 16 bit.");

  const auto& node = resource.m_Nodes[uiNodeIdx];

  xiiVisualScriptNode_MessageSender* pNode = xiiGetStaticRTTI<xiiVisualScriptNode_MessageSender>()->GetAllocator()->Allocate<xiiVisualScriptNode_MessageSender>();
  pNode->m_uiNodeID                        = static_cast<xiiUInt16>(uiNodeIdx);

  auto pMessage = node.m_pType->GetAllocator()->Allocate<xiiMessage>();

  // assign all property values
  {
    for (xiiUInt32 i = 0; i < node.m_uiNumProperties; ++i)
    {
      const xiiUInt32 uiProp = node.m_uiFirstProperty + i;
      const auto&     prop   = resource.m_Properties[uiProp];

      xiiAbstractProperty* pAbstract = pMessage->GetDynamicRTTI()->FindPropertyByName(prop.m_sName);
      if (pAbstract == nullptr)
      {
        if (prop.m_sName == "Delay" && prop.m_Value.CanConvertTo<xiiTime>())
        {
          pNode->m_Delay = prop.m_Value.ConvertTo<xiiTime>();
        }
        if (prop.m_sName == "Recursive" && prop.m_Value.CanConvertTo<bool>())
        {
          pNode->m_bRecursive = prop.m_Value.ConvertTo<bool>();
        }

        continue;
      }

      if (pAbstract->GetCategory() != xiiPropertyCategory::Member)
        continue;

      xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbstract);
      xiiReflectionUtils::SetMemberPropertyValue(pMember, pMessage, prop.m_Value);
    }
  }

  pNode->SetMessageToSend(pMessage);
  m_Nodes.PushBack(pNode);
}


void xiiVisualScriptInstance::CreateMessageHandlerNode(xiiUInt32 uiNodeIdx, const xiiVisualScriptResourceDescriptor& resource)
{
  XII_ASSERT_DEBUG(uiNodeIdx < xiiMath::MaxValue<xiiUInt16>(), "Max supported node index is 16 bit.");

  const auto& node = resource.m_Nodes[uiNodeIdx];

  xiiVisualScriptNode_MessageHandler* pNode = xiiGetStaticRTTI<xiiVisualScriptNode_MessageHandler>()->GetAllocator()->Allocate<xiiVisualScriptNode_MessageHandler>();
  pNode->m_uiNodeID                         = static_cast<xiiUInt16>(uiNodeIdx);
  pNode->m_pMessageTypeToHandle             = node.m_pType;

  m_Nodes.PushBack(pNode);
}

void xiiVisualScriptInstance::CreateFunctionCallNode(xiiUInt32 uiNodeIdx, const xiiVisualScriptResourceDescriptor& resource)
{
  XII_ASSERT_DEBUG(uiNodeIdx < xiiMath::MaxValue<xiiUInt16>(), "Max supported node index is 16 bit.");

  const auto& node = resource.m_Nodes[uiNodeIdx];

  xiiVisualScriptNode_FunctionCall* pNode =
    xiiGetStaticRTTI<xiiVisualScriptNode_FunctionCall>()->GetAllocator()->Allocate<xiiVisualScriptNode_FunctionCall>();
  pNode->m_uiNodeID = static_cast<xiiUInt16>(uiNodeIdx);

  pNode->m_pExpectedType = node.m_pType;

  if (pNode->m_pExpectedType != nullptr)
  {
    xiiStringBuilder sFunc = node.m_sTypeName.FindSubString("::");
    sFunc.Shrink(2, 0);

    const xiiScriptableFunctionAttribute* pSfAttr = nullptr;
    pNode->m_pFunctionToCall                      = SearchForScriptableFunctionOnType(pNode->m_pExpectedType, sFunc, pSfAttr);

    if (pNode->m_pFunctionToCall != nullptr)
    {
      pNode->m_ArgumentIsOutParamMask = 0;
      pNode->m_Arguments.SetCount(pNode->m_pFunctionToCall->GetArgumentCount());

      // initialize the variants to the proper type
      for (xiiUInt32 arg = 0; arg < pNode->m_pFunctionToCall->GetArgumentCount(); ++arg)
      {
        pNode->m_Arguments[arg] = xiiReflectionUtils::GetDefaultVariantFromType(pNode->m_pFunctionToCall->GetArgumentType(arg)->GetVariantType());
        xiiVisualScriptDataPinType::EnforceSupportedType(pNode->m_Arguments[arg]);

        if (pSfAttr->GetArgumentType(arg) != xiiScriptableFunctionAttribute::In) // out or inout
        {
          pNode->m_ArgumentIsOutParamMask |= XII_BIT(arg);
        }
      }

      xiiVariant tmpVal;

      // assign all property values
      for (xiiUInt32 uiPropIdx = 0; uiPropIdx < node.m_uiNumProperties; ++uiPropIdx)
      {
        const xiiUInt32 uiProp = node.m_uiFirstProperty + uiPropIdx;
        const auto&     prop   = resource.m_Properties[uiProp];

        if (prop.m_iMappingIndex >= 0 || prop.m_iMappingIndex < (xiiInt32)pNode->m_Arguments.GetCount())
        {
          xiiResult      couldConvert = XII_SUCCESS;
          const xiiRTTI* pTargetType  = pNode->m_pFunctionToCall->GetArgumentType(prop.m_iMappingIndex);
          if (pTargetType == xiiGetStaticRTTI<xiiVariant>())
          {
            tmpVal = prop.m_Value;
          }
          else
          {
            tmpVal = prop.m_Value.ConvertTo(pNode->m_Arguments[prop.m_iMappingIndex].GetType(), &couldConvert);
          }

          if (couldConvert.Succeeded())
          {
            pNode->m_Arguments[prop.m_iMappingIndex] = tmpVal;
          }
        }
      }
    }
    else
    {
      xiiLog::Error("Function '{}' does not exist on type '{}'", sFunc, node.m_pType->GetTypeName());
    }
  }
  else
  {
    xiiLog::Error("Expected target object type is null for vis script function call node '{}'", node.m_sTypeName);
  }

  m_Nodes.PushBack(pNode);
}

xiiAbstractFunctionProperty* xiiVisualScriptInstance::SearchForScriptableFunctionOnType(const xiiRTTI* pObjectType, xiiStringView sFuncName, const xiiScriptableFunctionAttribute*& out_pSfAttr) const
{
  if (sFuncName.IsEmpty())
    return nullptr;

  while (pObjectType != nullptr)
  {
    for (auto pFunc : pObjectType->GetFunctions())
    {
      if (sFuncName != pFunc->GetPropertyName())
        continue;

      out_pSfAttr = pFunc->GetAttributeByType<xiiScriptableFunctionAttribute>();

      if (out_pSfAttr == nullptr)
        continue;

      return pFunc;
    }

    pObjectType = pObjectType->GetParentType();
  }

  return nullptr;
}

void xiiVisualScriptInstance::ExecuteScript(xiiVisualScriptInstanceActivity* pActivity /*= nullptr*/)
{
  m_pActivity = pActivity;

  if (m_pActivity != nullptr)
  {
    m_pActivity->Clear();
  }

  const xiiUInt16 uiNodeCount = static_cast<xiiUInt16>(m_Nodes.GetCount());
  for (xiiUInt16 i = 0; i < uiNodeCount; ++i)
  {
    auto* pNode = m_Nodes[i];

    if (pNode->m_bStepNode)
    {
      ExecuteDependentNodes(i);

      // node stepping is always executed, even if the node only 'wants' to be executed on input change
      pNode->m_bStepNode = false;
      pNode->Execute(this, 0);
      pNode->m_bInputValuesChanged = false;
    }
  }
}

bool xiiVisualScriptInstance::HandleMessage(xiiMessage& msg)
{
  if (m_pMessageHandlers == nullptr)
    return false;

  xiiUInt32 uiFirstHandler = m_pMessageHandlers->LowerBound(msg.GetId());

  bool bHandled = false;

  while (uiFirstHandler < m_pMessageHandlers->GetCount())
  {
    const auto& data = (*m_pMessageHandlers).GetPair(uiFirstHandler);
    if (data.key != msg.GetId())
      break;

    const xiiUInt32 uiNodeId = data.value;
    m_Nodes[uiNodeId]->HandleMessage(&msg);

    bHandled = true;
    ++uiFirstHandler;
  }

  return bHandled;
}

void xiiVisualScriptInstance::ConnectExecutionPins(xiiUInt16 uiSourceNode, xiiUInt8 uiOutputSlot, xiiUInt16 uiTargetNode, xiiUInt8 uiTargetPin)
{
  auto& con          = m_ExecutionConnections[((xiiUInt32)uiSourceNode << 16) | (xiiUInt32)uiOutputSlot];
  con.m_uiTargetNode = uiTargetNode;
  con.m_uiTargetPin  = uiTargetPin;
}

void xiiVisualScriptInstance::ConnectDataPins(xiiUInt16 uiSourceNode, xiiUInt8 uiSourcePin, xiiVisualScriptDataPinType::Enum sourcePinType, xiiUInt16 uiTargetNode, xiiUInt8 uiTargetPin, xiiVisualScriptDataPinType::Enum targetPinType)
{
  DataPinConnection& con = m_DataConnections[((xiiUInt32)uiSourceNode << 16) | (xiiUInt32)uiSourcePin].ExpandAndGetRef();
  con.m_uiTargetNode     = uiTargetNode;
  con.m_uiTargetPin      = uiTargetPin;
  con.m_pTargetData      = m_Nodes[uiTargetNode]->GetInputPinDataPointer(uiTargetPin);
  con.m_AssignFunc       = FindDataPinAssignFunction(sourcePinType, targetPinType);
}

void xiiVisualScriptInstance::SetOutputPinValue(const xiiVisualScriptNode* pNode, xiiUInt8 uiPin, const void* pValue)
{
  const xiiUInt32 uiConnectionID = ((xiiUInt32)pNode->m_uiNodeID << 16) | (xiiUInt32)uiPin;

  xiiHybridArray<DataPinConnection, 2>* TargetNodeAndPins;
  if (!m_DataConnections.TryGetValue(uiConnectionID, TargetNodeAndPins))
    return;

  for (const DataPinConnection& TargetNodeAndPin : *TargetNodeAndPins)
  {
    if (TargetNodeAndPin.m_AssignFunc)
    {
      if (TargetNodeAndPin.m_AssignFunc(pValue, TargetNodeAndPin.m_pTargetData))
      {
        m_Nodes[TargetNodeAndPin.m_uiTargetNode]->m_bInputValuesChanged = true;
      }
    }
  }

  if (m_pActivity != nullptr)
  {
    m_pActivity->m_ActiveDataConnections.PushBack(uiConnectionID);
  }
}

void xiiVisualScriptInstance::ExecuteConnectedNodes(const xiiVisualScriptNode* pNode, xiiUInt16 uiNthTarget)
{
  XII_ASSERT_DEBUG(pNode->IsManuallyStepped(), "Only visual script nodes that are flagged as manually stepped may call ExecuteConnectedNodes().\n\
Otherwise visual script graph execution can end up in an endless recursion with stack-overflow.\n\
Override xiiVisualScriptNode::IsManuallyStepped() for type '{}' if necessary.",
                   pNode->GetDynamicRTTI()->GetTypeName());

  const xiiUInt32 uiConnectionID = ((xiiUInt32)pNode->m_uiNodeID << 16) | (xiiUInt32)uiNthTarget;

  ExecPinConnection TargetNode;
  if (!m_ExecutionConnections.TryGetValue(uiConnectionID, TargetNode))
    return;

  auto* pTargetNode = m_Nodes[TargetNode.m_uiTargetNode];

  ExecuteDependentNodes(TargetNode.m_uiTargetNode);

  pTargetNode->Execute(this, TargetNode.m_uiTargetPin);
  pTargetNode->m_bInputValuesChanged = false;

  if (m_pActivity != nullptr)
  {
    m_pActivity->m_ActiveExecutionConnections.PushBack(uiConnectionID);
  }
}

bool xiiVisualScriptInstance::HandlesMessage(const xiiMessage& msg) const
{
  if (m_pMessageHandlers == nullptr)
    return false;

  return m_pMessageHandlers->Contains(msg.GetId());
}



XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptInstance);
