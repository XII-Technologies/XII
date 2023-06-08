#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Sequence : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Sequence, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Sequence();
  ~xiiVisualScriptNode_Sequence();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Delay : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Delay, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Delay();
  ~xiiVisualScriptNode_Delay();

  virtual void     Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void*    GetInputPinDataPointer(xiiUInt8 uiPin) override;
  virtual bool     IsManuallyStepped() const override { return true; }
  virtual xiiInt32 HandlesMessagesWithID() const override;
  virtual void     HandleMessage(xiiMessage* pMsg) override;

private:
  xiiTime         m_Delay;
  xiiHashedString m_sMessage;
  bool            m_bMessageReceived = false;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_Log : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_Log, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_Log();
  ~xiiVisualScriptNode_Log();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;

  xiiString  m_sLog;
  xiiVariant m_Value0 = 0;
  xiiVariant m_Value1 = 0;
  xiiVariant m_Value2 = 0;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_MessageSender : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_MessageSender, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_MessageSender();
  ~xiiVisualScriptNode_MessageSender();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;
  virtual bool  IsManuallyStepped() const override { return true; }

  void SetMessageToSend(xiiUniquePtr<xiiMessage>&& pMsg);

  xiiTime m_Delay;
  bool    m_bRecursive = false;

private:
  xiiGameObjectHandle      m_hObject;
  xiiComponentHandle       m_hComponent;
  xiiUniquePtr<xiiMessage> m_pMessageToSend;

  xiiSmallArray<xiiUInt16, 8>                           m_PropertyIndexToMemoryOffset;
  xiiSmallArray<xiiEnum<xiiVisualScriptDataPinType>, 8> m_PropertyIndexToDataPinType;
  xiiBlob                                               m_ScratchMemory;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_MessageHandler : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_MessageHandler, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_MessageHandler();
  ~xiiVisualScriptNode_MessageHandler();

  virtual void     Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void*    GetInputPinDataPointer(xiiUInt8 uiPin) override { return nullptr; }
  virtual bool     IsManuallyStepped() const override { return true; }
  virtual xiiInt32 HandlesMessagesWithID() const override;
  virtual void     HandleMessage(xiiMessage* pMsg) override;

  const xiiRTTI*           m_pMessageTypeToHandle = nullptr;
  xiiUniquePtr<xiiMessage> m_pMsgCopy;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiVisualScriptNode_FunctionCall : public xiiVisualScriptNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptNode_FunctionCall, xiiVisualScriptNode);

public:
  xiiVisualScriptNode_FunctionCall();
  ~xiiVisualScriptNode_FunctionCall();

  virtual void  Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(xiiUInt8 uiPin) override;
  virtual bool  IsManuallyStepped() const override { return true; }

  static xiiResult ConvertArgumentToRequiredType(xiiVariant& ref_var, xiiVariantType::Enum type);

  const xiiRTTI*                     m_pExpectedType   = nullptr;
  const xiiAbstractFunctionProperty* m_pFunctionToCall = nullptr;
  xiiGameObjectHandle                m_hObject;
  xiiComponentHandle                 m_hComponent;
  xiiVariant                         m_ReturnValue;
  xiiHybridArray<xiiVariant, 4>      m_Arguments;
  xiiUInt16                          m_ArgumentIsOutParamMask = 0; // the n-th XII_BIT is set if m_Arguments[n] represents an out or inout parameter
};
