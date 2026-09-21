/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/Components/StateMachine/StateMachineResource.h>

/// Message that is sent by xiiStateMachineState_SendMsg once the state is entered.
struct XII_GAMEENGINE_DLL xiiMsgStateMachineStateChanged : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgStateMachineStateChanged, xiiEventMessage);

  xiiHashedString m_sOldStateName;
  xiiHashedString m_sNewStateName;

private:
  const char* GetOldStateName() const { return m_sOldStateName; }
  void        SetOldStateName(const char* szName) { m_sOldStateName.Assign(szName); }

  const char* GetNewStateName() const { return m_sNewStateName; }
  void        SetNewStateName(const char* szName) { m_sNewStateName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

/// A state machine state that sends a xiiMsgStateMachineStateChanged on state enter or exit to the owner of the
/// state machine instance. Currently only works for xiiStateMachineComponent.
///
/// Optionally it can also log a message on state enter or exit.
class xiiStateMachineState_SendMsg : public xiiStateMachineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineState_SendMsg, xiiStateMachineState);

public:
  xiiStateMachineState_SendMsg(xiiStringView sName = xiiStringView());
  ~xiiStateMachineState_SendMsg();

  virtual void OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const override;
  virtual void OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  xiiTime m_MessageDelay;

  bool m_bSendMessageOnEnter = true;
  bool m_bSendMessageOnExit  = false;
  bool m_bLogOnEnter         = false;
  bool m_bLogOnExit          = false;
};

//////////////////////////////////////////////////////////////////////////

/// A state machine state that sets the enabled flag on a game object and disables all other objects in the same group.
///
/// This state allows to easily switch the representation of a game object.
/// For instance you may have two objects states: normal and burning
/// You can basically just build two objects, one in the normal state, and one with all the effects needed for the fire.
/// Then you group both objects under a shared parent (e.g. with name 'visuals'), give both of them a name ('normal', 'burning') and disable one of them.
///
/// When the state machine transitions from the normal state to the burning state, you can then use this type of state
/// to say that from the 'visuals' group you want to activate the 'burning' object and deactivate all other objects in the same group.
///
/// Because the state activates one object and deactivates all others, you can have many different visuals and switch between them.
/// You can also only activate an object and keep the rest in the group as they are (e.g. to enable more and more effects).
/// If you only give a group path, but no object name, you can also use it to just disable all objects in a group.
/// If multiple objects in the same group have the same name, they will all get activated simultaneously.
///
/// Make sure that essential other objects (like the physics representation or other scripts) are located on other objects, that don't get deactivated.
class xiiStateMachineState_SwitchObject : public xiiStateMachineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineState_SwitchObject, xiiStateMachineState);

public:
  xiiStateMachineState_SwitchObject(xiiStringView sName = xiiStringView());
  ~xiiStateMachineState_SwitchObject();

  virtual void OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  xiiString m_sGroupPath;
  xiiString m_sObjectToEnable;
  bool      m_bDeactivateOthers = true;
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMEENGINE_DLL xiiStateMachineComponentManager : public xiiComponentManager<class xiiStateMachineComponent, xiiBlockStorageType::Compact>
{
public:
  xiiStateMachineComponentManager(xiiWorld* pWorld);
  ~xiiStateMachineComponentManager();

  virtual void Initialize() override;

  void Update(const xiiWorldModule::UpdateContext& context);

private:
  void ResourceEventHandler(const xiiResourceEvent& e);

  xiiHashSet<xiiComponentHandle> m_ComponentsToReload;
};

//////////////////////////////////////////////////////////////////////////

/// A component that holds a xiiStateMachineInstance using the xiiStateMachineDescription from the resource assigned to this component.
class XII_GAMEENGINE_DLL xiiStateMachineComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiStateMachineComponent, xiiComponent, xiiStateMachineComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiStateMachineComponent

public:
  xiiStateMachineComponent();
  xiiStateMachineComponent(xiiStateMachineComponent&& other);
  ~xiiStateMachineComponent();

  xiiStateMachineComponent& operator=(xiiStateMachineComponent&& other);

  /// Returns the xiiStateMachineInstance owned by this component
  xiiStateMachineInstance*       GetStateMachineInstance() { return m_pStateMachineInstance.Borrow(); }
  const xiiStateMachineInstance* GetStateMachineInstance() const { return m_pStateMachineInstance.Borrow(); }

  void                                 SetResource(const xiiStateMachineResourceHandle& hResource); // [ property ]
  const xiiStateMachineResourceHandle& GetResource() const { return m_hResource; }                  // [ property ]

  /// Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void        SetInitialState(const char* szName);                // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

  /// Sets the current state with the given name.
  bool SetState(xiiStringView sName); // [ scriptable ]

  /// Returns the name of the currently active state.
  xiiStringView GetCurrentState() const; // [ scriptable ]

  /// Sends a named event that state transitions can react to.
  void FireTransitionEvent(xiiStringView sEvent);

  void        SetBlackboardName(const char* szName);                  // [ property ]
  const char* GetBlackboardName() const { return m_sBlackboardName; } // [ property ]

private:
  friend class xiiStateMachineState_SendMsg;
  void SendStateChangedMsg(xiiMsgStateMachineStateChanged& msg, xiiTime delay);
  void InstantiateStateMachine();
  void Update();

  xiiStateMachineResourceHandle m_hResource;
  xiiHashedString               m_sInitialState;
  xiiHashedString               m_sBlackboardName;

  xiiUniquePtr<xiiStateMachineInstance> m_pStateMachineInstance;

  xiiEventMessageSender<xiiMsgStateMachineStateChanged> m_StateChangedSender; // [ event ]
};
