#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptPin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptPin::xiiVisualScriptPin(Type type, xiiStringView sName, const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, const xiiDocumentObject* pObject, xiiUInt32 uiDataPinIndex) :
  xiiPin(type, sName, pinDesc.GetColor(), pObject), m_pDataType(pinDesc.m_pDataType), m_DeductTypeFunc(pinDesc.m_DeductTypeFunc), m_uiDataPinIndex(uiDataPinIndex), m_ScriptDataType(pinDesc.m_ScriptDataType), m_bRequired(pinDesc.m_bRequired), m_bHasDynamicPinProperty(pinDesc.m_sDynamicPinProperty.IsEmpty() == false), m_bSplitExecution(pinDesc.m_bSplitExecution)
{
  if (pinDesc.IsExecutionPin())
  {
    m_Shape = Shape::Arrow;
  }
  else
  {
    m_Shape = (pinDesc.m_ScriptDataType == xiiVisualScriptDataType::Array || pinDesc.m_ScriptDataType == xiiVisualScriptDataType::Map) ? Shape::Rect : Shape::Circle;
  }
}

xiiVisualScriptPin::~xiiVisualScriptPin()
{
  auto pManager = static_cast<xiiVisualScriptNodeManager*>(const_cast<xiiDocumentObjectManager*>(GetParent()->GetDocumentObjectManager()));
  pManager->RemoveDeductedPinType(*this);
}

xiiVisualScriptDataType::Enum xiiVisualScriptPin::GetResolvedScriptDataType() const
{
  if (m_ScriptDataType == xiiVisualScriptDataType::AnyPointer || m_ScriptDataType == xiiVisualScriptDataType::Any)
  {
    auto pManager = static_cast<const xiiVisualScriptNodeManager*>(GetParent()->GetDocumentObjectManager());
    return pManager->GetDeductedType(*this);
  }

  return m_ScriptDataType;
}

xiiStringView xiiVisualScriptPin::GetDataTypeName() const
{
  xiiVisualScriptDataType::Enum resolvedDataType = GetResolvedScriptDataType();
  if (resolvedDataType == xiiVisualScriptDataType::Invalid)
  {
    return xiiVisualScriptDataType::GetName(m_ScriptDataType);
  }

  if ((resolvedDataType == xiiVisualScriptDataType::TypedPointer || resolvedDataType == xiiVisualScriptDataType::EnumValue) && m_pDataType != nullptr)
  {
    return m_pDataType->GetTypeName();
  }

  return xiiVisualScriptDataType::GetName(resolvedDataType);
}

bool xiiVisualScriptPin::CanConvertTo(const xiiVisualScriptPin& targetPin, bool bUseResolvedDataTypes /*= true*/) const
{
  xiiVisualScriptDataType::Enum sourceScriptDataType = bUseResolvedDataTypes ? GetResolvedScriptDataType() : GetScriptDataType();
  xiiVisualScriptDataType::Enum targetScriptDataType = bUseResolvedDataTypes ? targetPin.GetResolvedScriptDataType() : targetPin.GetScriptDataType();

  const xiiRTTI* pSourceDataType = m_pDataType;
  const xiiRTTI* pTargetDataType = targetPin.GetDataType();

  if (xiiVisualScriptDataType::IsPointer(sourceScriptDataType) && targetScriptDataType == xiiVisualScriptDataType::AnyPointer)
    return true;

  if (sourceScriptDataType == xiiVisualScriptDataType::TypedPointer && pSourceDataType != nullptr && targetScriptDataType == xiiVisualScriptDataType::TypedPointer && pTargetDataType != nullptr)
    return pSourceDataType->IsDerivedFrom(pTargetDataType);

  if (sourceScriptDataType == xiiVisualScriptDataType::EnumValue && pSourceDataType != nullptr && targetScriptDataType == xiiVisualScriptDataType::EnumValue && pTargetDataType != nullptr)
    return pSourceDataType == pTargetDataType;

  if (sourceScriptDataType == xiiVisualScriptDataType::Any || targetScriptDataType == xiiVisualScriptDataType::Any)
    return true;

  return xiiVisualScriptDataType::CanConvertTo(sourceScriptDataType, targetScriptDataType);
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptNodeManager::xiiVisualScriptNodeManager()
{
  m_NodeEvents.AddEventHandler(xiiMakeDelegate(&xiiVisualScriptNodeManager::NodeEventsHandler, this));
  m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiVisualScriptNodeManager::PropertyEventsHandler, this));
}

xiiVisualScriptNodeManager::~xiiVisualScriptNodeManager() = default;

xiiHashedString xiiVisualScriptNodeManager::GetScriptBaseClass() const
{
  xiiHashedString sBaseClass;
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    xiiVariant baseClass = GetRootObject()->GetChildren()[0]->GetTypeAccessor().GetValue("BaseClass");
    if (baseClass.IsA<xiiString>())
    {
      sBaseClass.Assign(baseClass.Get<xiiString>());
    }
  }
  return sBaseClass;
}

bool xiiVisualScriptNodeManager::IsFilteredByBaseClass(const xiiRTTI* pNodeType, const xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, const xiiHashedString& sBaseClass, bool bLogWarning /*= false*/) const
{
  if (nodeDesc.m_sFilterByBaseClass.IsEmpty() == false && nodeDesc.m_sFilterByBaseClass != sBaseClass)
  {
    if (bLogWarning)
    {
      xiiStringView sTypeName = pNodeType->GetTypeName();
      sTypeName.TrimWordStart(xiiVisualScriptNodeRegistry::s_szTypeNamePrefix);

      xiiLog::Warning("The base class function '{}' is not a function of the currently selected base class '{}' and will be skipped", sTypeName, sBaseClass);
    }

    return true;
  }

  return false;
}


xiiVisualScriptDataType::Enum xiiVisualScriptNodeManager::GetVariableType(xiiTempHashedString sName) const
{
  xiiVariant defaultValue;
  GetVariableDefaultValue(sName, defaultValue).IgnoreResult();
  return xiiVisualScriptDataType::FromVariantType(defaultValue.GetType());
}

xiiResult xiiVisualScriptNodeManager::GetVariableDefaultValue(xiiTempHashedString sName, xiiVariant& out_value) const
{
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    auto&     typeAccessor   = GetRootObject()->GetChildren()[0]->GetTypeAccessor();
    xiiUInt32 uiNumVariables = typeAccessor.GetCount("Variables");
    for (xiiUInt32 i = 0; i < uiNumVariables; ++i)
    {
      xiiVariant variableUuid = typeAccessor.GetValue("Variables", i);
      if (variableUuid.IsA<xiiUuid>() == false)
        continue;

      auto pVariableObject = GetObject(variableUuid.Get<xiiUuid>());
      if (pVariableObject == nullptr)
        continue;

      xiiVariant nameVar = pVariableObject->GetTypeAccessor().GetValue("Name");
      if (nameVar.IsA<xiiHashedString>() == false || nameVar.Get<xiiHashedString>() != sName)
        continue;

      out_value = pVariableObject->GetTypeAccessor().GetValue("DefaultValue");
      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

void xiiVisualScriptNodeManager::GetInputExecutionPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetInputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(*pPin);
    if (vsPin.IsExecutionPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void xiiVisualScriptNodeManager::GetOutputExecutionPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetOutputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(*pPin);
    if (vsPin.IsExecutionPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void xiiVisualScriptNodeManager::GetInputDataPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetInputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(*pPin);
    if (vsPin.IsDataPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void xiiVisualScriptNodeManager::GetOutputDataPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetOutputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = xiiStaticCast<const xiiVisualScriptPin&>(*pPin);
    if (vsPin.IsDataPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void xiiVisualScriptNodeManager::GetEntryNodes(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiDocumentObject*>& out_entryNodes) const
{
  xiiHybridArray<const xiiDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pObject);

  xiiHashSet<const xiiDocumentObject*>          visitedNodes;
  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;

  while (nodeStack.IsEmpty() == false)
  {
    const xiiDocumentObject* pCurrentNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pCurrentNode->GetType());
    if (xiiVisualScriptNodeDescription::Type::IsEntry(pNodeDesc->m_Type))
    {
      out_entryNodes.PushBack(pCurrentNode);
      continue;
    }

    GetInputExecutionPins(pCurrentNode, pins);
    for (auto pPin : pins)
    {
      auto connections = GetConnections(*pPin);
      for (auto pConnection : connections)
      {
        const xiiDocumentObject* pSourceNode = pConnection->GetSourcePin().GetParent();
        if (visitedNodes.Insert(pSourceNode))
          continue;

        nodeStack.PushBack(pSourceNode);
      }
    }
  }
}

// static
xiiStringView xiiVisualScriptNodeManager::GetNiceTypeName(const xiiDocumentObject* pObject)
{
  xiiStringView sTypeName = pObject->GetType()->GetTypeName();

  while (sTypeName.TrimWordStart(xiiVisualScriptNodeRegistry::s_szTypeNamePrefix) || sTypeName.TrimWordStart("Builtin_"))
  {
  }

  if (const char* szAngleBracket = sTypeName.FindSubString("<"))
  {
    sTypeName = xiiStringView(sTypeName.GetStartPointer(), szAngleBracket);
  }

  return sTypeName;
}

xiiStringView xiiVisualScriptNodeManager::GetNiceFunctionName(const xiiDocumentObject* pObject)
{
  xiiStringView sFunctionName = pObject->GetType()->GetTypeName();

  if (const char* szSeparator = sFunctionName.FindLastSubString("::"))
  {
    sFunctionName = xiiStringView(szSeparator + 2, sFunctionName.GetEndPointer());
  }

  return sFunctionName;
}

xiiVisualScriptDataType::Enum xiiVisualScriptNodeManager::GetDeductedType(const xiiVisualScriptPin& pin) const
{
  xiiEnum<xiiVisualScriptDataType> dataType = xiiVisualScriptDataType::Invalid;
  m_PinToDeductedType.TryGetValue(&pin, dataType);
  return dataType;
}

xiiVisualScriptDataType::Enum xiiVisualScriptNodeManager::GetDeductedType(const xiiDocumentObject* pObject) const
{
  xiiEnum<xiiVisualScriptDataType> dataType = xiiVisualScriptDataType::Invalid;
  m_ObjectToDeductedType.TryGetValue(pObject, dataType);
  return dataType;
}

bool xiiVisualScriptNodeManager::IsCoroutine(const xiiDocumentObject* pObject) const
{
  auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc != nullptr && xiiVisualScriptNodeDescription::Type::MakesOuterCoroutine(pNodeDesc->m_Type))
  {
    return true;
  }

  return m_CoroutineObjects.Contains(pObject);
}

bool xiiVisualScriptNodeManager::IsLoop(const xiiDocumentObject* pObject) const
{
  auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc != nullptr && xiiVisualScriptNodeDescription::Type::IsLoop(pNodeDesc->m_Type))
  {
    return true;
  }

  return false;
}

bool xiiVisualScriptNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType());
}

bool xiiVisualScriptNodeManager::InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const
{
  auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());

  if (pNodeDesc != nullptr && pNodeDesc->m_bHasDynamicPins)
  {
    xiiTempHashedString sPropNameHashed = xiiTempHashedString(pProp->GetPropertyName());
    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.m_sDynamicPinProperty == sPropNameHashed)
      {
        return true;
      }
    }

    for (auto& pinDesc : pNodeDesc->m_OutputPins)
    {
      if (pinDesc.m_sDynamicPinProperty == sPropNameHashed)
      {
        return true;
      }
    }
  }

  return false;
}

xiiStatus xiiVisualScriptNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  const xiiVisualScriptPin& pinSource = xiiStaticCast<const xiiVisualScriptPin&>(source);
  const xiiVisualScriptPin& pinTarget = xiiStaticCast<const xiiVisualScriptPin&>(target);

  if (pinSource.IsExecutionPin() != pinTarget.IsExecutionPin())
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus("Cannot connect data pins with execution pins.");
  }

  if (pinSource.IsDataPin() && pinSource.CanConvertTo(pinTarget, false) == false)
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus(xiiFmt("The pin data types are incompatible."));
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return xiiStatus("Connecting these pins would create a circle in the graph.");
  }

  // only one connection is allowed on DATA input pins, execution input pins may have multiple incoming connections
  if (pinTarget.IsDataPin() && HasConnections(pinTarget))
  {
    out_result = CanConnectResult::ConnectNto1;
    return xiiStatus(XII_FAILURE);
  }

  // only one outgoing connection is allowed on EXECUTION pins, data pins may have multiple outgoing connections
  if (pinSource.IsExecutionPin() && HasConnections(pinSource))
  {
    out_result = CanConnectResult::Connect1toN;
    return xiiStatus(XII_FAILURE);
  }

  out_result = CanConnectResult::ConnectNtoN;
  return xiiStatus(XII_SUCCESS);
}

void xiiVisualScriptNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());

  if (pNodeDesc == nullptr)
    return;

  xiiHybridArray<xiiString, 16> dynamicPinNames;
  auto                          CreatePins = [&](const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, xiiPin::Type type, xiiDynamicArray<xiiUniquePtr<xiiPin>>& out_pins, xiiUInt32& inout_dataPinIndex) {
    if (pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
    {
      GetDynamicPinNames(pObject, pinDesc.m_sDynamicPinProperty, pinDesc.m_sName, dynamicPinNames);
    }
    else
    {
      dynamicPinNames.Clear();
      dynamicPinNames.PushBack(pinDesc.m_sName.GetView());
    }

    for (xiiUInt32 i = 0; i < dynamicPinNames.GetCount(); ++i)
    {
      xiiUInt32 uiDataPinIndex = xiiInvalidIndex;
      if (pinDesc.IsDataPin())
      {
        uiDataPinIndex = inout_dataPinIndex;
        ++inout_dataPinIndex;
      }

      auto pPin = XII_DEFAULT_NEW(xiiVisualScriptPin, type, dynamicPinNames[i], pinDesc, pObject, uiDataPinIndex);
      out_pins.PushBack(pPin);
    }
  };

  xiiUInt32 uiDataPinIndex = 0;
  for (const auto& pinDesc : pNodeDesc->m_InputPins)
  {
    CreatePins(pinDesc, xiiPin::Type::Input, ref_node.m_Inputs, uiDataPinIndex);
  }

  uiDataPinIndex = 0;
  for (const auto& pinDesc : pNodeDesc->m_OutputPins)
  {
    CreatePins(pinDesc, xiiPin::Type::Output, ref_node.m_Outputs, uiDataPinIndex);
  }
}

void xiiVisualScriptNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const
{
  xiiHashedString sBaseClass = GetScriptBaseClass();

  for (auto it : xiiVisualScriptNodeRegistry::GetSingleton()->GetAllNodeTypes())
  {
    if (IsFilteredByBaseClass(it.Key(), it.Value(), sBaseClass))
      continue;

    if (!it.Key()->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
    {
      Types.PushBack(it.Key());
    }
  }
}

void xiiVisualScriptNodeManager::NodeEventsHandler(const xiiDocumentNodeManagerEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiDocumentNodeManagerEvent::Type::AfterPinsConnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin  = connection.GetTargetPin();
      DeductNodeTypeAndAllPinTypes(targetPin.GetParent());
      UpdateCoroutine(targetPin.GetParent(), connection);
    }
    break;

    case xiiDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin  = connection.GetTargetPin();
      DeductNodeTypeAndAllPinTypes(targetPin.GetParent(), &targetPin);
      UpdateCoroutine(targetPin.GetParent(), connection, false);
    }
    break;

    case xiiDocumentNodeManagerEvent::Type::AfterNodeAdded:
    {
      DeductNodeTypeAndAllPinTypes(e.m_pObject);
    }
    break;

    case xiiDocumentNodeManagerEvent::Type::BeforeNodeRemoved:
    {
      m_ObjectToDeductedType.Remove(e.m_pObject);
    }
    break;

    default:
      break;
  }
}

void xiiVisualScriptNodeManager::PropertyEventsHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (IsNode(e.m_pObject))
  {
    DeductNodeTypeAndAllPinTypes(e.m_pObject);
  }
  else if (e.m_sProperty == "Name" || e.m_sProperty == "DefaultValue") // a variable's name or default value has changed, re-run type deduction
  {
    for (auto pObject : GetRootObject()->GetChildren())
    {
      if (IsNode(pObject) == false)
        continue;

      DeductNodeTypeAndAllPinTypes(pObject);
    }
  }
}

void xiiVisualScriptNodeManager::RemoveDeductedPinType(const xiiVisualScriptPin& pin)
{
  m_PinToDeductedType.Remove(&pin);
}

void xiiVisualScriptNodeManager::DeductNodeTypeAndAllPinTypes(const xiiDocumentObject* pObject, const xiiPin* pDisconnectedPin /*= nullptr*/)
{
  auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc == nullptr || pNodeDesc->NeedsTypeDeduction() == false)
    return;

  if (pDisconnectedPin != nullptr && static_cast<const xiiVisualScriptPin*>(pDisconnectedPin)->NeedsTypeDeduction() == false)
    return;

  bool bNodeTypeChanged = false;
  {
    xiiEnum<xiiVisualScriptDataType> newDeductedType = pNodeDesc->m_DeductTypeFunc(pObject, static_cast<const xiiVisualScriptPin*>(pDisconnectedPin));
    xiiEnum<xiiVisualScriptDataType> oldDeductedType = xiiVisualScriptDataType::Invalid;
    m_ObjectToDeductedType.Insert(pObject, newDeductedType, &oldDeductedType);

    bNodeTypeChanged = (newDeductedType != oldDeductedType);
  }

  bool                                          bAnyInputPinChanged = false;
  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;
  GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (auto pFunc = pPin->GetDeductTypeFunc())
    {
      xiiEnum<xiiVisualScriptDataType> newDeductedType = pFunc(*pPin);
      xiiEnum<xiiVisualScriptDataType> oldDeductedType = xiiVisualScriptDataType::Invalid;
      m_PinToDeductedType.Insert(pPin, newDeductedType, &oldDeductedType);

      bAnyInputPinChanged |= (newDeductedType != oldDeductedType);
    }
  }

  bool bAnyOutputPinChanged = false;
  GetOutputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (auto pFunc = pPin->GetDeductTypeFunc())
    {
      xiiEnum<xiiVisualScriptDataType> newDeductedType = pFunc(*pPin);
      xiiEnum<xiiVisualScriptDataType> oldDeductedType = xiiVisualScriptDataType::Invalid;
      m_PinToDeductedType.Insert(pPin, newDeductedType, &oldDeductedType);

      bAnyOutputPinChanged |= (newDeductedType != oldDeductedType);
    }
  }

  if (bNodeTypeChanged || bAnyInputPinChanged || bAnyOutputPinChanged)
  {
    m_NodeChangedEvent.Broadcast(pObject);
  }

  // propagate to connected nodes
  if (bAnyOutputPinChanged)
  {
    for (auto pPin : pins)
    {
      if (pPin->NeedsTypeDeduction() == false)
        continue;

      auto connections = GetConnections(*pPin);
      for (auto& connection : connections)
      {
        DeductNodeTypeAndAllPinTypes(connection->GetTargetPin().GetParent());
      }
    }
  }
}

void xiiVisualScriptNodeManager::UpdateCoroutine(const xiiDocumentObject* pTargetNode, const xiiConnection& changedConnection, bool bIsAboutToDisconnect)
{
  auto vsPin = static_cast<const xiiVisualScriptPin&>(changedConnection.GetTargetPin());
  if (vsPin.IsExecutionPin() == false)
    return;

  xiiHybridArray<const xiiDocumentObject*, 16> entryNodes;
  GetEntryNodes(pTargetNode, entryNodes);

  for (auto pEntryNode : entryNodes)
  {
    const bool bWasCoroutine = m_CoroutineObjects.Contains(pEntryNode);
    const bool bIsCoroutine  = IsConnectedToCoroutine(pEntryNode, changedConnection, bIsAboutToDisconnect);

    if (bWasCoroutine != bIsCoroutine)
    {
      if (bIsCoroutine)
      {
        m_CoroutineObjects.Insert(pEntryNode);
      }
      else
      {
        m_CoroutineObjects.Remove(pEntryNode);
      }

      m_NodeChangedEvent.Broadcast(pEntryNode);
    }
  }
}

bool xiiVisualScriptNodeManager::IsConnectedToCoroutine(const xiiDocumentObject* pEntryNode, const xiiConnection& changedConnection, bool bIsAboutToDisconnect) const
{
  xiiHybridArray<const xiiDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pEntryNode);

  xiiHashSet<const xiiDocumentObject*>          visitedNodes;
  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;

  while (nodeStack.IsEmpty() == false)
  {
    const xiiDocumentObject* pCurrentNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pCurrentNode->GetType());
    if (xiiVisualScriptNodeDescription::Type::MakesOuterCoroutine(pNodeDesc->m_Type))
    {
      return true;
    }

    GetOutputExecutionPins(pCurrentNode, pins);
    for (auto pPin : pins)
    {
      if (pPin->SplitExecution())
        continue;

      auto connections = GetConnections(*pPin);
      for (auto pConnection : connections)
      {
        // the connection is about to be disconnected so we ignore it here
        if (bIsAboutToDisconnect && pConnection == &changedConnection)
          continue;

        const xiiDocumentObject* pTargetNode = pConnection->GetTargetPin().GetParent();
        if (visitedNodes.Insert(pTargetNode))
          continue;

        nodeStack.PushBack(pTargetNode);
      }
    }
  }

  return false;
}
