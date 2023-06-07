#pragma once

#include <ToolsFoundation/Command/Command.h>

class xiiDocumentObject;
class xiiCommandHistory;
class xiiPin;

class XII_TOOLSFOUNDATION_DLL xiiRemoveNodeCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRemoveNodeCommand, xiiCommand);

public:
  xiiRemoveNodeCommand();

public: // Properties
  xiiUuid m_Object;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override;

private:
  xiiDocumentObject* m_pObject = nullptr;
};


class XII_TOOLSFOUNDATION_DLL xiiMoveNodeCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMoveNodeCommand, xiiCommand);

public:
  xiiMoveNodeCommand();

public: // Properties
  xiiUuid m_Object;
  xiiVec2 m_NewPos = xiiVec2::ZeroVector();

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject* m_pObject = nullptr;
  xiiVec2            m_vOldPos = xiiVec2::ZeroVector();
};


class XII_TOOLSFOUNDATION_DLL xiiConnectNodePinsCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConnectNodePinsCommand, xiiCommand);

public:
  xiiConnectNodePinsCommand();

public: // Properties
  xiiUuid   m_ConnectionObject;
  xiiUuid   m_ObjectSource;
  xiiUuid   m_ObjectTarget;
  xiiString m_sSourcePin;
  xiiString m_sTargetPin;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject* m_pConnectionObject = nullptr;
  xiiDocumentObject* m_pObjectSource     = nullptr;
  xiiDocumentObject* m_pObjectTarget     = nullptr;
};


class XII_TOOLSFOUNDATION_DLL xiiDisconnectNodePinsCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDisconnectNodePinsCommand, xiiCommand);

public:
  xiiDisconnectNodePinsCommand();

public: // Properties
  xiiUuid m_ConnectionObject;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject*       m_pConnectionObject = nullptr;
  const xiiDocumentObject* m_pObjectSource     = nullptr;
  const xiiDocumentObject* m_pObjectTarget     = nullptr;
  xiiString                m_sSourcePin;
  xiiString                m_sTargetPin;
};


class XII_TOOLSFOUNDATION_DLL xiiNodeCommands
{
public:
  static xiiStatus AddAndConnectCommand(xiiCommandHistory* pHistory, const xiiRTTI* pConnectionType, const xiiPin& sourcePin, const xiiPin& targetPin);
  static xiiStatus DisconnectAndRemoveCommand(xiiCommandHistory* pHistory, const xiiUuid& connectionObject);
};
