#pragma once

#include <Core/World/Declarations.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class XII_GAMEENGINE_DLL xiiVisualScriptNode_VariableBase : public xiiVisualScriptNode
{
protected:
  xiiHashedString m_sVariable;

  void        SetVariable(const char* szVariable) { m_sVariable.Assign(szVariable); }
  const char* GetVariable() const { return m_sVariable; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_GetNumberProperty : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_GetNumberProperty, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_GetNumberProperty();
  ~xiiVisualScriptNode_GetNumberProperty();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_SetNumberProperty : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_SetNumberProperty, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_SetNumberProperty();
  ~xiiVisualScriptNode_SetNumberProperty();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiComponentHandle m_hComponent;
  double             m_fValue = 0;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_GetBoolProperty : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_GetBoolProperty, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_GetBoolProperty();
  ~xiiVisualScriptNode_GetBoolProperty();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_SetBoolProperty : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_SetBoolProperty, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_SetBoolProperty();
  ~xiiVisualScriptNode_SetBoolProperty();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiComponentHandle m_hComponent;
  bool               m_bValue = false;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_GetStringProperty : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_GetStringProperty, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_GetStringProperty();
  ~xiiVisualScriptNode_GetStringProperty();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_SetStringProperty : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_SetStringProperty, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_SetStringProperty();
  ~xiiVisualScriptNode_SetStringProperty();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiComponentHandle m_hComponent;
  xiiString          m_sValue;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Number : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Number, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Number();
  ~xiiVisualScriptNode_Number();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_StoreNumber : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_StoreNumber, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_StoreNumber();
  ~xiiVisualScriptNode_StoreNumber();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  double m_Value = 0;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Bool : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Bool, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Bool();
  ~xiiVisualScriptNode_Bool();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_StoreBool : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_StoreBool, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_StoreBool();
  ~xiiVisualScriptNode_StoreBool();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  bool m_Value = false;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_ToggleBool : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_ToggleBool, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_ToggleBool();
  ~xiiVisualScriptNode_ToggleBool();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_String : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_String, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_String();
  ~xiiVisualScriptNode_String();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_StoreString : public xiiVisualScriptNode_VariableBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_StoreString, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_StoreString();
  ~xiiVisualScriptNode_StoreString();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiString m_sValue;
};

//////////////////////////////////////////////////////////////////////////
