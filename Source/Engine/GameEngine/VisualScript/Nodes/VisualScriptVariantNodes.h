#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class XII_GAMEENGINE_DLL xiiVisualScriptNode_ConvertTo : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_ConvertTo, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_ConvertTo();
  ~xiiVisualScriptNode_ConvertTo();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiVariant m_Value = 0;
};
