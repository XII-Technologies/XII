#pragma once

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiStateMachineState;
class xiiStateMachineTransition;

class xiiStateMachinePin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachinePin, xiiPin);

public:
  xiiStateMachinePin(Type type, const xiiDocumentObject* pObject);
};

/// \brief A connection that represents a state machine transition. Since we can't chose different connection
/// types in the Editor we allow the user to switch the type in the properties.
class xiiStateMachineConnection : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineConnection, xiiReflectedClass);

public:
  xiiStateMachineTransition* m_pType = nullptr;
};

/// \brief Base class for nodes in the state machine graph
class xiiStateMachineNodeBase : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineNodeBase, xiiReflectedClass);
};

/// \brief A node that represents a state machine state. We don't use xiiStateMachineState directly to allow
/// the user to switch the type in the properties similar to what we do with transitions.
class xiiStateMachineNode : public xiiStateMachineNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineNode, xiiStateMachineNodeBase);

public:
  xiiString             m_sName;
  xiiStateMachineState* m_pType = nullptr;
};

/// \brief A node that represents "any" state machine state. This can be used if a transition with the same conditions
/// is possible from any other state in the state machine. Instead of creating many connections with the same properties
/// an "any" state can be used to make the graph much easier to read and to maintain.
///
/// Note that there is no "any" state at runtime but rather only the transition is stored.
class xiiStateMachineNodeAny : public xiiStateMachineNodeBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineNodeAny, xiiStateMachineNodeBase);
};

class xiiStateMachineNodeManager : public xiiDocumentNodeManager
{
public:
  xiiStateMachineNodeManager();
  ~xiiStateMachineNodeManager();

  bool                     IsInitialState(const xiiDocumentObject* pObject) const { return pObject == m_pInitialStateObject; }
  void                     SetInitialState(const xiiDocumentObject* pObject);
  const xiiDocumentObject* GetInitialState() const { return m_pInitialStateObject; }

  bool IsAnyState(const xiiDocumentObject* pObject) const;

private:
  virtual bool      InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node) override;

  virtual void           GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const override;
  virtual const xiiRTTI* GetConnectionType() const override;

  void ObjectHandler(const xiiDocumentObjectEvent& e);

private:
  const xiiDocumentObject* m_pInitialStateObject = nullptr;
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
  const xiiDocumentObject* m_pOldInitialStateObject = nullptr;
  const xiiDocumentObject* m_pNewInitialStateObject = nullptr;
};
