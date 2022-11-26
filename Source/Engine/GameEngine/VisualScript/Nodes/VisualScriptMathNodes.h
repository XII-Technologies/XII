#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

/// \brief Computes (a1 * a2) + (b1 * b2)
class XII_GAMEENGINE_DLL xiiVisualScriptNode_MultiplyAdd : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_MultiplyAdd, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_MultiplyAdd();
  ~xiiVisualScriptNode_MultiplyAdd();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  double m_Value1 = 0;
  double m_Value2 = 1;
  double m_Value3 = 0;
  double m_Value4 = 1;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes (a / b)
class XII_GAMEENGINE_DLL xiiVisualScriptNode_Div : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Div, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Div();
  ~xiiVisualScriptNode_Div();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  double m_Value1 = 1;
  double m_Value2 = 1;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes min(a, b)
class XII_GAMEENGINE_DLL xiiVisualScriptNode_Min : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Min, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Min();
  ~xiiVisualScriptNode_Min();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  double m_Value1 = 0;
  double m_Value2 = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes max(a, b)
class XII_GAMEENGINE_DLL xiiVisualScriptNode_Max : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Max, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Max();
  ~xiiVisualScriptNode_Max();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  double m_Value1 = 0;
  double m_Value2 = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes Clamp(Value, MinValue, MaxValue)
class XII_GAMEENGINE_DLL xiiVisualScriptNode_Clamp : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Clamp, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Clamp();
  ~xiiVisualScriptNode_Clamp();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  double m_Value    = 0;
  double m_MinValue = 0;
  double m_MaxValue = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes abs(value)
class XII_GAMEENGINE_DLL xiiVisualScriptNode_Abs : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Abs, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Abs();
  ~xiiVisualScriptNode_Abs();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  double m_Value = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes abs(value)
class XII_GAMEENGINE_DLL xiiVisualScriptNode_Sign : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Sign, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Sign();
  ~xiiVisualScriptNode_Sign();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  double m_Value = 0;
};

//////////////////////////////////////////////////////////////////////////
