#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

/// \brief Message that is sent by xiiStateMachineState_SendMsg once the state is entered.
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

/// \brief A state machine state implementation that sends a xiiMsgStateMachineStateChanged on state enter or exit to the owner of the
/// state machine instance. Currently only works for xiiStateMachineComponent.
///
/// Optionally it can also log a message on state enter or exit.
class xiiStateMachineState_SendMsg : public xiiStateMachineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineState_SendMsg, xiiStateMachineState);

public:
  xiiStateMachineState_SendMsg(xiiStringView sName = xiiStringView());
  ~xiiStateMachineState_SendMsg();

  virtual void OnEnter(xiiStateMachineInstance& instance, void* pInstanceData, const xiiStateMachineState* pFromState) const override;
  virtual void OnExit(xiiStateMachineInstance& instance, void* pInstanceData, const xiiStateMachineState* pToState) const override;

  virtual xiiResult Serialize(xiiStreamWriter& stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& stream) override;

  xiiTime m_MessageDelay;

  bool m_bSendMessageOnEnter = true;
  bool m_bSendMessageOnExit  = false;
  bool m_bLogOnEnter         = false;
  bool m_bLogOnExit          = false;
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

/// \brief A component that holds an xiiStateMachineInstance using the xiiStateMachineDescription from the resource assigned to this component.
class XII_GAMEENGINE_DLL xiiStateMachineComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiStateMachineComponent, xiiComponent, xiiStateMachineComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

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

  /// \brief Returns the xiiStateMachineInstance owned by this component
  xiiStateMachineInstance*       GetStateMachineInstance() { return m_pStateMachineInstance.Borrow(); }
  const xiiStateMachineInstance* GetStateMachineInstance() const { return m_pStateMachineInstance.Borrow(); }

  void                                 SetResource(const xiiStateMachineResourceHandle& hResource);
  const xiiStateMachineResourceHandle& GetResource() const { return m_hResource; }

  void        SetResourceFile(const char* szFile); // [ property ]
  const char* GetResourceFile() const;             // [ property ]

  /// \brief Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void        SetInitialState(const char* szName);                // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

  /// \brief Sets the current state with the given name.
  bool SetState(xiiStringView sName); // [ scriptable ]

private:
  friend class xiiStateMachineState_SendMsg;
  void SendStateChangedMsg(xiiMsgStateMachineStateChanged& msg, xiiTime delay);
  void InstantiateStateMachine();
  void Update();

  xiiStateMachineResourceHandle m_hResource;
  xiiHashedString               m_sInitialState;

  xiiUniquePtr<xiiStateMachineInstance> m_pStateMachineInstance;

  xiiEventMessageSender<xiiMsgStateMachineStateChanged> m_StateChangedSender; // [ event ]
};
