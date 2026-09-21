/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Utils/Blackboard.h>
#include <GameEngine/Components/StateMachine/StateMachineResource.h>

/// A state machine state implementation that represents another state machine nested within this state. This can be used to build hierarchical state machines.
class XII_GAMEENGINE_DLL xiiStateMachineState_NestedStateMachine : public xiiStateMachineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineState_NestedStateMachine, xiiStateMachineState);

public:
  xiiStateMachineState_NestedStateMachine(xiiStringView sName = xiiStringView());
  ~xiiStateMachineState_NestedStateMachine();

  virtual void OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const override;
  virtual void OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const override;
  virtual void Update(xiiStateMachineInstance& ref_instance, void* pInstanceData, xiiTime deltaTime) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) override;

  void                                 SetResource(const xiiStateMachineResourceHandle& hResource); // [ property ]
  const xiiStateMachineResourceHandle& GetResource() const { return m_hResource; }                  // [ property ]

  /// Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void        SetInitialState(const char* szName);                // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

private:
  xiiStateMachineResourceHandle m_hResource;
  xiiHashedString               m_sInitialState;

  // Should the inner state machine keep its current state on exit and re-enter or should it exit as well and re-enter the initial state again.
  bool m_bKeepCurrentStateOnExit = false;

  struct InstanceData
  {
    xiiUniquePtr<xiiStateMachineInstance> m_pStateMachineInstance;
  };
};

//////////////////////////////////////////////////////////////////////////

/// A state machine state implementation that combines multiple sub states into one.
///
/// Can be used to build states in a more modular way. All calls are simply redirected to all sub states,
/// e.g. when entered it calls OnEnter on all its sub states.
class XII_GAMEENGINE_DLL xiiStateMachineState_Compound : public xiiStateMachineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineState_Compound, xiiStateMachineState);

public:
  xiiStateMachineState_Compound(xiiStringView sName = xiiStringView());
  ~xiiStateMachineState_Compound();

  virtual void OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const override;
  virtual void OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const override;
  virtual void Update(xiiStateMachineInstance& ref_instance, void* pInstanceData, xiiTime deltaTime) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) override;

  xiiSmallArray<xiiStateMachineState*, 2> m_SubStates;

private:
  xiiStateMachineInternal::Compound m_Compound;
};

//////////////////////////////////////////////////////////////////////////

/// An enum that represents the operator of a comparison
struct XII_GAMEENGINE_DLL xiiStateMachineLogicOperator
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    And,
    Or,

    Default = And
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiStateMachineLogicOperator);

//////////////////////////////////////////////////////////////////////////

/// A state machine transition implementation that checks the instance's blackboard for the given conditions.
class XII_GAMEENGINE_DLL xiiStateMachineTransition_BlackboardConditions : public xiiStateMachineTransition
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineTransition_BlackboardConditions, xiiStateMachineTransition);

public:
  xiiStateMachineTransition_BlackboardConditions();
  ~xiiStateMachineTransition_BlackboardConditions();

  virtual bool IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  xiiEnum<xiiStateMachineLogicOperator>     m_Operator;
  xiiHybridArray<xiiBlackboardCondition, 2> m_Conditions;
};

//////////////////////////////////////////////////////////////////////////

/// A state machine transition implementation that triggers after the given time
class XII_GAMEENGINE_DLL xiiStateMachineTransition_Timeout : public xiiStateMachineTransition
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineTransition_Timeout, xiiStateMachineTransition);

public:
  xiiStateMachineTransition_Timeout();
  ~xiiStateMachineTransition_Timeout();

  virtual bool IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  xiiTime m_Timeout;
};

//////////////////////////////////////////////////////////////////////////

/// A state machine transition implementation that combines multiple sub transition into one.
///
/// Can be used to build transitions in a more modular way. All calls are simply redirected to all sub transitions
/// and then combined with the given logic operator (AND, OR).
class XII_GAMEENGINE_DLL xiiStateMachineTransition_Compound : public xiiStateMachineTransition
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineTransition_Compound, xiiStateMachineTransition);

public:
  xiiStateMachineTransition_Compound();
  ~xiiStateMachineTransition_Compound();

  virtual bool IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) override;

  xiiEnum<xiiStateMachineLogicOperator>        m_Operator;
  xiiSmallArray<xiiStateMachineTransition*, 2> m_SubTransitions;

private:
  xiiStateMachineInternal::Compound m_Compound;
};

//////////////////////////////////////////////////////////////////////////

/// A state machine transition implementation that triggers when a 'transition event' is sent.
class XII_GAMEENGINE_DLL xiiStateMachineTransition_TransitionEvent : public xiiStateMachineTransition
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineTransition_TransitionEvent, xiiStateMachineTransition);

public:
  xiiStateMachineTransition_TransitionEvent();
  ~xiiStateMachineTransition_TransitionEvent();

  virtual bool IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

  xiiHashedString m_sEventName;
};
