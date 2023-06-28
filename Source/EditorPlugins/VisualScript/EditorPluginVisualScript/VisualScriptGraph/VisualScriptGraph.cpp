#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptPin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptPin::xiiVisualScriptPin(Type type, xiiStringView sName, const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, const xiiDocumentObject* pObject, xiiUInt32 uiPinIndex) :
  xiiPin(type, sName, pinDesc.GetColor(), pObject), m_pDataType(pinDesc.m_pDataType), m_uiPinIndex(uiPinIndex), m_ScriptDataType(pinDesc.m_ScriptDataType), m_bRequired(pinDesc.m_bRequired), m_bHasDynamicPinProperty(pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
{
  m_Shape = pinDesc.IsExecutionPin() ? Shape::Arrow : Shape::Circle;
}

xiiStringView xiiVisualScriptPin::GetDataTypeName(xiiVisualScriptDataType::Enum deductedType) const
{
  if (m_ScriptDataType == xiiVisualScriptDataType::TypedPointer)
  {
    return m_pDataType->GetTypeName();
  }

  xiiVisualScriptDataType::Enum finalDataType = m_ScriptDataType;
  if (finalDataType == xiiVisualScriptDataType::Any && deductedType != xiiVisualScriptDataType::Invalid)
    finalDataType = deductedType;

  return xiiVisualScriptDataType::GetName(finalDataType);
}

bool xiiVisualScriptPin::CanConvertTo(const xiiVisualScriptPin& targetPin, xiiVisualScriptDataType::Enum deductedSourceDataType /*= xiiVisualScriptDataType::Invalid*/, xiiVisualScriptDataType::Enum deductedTargetDataType /*= xiiVisualScriptDataType::Invalid*/) const
{
  xiiVisualScriptDataType::Enum sourceScriptDataType = m_ScriptDataType;
  const xiiRTTI*                pSourceDataType      = m_pDataType;
  if (sourceScriptDataType == xiiVisualScriptDataType::Any && deductedSourceDataType != xiiVisualScriptDataType::Invalid)
  {
    sourceScriptDataType = deductedSourceDataType;
    pSourceDataType      = xiiVisualScriptDataType::GetRtti(sourceScriptDataType);
  }

  xiiVisualScriptDataType::Enum targetScriptDataType = targetPin.GetScriptDataType();
  const xiiRTTI*                pTargetDataType      = targetPin.GetDataType();
  XII_ASSERT_DEV(targetScriptDataType != xiiVisualScriptDataType::Invalid, "Invalid script data type '{}'", targetPin.GetDataTypeName(deductedTargetDataType));
  if (targetScriptDataType == xiiVisualScriptDataType::Any && deductedTargetDataType != xiiVisualScriptDataType::Invalid)
  {
    targetScriptDataType = deductedTargetDataType;
    pTargetDataType      = xiiVisualScriptDataType::GetRtti(targetScriptDataType);
  }

  if (sourceScriptDataType == xiiVisualScriptDataType::TypedPointer && pSourceDataType != nullptr &&
      targetScriptDataType == xiiVisualScriptDataType::TypedPointer && pTargetDataType != nullptr)
    return pSourceDataType->IsDerivedFrom(pTargetDataType);

  if (sourceScriptDataType == xiiVisualScriptDataType::Any ||
      targetScriptDataType == xiiVisualScriptDataType::Any)
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

// static
xiiStringView xiiVisualScriptNodeManager::GetNiceTypeName(const xiiDocumentObject* pObject)
{
  xiiStringView sTypeName = pObject->GetType()->GetTypeName();
  sTypeName.TrimWordStart(xiiVisualScriptNodeRegistry::s_szTypeNamePrefix, "Builtin_");

  if (const char* szAngleBracket = sTypeName.FindSubString("<"))
    sTypeName = xiiStringView(sTypeName.GetStartPointer(), szAngleBracket);

  return sTypeName;
}

void xiiVisualScriptNodeManager::DeductType(const xiiDocumentObject* pObject, const xiiPin* pChangedPin, bool bConnected)
{
  if (pChangedPin != nullptr)
  {
    auto pVsPin = xiiStaticCast<const xiiVisualScriptPin*>(pChangedPin);
    if (pVsPin->GetScriptDataType() != xiiVisualScriptDataType::Any)
      return;
  }

  xiiVisualScriptDataType::Enum deductedType = xiiVisualScriptDataType::Invalid;

  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;
  GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (pPin->GetScriptDataType() != xiiVisualScriptDataType::Any)
      continue;

    // the pin is about to be disconnected so we ignore it here
    if (bConnected == false && pPin == pChangedPin)
      continue;

    deductedType = xiiMath::Max(deductedType, GetDeductedType(*pPin));
  }

  if (deductedType == xiiVisualScriptDataType::Invalid)
  {
    auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
    if (typeVar.IsValid())
    {
      deductedType = static_cast<xiiVisualScriptDataType::Enum>(typeVar.Get<xiiInt64>());
    }
  }

  xiiEnum<xiiVisualScriptDataType> oldDeductedType = xiiVisualScriptDataType::Invalid;
  m_ObjectToDeductedType.Insert(pObject, deductedType, &oldDeductedType);

  if (deductedType != oldDeductedType)
  {
    m_DeductedTypeChangedEvent.Broadcast(pObject);
  }

  GetOutputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (pPin->GetScriptDataType() != xiiVisualScriptDataType::Any)
      continue;

    auto connections = GetConnections(*pPin);
    if (connections.IsEmpty() == false)
    {
      for (auto& connection : connections)
      {
        auto& targetPin = connection->GetTargetPin();
        DeductType(targetPin.GetParent(), &targetPin);
      }
    }
  }
}

xiiVisualScriptDataType::Enum xiiVisualScriptNodeManager::GetDeductedType(const xiiVisualScriptPin& pin) const
{
  if (pin.GetType() == xiiPin::Type::Input)
  {
    auto connections = GetConnections(pin);
    if (connections.IsEmpty() == false)
    {
      return GetDeductedType(static_cast<const xiiVisualScriptPin&>(connections[0]->GetSourcePin()));
    }
    else
    {
      xiiVariant var = pin.GetParent()->GetTypeAccessor().GetValue(pin.GetName());
      return xiiVisualScriptDataType::FromVariantType(var.GetType());
    }

    return xiiVisualScriptDataType::Invalid;
  }

  if (pin.GetScriptDataType() == xiiVisualScriptDataType::Any)
  {
    return GetDeductedType(pin.GetParent());
  }

  return pin.GetScriptDataType();
}

xiiVisualScriptDataType::Enum xiiVisualScriptNodeManager::GetDeductedType(const xiiDocumentObject* pObject) const
{
  xiiEnum<xiiVisualScriptDataType> dataType = xiiVisualScriptDataType::Invalid;
  m_ObjectToDeductedType.TryGetValue(pObject, dataType);
  return dataType;
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
    xiiTempHashedString sPropNameHashed = xiiStringView(pProp->GetPropertyName());
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

  if (pinSource.IsDataPin() && pinSource.CanConvertTo(pinTarget) == false)
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
  auto                          CreatePins = [&](const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, xiiPin::Type type, xiiDynamicArray<xiiUniquePtr<xiiPin>>& out_pins) {
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
      auto pPin = XII_DEFAULT_NEW(xiiVisualScriptPin, type, dynamicPinNames[i], pinDesc, pObject, out_pins.GetCount());
      out_pins.PushBack(pPin);
    }
  };

  for (const auto& pinDesc : pNodeDesc->m_InputPins)
  {
    CreatePins(pinDesc, xiiPin::Type::Input, ref_node.m_Inputs);
  }

  for (const auto& pinDesc : pNodeDesc->m_OutputPins)
  {
    CreatePins(pinDesc, xiiPin::Type::Output, ref_node.m_Outputs);
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
      DeductType(targetPin.GetParent(), &targetPin, true);
    }
    break;

    case xiiDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin  = connection.GetTargetPin();
      DeductType(targetPin.GetParent(), &targetPin, false);
    }
    break;

    case xiiDocumentNodeManagerEvent::Type::AfterNodeAdded:
    {
      auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(e.m_pObject->GetType());
      if (pNodeDesc->m_bNeedsDataTypeDeduction)
      {
        DeductType(e.m_pObject);
      }
    }
    break;

    default:
      break;
  }
}

void xiiVisualScriptNodeManager::PropertyEventsHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (IsNode(e.m_pObject) == false)
    return;

  auto pNodeDesc = xiiVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(e.m_pObject->GetType());

  if (pNodeDesc->m_bNeedsDataTypeDeduction && e.m_EventType == xiiDocumentObjectPropertyEvent::Type::PropertySet)
  {
    DeductType(e.m_pObject);
  }
}
