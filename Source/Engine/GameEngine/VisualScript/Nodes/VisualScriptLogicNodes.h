#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

struct XII_GAMEENGINE_DLL xiiLogicOperator
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Equal,
    Unequal,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    Default = Equal,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiLogicOperator);

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Compare : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Compare, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Compare();
  ~xiiVisualScriptNode_Compare();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiEnum<xiiLogicOperator> m_Operator;

  double m_Value1 = 0.0;
  double m_Value2 = 0.0;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_CompareExec : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_CompareExec, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_CompareExec();
  ~xiiVisualScriptNode_CompareExec();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiEnum<xiiLogicOperator> m_Operator;

  double m_Value1 = 0.0;
  double m_Value2 = 0.0;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_If : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_If, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_If();
  ~xiiVisualScriptNode_If();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  bool m_Value = false;
  ;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Logic : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Logic, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Logic();
  ~xiiVisualScriptNode_Logic();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  bool m_Value1 = false;
  bool m_Value2 = false;
};

//////////////////////////////////////////////////////////////////////////
