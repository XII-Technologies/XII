#pragma once

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class xiiVisualScriptPin;

class xiiVisualScriptTypeDeduction
{
public:
  static xiiVisualScriptDataType::Enum DeductFromNodeDataType(const xiiVisualScriptPin& pin);
  static xiiVisualScriptDataType::Enum DeductFromTypeProperty(const xiiVisualScriptPin& pin);
  static xiiVisualScriptDataType::Enum DeductFromExpressionInput(const xiiVisualScriptPin& pin);
  static xiiVisualScriptDataType::Enum DeductFromExpressionOutput(const xiiVisualScriptPin& pin);

  static xiiVisualScriptDataType::Enum DeductFromAllInputPins(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin);
  static xiiVisualScriptDataType::Enum DeductFromVariableNameProperty(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin);
  static xiiVisualScriptDataType::Enum DeductFromScriptDataTypeProperty(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin);
  static xiiVisualScriptDataType::Enum DeductFromPropertyProperty(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin);
  static xiiVisualScriptDataType::Enum DeductDummy(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin);

  static const xiiRTTI*             GetReflectedType(const xiiDocumentObject* pObject);
  static const xiiAbstractProperty* GetReflectedProperty(const xiiDocumentObject* pObject);

private:
  static xiiVisualScriptDataType::Enum DeductFromExpressionVariable(const xiiVisualScriptPin& pin, xiiStringView sPropertyName);
};
