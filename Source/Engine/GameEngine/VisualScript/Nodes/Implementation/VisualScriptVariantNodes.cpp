#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/Nodes/VisualScriptVariantNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_ConvertTo, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_ConvertTo>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Variant"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    XII_MEMBER_PROPERTY("Value", m_Value)->AddAttributes(new xiiVisScriptDataPinInAttribute(0, xiiVisualScriptDataPinType::Variant), new xiiDefaultValueAttribute(0)),
    // Data Pins (Output)
    XII_OUTPUT_DATA_PIN("Bool", 0, xiiVisualScriptDataPinType::Boolean),
    XII_OUTPUT_DATA_PIN("Number", 1, xiiVisualScriptDataPinType::Number),
    XII_OUTPUT_DATA_PIN("String", 2, xiiVisualScriptDataPinType::String),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_ConvertTo::xiiVisualScriptNode_ConvertTo()  = default;
xiiVisualScriptNode_ConvertTo::~xiiVisualScriptNode_ConvertTo() = default;

void xiiVisualScriptNode_ConvertTo::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    bool bResult = m_Value.ConvertTo<bool>();
    pInstance->SetOutputPinValue(this, 0, &bResult);

    double fResult = m_Value.ConvertTo<double>();
    pInstance->SetOutputPinValue(this, 1, &fResult);

    xiiString sResult = m_Value.ConvertTo<xiiString>();
    pInstance->SetOutputPinValue(this, 2, &sResult);
  }
}

void* xiiVisualScriptNode_ConvertTo::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value;
  }

  return nullptr;
}


XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_Implementation_VisualScriptVariantNodes);
