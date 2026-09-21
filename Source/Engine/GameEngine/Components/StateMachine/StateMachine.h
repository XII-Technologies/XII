/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/Components/StateMachine/Implementation/StateMachineInstanceData.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class xiiComponent;
class xiiWorld;
class xiiBlackboard;
class xiiStateMachineInstance;

/// Base class for a state in a state machine.
///
/// Note that states are shared between multiple instances and thus
/// shouldn't modify any data on their own but always operate on the passed instance and instance data.
/// \see xiiStateMachineInstanceDataDesc
class XII_GAMEENGINE_DLL xiiStateMachineState : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineState, xiiReflectedClass);

public:
  xiiStateMachineState(xiiStringView sName = xiiStringView());

  void                   SetName(xiiStringView sName);
  xiiStringView          GetName() const { return m_sName; }
  const xiiHashedString& GetNameHashed() const { return m_sName; }

  virtual void OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const = 0;
  virtual void OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const;
  virtual void Update(xiiStateMachineInstance& ref_instance, void* pInstanceData, xiiTime deltaTime) const;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream);

  /// Returns whether this state needs additional instance data and if so fills the out_desc.
  ///
  /// \see xiiStateMachineInstanceDataDesc
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc);

private:
  // These are dummy functions for the scripting reflection
  void Reflection_OnEnter(xiiStateMachineInstance* pStateMachineInstance, const xiiStateMachineState* pFromState);
  void Reflection_OnExit(xiiStateMachineInstance* pStateMachineInstance, const xiiStateMachineState* pToState);
  void Reflection_Update(xiiStateMachineInstance* pStateMachineInstance, xiiTime deltaTime);

  xiiHashedString m_sName;
};

class XII_GAMEENGINE_DLL xiiStateMachineState_Empty final : public xiiStateMachineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineState_Empty, xiiStateMachineState);

public:
  xiiStateMachineState_Empty(xiiStringView sName = xiiStringView());
  ~xiiStateMachineState_Empty() = default;

  virtual void OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const override {}
};

struct xiiStateMachineState_ScriptBaseClassFunctions
{
  enum Enum
  {
    OnEnter,
    OnExit,
    Update,

    Count
  };
};

/// Base class for a transition in a state machine. The target state of a transition is automatically set
/// once its condition has been met.
///
/// Same as with states, transitions are also shared between multiple instances and thus
/// should decide their condition based on the passed instance and instance data.
/// \see xiiStateMachineInstanceDataDesc
class XII_GAMEENGINE_DLL xiiStateMachineTransition : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStateMachineTransition, xiiReflectedClass);

  virtual bool IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const = 0;

  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream);

  /// Returns whether this transition needs additional instance data and if so fills the out_desc.
  ///
  /// \see xiiStateMachineInstanceDataDesc
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc);
};

/// The state machine description defines the structure of a state machine like e.g.
/// what states it has and how to transition between them.
/// Once an instance is created from a description it is not allowed to change the description afterwards.
class XII_GAMEENGINE_DLL xiiStateMachineDescription : public xiiRefCounted
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiStateMachineDescription);

public:
  xiiStateMachineDescription();
  ~xiiStateMachineDescription();

  /// Adds the given state to the description and returns the state index.
  xiiUInt32 AddState(xiiUniquePtr<xiiStateMachineState>&& pState);

  /// Adds the given transition between the two given states. A uiFromStateIndex of xiiInvalidIndex generates a transition that can be done from any other possible state.
  void AddTransition(xiiUInt32 uiFromStateIndex, xiiUInt32 uiToStateIndex, xiiUniquePtr<xiiStateMachineTransition>&& pTransistion);

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

private:
  friend class xiiStateMachineInstance;

  struct TransitionContext
  {
    xiiUniquePtr<xiiStateMachineTransition> m_pTransition;
    xiiUInt32                               m_uiToStateIndex       = 0;
    xiiUInt32                               m_uiInstanceDataOffset = xiiInvalidIndex;
  };

  using TransitionArray = xiiSmallArray<TransitionContext, 2>;
  TransitionArray m_FromAnyTransitions;

  struct StateContext
  {
    xiiUniquePtr<xiiStateMachineState> m_pState;
    TransitionArray                    m_Transitions;
    xiiUInt32                          m_uiInstanceDataOffset = xiiInvalidIndex;
  };

  xiiDynamicArray<StateContext>            m_States;
  xiiHashTable<xiiHashedString, xiiUInt32> m_StateNameToIndexTable;

  xiiInstanceDataAllocator m_InstanceDataAllocator;
};

/// The state machine instance represents the actual state machine.
/// Typically it is created from a description but for small use cases it can also be used without a description.
class XII_GAMEENGINE_DLL xiiStateMachineInstance
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiStateMachineInstance);

public:
  xiiStateMachineInstance(xiiReflectedClass& ref_owner, const xiiSharedPtr<const xiiStateMachineDescription>& pDescription = nullptr);
  ~xiiStateMachineInstance();

  xiiResult             SetState(xiiStateMachineState* pState);
  xiiResult             SetState(xiiUInt32 uiStateIndex);
  xiiResult             SetState(const xiiHashedString& sStateName);
  xiiResult             SetStateOrFallback(const xiiHashedString& sStateName, xiiUInt32 uiFallbackStateIndex = 0);
  xiiStateMachineState* GetCurrentState() { return m_pCurrentState; }

  void Update(xiiTime deltaTime);

  xiiReflectedClass& GetOwner() { return m_Owner; }
  xiiWorld*          GetOwnerWorld();

  void                               SetBlackboard(const xiiSharedPtr<xiiBlackboard>& pBlackboard);
  const xiiSharedPtr<xiiBlackboard>& GetBlackboard() const { return m_pBlackboard; }

  /// Returns how long the state machine is in its current state
  xiiTime GetTimeInCurrentState() const { return m_TimeInCurrentState; }

  /// Sends a named event that state transitions can react to.
  void FireTransitionEvent(xiiStringView sEvent);

  xiiStringView GetCurrentTransitionEvent() const { return m_sCurrentTransitionEvent; }

private:
  XII_ALLOW_PRIVATE_PROPERTIES(xiiStateMachineInstance);

  bool           Reflection_SetState(const xiiHashedString& sStateName);
  xiiComponent*  Reflection_GetOwnerComponent() const;
  xiiBlackboard* Reflection_GetBlackboard() const { return m_pBlackboard.Borrow(); }

  void      SetStateInternal(xiiUInt32 uiStateIndex);
  void      EnterCurrentState(const xiiStateMachineState* pFromState);
  void      ExitCurrentState(const xiiStateMachineState* pToState);
  xiiUInt32 FindNewStateToTransitionTo();

  XII_ALWAYS_INLINE void* GetInstanceData(xiiUInt32 uiOffset)
  {
    return xiiInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), uiOffset);
  }

  XII_ALWAYS_INLINE void* GetCurrentStateInstanceData()
  {
    if (m_pDescription != nullptr && m_uiCurrentStateIndex < m_pDescription->m_States.GetCount())
    {
      return GetInstanceData(m_pDescription->m_States[m_uiCurrentStateIndex].m_uiInstanceDataOffset);
    }
    return nullptr;
  }

  xiiReflectedClass&                             m_Owner;
  xiiSharedPtr<const xiiStateMachineDescription> m_pDescription;
  xiiSharedPtr<xiiBlackboard>                    m_pBlackboard;

  xiiStateMachineState* m_pCurrentState       = nullptr;
  xiiUInt32             m_uiCurrentStateIndex = xiiInvalidIndex;
  xiiTime               m_TimeInCurrentState;
  xiiStringView         m_sCurrentTransitionEvent;

  const xiiStateMachineDescription::TransitionArray* m_pCurrentTransitions = nullptr;

  xiiBlob m_InstanceData;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiStateMachineInstance);
