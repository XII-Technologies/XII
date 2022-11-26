#pragma once

#include <Foundation/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class XII_SAMPLEGAMEPLUGIN_DLL xiiVisualScriptNode_SampleNode : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_SampleNode, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_SampleNode();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiString m_sPrint;
  double    m_Value = 0;
};
