#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/Nodes/VisualScriptStringNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_IsEqual, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_IsEqual>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("String"),
    new xiiTitleAttribute("IsEqual: {Value1} = {Value2}"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Properties
    XII_MEMBER_PROPERTY("IgnoreCase", m_bIgnoreCase),

    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value1", 0, xiiVisualScriptDataPinType::String, m_sValue1),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value2", 1, xiiVisualScriptDataPinType::String, m_sValue2),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Boolean),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_IsEqual::xiiVisualScriptNode_IsEqual()  = default;
xiiVisualScriptNode_IsEqual::~xiiVisualScriptNode_IsEqual() = default;

void xiiVisualScriptNode_IsEqual::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  // we can skip this and save some performance, if the inputs did not change, because the result won't have changed either
  if (m_bInputValuesChanged)
  {
    bool result;
    if (m_bIgnoreCase)
    {
      result = m_sValue1.IsEqual_NoCase(m_sValue2);
    }
    else
    {
      result = m_sValue1.IsEqual(m_sValue2);
    }

    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_IsEqual::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_sValue1;

    case 1:
      return &m_sValue2;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Switch, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Switch>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("String"),
    new xiiTitleAttribute("Switch"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Properties
    XII_MEMBER_PROPERTY("IgnoreCase", m_bIgnoreCase),
    // Execution Pins
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("OnCase1", 0),
    XII_OUTPUT_EXECUTION_PIN("OnCase2", 1),
    XII_OUTPUT_EXECUTION_PIN("OnCase3", 2),
    XII_OUTPUT_EXECUTION_PIN("OnCase4", 3),
    XII_OUTPUT_EXECUTION_PIN("OnDefault", 4),
    // Data Pins (Input)
    XII_INPUT_DATA_PIN("Input", 0, xiiVisualScriptDataPinType::String),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Case1", 1, xiiVisualScriptDataPinType::String, m_sCase1),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Case2", 2, xiiVisualScriptDataPinType::String, m_sCase2),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Case3", 3, xiiVisualScriptDataPinType::String, m_sCase3),
    XII_INPUT_DATA_PIN_AND_PROPERTY("Case4", 4, xiiVisualScriptDataPinType::String, m_sCase4),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Switch::xiiVisualScriptNode_Switch() {}
xiiVisualScriptNode_Switch::~xiiVisualScriptNode_Switch() {}

void xiiVisualScriptNode_Switch::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bIgnoreCase)
  {
    if (m_sInput.IsEqual_NoCase(m_sCase1))
    {
      pInstance->ExecuteConnectedNodes(this, 0);
    }
    else if (m_sInput.IsEqual_NoCase(m_sCase2))
    {
      pInstance->ExecuteConnectedNodes(this, 1);
    }
    else if (m_sInput.IsEqual_NoCase(m_sCase3))
    {
      pInstance->ExecuteConnectedNodes(this, 2);
    }
    else if (m_sInput.IsEqual_NoCase(m_sCase4))
    {
      pInstance->ExecuteConnectedNodes(this, 3);
    }
    else
    {
      pInstance->ExecuteConnectedNodes(this, 4);
    }
  }
  else
  {
    if (m_sInput.IsEqual(m_sCase1))
    {
      pInstance->ExecuteConnectedNodes(this, 0);
    }
    else if (m_sInput.IsEqual(m_sCase2))
    {
      pInstance->ExecuteConnectedNodes(this, 1);
    }
    else if (m_sInput.IsEqual(m_sCase3))
    {
      pInstance->ExecuteConnectedNodes(this, 2);
    }
    else if (m_sInput.IsEqual(m_sCase4))
    {
      pInstance->ExecuteConnectedNodes(this, 3);
    }
    else
    {
      pInstance->ExecuteConnectedNodes(this, 4);
    }
  }
}

void* xiiVisualScriptNode_Switch::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_sInput;

    case 1:
      return &m_sCase1;

    case 2:
      return &m_sCase2;

    case 3:
      return &m_sCase3;

    case 4:
      return &m_sCase4;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Format, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Format>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("String"),
    new xiiTitleAttribute("Format: '{Format}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Properties
    XII_MEMBER_PROPERTY("Format", m_sFormat)->AddAttributes(new xiiDefaultValueAttribute(xiiStringView("Value1: {0}, Value2: {1}, Value3: {2}"))),
    // Data Pins
    XII_MEMBER_PROPERTY("Value0", m_Value0)->AddAttributes(new xiiVisScriptDataPinInAttribute(0, xiiVisualScriptDataPinType::Variant), new xiiDefaultValueAttribute(0)),
    XII_MEMBER_PROPERTY("Value1", m_Value1)->AddAttributes(new xiiVisScriptDataPinInAttribute(1, xiiVisualScriptDataPinType::Variant), new xiiDefaultValueAttribute(0)),
    XII_MEMBER_PROPERTY("Value2", m_Value2)->AddAttributes(new xiiVisScriptDataPinInAttribute(2, xiiVisualScriptDataPinType::Variant), new xiiDefaultValueAttribute(0)),
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::String)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Format::xiiVisualScriptNode_Format() {}
xiiVisualScriptNode_Format::~xiiVisualScriptNode_Format() {}

void xiiVisualScriptNode_Format::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    xiiStringBuilder sb;
    sb.Format(m_sFormat, m_Value0, m_Value1, m_Value2);

    xiiString result = sb;
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_Format::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value0;
    case 1:
      return &m_Value1;
    case 2:
      return &m_Value2;
  }

  return nullptr;
}
