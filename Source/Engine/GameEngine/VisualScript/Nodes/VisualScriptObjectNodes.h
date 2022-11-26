#pragma once

#include <Core/World/Declarations.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_DeleteObject : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_DeleteObject, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_DeleteObject();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiGameObjectHandle m_hObject;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_ActivateObject : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_ActivateObject, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_ActivateObject();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiGameObjectHandle m_hObject;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_ActivateComponent : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_ActivateComponent, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_ActivateComponent();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_HasName : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_HasName, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_HasName();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiGameObjectHandle m_hObject;
  xiiString           m_sObjectName;
};

//////////////////////////////////////////////////////////////////////////
