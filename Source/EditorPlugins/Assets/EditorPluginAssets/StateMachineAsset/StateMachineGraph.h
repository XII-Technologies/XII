#pragma once

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiStateMachinePin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachinePin, xiiPin);

public:
  xiiStateMachinePin(Type type, const xiiDocumentObject* pObject);
};

class xiiStateMachineNodeManager : public xiiDocumentNodeManager
{
public:
  xiiStateMachineNodeManager();
  ~xiiStateMachineNodeManager();

  bool                     IsInitialState(const xiiDocumentObject* pObject) const;
  const xiiDocumentObject* GetInitialState() const;

  bool IsAnyState(const xiiDocumentObject* pObject) const;

private:
  virtual bool      InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node) override;

  virtual void           GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const override;
  virtual const xiiRTTI* GetConnectionType() const override;

  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
};

class xiiStateMachine_SetInitialStateCommand : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachine_SetInitialStateCommand, xiiCommand);

public:
  xiiStateMachine_SetInitialStateCommand();

public: // Properties
  xiiUuid m_NewInitialStateObject;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override {}

private:
  xiiDocumentObject* m_pOldInitialStateObject = nullptr;
  xiiDocumentObject* m_pNewInitialStateObject = nullptr;
};
