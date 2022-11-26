#pragma once

#include <Core/World/Declarations.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_GetScriptOwner : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_GetScriptOwner, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_GetScriptOwner();
  ~xiiVisualScriptNode_GetScriptOwner();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_GetComponentOwner : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_GetComponentOwner, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_GetComponentOwner();
  ~xiiVisualScriptNode_GetComponentOwner();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_FindChildObject : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_FindChildObject, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_FindChildObject();
  ~xiiVisualScriptNode_FindChildObject();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiGameObjectHandle m_hObject;
  xiiString           m_sChildObjectName;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_FindComponent : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_FindComponent, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_FindComponent();
  ~xiiVisualScriptNode_FindComponent();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiGameObjectHandle m_hObject;
  xiiString           m_sType;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_QueryGlobalObject : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_QueryGlobalObject, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_QueryGlobalObject();
  ~xiiVisualScriptNode_QueryGlobalObject();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiString m_sObjectName;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_FindParent : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_FindParent, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_FindParent();
  ~xiiVisualScriptNode_FindParent();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

private:
  xiiGameObjectHandle m_hObject;
  xiiString           m_sObjectName;
};

//////////////////////////////////////////////////////////////////////////
