#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/Nodes/VisualScriptLogicNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiLogicOperator, 1)
XII_ENUM_CONSTANTS(xiiLogicOperator::Equal, xiiLogicOperator::Unequal, xiiLogicOperator::Less, xiiLogicOperator::LessEqual, xiiLogicOperator::Greater, xiiLogicOperator::GreaterEqual)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Compare, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Compare>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("Compare: {Value1} {Operator} {Value2}"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Properties
    XII_ENUM_MEMBER_PROPERTY("Operator", xiiLogicOperator, m_Operator),

    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value1", 0, xiiVisualScriptDataPinType::Number, m_Value1),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value2", 1, xiiVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Boolean),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Compare::xiiVisualScriptNode_Compare()  = default;
xiiVisualScriptNode_Compare::~xiiVisualScriptNode_Compare() = default;

void xiiVisualScriptNode_Compare::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  // we can skip this and save some performance, if the inputs did not change, because the result won't have changed either
  if (m_bInputValuesChanged)
  {
    bool result = true;

    switch (m_Operator)
    {
      case xiiLogicOperator::Equal:
        result = m_Value1 == m_Value2;
        break;
      case xiiLogicOperator::Unequal:
        result = m_Value1 != m_Value2;
        break;
      case xiiLogicOperator::Less:
        result = m_Value1 < m_Value2;
        break;
      case xiiLogicOperator::LessEqual:
        result = m_Value1 <= m_Value2;
        break;
      case xiiLogicOperator::Greater:
        result = m_Value1 > m_Value2;
        break;
      case xiiLogicOperator::GreaterEqual:
        result = m_Value1 >= m_Value2;
        break;
      default:
        XII_ASSERT_NOT_IMPLEMENTED;
    }

    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_Compare::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value1;

    case 1:
      return &m_Value2;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_CompareExec, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_CompareExec>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("Compare: {Value1} {Operator} {Value2}"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Properties
    XII_ENUM_MEMBER_PROPERTY("Operator", xiiLogicOperator, m_Operator),
    // Execution Pins
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("OnTrue", 0),
    XII_OUTPUT_EXECUTION_PIN("OnFalse", 1),
    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value1", 0, xiiVisualScriptDataPinType::Number, m_Value1),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value2", 1, xiiVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Boolean),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_CompareExec::xiiVisualScriptNode_CompareExec()  = default;
xiiVisualScriptNode_CompareExec::~xiiVisualScriptNode_CompareExec() = default;

void xiiVisualScriptNode_CompareExec::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  bool result = true;

  switch (m_Operator)
  {
    case xiiLogicOperator::Equal:
      result = m_Value1 == m_Value2;
      break;
    case xiiLogicOperator::Unequal:
      result = m_Value1 != m_Value2;
      break;
    case xiiLogicOperator::Less:
      result = m_Value1 < m_Value2;
      break;
    case xiiLogicOperator::LessEqual:
      result = m_Value1 <= m_Value2;
      break;
    case xiiLogicOperator::Greater:
      result = m_Value1 > m_Value2;
      break;
    case xiiLogicOperator::GreaterEqual:
      result = m_Value1 >= m_Value2;
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  pInstance->SetOutputPinValue(this, 0, &result);
  pInstance->ExecuteConnectedNodes(this, result ? 0 : 1);
}

void* xiiVisualScriptNode_CompareExec::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value1;

    case 1:
      return &m_Value2;
  }

  return nullptr;
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_If, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_If>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic")
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Execution Pins
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("OnTrue", 0),
    XII_OUTPUT_EXECUTION_PIN("OnFalse", 1),
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Bool", 0, xiiVisualScriptDataPinType::Boolean),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_If::xiiVisualScriptNode_If()  = default;
xiiVisualScriptNode_If::~xiiVisualScriptNode_If() = default;

void xiiVisualScriptNode_If::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, m_Value ? 0 : 1);
}

void* xiiVisualScriptNode_If::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Logic, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Logic>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic")
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("a", 0, xiiVisualScriptDataPinType::Boolean, m_Value1),
    XII_INPUT_DATA_PIN_AND_PROPERTY("b", 1, xiiVisualScriptDataPinType::Boolean, m_Value2),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("AorB", 0, xiiVisualScriptDataPinType::Boolean),
    XII_OUTPUT_DATA_PIN("AandB", 1, xiiVisualScriptDataPinType::Boolean),
    XII_OUTPUT_DATA_PIN("AxorB", 2, xiiVisualScriptDataPinType::Boolean),
    XII_OUTPUT_DATA_PIN("notA", 3, xiiVisualScriptDataPinType::Boolean),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Logic::xiiVisualScriptNode_Logic()  = default;
xiiVisualScriptNode_Logic::~xiiVisualScriptNode_Logic() = default;

void xiiVisualScriptNode_Logic::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const bool Or   = m_Value1 || m_Value2;
    const bool And  = m_Value1 && m_Value2;
    const bool Xor  = m_Value1 ^ m_Value2;
    const bool NotA = !m_Value1;

    pInstance->SetOutputPinValue(this, 0, &Or);
    pInstance->SetOutputPinValue(this, 1, &And);
    pInstance->SetOutputPinValue(this, 2, &Xor);
    pInstance->SetOutputPinValue(this, 3, &NotA);
  }
}

void* xiiVisualScriptNode_Logic::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value1;

    case 1:
      return &m_Value2;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_Implementation_VisualScriptLogicNodes);
