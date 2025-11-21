#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromNodeDataType(const xiiVisualScriptPin& pin)
{
  auto pObject  = pin.GetParent();
  auto pManager = static_cast<const xiiVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->GetDeductedType(pObject);
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromTypeProperty(const xiiVisualScriptPin& pin)
{
  if (auto pType = GetReflectedType(pin.GetParent()))
  {
    return xiiVisualScriptDataType::FromRtti(pType);
  }

  return xiiVisualScriptDataType::Invalid;
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromExpressionInput(const xiiVisualScriptPin& pin)
{
  return DeductFromExpressionVariable(pin, "Inputs");
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromExpressionOutput(const xiiVisualScriptPin& pin)
{
  return DeductFromExpressionVariable(pin, "Outputs");
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromAllInputPins(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin)
{
  auto pManager = static_cast<const xiiVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

  xiiVisualScriptDataType::Enum deductedType = xiiVisualScriptDataType::Invalid;

  xiiHybridArray<const xiiVisualScriptPin*, 16> pins;
  pManager->GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (pPin->GetScriptDataType() != xiiVisualScriptDataType::Any)
      continue;

    // the pin is about to be disconnected so we ignore it here
    if (pPin == pDisconnectedPin)
      continue;

    xiiVisualScriptDataType::Enum pinDataType = xiiVisualScriptDataType::Invalid;
    auto                          connections = pManager->GetConnections(*pPin);
    if (connections.IsEmpty() == false)
    {
      pinDataType = static_cast<const xiiVisualScriptPin&>(connections[0]->GetSourcePin()).GetResolvedScriptDataType();
    }
    else
    {
      xiiVariant var = pObject->GetTypeAccessor().GetValue(pPin->GetName());
      pinDataType    = xiiVisualScriptDataType::FromVariantType(var.GetType());
    }

    deductedType = xiiMath::Max(deductedType, pinDataType);
  }

  return deductedType;
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromVariableNameProperty(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin)
{
  auto nameVar = pObject->GetTypeAccessor().GetValue("Name");
  if (nameVar.IsA<xiiString>())
  {
    auto pManager = static_cast<const xiiVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    return pManager->GetVariableType(xiiTempHashedString(nameVar.Get<xiiString>()));
  }
  else if (nameVar.IsA<xiiStringView>())
  {
    auto pManager = static_cast<const xiiVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    return pManager->GetVariableType(xiiTempHashedString(nameVar.Get<xiiStringView>()));
  }

  return xiiVisualScriptDataType::Invalid;
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromScriptDataTypeProperty(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin)
{
  auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<xiiInt64>())
  {
    return static_cast<xiiVisualScriptDataType::Enum>(typeVar.Get<xiiInt64>());
  }

  return xiiVisualScriptDataType::Invalid;
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromPropertyProperty(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin)
{
  if (auto pProperty = GetReflectedProperty(pObject))
  {
    return xiiVisualScriptDataType::FromRtti(pProperty->GetSpecificType());
  }

  return xiiVisualScriptDataType::Invalid;
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductDummy(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin)
{
  // nothing to do here
  return xiiVisualScriptDataType::Float;
}

// static
const xiiRTTI* xiiVisualScriptTypeDeduction::GetReflectedType(const xiiDocumentObject* pObject)
{
  auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<xiiString>() == false)
    return nullptr;

  const xiiString& sTypeName = typeVar.Get<xiiString>();
  if (sTypeName.IsEmpty())
    return nullptr;

  const xiiRTTI* pType = xiiRTTI::FindTypeByName(sTypeName);
  if (pType == nullptr && sTypeName.StartsWith("xii") == false)
  {
    xiiStringBuilder sFullTypeName;
    sFullTypeName.Set("xii", typeVar.Get<xiiString>());
    pType = xiiRTTI::FindTypeByName(sFullTypeName);
  }

  if (pType == nullptr)
  {
    xiiLog::Error("'{}' is not a valid type", typeVar.Get<xiiString>());
    return nullptr;
  }

  return pType;
}

// static
const xiiAbstractProperty* xiiVisualScriptTypeDeduction::GetReflectedProperty(const xiiDocumentObject* pObject)
{
  auto pType = GetReflectedType(pObject);
  if (pType == nullptr)
    return nullptr;

  auto propertyVar = pObject->GetTypeAccessor().GetValue("Property");
  if (propertyVar.IsA<xiiString>() == false)
    return nullptr;

  const xiiString& sPropertyName = propertyVar.Get<xiiString>();
  if (sPropertyName.IsEmpty())
    return nullptr;

  const xiiAbstractProperty* pProperty = pType->FindPropertyByName(propertyVar.Get<xiiString>());

  if (pProperty == nullptr)
  {
    xiiLog::Error("'{}' is not a valid property of '{}'", propertyVar.Get<xiiString>(), pType->GetTypeName());
    return nullptr;
  }

  return pProperty;
}

// static
xiiVisualScriptDataType::Enum xiiVisualScriptTypeDeduction::DeductFromExpressionVariable(const xiiVisualScriptPin& pin, xiiStringView sPropertyName)
{
  auto pObject = pin.GetParent();

  xiiVariant varList = pObject->GetTypeAccessor().GetValue(sPropertyName);
  if (varList.IsA<xiiVariantArray>() == false)
    return xiiVisualScriptDataType::Invalid;

  xiiVariant var = varList[pin.GetDataPinIndex()];
  if (var.IsA<xiiUuid>() == false)
    return xiiVisualScriptDataType::Invalid;

  const xiiDocumentObject* pVarObject = pObject->GetDocumentObjectManager()->GetObject(var.Get<xiiUuid>());
  if (pVarObject == nullptr)
    return xiiVisualScriptDataType::Invalid;

  xiiVariant typeVar = pVarObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<xiiInt64>() == false)
    return xiiVisualScriptDataType::Invalid;

  auto expressionDataType = static_cast<xiiVisualScriptExpressionDataType::Enum>(typeVar.Get<xiiInt64>());
  return xiiVisualScriptExpressionDataType::GetVisualScriptDataType(expressionDataType);
}
