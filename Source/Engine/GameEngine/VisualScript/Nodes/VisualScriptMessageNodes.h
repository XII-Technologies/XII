#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>



//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_ScriptStartEvent : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_ScriptStartEvent, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_ScriptStartEvent();
  ~xiiVisualScriptNode_ScriptStartEvent();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_ScriptUpdateEvent : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_ScriptUpdateEvent, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_ScriptUpdateEvent();
  ~xiiVisualScriptNode_ScriptUpdateEvent();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_GenericEvent : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_GenericEvent, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_GenericEvent();
  ~xiiVisualScriptNode_GenericEvent();

  virtual void     Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void*    GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
  virtual xiiInt32 HandlesMessagesWithID() const override;
  virtual void     HandleMessage(xiiMessage* pMsg) override;

  const char* GetMessage() const { return m_sMessage; }
  void        SetMessage(const char* s) { m_sMessage.Assign(s); }

private:
  xiiHashedString m_sMessage;
  xiiVariant      m_Value;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_PhysicsTriggerEvent : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_PhysicsTriggerEvent, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_PhysicsTriggerEvent();
  ~xiiVisualScriptNode_PhysicsTriggerEvent();

  virtual void     Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void*    GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
  virtual xiiInt32 HandlesMessagesWithID() const override;
  virtual void     HandleMessage(xiiMessage* pMsg) override;

  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); }
  void        SetTriggerMessage(const char* s) { m_sTriggerMessage.Assign(s); }

private:
  xiiHashedString       m_sTriggerMessage;
  xiiGameObjectHandle   m_hObject;
  xiiTriggerState::Enum m_State;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_InputState : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_InputState, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_InputState();
  ~xiiVisualScriptNode_InputState();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiString          m_sInputAction;
  bool               m_bOnlyKeyPressed = false;
  xiiComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_InputEvent : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_InputEvent, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_InputEvent();
  ~xiiVisualScriptNode_InputEvent();

  virtual void     Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void*    GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
  virtual xiiInt32 HandlesMessagesWithID() const override;
  virtual void     HandleMessage(xiiMessage* pMsg) override;

  const char* GetInputAction() const { return m_sInputAction.GetData(); }
  void        SetInputAction(const char* s) { m_sInputAction.Assign(s); }

private:
  xiiHashedString       m_sInputAction;
  xiiGameObjectHandle   m_hSenderObject;
  xiiComponentHandle    m_hSenderComponent;
  xiiTriggerState::Enum m_State;
};

//////////////////////////////////////////////////////////////////////////
