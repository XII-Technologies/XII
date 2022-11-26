#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/Nodes/VisualScriptMathExpressionNode.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_MathExpression, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_MathExpression>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Math"),
    new xiiTitleAttribute("Expression '{Expression}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_INPUT_DATA_PIN_AND_PROPERTY("a", 0, xiiVisualScriptDataPinType::Number, m_ValueA)->AddAttributes(new xiiDefaultValueAttribute(0.0)),
    XII_INPUT_DATA_PIN_AND_PROPERTY("b", 1, xiiVisualScriptDataPinType::Number, m_ValueB)->AddAttributes(new xiiDefaultValueAttribute(1.0)),
    XII_INPUT_DATA_PIN_AND_PROPERTY("c", 2, xiiVisualScriptDataPinType::Number, m_ValueC)->AddAttributes(new xiiDefaultValueAttribute(2.0)),
    XII_INPUT_DATA_PIN_AND_PROPERTY("d", 3, xiiVisualScriptDataPinType::Number, m_ValueD)->AddAttributes(new xiiDefaultValueAttribute(3.0)),
    XII_ACCESSOR_PROPERTY("Expression", GetExpression, SetExpression),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Result", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_MathExpression::xiiVisualScriptNode_MathExpression() {}
xiiVisualScriptNode_MathExpression::~xiiVisualScriptNode_MathExpression() {}

static xiiHashedString s_sA = xiiMakeHashedString("a");
static xiiHashedString s_sB = xiiMakeHashedString("b");
static xiiHashedString s_sC = xiiMakeHashedString("c");
static xiiHashedString s_sD = xiiMakeHashedString("d");

void xiiVisualScriptNode_MathExpression::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    xiiMathExpression::Input inputs[] =
      {
        {s_sA, static_cast<float>(m_ValueA)},
        {s_sB, static_cast<float>(m_ValueB)},
        {s_sC, static_cast<float>(m_ValueC)},
        {s_sD, static_cast<float>(m_ValueD)},
      };

    const double result = m_mMathExpression.Evaluate(inputs);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* xiiVisualScriptNode_MathExpression::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_ValueA;
    case 1:
      return &m_ValueB;
    case 2:
      return &m_ValueC;
    case 3:
      return &m_ValueD;
  }

  return nullptr;
}

const char* xiiVisualScriptNode_MathExpression::GetExpression() const
{
  return m_mMathExpression.GetExpressionString();
}

void xiiVisualScriptNode_MathExpression::SetExpression(const char* e)
{
  m_mMathExpression.Reset(e);
}



XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptMathExpressionNode);
