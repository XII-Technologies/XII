#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class XII_GAMEENGINE_DLL xiiVisualScriptNode_IsEqual : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_IsEqual, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_IsEqual();
  ~xiiVisualScriptNode_IsEqual();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiString m_sValue1;
  xiiString m_sValue2;

  bool m_bIgnoreCase = false;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Switch : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Switch, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Switch();
  ~xiiVisualScriptNode_Switch();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiString m_sInput;
  xiiString m_sCase1;
  xiiString m_sCase2;
  xiiString m_sCase3;
  xiiString m_sCase4;

  bool m_bIgnoreCase = false;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Format : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Format, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Format();
  ~xiiVisualScriptNode_Format();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiString  m_sFormat;
  xiiVariant m_Value0 = 0;
  xiiVariant m_Value1 = 0;
  xiiVariant m_Value2 = 0;
};
