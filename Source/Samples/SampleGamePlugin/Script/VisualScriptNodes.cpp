#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Foundation/Logging/Log.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <SampleGamePlugin/Script/VisualScriptNodes.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_SampleNode, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_SampleNode>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Sample"),
    new xiiTitleAttribute("Sample Node '{Value}'"),
  }
    XII_END_ATTRIBUTES;
    XII_BEGIN_PROPERTIES
  {
    // Execution Pins
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    XII_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, xiiVisualScriptDataPinType::Number, m_Value),
    XII_OUTPUT_DATA_PIN("NewValue", 0, xiiVisualScriptDataPinType::Number),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_SampleNode::xiiVisualScriptNode_SampleNode()
{
  m_sPrint = "Value: {0}";
}

void xiiVisualScriptNode_SampleNode::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    // do stuff that depends on input values
    // skipping pInstance->SetOutputPinValue is valid, if the result cannot have changed (and saves some performance)
  }

  xiiLog::Info(m_sPrint, m_Value);
  m_Value += 1;

  pInstance->SetOutputPinValue(this, 0, &m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_SampleNode::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value;
  }

  return nullptr;
}
