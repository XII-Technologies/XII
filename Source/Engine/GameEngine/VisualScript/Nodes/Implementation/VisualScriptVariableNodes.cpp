#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptVariableNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_GetNumberProperty, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_GetNumberProperty>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Properties"),
    new xiiTitleAttribute("Get Number Property '{Name}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    XII_OUTPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_GetNumberProperty::xiiVisualScriptNode_GetNumberProperty()  = default;
xiiVisualScriptNode_GetNumberProperty::~xiiVisualScriptNode_GetNumberProperty() = default;

void xiiVisualScriptNode_GetNumberProperty::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  double value = 0;

  xiiComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    xiiAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == xiiPropertyCategory::Member)
    {
      xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbsProp);

      xiiVariant var = xiiReflectionUtils::GetMemberPropertyValue(pMember, pComponent);

      if (var.CanConvertTo<double>())
      {
        value = var.ConvertTo<double>();
        pInstance->SetOutputPinValue(this, 0, &value);
        return;
      }
    }
  }

  xiiLog::Warning("Script: Number Property '{0}' could not be found on the given component.", m_sVariable);
}


void* xiiVisualScriptNode_GetNumberProperty::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_hComponent;
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_SetNumberProperty, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_SetNumberProperty>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Properties"),
    new xiiTitleAttribute("Set Number Property '{Name}' = {Value}"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value", 1, xiiVisualScriptDataPinType::Number, m_fValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_SetNumberProperty::xiiVisualScriptNode_SetNumberProperty()  = default;
xiiVisualScriptNode_SetNumberProperty::~xiiVisualScriptNode_SetNumberProperty() = default;

void xiiVisualScriptNode_SetNumberProperty::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  xiiComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    xiiAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == xiiPropertyCategory::Member)
    {
      xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbsProp);

      xiiReflectionUtils::SetMemberPropertyValue(pMember, pComponent, m_fValue);
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_SetNumberProperty::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
    case 1:
      return &m_fValue;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_GetBoolProperty, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_GetBoolProperty>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Properties"),
    new xiiTitleAttribute("Get Bool Property '{Name}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    XII_OUTPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Boolean),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_GetBoolProperty::xiiVisualScriptNode_GetBoolProperty()  = default;
xiiVisualScriptNode_GetBoolProperty::~xiiVisualScriptNode_GetBoolProperty() = default;

void xiiVisualScriptNode_GetBoolProperty::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  bool value = 0;

  xiiComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    xiiAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == xiiPropertyCategory::Member)
    {
      xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbsProp);

      xiiVariant var = xiiReflectionUtils::GetMemberPropertyValue(pMember, pComponent);

      if (var.CanConvertTo<bool>())
      {
        value = var.ConvertTo<bool>();
        pInstance->SetOutputPinValue(this, 0, &value);
        return;
      }
    }
  }

  xiiLog::Warning("Script: Bool Property '{0}' could not be found on the given component.", m_sVariable);
}


void* xiiVisualScriptNode_GetBoolProperty::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_hComponent;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_SetBoolProperty, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_SetBoolProperty>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Properties"),
    new xiiTitleAttribute("Set Bool Property '{Name}' = {Value}"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value", 1, xiiVisualScriptDataPinType::Boolean, m_bValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_SetBoolProperty::xiiVisualScriptNode_SetBoolProperty()  = default;
xiiVisualScriptNode_SetBoolProperty::~xiiVisualScriptNode_SetBoolProperty() = default;

void xiiVisualScriptNode_SetBoolProperty::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  xiiComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    xiiAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == xiiPropertyCategory::Member)
    {
      xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbsProp);

      xiiReflectionUtils::SetMemberPropertyValue(pMember, pComponent, m_bValue);
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_SetBoolProperty::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
    case 1:
      return &m_bValue;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_GetStringProperty, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_GetStringProperty>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Properties"),
    new xiiTitleAttribute("Get String Property '{Name}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    XII_OUTPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::String),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_GetStringProperty::xiiVisualScriptNode_GetStringProperty()  = default;
xiiVisualScriptNode_GetStringProperty::~xiiVisualScriptNode_GetStringProperty() = default;

void xiiVisualScriptNode_GetStringProperty::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  xiiString value;

  xiiComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    xiiAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == xiiPropertyCategory::Member)
    {
      xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbsProp);

      xiiVariant var = xiiReflectionUtils::GetMemberPropertyValue(pMember, pComponent);

      if (var.CanConvertTo<xiiString>())
      {
        value = var.ConvertTo<xiiString>();
        pInstance->SetOutputPinValue(this, 0, &value);
        return;
      }
    }
  }

  xiiLog::Warning("Script: String Property '{0}' could not be found on the given component.", m_sVariable);
}


void* xiiVisualScriptNode_GetStringProperty::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_hComponent;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_SetStringProperty, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_SetStringProperty>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Properties"),
    new xiiTitleAttribute("Set String Property '{Name}' = {Value}"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    XII_INPUT_DATA_PIN("Component", 0, xiiVisualScriptDataPinType::ComponentHandle),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value", 1, xiiVisualScriptDataPinType::String, m_sValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_SetStringProperty::xiiVisualScriptNode_SetStringProperty()  = default;
xiiVisualScriptNode_SetStringProperty::~xiiVisualScriptNode_SetStringProperty() = default;

void xiiVisualScriptNode_SetStringProperty::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  xiiComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    xiiAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == xiiPropertyCategory::Member)
    {
      xiiAbstractMemberProperty* pMember = static_cast<xiiAbstractMemberProperty*>(pAbsProp);

      xiiReflectionUtils::SetMemberPropertyValue(pMember, pComponent, m_sValue);
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_SetStringProperty::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
    case 1:
      return &m_sValue;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Number, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Number>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Variables"),
    new xiiTitleAttribute("Number '{Name}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_OUTPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Number::xiiVisualScriptNode_Number()  = default;
xiiVisualScriptNode_Number::~xiiVisualScriptNode_Number() = default;

void xiiVisualScriptNode_Number::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  double fCurrentValue = 0.0;
  pInstance->GetLocalVariables().RetrieveDouble(m_sVariable, fCurrentValue, 0.0);
  pInstance->SetOutputPinValue(this, 0, &fCurrentValue);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_StoreNumber, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_StoreNumber>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Variables"),
    new xiiTitleAttribute("Store Number '{Name}' = {Value}"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, xiiVisualScriptDataPinType::Number, m_Value),
    XII_OUTPUT_DATA_PIN("StoredValue", 0, xiiVisualScriptDataPinType::Number)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_StoreNumber::xiiVisualScriptNode_StoreNumber()  = default;
xiiVisualScriptNode_StoreNumber::~xiiVisualScriptNode_StoreNumber() = default;

void xiiVisualScriptNode_StoreNumber::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  pInstance->GetLocalVariables().StoreDouble(m_sVariable, m_Value);
  pInstance->SetOutputPinValue(this, 0, &m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_StoreNumber::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_Value;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Bool, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Bool>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Variables"),
    new xiiTitleAttribute("Bool '{Name}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_OUTPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Boolean),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Bool::xiiVisualScriptNode_Bool()  = default;
xiiVisualScriptNode_Bool::~xiiVisualScriptNode_Bool() = default;

void xiiVisualScriptNode_Bool::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  bool bCurrentValue = false;
  pInstance->GetLocalVariables().RetrieveBool(m_sVariable, bCurrentValue, false);
  pInstance->SetOutputPinValue(this, 0, &bCurrentValue);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_StoreBool, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_StoreBool>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Variables"),
    new xiiTitleAttribute("Store Bool '{Name}' = {Value}"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, xiiVisualScriptDataPinType::Boolean, m_Value),
    XII_OUTPUT_DATA_PIN("StoredValue", 0, xiiVisualScriptDataPinType::Boolean)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_StoreBool::xiiVisualScriptNode_StoreBool()  = default;
xiiVisualScriptNode_StoreBool::~xiiVisualScriptNode_StoreBool() = default;

void xiiVisualScriptNode_StoreBool::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  pInstance->GetLocalVariables().StoreBool(m_sVariable, m_Value);
  pInstance->SetOutputPinValue(this, 0, &m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_StoreBool::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_Value;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_ToggleBool, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_ToggleBool>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Variables"),
    new xiiTitleAttribute("Toggle Bool '{Name}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("OnTrue", 0),
    XII_OUTPUT_EXECUTION_PIN("OnFalse", 1),
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Boolean),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_ToggleBool::xiiVisualScriptNode_ToggleBool()  = default;
xiiVisualScriptNode_ToggleBool::~xiiVisualScriptNode_ToggleBool() = default;

void xiiVisualScriptNode_ToggleBool::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  bool bCurrentValue = false;
  pInstance->GetLocalVariables().RetrieveBool(m_sVariable, bCurrentValue, false);
  bCurrentValue = !bCurrentValue;

  pInstance->GetLocalVariables().StoreBool(m_sVariable, bCurrentValue);
  pInstance->SetOutputPinValue(this, 0, &bCurrentValue);

  pInstance->ExecuteConnectedNodes(this, bCurrentValue ? 0 : 1);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_String, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_String>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Variables"),
    new xiiTitleAttribute("String '{Name}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_OUTPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::String),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_String::xiiVisualScriptNode_String()  = default;
xiiVisualScriptNode_String::~xiiVisualScriptNode_String() = default;

void xiiVisualScriptNode_String::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  xiiString sCurrentValue;
  pInstance->GetLocalVariables().RetrieveString(m_sVariable, sCurrentValue, "");
  pInstance->SetOutputPinValue(this, 0, &sCurrentValue);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_StoreString, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_StoreString>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Variables"),
    new xiiTitleAttribute("Store String '{Name}' = {Value}"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, xiiVisualScriptDataPinType::String, m_sValue),
    XII_OUTPUT_DATA_PIN("StoredValue", 0, xiiVisualScriptDataPinType::String)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_StoreString::xiiVisualScriptNode_StoreString()  = default;
xiiVisualScriptNode_StoreString::~xiiVisualScriptNode_StoreString() = default;

void xiiVisualScriptNode_StoreString::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  pInstance->GetLocalVariables().StoreString(m_sVariable, m_sValue);
  pInstance->SetOutputPinValue(this, 0, &m_sValue);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_StoreString::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  return &m_sValue;
}

//////////////////////////////////////////////////////////////////////////



XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_Implementation_VisualScriptVariableNodes);
