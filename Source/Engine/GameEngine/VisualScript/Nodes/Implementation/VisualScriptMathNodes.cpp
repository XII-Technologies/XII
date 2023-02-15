#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/Nodes/VisualScriptMathNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_MultiplyAdd, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_MultiplyAdd>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math"),
    new xiiTitleAttribute("'{a1}*{a2} + {b1}*{b2}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("a1", 0, xiiVisualScriptDataPinType::Number, m_Value1),
    XII_INPUT_DATA_PIN_AND_PROPERTY("a2", 1, xiiVisualScriptDataPinType::Number, m_Value2)->AddAttributes(new xiiDefaultValueAttribute(1.0)),
    XII_INPUT_DATA_PIN_AND_PROPERTY("b1", 2, xiiVisualScriptDataPinType::Number, m_Value3),
    XII_INPUT_DATA_PIN_AND_PROPERTY("b2", 3, xiiVisualScriptDataPinType::Number, m_Value4)->AddAttributes(new xiiDefaultValueAttribute(1.0)),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_MultiplyAdd::xiiVisualScriptNode_MultiplyAdd() {}
xiiVisualScriptNode_MultiplyAdd::~xiiVisualScriptNode_MultiplyAdd() {}

void xiiVisualScriptNode_MultiplyAdd::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = m_Value1 * m_Value2 + m_Value3 * m_Value4;
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_MultiplyAdd::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value1;
    case 1:
      return &m_Value2;
    case 2:
      return &m_Value3;
    case 3:
      return &m_Value4;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Div, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Div>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math"),
    new xiiTitleAttribute("Div: {a} / {b}"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("a", 0, xiiVisualScriptDataPinType::Number, m_Value1)->AddAttributes(new xiiDefaultValueAttribute(1.0)),
    XII_INPUT_DATA_PIN_AND_PROPERTY("b", 1, xiiVisualScriptDataPinType::Number, m_Value2)->AddAttributes(new xiiDefaultValueAttribute(1.0)),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Div::xiiVisualScriptNode_Div() {}
xiiVisualScriptNode_Div::~xiiVisualScriptNode_Div() {}

void xiiVisualScriptNode_Div::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = m_Value1 / m_Value2;
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_Div::GetInputPinDataPointer(xiiUInt8 uiPin)
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
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Min, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Min>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math"),
    new xiiTitleAttribute("Min ({a}, {b})"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("a", 0, xiiVisualScriptDataPinType::Number, m_Value1),
    XII_INPUT_DATA_PIN_AND_PROPERTY("b", 1, xiiVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Min::xiiVisualScriptNode_Min() {}
xiiVisualScriptNode_Min::~xiiVisualScriptNode_Min() {}

void xiiVisualScriptNode_Min::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = xiiMath::Min(m_Value1, m_Value2);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_Min::GetInputPinDataPointer(xiiUInt8 uiPin)
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
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Max, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Max>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math"),
    new xiiTitleAttribute("Max ({a}, {b})"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("a", 0, xiiVisualScriptDataPinType::Number, m_Value1),
    XII_INPUT_DATA_PIN_AND_PROPERTY("b", 1, xiiVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Max::xiiVisualScriptNode_Max() {}
xiiVisualScriptNode_Max::~xiiVisualScriptNode_Max() {}

void xiiVisualScriptNode_Max::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = xiiMath::Max(m_Value1, m_Value2);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_Max::GetInputPinDataPointer(xiiUInt8 uiPin)
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
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Clamp, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Clamp>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math"),
    new xiiTitleAttribute("Clamp ({Min}, {Max})"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Number),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Min", 1, xiiVisualScriptDataPinType::Number, m_MinValue),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Max", 2, xiiVisualScriptDataPinType::Number, m_MaxValue),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Clamp::xiiVisualScriptNode_Clamp() {}
xiiVisualScriptNode_Clamp::~xiiVisualScriptNode_Clamp() {}

void xiiVisualScriptNode_Clamp::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = xiiMath::Clamp(m_Value, m_MinValue, m_MaxValue);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_Clamp::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value;
    case 1:
      return &m_MinValue;
    case 2:
      return &m_MaxValue;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Abs, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Abs>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math")
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Number),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Abs::xiiVisualScriptNode_Abs() {}
xiiVisualScriptNode_Abs::~xiiVisualScriptNode_Abs() {}

void xiiVisualScriptNode_Abs::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = xiiMath::Abs(m_Value);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_Abs::GetInputPinDataPointer(xiiUInt8 uiPin)
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
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Sign, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Sign>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math")
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Value", 0, xiiVisualScriptDataPinType::Number),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Sign::xiiVisualScriptNode_Sign() {}
xiiVisualScriptNode_Sign::~xiiVisualScriptNode_Sign() {}

void xiiVisualScriptNode_Sign::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = xiiMath::Sign(m_Value);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_Sign::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_Implementation_VisualScriptMathNodes);
